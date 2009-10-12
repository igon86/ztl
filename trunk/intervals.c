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

int compIntervalli(intervallo_t * a, intervallo_t * b)
{
    long diff = a->inizio - b->inizio;
    if (diff == 0) {
	diff = (a->fine - b->fine);
	if (diff == 0)
	    return 0;
	else if (a->fine > b->fine)
	    return 1;
	else
	    return -1;
    } else if (diff > 0)
	return 1;
    else
	return -1;
}

int validaTarga(char r[])
{
    int i;
    char *fmt = FORMATO_TARGA;
    for (i = 0; i < LTARGA; i++) {
	if (fmt[i] == 'A') {
	    if (!isupper(r[i])) {
		return 0;
	    }
	} else if (fmt[i] == 'D') {
	    if (!isdigit(r[i])) {
		return 0;
	    }
	}
    }
    return 1;

}

int calcolaTime(char *r, time_t * ret)
{
    struct tm bdt;
    char *temp;
    temp = strptime(r, "%d/%m/%Y-%H:%M", &bdt);
    if ((temp < r + LESTREMO)) {
		/**la stptime non ha letto tutti i caratteri*/
	return 0;
    }
    bdt.tm_isdst = -1;
    bdt.tm_sec = 0;
    *ret = mktime(&bdt);

    return 1;
}

int validaPassaggio(char r[])
{
    time_t tempo;

    if (validaTarga(r) && calcolaTime(r + LTARGA + 1, &tempo)) {
	return 1;
    } else {
	return 0;
    }
}

permesso_t *convertiRecord(char r[])
{
	/**mi alloco tutte le strutture dati necessarie a compilare la
	 * struct permesso_t*/
    char test[LRECORD + 1];
    char *temp;
    time_t inizio;
    time_t fine;
    if (!validaTarga(r)) {
	errno = EINVAL;
	return NULL;
    }
    temp = r + LTARGA + 1;
    if (!calcolaTime(temp, &inizio)) {
	errno = EINVAL;
	return NULL;
    }
    temp = temp + LESTREMO + 1;
    if (!calcolaTime(temp, &fine)) {
	errno = EINVAL;
	return NULL;
    }
    if (inizio > fine) {
	errno = EINVAL;
	return NULL;
    } else {
	permesso_t *p = (permesso_t *) malloc(sizeof(permesso_t));
	if (!p) {
	    errno = ENOMEM;
	    return NULL;
	}
	strncpy(p->targa, r, 7);
	p->targa[LTARGA] = '\0';
	p->in.inizio = inizio;
	p->in.fine = fine;
		/**controllo che la data sia valida*/
	convertiPermesso(p, test);
	if (strncmp(r, test, LRECORD) != 0) {
	    free(p);
	    errno = EINVAL;
	    return NULL;
	}
		/**la stringa ha passato tutti i test*/
	return p;
    }
}

int convertiPermesso(permesso_t * pp, char r[])
{
    struct tm temp;
    if (pp == NULL) {
	errno = EINVAL;
	return -1;
    }
    strncpy(r, pp->targa, LTARGA);
    r = r + LTARGA;
    localtime_r(&(pp->in.inizio), &temp);
    sprintf(r, " " FORMATO_TIME, temp.tm_mday, temp.tm_mon + 1,
	    temp.tm_year + STARTING_YEAR, temp.tm_hour, temp.tm_min);
    r = r + LESTREMO + 1;
    localtime_r(&(pp->in.fine), &temp);
    sprintf(r, " " FORMATO_TIME, temp.tm_mday, temp.tm_mon + 1,
	    temp.tm_year + STARTING_YEAR, temp.tm_hour, temp.tm_min);
    r = r + LESTREMO + 1;
    *r = '\0';
    return 0;
}

void stampaIntervallo(intervallo_t * inter)
{
    char inizio[CTIMELENGTH];
    char fine[CTIMELENGTH];
    ctime_r(&(inter->inizio), inizio);
    ctime_r(&(inter->fine), fine);
    printf("Inizio: %s", inizio);
    printf("Fine: %s", fine);
}

void stampaPermesso(permesso_t * pr)
{
    printf("Targa: %s\n", pr->targa);
    stampaIntervallo(&(pr->in));
}
