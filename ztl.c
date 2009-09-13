#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "permserver.h"
#include "macro.h"
#include "lcscom.h"
#include "intervals.h"

//PILA DI INTERAZIONE tra worker e writer

#define MAXRETRY 5
#define LUNGPASSAGGIO 24

static int working = 0;

/******************************************************************************************************
					Thread Worker
******************************************************************************************************/

static void checkPassaggio(char *temp){
	message_t richiesta,risposta;
	char targa[LTARGA];
	time_t tempo;

	if(!validaTarga(temp)){
		fprintf(stderr,"Problema nella lettura di un passaggio da stdin");
		//pulizia
		exit(EXIT_FAILURE);
	}
	strncpy(targa,temp,7);
	temp = temp+LTARGA+1;
	calcolaTime(temp,&tempo);
	channel_t sock = openConnection(SOCKET);
	
}

/******************************************************************************************************
				Funzioni per la connessione al server
******************************************************************************************************/

static channel_t connetti(){
	int numretry = 1;
	channel_t connessione = -1;

	while(! working && numretry++ < MAXRETRY){
		channel_t connessione = openConnection(SOCKET);
		if( connessione != -1 && connessione != SNAMETOOLONG ) working = 1;
	}
	
	return connessione;
}


/******************************************************************************************************
				Funzioni per la gestione dei segnali
******************************************************************************************************/

static void termina(){
	working=0;
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
	char temp[LUNGPASSAGGIO];

	pthread_t tid_writer,tid_worker;

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

	/* apertura logfile*/
	//queste vanno modificate con le chiamate alle funzioni di pulizia
	ec_meno1(fp = fopen(argv[1],"w"),"problema nell'apertura del file di log");
	
	//manca la creazione del thread worker
	while(working){
		/* lettura di un passaggio dallo stdin*/
		read(temp,LUNGPASSAGGIO,stdin);
		/* passaggio del lavoro a un thread worker*/
		if(! (pthread_create(&tid_worker,NULL,checkPassaggio,temp) == 0) ){
			//gestione errore creazione thread worker
		}
	}	
	
}
