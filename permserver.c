/** 
\file permserver.c
\author	Andrea Lottarini
\brief server dei permessi ZTL
 
Implementazione delle funzionalita offerte dal server dei permessi

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ptree.h"
#include "lcscom.h"
#include "permserver.h"
#include "macro.h"


/* flag di terminazione dell'applicazione, viene settato a zero dagli handler dei segnali SIGTERM e SIGINT*/
static volatile sig_atomic_t working = 1;

/* struttura dati per l'albero dei permessi e relativo mutex associato*/
static nodo_t *tree;
static pthread_mutex_t mtxtree = PTHREAD_MUTEX_INITIALIZER;

/* numero di thread attivi e relativo semaforo*/
static int numThread = 0;
static pthread_mutex_t mtxnumThread = PTHREAD_MUTEX_INITIALIZER;

/* data di ultima modifica del file dei permessi*/
static time_t lastModified;

/******************************************************************************************************
				Funzioni per la gestione dei thread
******************************************************************************************************/

static void addThread()
{
    ec_non0(pthread_mutex_lock(&mtxnumThread),
	    "problema nell'eseguire la lock su mtxnumThread");
    numThread++;
    ec_non0(pthread_mutex_unlock(&mtxnumThread),
	    "problema nell'eseguire la unlock su mtxnumThread");
}

static void removeThread()
{
    ec_non0(pthread_mutex_lock(&mtxnumThread),
	    "problema nell'eseguire la lock su mtxnumthread");
    numThread--;
    ec_non0(pthread_mutex_unlock(&mtxnumThread),
	    "problema nell'eseguire la unlock su mtxnumthread");
}

/******************************************************************************************************
					Funzione thread worker
******************************************************************************************************/

/** funzione che effettua il parsing della richiesta del client per poter invocare checkPerm 
e ritorna il messaggio di risposta da inviare al client*/
static message_t checkPermesso(message_t * request)
{
	/** risposta ottenuta dal server*/
    message_t response;
	/** campo targa nel MSG_CHK*/
    char targa[LTARGA];
	/**campo passaggio nel MSG_CHK*/
    char *passaggio;
	/** conversione del passaggio in time_t*/
    time_t tempo;

    /* effettuo il parsing della stringa per poter invocare la checkPerm definita nella libreria ZTL */
    strncpy(targa, request->buffer, 7);

    passaggio = request->buffer + LTARGA + 1;
    calcolaTime(passaggio, &tempo);

    pthread_mutex_lock(&mtxtree);

    response.length = 0;
    response.buffer = NULL;

    if (checkPerm(targa, tempo, tree)) {
	response.type = MSG_OK;
    } else {
	response.type = MSG_NO;
    }

    pthread_mutex_unlock(&mtxtree);
    return response;
}

/******************************************************************************************************
				Implementazione thread writer
******************************************************************************************************/

static void *writer(void *arg)
{
	/** variabile locale per salvare le statistiche (data di ultima modifica) sul file dei permessi*/
    struct stat mod;
	/** riferimento al file da cui caricare l'albero dei permessi*/
    FILE *file = NULL;
	/** riferimento al vecchio albero, utilizzato per invocare la free*/
    nodo_t *temp;


    file = fopen((char *) arg, "r");
    while (working) {

	ec_meno1(stat((char *) arg, &mod),
		 "problema nella lettura della data di modifica del file dei permessi");
	if (mod.st_mtime > lastModified) {
#if DEBUG
	    printf("PERMSERVE_WRITER: RILEVATA MODIFICA....");
	    fflush(stdout);
#endif
	    lastModified = mod.st_mtime;

			/** aggiornamento dell'albero dei permessi */

	    pthread_mutex_lock(&mtxtree);
	    temp = tree;
	    tree = NULL;
	    ec_meno1(loadPerm(file, &tree),
		     "problema nel caricamento dell'albero dei permessi");
	    pthread_mutex_unlock(&mtxtree);

	    freeTree(temp);
	}
	sleep(NSEC);
    }

    fclose(file);

    pthread_exit((void *) NULL);
}

/******************************************************************************************************
				Implementazione thread worker
******************************************************************************************************/

