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
pthread_mutex_t mtxnumThread	= PTHREAD_MUTEX_INITIALIZER;

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

static message_t checkPermesso(){
	message_t response;
 
	pthread_mutex_lock(&mtxtree);
	//posso anco suppore che la stringa sia ben formattata

	pthread_mutex_unlock(&mtxtree);
	return response;
}

/******************************************************************************************************
				Implementazione thread writer
******************************************************************************************************/

static void* writer(void* arg){
	struct stat mod;
	FILE* file;
	
	while(working){
		ec_null(file = fopen(arg,"r"),"problema nell'apertura del file dei permessi");
		//aggiungere comando per mettere working a zero contestualmente alla scoperta dell'errore -> oppure la exit chiude tutto?
		ec_meno1(fstat((int) file,&mod),"problema nella lettura della data di modifica del file dei permessi");
		if(mod.st_mtime > lastModified){
			lastModified = mod.st_mtime;
			
			/** aggiornamento dell'albero dei permessi */

			pthread_mutex_lock(&mtxtree);
			ec_meno1(loadPerm(file,&tree),"problema nel caricamento dell'albero dei permessi");
			pthread_mutex_unlock(&mtxtree);
		}
		sleep(NSEC);
	}
}

/******************************************************************************************************
				Implementazione thread worker
******************************************************************************************************/

static void* worker(void* arg){
	message_t request,response;
	int result;
	
	ec_meno1(result = receiveMessage(*((channel_t *) arg),&request),"Problema ricezione messaggio");
	
	if(result == SEOF){
		//gestione chiusura socket
	}
	if(request.type != 'C'){
		//gestione messaggio impossibile
	}
	
	//checkPermesso ritorna direttamente il messaggio da inviare al client
	response = checkPermesso(request);

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
	working=0;
}

static void closeServer(FILE* fp,nodo_t* tree){
	if(fp) close(fp);
	if(tree) free(tree);
}

/******************************************************************************************************
						MAIN
******************************************************************************************************/

int main(int argc, char *argv[]){

	struct sigaction term, pipe;
	FILE* file;
	nodo_t* tree;
	pthread_t tid_writer,tid_worker;
	struct stat mod;
	
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
	ec_null(file = fopen(argv[1],"r"),"problema nell'apertura del file dei permessi");
	ec_meno1(fstat((int) file,&mod),"problema nella lettura della data di modifica del file dei permessi");
	lastModified = mod.st_mtime;
	ec_meno1(loadPerm(file,&tree),"problema nel caricamento dell'albero dei permessi");
	
	
	/*creazione della socket*/
	serverChannel_t com = createServerChannel(SOCKET);
	ec_meno1(com,"problema nella creazione della server socket");

	/*creazione thread writer*/
	if(! (pthread_create(&tid_writer,NULL,writer,argv[1]) == 0) ){
		//gestione errore creazione thread writer
	}
	
	while(working){
		/* socket da assegnare a un worker*/
		channel_t new;
		
		new = acceptConnection(com);
		ec_meno1(new,"problema nell'accettare una connessione");

		if(! (pthread_create(&tid_worker,NULL,worker,&new) == 0) ){
			//gestione errore creazione thread writer
		}
		else{
			/* tutto e` andato a buon fine incremento il numero dei thread*/
			addThread();
		}
	}
	//qui working e` stato modificato quindi devo attendere la terminazione di tutti i worker + la terminazione del thread writer (con join esplicita)

	closeServer(tree,file);

	return 0;
}
