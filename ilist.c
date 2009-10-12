/** 
\file ilist.c
\author	Andrea Lottarini

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include "ilist.h"
#include <stdlib.h>
#include <errno.h>


int checkTime(elem_t * l, time_t t)
{
    if (l == NULL)
	return 0;
    if (t < l->in.inizio)
	return checkTime(l->next, t);
    else {
	if (t < l->in.fine)
	    return 1;
	else
	    return checkTime(l->next, t);
    }
}

int addIntervallo(elem_t ** l, intervallo_t * inter)
{
    elem_t *temp;
    elem_t *new;
	/**caso lista vuota
	 * lo inserisco come primo elemento*/
    if (inter == NULL) {
	errno = EINVAL;
	return -1;
    }
    if (*l == NULL) {
	*l = malloc(sizeof(elem_t));
	(*l)->in = *inter;
	(*l)->next = NULL;
	return 0;
    }
    temp = *l;
    while (temp->next != NULL
	   && compIntervalli(inter, &(temp->next->in)) > 0) {
	temp = temp->next;
    }
	/**elemento gia inserito*/
    if (temp->next != NULL
	&& compIntervalli(inter, &(temp->next->in)) == 0) {
	    /**Non è un errore,dato che l'elemento è gia presente l'aggiunta
	    si considera effettuata con successo*/
	return 0;
    }
	/**elemento da inserire*/
    new = malloc(sizeof(elem_t));
    if (!new) {
	errno = ENOMEM;
	return -1;
    }
    new->in = *inter;
    new->next = temp->next;
    temp->next = new;
    return 0;
}

int removeIntervallo(elem_t ** l, intervallo_t * inter)
{
    elem_t *temp;
    if (inter == NULL) {
	errno = EINVAL;
	return -1;
    }
    if (*l == NULL) {
	    /**non ho niente da rimuovere*/
	return 0;
    } else {
	temp = *l;
	if (compIntervalli(inter, &(temp->in))) {
	    return removeIntervallo(&(temp->next), inter);
	} else {
			/**ho individuato il nodo da eliminare*/
	    elem_t *next = temp->next;
	    *l = next;
	    free(temp);
	    return 0;
	}
    }
}

elem_t *findIntervallo(elem_t * l, intervallo_t * inter)
{
    elem_t *temp;
    int cmp;
    if (inter == NULL) {
	errno = EINVAL;
	return NULL;
    }
    temp = l;
    while (temp != NULL) {
	cmp = compIntervalli(inter, &(temp->in));
	if (cmp == 0)
	    return temp;
	else
	    temp = temp->next;
    }
    return NULL;
}

void printListIntervallo(elem_t * l)
{
    if (l != NULL) {
	stampaIntervallo(&(l->in));
	printListIntervallo(l->next);
    }
}

void deallocaListIntervallo(elem_t * l)
{
    if (l != NULL) {
	deallocaListIntervallo(l->next);
	free(l);
    }
}
