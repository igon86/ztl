/** 
\file istack.h
\author	Andrea Lottarini
\brief Struttura dati condivisa del processo ZTL
 
Strutture dati condivisa tra Thread writer e thread worker del processo ZTL

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#ifndef __ISTACK_H
#define __ISTACK_H

#include "intervals.h"

#include <string.h>
#include <stdlib.h>

/** lista di Infrazioni */
typedef struct infr {
	/** targa del veicolo*/
    char passaggio[LPASSAGGIO + 1];
	/** elemento succssivo */
    struct infr *next;
} infr_t;

/** aggiunge un infrazione allo stack (allocando la memoria necessaria)
    la lista consente l'inserimento di infrazioni replicate, queste
	verrano comunque eliminate dallo script di generazione delle
	multe

    \param l puntatore al puntatore allo stack cui aggiungere l'infrazione
    \param t targa relativa all'infrazione
	\param i stringa contenente il momento dell'infrazione

    \retval -1 se si e' verificato un errore
    \retval 0 se tutto e' andato a buon fine

 */
int addInfrazione(infr_t ** l, const char *t);

/** ritorna la prima infrazione presente nello stack

   \param l puntatore al puntatore dello stack da cui estrarre un elemento

    \retval puntatore alla prima infrazione presente nello stack
 */

infr_t *estraiInfrazione(infr_t ** l);

/** ritorna la dimensione dello stack
	param l puntatore allo stack

	/retval dimensione dello stack
*/
int size(infr_t * l);

/**Dealloca lo stack

/param l puntatore allo stack da deallocare
*/

void freeStack(infr_t * l);

#endif
