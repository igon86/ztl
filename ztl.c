#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <mcheck.h>

#include "permserver.h"
#include "macro.h"
#include "lcscom.h"
#include "intervals.h"
#include "istack.h"
#include "ztl.h"

/* struttura dati di interazione thread worker/ thread writer e relativo semaforo/ condition variable*/
static infr_t* stack = NULL;
pthread_mutex_t mtxstack = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

/* numero di thread attivi e relativo semaforo*/
static int numThread = 0;
pthread_mutex_t mtxnumThread = PTHREAD_MUTEX_INITIALIZER;

#define MAXRETRY 5
#define LUNGPASSAGGIO 25

static int working = 1;

/******************************************************************************************************
				Funzioni per la gestione dei thread
******************************************************************************************************/

static void addThread(){
	ec_non0( pthread_mutex_lock(&mtxnumThread) ,"Problema nell'eseguire la lock su mtxnumThread");

	numThread++;

	ec_non0( pthread_mutex_unlock(&mtxnumThread) ,"Problema nell'eseguire la unlock su mtxnumThread");
}

static void removeThread(){
	ec_non0( pthread_mutex_lock(&mtxnumThread) ,"Problema nell'eseguire la lock su mtxnumThread");

	numThread--;

	ec_non0( pthread_mutex_unlock(&mtxnumThread) ,"Problema nell'eseguire la unlock su mtxnumThread");
}

static void closeWorker(char* err,void* arg ){

	fprintf(stderr,"%s",err);

	free(arg);
	removeThread();

	pthread_exit((void*) NULL);
}

static void closeAndWrite(char* err,channel_t sock,void* arg,char* targa,char* passaggio){

		fprintf(stderr,"%s",err);

		pthread_mutex_lock(&mtxstack);

		addInfrazione(&stack,targa,passaggio);
		pthread_cond_signal(&empty);

		pthread_mutex_unlock(&mtxstack);

		/** pulizia*/

		closeConnection(sock);
		free(arg);

		removeThread();	

		pthread_exit((void*) NULL);
}

/******************************************************************************************************
					Thread Writer
******************************************************************************************************/

/**
Semplice funzione che verifica l'esaurimento
delle infrazioni da salvare su file di log

/param i stack da esaminare

/retval 1 nel caso ci siano altri elementi da salvare
/retval 0 se non vi sono altre infrazioni
*/
static int testStack(){
	int ret,s;
	pthread_mutex_lock(&mtxstack); 
	s = size(stack);
	ret =  ((numThread + s) > 0);
	pthread_mutex_unlock(&mtxstack);
#if DEBUG
	printf("TESTSTACK: %d -> %d + %d\n",ret,numThread,s);
	fflush(stdout);
#endif
	return ret;
}
/**
Thread writer del client ZTL.

Interagisce tramite una struttura dati istack
con i worker ztl e scrive le infrazione su un file passato come argomento

\param arg file pointer al file delle infrazioni passato come argomento al main
 */
static void* writer(void* arg){
	
	infr_t* inf = NULL;
	
	//NON FUNZIONA SE: ESCO PER EOF su passaggi
	while(working || testStack() ){
		ec_non0(pthread_mutex_lock(&mtxstack),"Problema nell'eseguire la lock sullo stack");
		if((inf = estraiInfrazione(&stack)) == NULL){
			printf("ZTL_WRITER: MI BLOCCO\n");
			pthread_cond_wait(&empty,&mtxstack);
		}
		if(inf){
#if DEBUG
			printf("ZTL_WRITER: Ho roba da scrivere\n");
			fflush(stdout);
#endif			
			fprintf((FILE*) arg,"%s %s\n",inf->targa,inf->infrazione);
			free(inf);
			inf = NULL;
		}
		else{
#if DEBUG
			printf("ZTL_WRITER:  sveglia fasulla\n");
			fflush(stdout);
#endif
		}
		pthread_mutex_unlock(&mtxstack);
		
	}
	printf("ZTL_WRITER: EXITING THE LOOP\n");

	pthread_exit((void*) NULL);
}

/******************************************************************************************************
                                        Thread Worker
******************************************************************************************************/

/**
Thread worker del client ZTL.

Interagisce con il server dei permessi ZTL per verificare un permesso
e quindi con il thread writer scrivendo l'eventuale infrazione in una struttura
datti condivisa di tipo istack

\param arg stringa contenente il passaggio da verificare
 */
