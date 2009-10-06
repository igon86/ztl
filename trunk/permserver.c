/** 
\file permserver.c
\author	Andrea Lottarini
\brief server dei permessi ZTL
 
Implementazione delle funzionalita offerte dal server dei permessi

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include "ptree.h"
#include "lcscom.h"
#include "permserver.h"
#include "macro.h"

#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mcheck.h>

/* flag di terminazione dell'applicazione, viene settato a zero dagli handler dei segnali SIGTERM e SIGINT*/
volatile sig_atomic_t working = 1;

/* struttura dati per l'albero dei permessi e relativo mutex associato*/
nodo_t *tree=NULL;
pthread_mutex_t mtxtree	= PTHREAD_MUTEX_INITIALIZER;

/* numero di thread attivi e relativo semaforo*/
static int numThread = 0;
pthread_mutex_t mtxnumThread = PTHREAD_MUTEX_INITIALIZER;

/* data di ultima modifica del file dei permessi*/
time_t lastModified;

/******************************************************************************************************
				Funzioni per la gestione dei thread
******************************************************************************************************/

static void addThread(){
	pthread_mutex_lock(&mtxnumThread);
	numThread++;
	pthread_mutex_unlock(&mtxnumThread);
}

static void removeThread(){
	pthread_mutex_lock(&mtxnumThread);
	numThread--;
	pthread_mutex_unlock(&mtxnumThread);
}

/******************************************************************************************************
					Funzione thread worker
******************************************************************************************************/

static message_t checkPermesso(message_t* request){
	/** risposta ottenuta dal server*/
	message_t response;
	/** campo targa nel MSG_CHK*/
	char targa[LTARGA];
	/**campo passaggio nel MSG_CHK*/
	char* passaggio;
	/** conversione del passaggio in time_t*/
	time_t tempo;

 	strncpy(targa,request->buffer,7);
	
	passaggio = request->buffer+LTARGA+1;
	calcolaTime(passaggio,&tempo);

	pthread_mutex_lock(&mtxtree);

	response.type = MSG_OK;
	response.length = 0;
	response.buffer = NULL;

	if(checkPerm(targa,tempo,tree)){
		response.type = MSG_OK;
	}
	else{
		response.type = MSG_NO;	
	}

	pthread_mutex_unlock(&mtxtree);
	return response;
}

/******************************************************************************************************
				Implementazione thread writer
******************************************************************************************************/

static void* writer(void* arg){
	/** variabile locale per salvare le statistiche (data di ultima modifica) sul file dei permessi*/
	struct stat mod;
	/** riferimento al file da cui caricare l'albero dei permessi*/
	FILE* file = NULL;
	/** riferimento al vecchio albero, utilizzato per invocare la free*/
	nodo_t* temp;


	file = fopen((char*) arg,"r");
	while(working){
		//aggiungere comando per mettere working a zero contestualmente alla scoperta dell'errore -> oppure la exit chiude tutto?
		ec_meno1(stat((char*) arg,&mod),"problema nella lettura della data di modifica del file dei permessi");
		if(mod.st_mtime > lastModified){
#if DEBUG
			printf("PERMSERVE_WRITER: RILEVATA MODIFICA....");
			fflush(stdout);
#endif
			lastModified = mod.st_mtime;
			
			/** aggiornamento dell'albero dei permessi */

			pthread_mutex_lock(&mtxtree);
			temp = tree;
			tree=NULL;
			ec_meno1(loadPerm(file,&tree),"problema nel caricamento dell'albero dei permessi");
			pthread_mutex_unlock(&mtxtree);

			freeTree(temp);
#if DEBUG
			printf("AGGIORNATO ALBERO \n");
			fflush(stdout);
#endif
		}
		sleep(NSEC);
	}
	
	fclose(file);
#if DEBUG
	printf("PERMSERVER_WRITER EXIT.....");
	fflush(stdout);
#endif
	pthread_exit((void*) NULL);
}

/******************************************************************************************************
				Implementazione thread worker
******************************************************************************************************/

