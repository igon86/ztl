/** 
\file ztl.c
\author	Andrea Lottarini
\brief client dei permessi ZTL
 
Implementazione delle funzionalita offerte dal client ZTL

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "permserver.h"
#include "macro.h"
#include "lcscom.h"
#include "intervals.h"
#include "istack.h"
#include "ztl.h"

/* struttura dati di interazione thread worker/ thread writer e relativo semaforo/ condition variable*/
static infr_t *stack = NULL;
static pthread_mutex_t mtxstack = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

/* numero di thread attivi e relativo semaforo*/
static int numThread = 0;
static pthread_mutex_t mtxnumThread = PTHREAD_MUTEX_INITIALIZER;

#define MAXRETRY 5

static volatile sig_atomic_t working = 1;

/******************************************************************************************************
				Funzioni per la gestione dei thread
******************************************************************************************************/

static void addThread()
{
    ec_non0(pthread_mutex_lock(&mtxnumThread),
	    "Problema nell'eseguire la lock su mtxnumThread");

    numThread++;

    ec_non0(pthread_mutex_unlock(&mtxnumThread),
	    "Problema nell'eseguire la unlock su mtxnumThread");
}

static void removeThread()
{
    ec_non0(pthread_mutex_lock(&mtxnumThread),
	    "Problema nell'eseguire la lock su mtxnumThread");

    numThread--;

    ec_non0(pthread_mutex_unlock(&mtxnumThread),
	    "Problema nell'eseguire la unlock su mtxnumThread");
}

static void closeWorker(char *err, void *arg)
{

    fprintf(stderr, "%s", err);

    free(arg);
    removeThread();

    pthread_exit((void *) NULL);
}