static void *worker(void *arg)
{
	/** richiesta inviata al server e relativa risposta*/
    message_t request, response;
    int result;

    result = receiveMessage((channel_t) arg, &request);

    if (result == SEOF) {
	removeThread();
	pthread_exit((void *) NULL);
    }
    if (result == -1) {
	perror
	    ("PERMSERVER_WORKER: Problema nella ricezione di una richiesta");
	exit(EXIT_FAILURE);
    }

	/** checkPermesso e` un metodo apposito che ritorna direttamente il messaggio da inviare al client*/
    response = checkPermesso(&request);

    result = sendMessage((channel_t) arg, &response);

    if (result == SEOF || result == -1) {
	perror("PERMSERVER_WORKER: Problema nell'invio risposta");
	pthread_exit((void *) NULL);
    }

    closeConnection((channel_t) arg);
    free(request.buffer);
    free(response.buffer);
    removeThread();

    pthread_exit((void *) NULL);
}

/******************************************************************************************************
			Funzioni per la gestione dei segnali e terminazione
******************************************************************************************************/

/* funzione eseguita come gestione del segnale SIGINT e SIGTERM*/
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

/* funzione di pulizia delle strutture dati associare al server*/
static void closeServer(FILE * fp, nodo_t * tree, pthread_t tid_writer,
			serverChannel_t com)
{

	/** variabile temporanea utilizzata per salvare lo stato dei thread e successivamente il loro numero*/
    int status;

    printf("CHIUSURA SERVER ZTL\n");

    if (errno == 0 || errno == EINTR) {
	status = true;
    } else {
	status = false;
    }

    /* Join sul writer */
    if (tid_writer) {
	pthread_join(tid_writer, NULL);
    }

    /* chiusura del file dei permessi */
    if (fp) {
	fclose(fp);
    }

    /* free sull'albero dei permessi */
    freeTree(tree);

    /* chiusura della server socket */
    if (com) {
	closeSocket(com);
	unlink(SOCKET);
    }

    /* attesa della terminazione di tutti i thread */
    while (numThread) {
	sleep(1);
    }

    if (status) {
	exit(EXIT_SUCCESS);
    } else {
	exit(EXIT_FAILURE);
    }

}

/******************************************************************************************************
						MAIN
******************************************************************************************************/

int main(int argc, char *argv[])
{
    /* file pointer al file dei permessi */
    FILE *file;
    /* id unico del thread writer e id dell'ultimo thred worker creato */
    pthread_t tid_writer, tid_worker;
    /* server socket */
    serverChannel_t com;
    /* socket da assegnare a un worker */
    channel_t new;
    /* struttura dati per il salvataggio delle statistiche del file dei permessi */
    struct stat mod;

    /* Controllo numero argomenti */
    if (argc != 2) {
	fprintf(stderr, "Numero parametri invalido\n");
	exit(EXIT_FAILURE);
    }

    /* inizializzazione handler segnali */
    (void) handle_signals();

    /*primo caricamento albero dei permessi */
    ec_meno1(stat(argv[1], &mod),
	     "problema nella lettura della data di modifica del file dei permessi");
    lastModified = mod.st_mtime;

    ec_null(file =
	    fopen(argv[1], "r"),
	    "problema nell'apertura del file dei permessi");
    ec_meno1(loadPerm(file, &tree),
	     "problema nel caricamento dell'albero dei permessi");


    /*creazione della server socket */
    com = createServerChannel(SOCKET);

    if (com == -1) {
	fprintf(stderr,
		"PERMSERVER: problema nell'apertura della server socket\n");
	exit(EXIT_FAILURE);
    }
    if (com == SNAMETOOLONG) {
	fprintf(stderr, "il pathname della socket e' troppo lungo\n");
	exit(EXIT_FAILURE);
    }

    /*creazione thread writer */
    ec_non0_c(pthread_create(&tid_writer, NULL, writer, argv[1]),
	      "Problema nella creazione del thread writer",
	      closeServer(file, tree, 0, com));

    while (working) {

	ec_meno1_c(new =
		   acceptConnection(com),
		   "PERMSERVER: problema nell'accettare una nuova connessione",
		   closeServer(file, tree, tid_writer, com));

	ec_non0_c(pthread_create(&tid_worker, NULL, worker, (void *) new),
		  "PERMSERVER: Problema nel creare un nuovo thread\n",
		  closeServer(file, tree, tid_writer, com));

	ec_non0_c(pthread_detach(tid_worker),
		  "PERMSERVER: Problema nell'eseguire la detach su un nuovo thread\n",
		  closeServer(file, tree, tid_writer, com));

	addThread();

    }

    /* setto errno per il test in closeServer */
    errno = 0;
    closeServer(file, tree, tid_writer, com);

    return 0;
}
