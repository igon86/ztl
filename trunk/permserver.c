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

/* flag di terminazione dell'applicazione, viene settato a zero dagli handler dei segnali SIGTERM e SIGINT*/
volatile sig_atomic_t working = 1;

/* struttura dati per l'albero dei permessi e relativo mutex associato*/
nodo_t *tree=NULL;
pthread_mutex_t mtxtree	= PTHREAD_MUTEX_INITIALIZER;

/* numero di thread attivi e relativo semaforo*/
int numThread = 0;
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
					Funzioni thread worker
******************************************************************************************************/

static message_t checkPermesso(message_t* request){
	message_t response;
	//ASSUMO CHE MI ARRIVINO PERMESSI A GARBO!!! E SOLO MSG_CHK
	char targa[LTARGA];
	char* passaggio;
	time_t tempo;

 	strncpy(targa,request->buffer,7);
	
	passaggio = request->buffer+LTARGA+1;
	calcolaTime(passaggio,&tempo);

	pthread_mutex_lock(&mtxtree);
	
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
	struct stat mod;
	FILE* file = NULL;
	
	while(working){
		//aggiungere comando per mettere working a zero contestualmente alla scoperta dell'errore -> oppure la exit chiude tutto?
		ec_meno1(stat((char*) arg,&mod),"problema nella lettura della data di modifica del file dei permessi");
		file = fopen((char*) arg,"r");
		if(mod.st_mtime > lastModified){
#if DEBUG
			printf("WRITE: RILEVATA MODIFICA\n");
			fflush(stdout);
#endif
			lastModified = mod.st_mtime;
			
			/** aggiornamento dell'albero dei permessi */

			pthread_mutex_lock(&mtxtree);
			ec_meno1(loadPerm(file,&tree),"problema nel caricamento dell'albero dei permessi");
			pthread_mutex_unlock(&mtxtree);
		}
		sleep(NSEC);
	}
	
	if(file) fclose(file);
#if DEBUG
	printf("THREAD WRITER EXIT.....");
	fflush(stdout);
#endif
	pthread_exit((void*) NULL);
}

/******************************************************************************************************
				Implementazione thread worker
******************************************************************************************************/

static void* worker(void* arg){
	message_t request,response;
	int result;
	
	ec_meno1(result = receiveMessage(*((channel_t *) arg),&request),"Problema ricezione messaggio");

#if DEBUG
	printf("Worker: Ricevuto un %c su %s",request.type,request.buffer);
#endif
	
	if(result == SEOF){
		//gestione chiusura socket -> ma non e` uguale a prima?
	}
	if(request.type != 'C'){
		//gestione messaggio impossibile
	}
	
	//checkPermesso ritorna direttamente il messaggio da inviare al client
	response = checkPermesso(&request);

	ec_meno1(result = sendMessage(*((channel_t *) arg),&response),"Problema nell'invio risposta");

	if(result == SEOF){
		//gestione chiusura socket -> impossibile in questo caso vero?
	}
	
	removeThread();
	
	return 0;
}

/******************************************************************************************************
				Funzioni per la gestione dei segnali
******************************************************************************************************/

static void termina(){
	write(1,"METTO WORKING A 0\n",18);
	working=0;
}

static void closeServer(FILE* fp,nodo_t* tree,pthread_t tid_writer,serverChannel_t com){

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

	if(fp) close(fp);
	if(tree) free(tree);
	if(com){ 
		close(com);
		unlink(SOCKET);
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
	channel_t new,test;
	/* struttura dati per il salvataggio delle statistiche del file dei permessi*/
	struct stat mod;
	int status;
	

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
		fprintf(stderr,"problema nell'apertura della server socket\n");
		exit(EXIT_FAILURE);
	}
	if ( com == SNAMETOOLONG ){ 
		fprintf(stderr,"il pathname della socket e' troppo lungo\n");
		exit(EXIT_FAILURE);
	}

#if DEBUG
	printf("SERVER SOCKET CREATA\n");
#endif	

	/*creazione thread writer*/
	if(! (pthread_create(&tid_writer,NULL,writer,argv[1]) == 0) ){
		//gestione errore creazione thread writer
	}

#if DEBUG
	printf("THREAD WRITER CREATO\n");
#endif	
	
	ec_meno1_c(test = acceptConnection(com),"problema nell'accettare una connessione",closeServer(file,tree,tid_writer,com));
	
#if DEBUG
	printf("CONNESSIONE DI TEST AVVIATA\n");
#endif	
	while(working){
		
		ec_meno1_c(new = acceptConnection(com),"problema nell'accettare una connessione",closeServer(file,tree,tid_writer,com));

		if(! (pthread_create(&tid_worker,NULL,worker,&new) == 0) ){
#if DEBUG
			printf("PROBLEMA NELLA CREAZIONE DI UN THREAD WORKER!!!\n");
#endif	
		}
		else{
			/* tutto e` andato a buon fine incremento il numero dei thread*/
			addThread();
		}
	}
	//qui working e` stato modificato quindi devo attendere la terminazione di tutti i worker + la terminazione del thread writer (con join esplicita)
	
	closeServer(file,tree,tid_writer,com);

	return 0;
}