static void closeAndWrite(char *err, channel_t sock, void *arg)
{

    fprintf(stderr, "%s", err);

    pthread_mutex_lock(&mtxstack);

    addInfrazione(&stack, arg);
    pthread_cond_signal(&empty);

    pthread_mutex_unlock(&mtxstack);

		/** pulizia*/
    if (sock != -1) {
	closeConnection(sock);
    }
    free(arg);

    removeThread();

    pthread_exit((void *) NULL);
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
static int testStack()
{
    int ret, s;
    pthread_mutex_lock(&mtxstack);
    s = size(stack);
    ret = ((numThread + s) > 0);
    pthread_mutex_unlock(&mtxstack);
#if DEBUG
    printf("TESTSTACK: %d -> %d + %d\n", ret, numThread, s);
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
static void *writer(void *arg)
{

    infr_t *inf = NULL;

    while (working || testStack()) {

	ec_non0(pthread_mutex_lock(&mtxstack),
		"Problema nell'eseguire la lock sullo stack");

	if ((inf = estraiInfrazione(&stack)) == NULL) {
	    pthread_cond_wait(&empty, &mtxstack);
	}
	if (inf) {
	    fprintf((FILE *) arg, "%s", inf->passaggio);
	    free(inf);
	    inf = NULL;
	}

	pthread_mutex_unlock(&mtxstack);

    }

    pthread_exit((void *) NULL);
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
static void *worker(void *arg)
{

    message_t richiesta, risposta;
    channel_t sock;
    char *s = (char *) arg;
    int results;

    /* verifico il passaggio letto */
    if (!validaPassaggio(s)) {
	free(arg);
	fprintf(stderr, "ZTL: Problema in un passaggio letto da stdin\n");
	/* L'errore non e` recuperabile quindi termino l'applicazione*/
	removeThread();
	exit(EXIT_FAILURE);
    }

    /*inizializzo la struttura richiesta */
    richiesta.type = MSG_CHECK;
    richiesta.length = LPASSAGGIO + 1;
    richiesta.buffer = s;

    /* Apro la socket verso il server */
    sock = openConnection(SOCKET);

    if (sock == -1) {
	closeAndWrite
	    ("ZTL_WORKER: problema nell'apertura della socket verso il server -> permesso non valido\n",
	     sock, arg);
    }
    if (sock == SNAMETOOLONG) {
	fprintf(stderr, "il pathname della socket e' troppo lungo\n");
	exit(EXIT_FAILURE);
    }

    /* invio la richiesta al server */
    results = sendMessage(sock, &richiesta);

    if (results == -1 || results == SEOF) {

	closeAndWrite
	    ("ZTL_WORKER: problema nell'invio della richiesta al server -> permesso non valido\n",
	     sock, arg);

    }

	printf("INVIATA RICHIESTA\n");

    /* ricevo la risposta */
    results = receiveMessage(sock, &risposta);

    if (results == -1 || results == SEOF) {

	closeAndWrite
	    ("ZTL_WORKER: problema nella ricezione della risposta dal server -> permesso non valido\n",
	     sock, arg);

    }
	
	printf("RICEVUTA RISPOSTA\n");
    /* verifico la validita` del permesso */
    if (risposta.type == MSG_OK) {
	printf("ZTL: PERMESSO VALIDO\n");
    } else {
	printf("ZTL: PERMESSO NON VALIDO\n");
	pthread_mutex_lock(&mtxstack);

	addInfrazione(&stack, s);
	pthread_cond_signal(&empty);

	pthread_mutex_unlock(&mtxstack);
    }

    closeConnection(sock);
    free(arg);

    removeThread();

    pthread_exit((void *) NULL);
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
static channel_t connetti()
{
    int numretry = 1;
    channel_t connessione = -1;

    while (connessione == -1 && numretry++ < MAXRETRY) {

	connessione = openConnection(SOCKET);

	if (connessione == -1) {
	    /* il server potrebbe non essere ancora attivo */
	    sleep(1);
	}
	if (connessione == SNAMETOOLONG) {
	    fprintf(stderr,
		    "Problema nella connessione al server: nome della socket troppo lungo");
	    exit(EXIT_FAILURE);
	}
    }

    return connessione;
}


/******************************************************************************************************
                            Funzioni per la gestione dei segnali e terminazione
******************************************************************************************************/

static void termina()
{
    working = 0;
}

/* funzione per l'installazione dei gestori dei segnali */
static int handle_signals(void)
{

    /* maschera per l'installazione degli handler dei segnali */
    sigset_t set;
    /* strutture dati per la gestione dei segnali */
    struct sigaction term, pipe;

    /* assegno una maschera temporanea */
    ec_meno1(sigfillset(&set),
	     "Problema nell'inizializzazione della maschera segnali\n");
    ec_meno1(sigprocmask(SIG_SETMASK, &set, NULL),
	     "Problema nell'assegnazione maschera ai segnali\n");

    /* installazione del gestore di SIGTERM e SIGINT */
    bzero(&term, sizeof(term));
    term.sa_handler = termina;

    ec_meno1(sigaction(SIGTERM, &term, NULL),
	     "problema nell'installazione del gestore di SIGTERM");
    ec_meno1(sigaction(SIGINT, &term, NULL),
	     "problema nell'installazione del gestore di SIGINT");

    /* installazione del gestore di SIGPIPE */
    bzero(&pipe, sizeof(pipe));
    pipe.sa_handler = SIG_IGN;

    ec_meno1(sigaction(SIGPIPE, &pipe, NULL),
	     "problema nell'installazione del gestore di SIGPIPE");

    /* tolgo la maschera */
    ec_meno1(sigemptyset(&set),
	     "Problema nella creazione della maschera finale\n");
    ec_meno1(sigprocmask(SIG_SETMASK, &set, NULL),
	     "Problema nella rimozione della maschera segnali\n");

    return 0;
}

/* funzione di pulizia delle strutture dati del processo ZTL*/
static void closeClient(pthread_t tid_writer, FILE * fp)
{
	printf("CHIUSURA CLIENT ZTL\n");
    /* notifico la terminazione dell'applicazione ad eventuali thread ancora attivi */
    working = 0;
    /* Attendo la terminazione di tutti i thread worker */
    while (numThread) {
	sleep(1);
    }

    /* Join sul writer */
    pthread_mutex_lock(&mtxstack);
    /* si manda un segnale di modo che il writer si renda conto che le richieste da servire sono terminate */
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&mtxstack);

    pthread_join(tid_writer, NULL);

    /* chiudo il file di log */
    fclose(fp);

}

/******************************************************************************************************
                                                MAIN
******************************************************************************************************/

int main(int argc, char *argv[])
{

    /* riferimento al file di log */
    FILE *fp;
    /* connessione al server */
    channel_t connessione;
    /* buffer per le stringhe lette da stdin */
    char *temp;
    /* riferimenti al thread writer e all'ultimo thread worker attivato */
    pthread_t tid_writer, tid_worker;


    /* primo controllo validita argomenti */
    if (argc != 2) {
	fprintf(stderr, "Numero parametri invalido\n");
	exit(EXIT_FAILURE);
    }

    /* installazione del gestore dei segnali */
    handle_signals();

    /* connessione con il server */
    ec_meno1(connessione =
	     connetti(), "ZTL: Problema nella connessione al server\n");
    /* la socket e` solo di prova, la chiudo immediatamente */
    close(connessione);

#if DEBUG
    printf("CONNESSO AL SERVER\n");
    fflush(stdout);
#endif

    /* apertura logfile */
    //queste vanno modificate con le chiamate alle funzioni di pulizia
    ec_null(fp =
	    fopen(argv[1], "w"),
	    "problema nell'apertura del file di log\n");

    pthread_create(&tid_writer, NULL, writer, fp);

    while (working) {

	ec_null(temp =
		(char *) malloc(LPASSAGGIO + 1),
		"Problema nell'allocazione della memoria");

	/* lettura di un passaggio dallo stdin */
	if (fread(temp, LPASSAGGIO, 1, stdin) == 0) {

	    free(temp);
	    if (feof(stdin) || errno == EINTR) {
		closeClient(tid_writer, fp);
		exit(EXIT_SUCCESS);
	    } else {
		closeClient(tid_writer, fp);
		exit(EXIT_FAILURE);
	    }
	}
	temp[LPASSAGGIO] = '\0';

	/* passaggio del lavoro a un thread worker */
	if (!(pthread_create(&tid_worker, NULL, worker, temp) == 0)) {
#if DEBUG
	    printf("PROBLEMA NELLA CREAZIONE DI UN THREAD WORKER!!!\n");
#endif
	    exit(EXIT_FAILURE);
	} else {
	    addThread();
	}
	pthread_detach(tid_worker);
    }

    closeClient(tid_writer, fp);

    return 0;

}
