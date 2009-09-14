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
#define LUNGPASSAGGIO 25

static int working = 0;

/******************************************************************************************************
					Thread Worker
******************************************************************************************************/

static void checkPassaggio(char *s){

	message_t richiesta,risposta;
	char targa[LTARGA];
	char* temp;
	time_t tempo;
	channel_t sock;

#if DEBUG
	printf("WORKER: Sto analizzando: %s",s);
	//if(s[LUNGPASSAGGIO-1] == '\n') printf("FORMATO GIA A POSTO");
#endif
	//QUESTA E` ROBA DI INTERVALS CHE MI DOVREBBE DARE UNA QUALCHE VALIDAZIONE SULLA TARGA -> SE ZTL VIENE SEMPRE FEEDDATO
	// DA PASSAGGI E QUINDI DA ROBA RAGIONEVOLE PROBABILMENTE NON C'E ALCUN BISOGNO

	if(!validaTarga(s)){
		fprintf(stderr,"Problema nella lettura di un passaggio da stdin\n");
		exit(EXIT_FAILURE);
	}

#if DEBUG
	printf(".");
#endif

	strncpy(targa,s,7);
	temp = s+LTARGA+1;
	calcolaTime(temp,&tempo);

#if DEBUG
	printf(".");
	fflush(stdout);
#endif

	richiesta.type = MSG_CHECK;
	richiesta.length = LUNGPASSAGGIO;
	richiesta.buffer = s;

#if DEBUG
	printf(".",s);
#endif
	
	sock = openConnection(SOCKET);

#if DEBUG
	printf(".");
	fflush(stdout);
#endif

	ec_meno1(sendMessage(sock, &richiesta),"WORKER: Problema nell'invio di MSG_CHK");

#if DEBUG
	printf("...INVIATA RICHIESTA\n",s);
#endif

	ec_meno1(receiveMessage(sock, &risposta),"WORKER: Problema nella ricezione di una risposta");
	if(risposta.type == MSG_OK ){
		printf("PERMESSO VALIDO\n");
	}
	else{
		printf("PERMESSO NON VALIDO\n");
	}
}

/******************************************************************************************************
				Funzioni per la connessione al server
******************************************************************************************************/

static channel_t connetti(){
	int numretry = 1;
	channel_t connessione = -1;

	while(! working && numretry++ < MAXRETRY){
#if DEBUG
			printf("TENTATIVO NUM: %d...",numretry);
#endif	
		connessione = openConnection(SOCKET);
		if( connessione != -1 && connessione != SNAMETOOLONG ){
#if DEBUG
			printf("CONNESSIONE AVVENUTA CON SUCCESSO\n");
#endif	
			working = 1;
		}
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
	char* temp;

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

#if DEBUG
	printf("CONNESSO AL SERVER\n");
	fflush(stdout);
#endif

	/* apertura logfile*/
	//queste vanno modificate con le chiamate alle funzioni di pulizia
	ec_null(fp = fopen(argv[1],"w"),"problema nell'apertura del file di log\n");
	
	//manca la creazione del thread worker
	while(working){
		
		temp = (char*) malloc(LUNGPASSAGGIO);
		/* lettura di un passaggio dallo stdin*/
		fread(temp,LUNGPASSAGGIO,1,stdin);

		/* passaggio del lavoro a un thread worker*/
		if(! (pthread_create(&tid_worker,NULL,checkPassaggio,temp) == 0) ){
#if DEBUG
			printf("PROBLEMA NELLA CREAZIONE DI UN THREAD WORKER!!!\n");
#endif	
		}
	}	
	
}
