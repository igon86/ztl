/** 
\file istack.c
\author	Andrea Lottarini
\brief pila delle infrazioni
 
Implementazione della pila di infrazioni utilizzata dal client ZTL

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include "istack.h"


int addInfrazione(infr_t ** l, const char *s)
{

    infr_t *temp = *l;
    *l = (infr_t *) malloc(sizeof(infr_t));
    if (*l) {
	strncpy((*l)->passaggio, s, LPASSAGGIO);
	(*l)->passaggio[LPASSAGGIO] = '\0';

	(*l)->next = temp;
    } else {
	return -1;
    }
    return 0;
}

infr_t *estraiInfrazione(infr_t ** l)
{
    infr_t *ret = *l;

    if (*l != NULL) {
	*l = (*l)->next;
    }

    return ret;
}


int size(infr_t * l)
{
    if (l != NULL) {
	return 1 + size(l->next);
    } else {
	return 0;
    }
}

void freeStack(infr_t * l)
{
    if (l != NULL) {
	free(l->next);
	free(l);
    }
}
