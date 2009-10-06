/** 
\file intervals.c
\author	Andrea Lottarini

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

/**aggiunto per la strptime*/
#define _XOPEN_SOURCE

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include "intervals.h"



/**
Confronta 2 intervalli

\param a,b puntatori agli intervalli da confrontare

\retval 0 i due intervalli sono uguali
\retval 1 l'intervallo a e` maggiore dell'intervallo b
\retval -1 altrimenti
*/
int compIntervalli(intervallo_t *a,intervallo_t *b) {
	long diff = a->inizio - b->inizio;
	if (diff==0) {
		diff=(a->fine - b->fine);
		if(diff==0) return 0;
		else if(a->fine > b->fine)return 1;
		else return -1;
	}
	else if (diff>0)return 1;
	else return -1;
}


/** 
Verifica che la stringa passata per argomento contenga una targa (nei primi 7 caratteri) 

\param r stringa da verificare

\retval 1 se la stringa rispetta il formato
\retval 0 altrimenti
*/
int validaTarga(char r[]) {
	int i;
	char *fmt=FORMATO_TARGA;
	for (i=0; i<LTARGA; i++) {
		if (fmt[i]=='A') {
			if (!isupper(r[i])) {
				return 0;
			}
		} else if (fmt[i]=='D') {
			if (!isdigit(r[i])) {
				return 0;
			}
		}
	}
	return 1;

}
/**
Trasforma una stringa gg/mm/aaaa-hh:mm nella corrispondente
struct tm e quindi in un time_t

\param r stringa da convertire
\param ret puntatore al time_t in cui salvare la conversione

\retval 1 se la conversione è andata a buon fine
\retval 0 altrimenti
 */
int calcolaTime(char *r, time_t * ret) {
	struct tm bdt;
	char *temp;
	temp = strptime(r,"%d/%m/%Y-%H:%M",&bdt);
	if((temp<r+LESTREMO)){
		printf("PROBLEMA NELLA STRPTIME DENTRO CALCOLA-TIME");
	      	/**la stptime non ha letto tutti i caratteri*/
	      	return 0;
	}
	bdt.tm_isdst=-1;
	bdt.tm_sec=0;
	*ret = mktime(&bdt);
	if(*ret == -1){
		printf("PROBLEMA NELLA MKTIME DENTRO CALCOLA-TIME");
	}
	return 1;
}

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
permesso_t* convertiRecord(char r[]) {
	/**mi alloco tutte le strutture dati necessarie a compilare la
	 * struct permesso_t*/
	char test[LRECORD+1];
	char *temp;
	time_t inizio;
	time_t fine;
	if (!validaTarga(r)) {
		errno = EINVAL;
		return NULL;
	}
	temp = r+LTARGA+1;
	if(!calcolaTime(temp,&inizio)){
	    errno = EINVAL;
	    return NULL;
	}
	temp=temp+LESTREMO+1;
	if(!calcolaTime(temp,&fine)){
	    errno=EINVAL;
	    return NULL;
	}
	if(inizio>fine) {
		errno = EINVAL;
		return NULL;
	}
	else{
		permesso_t *p = (permesso_t *) malloc(sizeof(permesso_t));
		if(!p){
		    errno = ENOMEM;
		    return NULL;
		}
		strncpy(p->targa,r,7);
		p->targa[LTARGA]='\0';
		p->in.inizio=inizio;
		p->in.fine=fine;
		/**controllo che la data sia valida*/
		convertiPermesso(p,test);
		if(strncmp(r,test,LRECORD)!=0){
			free(p);
		    errno = EINVAL;
		    return NULL;
		}
		/**la stringa ha passato tutti i test*/
		return p;
	}
}

/** Trasforma una struttura di tipo permesso in una stringa r nel formato

 XXXXXXX gg/mm/aaaa-hh:mm gg/mm/aaaa-hh:mm

 \param r  la stringa da riempire (conterra' il record secondo il formato)
 \param pp permesso da convertire

 \retval  0 se tutto e' andato a buon fine
 \retval -1 altrimenti (setta errno)
 in questo caso il contenuto di r non viene modificato
 */
int convertiPermesso(permesso_t* pp, char r[]) {
	struct tm temp;
	if(pp==NULL){
	    errno = EINVAL;
	    return -1;
	}
	strncpy(r, pp->targa, LTARGA);
	r=r+LTARGA;
	localtime_r(&(pp->in.inizio),&temp);
	sprintf(r, " " FORMATO_TIME, temp.tm_mday, temp.tm_mon+1,
			temp.tm_year+1900, temp.tm_hour, temp.tm_min);
	r =r + LESTREMO+1;
	localtime_r(&(pp->in.fine),&temp);
	sprintf(r, " " FORMATO_TIME, temp.tm_mday, temp.tm_mon+1,
			temp.tm_year+1900, temp.tm_hour, temp.tm_min);
	r =r + LESTREMO+1;
	*r='\0';
	return 0;
}
/**metodo custom per comporre una stampa di un intervallo*/

/** Effettua la stampa su stdout di un intervallo in formato ctime/ctime_r

 \param inter l'intervallo da stampare
 */
void stampaIntervallo(intervallo_t* inter) {
	char inizio[CTIMELENGTH];
	char fine[CTIMELENGTH];
	ctime_r(&(inter->inizio), inizio);
	ctime_r(&(inter->fine), fine);
	printf("Inizio: %s", inizio);
	printf("Fine: %s", fine);
}

/** Effettua la stampa su stdout di un permesso (targa,intervallo)

 \param pr la struttura permesso da stampare
 */
void stampaPermesso(permesso_t* pr) {
	printf("Targa: %s\n", pr->targa);
	stampaIntervallo(&(pr->in));
}