static void* worker(void* arg){

        message_t richiesta,risposta;
        char targa[LTARGA];
        char* temp;
        time_t tempo;
        channel_t sock;
	char* s = (char*) arg;

#if DEBUG
        printf("ZTL_WORKER: Sto analizzando: %s",s);
        //if(s[LUNGPASSAGGIO-1] == '\n') printf("FORMATO GIA A POSTO");
#endif

        if(!validaTarga(s)){
                fprintf(stderr,"ZTL: Problema nella targa di un passaggio letto da stdin\n");
                exit(EXIT_FAILURE);
        }

#if DEBUG
        printf(".");
#endif

        strncpy(targa,s,7);
        temp = s+LTARGA+1;

        if(!calcolaTime(temp,&tempo)){
		fprintf(stderr,"Problema nellintervallo di un passaggio letto da stdin\n");
                exit(EXIT_FAILURE);
	}

#if DEBUG
        printf(".");
        fflush(stdout);
#endif

        richiesta.type = MSG_CHECK;
        richiesta.length = LUNGPASSAGGIO+1;
        richiesta.buffer = s;

#if DEBUG
        printf(".");
#endif
        
        sock = openConnection(SOCKET);
	
	if ( sock == -1 ){

		closeAndWrite("ZTL_WORKER: problema nell'apertura della socket verso il server -> permesso non valido\n",sock,arg,s,temp);

	}
	if ( sock == SNAMETOOLONG ){ 
		fprintf(stderr,"il pathname della socket e' troppo lungo\n");
		exit(EXIT_FAILURE);
	}
#if DEBUG
        printf(".");
        fflush(stdout);
#endif
	int results;

        results = sendMessage(sock, &richiesta);
	
	if(results == -1 || results == SEOF){
#if DEBUG
		printf("ZTL: PERMESSO NON VALIDO (mi hanno chiuso non ho inviato la richiesta)\n");
#endif
		pthread_mutex_lock(&mtxstack);

		addInfrazione(&stack,s,temp);
		pthread_cond_signal(&empty);

		pthread_mutex_unlock(&mtxstack);
		closeConnection(sock);
		free(arg);

		removeThread();
		return NULL;
	}

#if DEBUG
        printf("...INVIATA RICHIESTA\n");
#endif

        results = receiveMessage(sock, &risposta);

	if(results == -1 || results == SEOF){
#if DEBUG
		printf("ZTL: PERMESSO NON VALIDO (mi hanno chiuso non ho ricevuto risposta per %sNON DEVE SUCCEDERE CAZZO)\n",s);
#endif
		pthread_mutex_lock(&mtxstack);

		addInfrazione(&stack,s,temp);
		pthread_cond_signal(&empty);

		pthread_mutex_unlock(&mtxstack);
		removeThread();
		return NULL;
	}

        if(risposta.type == MSG_OK ){
                printf("ZTL: PERMESSO VALIDO\n");
        }
        else{
                printf("ZTL: PERMESSO NON VALIDO\n");
		pthread_mutex_lock(&mtxstack);

		addInfrazione(&stack,s,temp);
		pthread_cond_signal(&empty);

		pthread_mutex_unlock(&mtxstack);
        }

	closeConnection(sock);
	free(arg);

	removeThread();

	pthread_exit((void*) NULL);
}

/******************************************************************************************************
                                Funzioni per la connessione al server
******************************************************************************************************/

/**
Funzione del client ZTL per la prima connessione al server ZTL.

Questa funzione tenta per 5 volte di stabilire una connessione sulla server socket
del server ZTL.

\retval -1 Se non si e` potuto stabilire la connessione
\retval channel_t della nuova connessione
 */
static channel_t connetti(){
        int numretry = 1;
        channel_t connessione = -1;

        while(connessione == -1 && numretry++ < MAXRETRY){
#if DEBUG
                        printf("TENTATIVO NUM: %d...",numretry-1);
#endif  
                connessione = openConnection(SOCKET);
                if( connessione != -1 && connessione != SNAMETOOLONG ){
#if DEBUG
                        printf("CONNESSIONE AVVENUTA CON SUCCESSO\n");
#endif  
                }
        }
        
        return connessione;
}


/******************************************************************************************************
                            Funzioni per la gestione dei segnali e terminazione
******************************************************************************************************/

