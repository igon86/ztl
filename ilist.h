/**
   \file
   \author lcs09

   \brief lista di intervalli temporali
*/
#ifndef __ILIST__H
#define __ILIST__H

/** include aggiunti*/
#include "intervals.h"

/** lista di intervalli */
typedef struct elem {
  /** ampiezza intervallo*/
  intervallo_t in;
  /** elemento succssivo */
  struct elem* next;
} elem_t;

/**
Verifica che esista un intervallo nella lista contenente il time_t passato per argomento

\param l puntatore alla lista in cui effettuare la ricerca
\param t time_t da ricercare

\retval 1 se il time_t e` contenuto in uno degli intervalli della lista
\retval 0 altrimenti
*/
int checkTime(elem_t *l,time_t t);

/** aggiunge un intervallo alla lista (allocando la memoria necessaria)
    nella lista NON ci sono intervalli replicati se l'intervallo
    e' gia' presente non modifica la lista

    \param l puntatore al puntatore alla lista cui aggiungere l'intervallo
    \param inter intervallo da aggiungere, viene copiato nel nuovo
           elemento della lista

    \retval -1 se si e' verificato un errore
               (l non viene modificata in questo caso )
    \retval 0 se tutto e' andato a buon fine

 */
int addIntervallo (elem_t ** l,intervallo_t* inter);

/** rimuove un intervallo dato da una lista

   \param l puntatore al puntatore alla lista cui aggiungere l'intervallo
    \param inter intervallo da eliminare

    \retval -1 se si e' verificato un errore (l non viene modificata in questo caso )
    \retval 0 se tutto e' andato a buon fine
 */

int removeIntervallo (elem_t ** l,intervallo_t* inter);

/** cerca nella lista un intervallo
    \param l la lista degli intervalli
    \param inter l'intervallo da cercare

    \retval NULL se l'intervall onon e' presente
    \retval p puntatore all'elemento della lista che contiene l'intervallo */
elem_t * findIntervallo (elem_t * l,intervallo_t* inter) ;

/** stampa uan lista di intervalli con formato ctime/ctime_r
    \param l la lista degli intervalli da stampare */
void printListIntervallo (elem_t * l);

/** dealloca tutta le lista degli intervalli
    \param l la lista degli intervalli da deallocare*/
void deallocaListIntervallo (elem_t * l);
#endif
