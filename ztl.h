/** 
\file ztl.h
\author	Andrea Lottarini
\brief Strutture dati e funzioni del client dei permessi
 
Definizione delle strutture dati e delle funzioni del client dei permessi ZTL

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#ifndef PERMSERVER_H_
#define PERMSERVER_H_

#include "lcscom.h"

#define LUNGPASSAGGIO 25

typedef struct {
  /** inizio intervallo di tempo */
  channel_t com;
  /** fine intervallo di tempo */
  char[LUNGPASSAGGIO];
} job_t;

#endif