static void* worker(void* arg){
	/** richiesta inviata al server e relativa risposta*/
	message_t request,response;
	int result;

	result = receiveMessage( (channel_t) arg,&request);

	if(result == SEOF){
		printf("PERMSERVER_WORKER: nessun messaggio\n");
		removeThread();
		pthread_exit((void*) NULL);;
	}
	if(result == -1){
		perror("E` successo sto bordellO: ");
		exit(EXIT_FAILURE);
	}

#if DEBUG
	printf("PERMSERVER_Worker: Ricevuto un %c su %s",request.type,request.buffer);
#endif
	
	
	//checkPermesso ritorna direttamente il messaggio da inviare al client
	response = checkPermesso(&request);

	ec_meno1(result = sendMessage( (channel_t) arg,&response),"Problema nell'invio risposta");

	if(result == SEOF){
		//gestione chiusura socket -> impossibile in questo caso vero?
	}
	
	closeConnection((int) ((channel_t) arg) );
	free(request.buffer);
	free(response.buffer);
	removeThread();
	
	pthread_exit((void*) NULL);
}

/******************************************************************************************************
			Funzioni per la gestione dei segnali e terminazione
******************************************************************************************************/

static void termina(){
	working=0;
}

static void closeServer(FILE* fp,nodo_t* tree,pthread_t tid_writer,serverChannel_t com){
	/** variabile temporanea utilizzata per salvare lo stato dei thread e successivamente il loro numero*/
	int status;

	/* Join sul writer */
#if DEBUG
	printf("JOIN THREAD WRITER...");
	fflush(stdout);
#endif

	pthread_join(tid_writer,(void*) &status);

#if DEBUG
	printf("COMPLETATA!\n");
	fflush(stdout);
#endif

	if(fp){
#if DEBUG
	printf("CHIUDO FILE\n");
	fflush(stdout);
#endif
		fclose(fp);
	}
#if DEBUG
	printf("FREE SULL'ALBERO\n");
	fflush(stdout);
#endif
	freeTree(tree);

	if(com){ 
		close(com);
		unlink(SOCKET);
	}
#if DEBUG
	printf("ATTESA THREAD!\n");
	fflush(stdout);
#endif
	while(numThread){
		sleep(1);
	}

	exit(EXIT_SUCCESS);
}

/******************************************************************************************************
						MAIN
******************************************************************************************************/

int main(int argc, char *argv[]){
	
	/* strutture dati per la gestione dei segnali*/
	struct sigaction term, pipe;
	FILE* file;
	/* id unico del thread writer e id dell'ultimo thred worker creato*/
	pthread_t tid_writer,tid_worker;
	/* server socket*/
	serverChannel_t com;
	/* socket da assegnare a un worker*/
	channel_t new;
	/* struttura dati per il salvataggio delle statistiche del file dei permessi*/
	struct stat mod;

	//mtrace();

	/* Controllo numero argomenti */
	if (argc!=2){
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

	/*primo caricamento albero dei permessi*/
	ec_meno1(stat(argv[1],&mod),"problema nella lettura della data di modifica del file dei permessi");
	lastModified = mod.st_mtime;

	ec_null(file = fopen(argv[1],"r"),"problema nell'apertura del file dei permessi");
	ec_meno1(loadPerm(file,&tree),"problema nel caricamento dell'albero dei permessi");
	
#if DEBUG
	printf("APERTURA FILE COMPLETATA\n");
	
#endif	


	/*creazione della server socket*/
	com = createServerChannel(SOCKET);
	if ( com == -1 ){
		fprintf(stderr,"PERMSERVER: problema nell'apertura della server socket\n");
		exit(EXIT_FAILURE);
	}
	if ( com == SNAMETOOLONG ){ 
		fprintf(stderr,"il pathname della socket e' troppo lungo\n");
		exit(EXIT_FAILURE);
	}

#if DEBUG
	printf("PERMSERVER: SERVER SOCKET CREATA\n");
#endif	

	/*creazione thread writer*/
	if(! (pthread_create(&tid_writer,NULL,writer,argv[1]) == 0) ){
		//gestione errore creazione thread writer
	}

#if DEBUG
	printf("THREAD WRITER CREATO\n");
#endif	

	while(working){

		ec_meno1_c(new = acceptConnection(com),"PERMSERVER_MAIN: problema nell'accettare una connessione",closeServer(file,tree,tid_writer,com));
		
		if(! (pthread_create(&tid_worker,NULL,worker,(void*) new) == 0) ){
#if DEBUG
			printf("PERMSERVER: PROBLEMA NELLA CREAZIONE DI UN THREAD WORKER!!!\n");
#endif	
		}
		else{
			pthread_detach(tid_worker);
			addThread();
		}

	}
	//qui working e` stato modificato quindi devo attendere la terminazione di tutti i worker + la terminazione del thread writer (con join esplicita)
	
	printf("PERMSERVER: STO USCENDO DAL MAIN LOOP");
	closeServer(file,tree,tid_writer,com);

	return 0;
}