static void termina(){
        working=0;
}

static void closeClient(pthread_t tid_writer,FILE* fp){
	
	int status;

	/* Join sul writer */
#if DEBUG
	printf("WAITING FOR THREADS");
	fflush(stdout);
#endif	
	while(numThread){
		sleep(1);
	}
#if DEBUG
	printf("JOIN THREAD WRITER...");
	fflush(stdout);
#endif
	pthread_mutex_lock(&mtxstack);
#if DEBUG
	printf("...SVEGLIANDO... :)");
	fflush(stdout);
#endif
	pthread_cond_signal(&empty);
	pthread_mutex_unlock(&mtxstack);
#if DEBUG
	printf("MANDATA LA SVEGLIA");
	fflush(stdout);
#endif
	pthread_join(tid_writer,(void*) &status);

	
	fclose(fp);
#if DEBUG
	printf("DONE!!");
	fflush(stdout);
#endif	
	exit(EXIT_SUCCESS);
}

/******************************************************************************************************
                                                MAIN
******************************************************************************************************/

int main(int argc,char *argv[]){
        
        /* riferimento al file di log*/
        FILE* fp;
        /* strutture di supporto per la gestione dei segnali*/
        struct sigaction term, pipe;
        /* connessione al server*/
        channel_t connessione;
        /* buffer per le stringhe lette da stdin*/
        char* temp;
	/* riferimenti al thread writer e all'ultimo thread worker attivato*/
        pthread_t tid_writer,tid_worker;

	//mtrace();

        /* primo controllo validita argomenti*/
        if(argc != 2){
                fprintf(stderr,"Numero parametri invalido\n");
                exit(EXIT_FAILURE);
        }

        /* installazione del gestore di SIGTERM e SIGINT*/
        bzero(&term,sizeof(term));
        term.sa_handler = termina;
        
        ec_meno1(sigaction ( SIGTERM, &term, NULL ), "problema nell'installazione del gestore di SIGTERM");
        ec_meno1(sigaction ( SIGINT, &term, NULL ), "problema nell'installazione del gestore di SIGINT");       
        
        /* installazione del gestore di SIGPIPE*/
        bzero(&pipe,sizeof(pipe));
        pipe.sa_handler = SIG_IGN;
        
        ec_meno1(sigaction ( SIGPIPE, &pipe, NULL ),"problema nell'installazione del gestore di SIGPIPE");

        /* connessione con il server*/
        ec_meno1(connessione = connetti(),"Problema nella connessione al server\n");
	/* la socket e` solo di prova, la chiudo immediatamente*/
	close(connessione);

#if DEBUG
        printf("CONNESSO AL SERVER\n");
        fflush(stdout);
#endif

        /* apertura logfile*/
        //queste vanno modificate con le chiamate alle funzioni di pulizia
        ec_null(fp = fopen(argv[1],"w"),"problema nell'apertura del file di log\n");
        
       	pthread_create(&tid_writer,NULL,writer,fp);

        while(working){
                
                ec_null(temp = (char*) malloc(LUNGPASSAGGIO+1),"Problema nell'allocazione della memoria");
                /* lettura di un passaggio dallo stdin*/
                if(fread(temp,LUNGPASSAGGIO,1,stdin) == 0){
#if DEBUG	
                        printf("LETTO EOF 0 errore_> muoio male\n");
#endif  
			free(temp);
			if(feof(stdin) || errno==EINTR){
				working = 0;
				closeClient(tid_writer,fp);
				exit(EXIT_SUCCESS);
			}
			else{
			//mettere la gestione di EINTR
#if DEBUG	
                        	printf("CON UN ERRORACCIO IN ERRNO\n");
#endif  
				exit(EXIT_FAILURE);
			}
		}
		temp[LUNGPASSAGGIO]='\0';
                /* passaggio del lavoro a un thread worker*/
                if(! (pthread_create(&tid_worker,NULL,worker,temp) == 0) ){
#if DEBUG	
                        printf("PROBLEMA NELLA CREAZIONE DI UN THREAD WORKER!!!\n");
#endif  
			exit(EXIT_FAILURE);
                }
		else{
			addThread();
		}
		pthread_detach(tid_worker);
        }      
#if DEBUG	
        printf("STO PER FARE LA CLOSE CLIENT\n");
#endif  
	closeClient(tid_writer, fp); 

	return 0;
        
}
