/**
\file
\author lcs09

\brief Rappresentazione dei permessi e degli intervalli temporali

 */


#ifndef __INTERVALS__H
#define __INTERVALS__H

/**include aggiunti*/
#include <time.h>

/** Lunghezza di un record restituito dalla ctime*/
#define CTIMELENGTH 26
/** Formato di un record time_t (per la stampa) */
#define FORMATO_TIME "%02d/%02d/%04d-%02d:%02d"
/** Formato di una targa*/
#define FORMATO_TARGA "AADDDAA"


/** Lunghezza di una stringa che rappresenta una targa */
#define LTARGA 7
/** Lunghezza di una stringa che rappresenta un estremo di un intervallo temporale */
#define LESTREMO 16
/** Lunghezza di una stringa che rappesenta un permesso (targa + estremi intervallo)*/
#define LRECORD (LTARGA+2*(LESTREMO)+1+1)
/** Lunghezza di una stringa che rappresenta un passaggio (targa + data del passaggio)*/
#define LPASSAGGIO 25

#define STARTING_YEAR 1900

/** Rappresentazione di un intervallo di tempo

per il tipo 'time_t' vedi "man asctime". L'inizio e la fine sono compresi fra il 01/01/200 ed il 01/01/2020 */

typedef struct {
  /** inizio intervallo di tempo */
    time_t inizio;
  /** fine intervallo di tempo */
    time_t fine;
} intervallo_t;

/** rappresentazione di un permesso ZTL come coppia targa intervallo */
typedef struct
{
  /** targa veicolo */
  char targa[LTARGA + 1];
  /** intervallo di tempo permesso*/
  intervallo_t in;
} permesso_t;

int compIntervalli(intervallo_t *a,intervallo_t *b);

/** 
Verifica che la stringa passata per argomento contenga una targa (nei primi 7 caratteri) 

\param r stringa da verificare

\retval 1 se la stringa rispetta il formato
\retval 0 altrimenti
*/
int validaTarga(char r[]);

/**
Trasforma una stringa gg/mm/aaaa-hh:mm nella corrispondente
struct tm e quindi in un time_t

\param r stringa da convertire
\param ret puntatore al time_t in cui salvare la conversione

\retval 1 se la conversione è andata a buon fine
\retval 0 altrimenti
 */
int calcolaTime(char *r, time_t * ret);

/** Verifica la validita di un passaggio

	XXXXXXX gg/mm/aaaa-hh:mm

	che rappresenta un passaggio del veicolo con targa XXXXXXX
	alla data gg/mm/aaaa-hh:mm

	\param r stringa da analizzare

	\retval 1 se la stringa rappresenta un passaggio valido
	\retval 0 altrimenti
*/
int validaPassaggio(char r[]);

/** Trasforma una stringa

     XXXXXXX gg/mm/aaaa-hh:mm gg/mm/aaaa-hh:mm

    (che rappresenta un intervallo di permesso per la targa XXXXXXX
    in una struttura di tipo permesso_t - notare che c'e' un
    SINGOLO SPAZIO fra targa e primo estremo e fra primo e secondo estremo)

    L'implementazione e' utilizzabile in un ambiente multithreaded
    (quindi ad esempio usa ctime_r/localtime_r invece che ctime/localtime etc).

  \retval p (puntatore alla nuova struttura)
  \retval NULL se si e' verificato un errore (setta errno)
 */

permesso_t* convertiRecord(char r[]);


/** Trasforma una struttura di tipo permesso in una stringa r nel formato

    XXXXXXX gg/mm/aaaa-hh:mm gg/mm/aaaa-hh:mm

    \param r  la stringa da riempire (conterra' il record secondo il formato)
    \param pp permesso da convertire

    \retval  0 se tutto e' andato a buon fine
    \retval -1 altrimenti (setta errno)
             in questo caso il contenuto di r non viene modificato
*/

int convertiPermesso(permesso_t* pp ,char r[]) ;



/** Effettua la stampa su stdout di un intervallo in formato ctime/ctime_r

\param inter l'intervallo da stampare
 */
void stampaIntervallo(intervallo_t* inter);

/** Effettua la stampa su stdout di un permesso (targa,intervallo)

\param pr la struttura permesso da stampare
 */
void stampaPermesso(permesso_t* pr);
#endif
