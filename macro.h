/** 
\file macro.h
\author	Andrea Lottarini
\brief Macro 
 
Definizione delle macro necessarie per la gestione dei valori di ritorno
delle chiamate di sistema

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#ifndef MACRO_H_
#define MACRO_H_

#include <stdlib.h>
#include <errno.h>

typedef int bool;
#define true 1
#define false 0

#define ec_meno1(s,m) \
	if ( (s) == -1 ) { perror(m); exit(errno); }

#define ec_meno1_c(s,m,c) \
	if ( (s) == -1 ) { perror(m); c; }
	
#define ec_null(s,m) \
	if ( (s) == NULL ) { perror(m); exit(errno); }

#define ec_non0(s,m) \
	if ( (s) != 0 ) { perror(m); exit(errno); }
	
#define ec_non0_c(s,m,c) \
	if ( (s) != 0 ) { perror(m); c; }

#endif /*MACRO_H_*/
