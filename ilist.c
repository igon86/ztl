/** 
\file ilist.c
\author	Andrea Lottarini

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include "ilist.h"
#include <stdlib.h>
#include <errno.h>


/**
Verifica che esista un intervallo nella lista contenente il time_t passato per argomento

\param l puntatore alla lista in cui effettuare la ricerca
\param t time_t da ricercare

\retval 1 se il time_t e` contenuto in uno degli intervalli della lista
\retval 0 altrimenti
*/
int checkTime(elem_t *l,time_t t) {
	if(l==NULL) return 0;
	if(t < l->in.inizio) return checkTime(l->next,t);
	else {
		if(t < l->in.fine) return 1;
		else return checkTime(l->next,t);
	}
}

/** aggiunge un intervallo alla lista (allocando la memoria necessaria)
 nella lista NON ci sono intervalli replicati se l'intervallo
 e' gia' presente non modifica la lista

 \param l puntatore al puntatore alla lista cui aggiungere l'intervallo
 \param inter intervallo da aggiungere, viene copiato nel nuovo
 elemento della lista

 \retval -1 se si e' verificato un errore
 (l non viene modificata in questocaso )
 \retval 0 se tutto e' andato a buon fine
 */
int addIntervallo(elem_t ** l, intervallo_t* inter) {
	elem_t *temp;
	elem_t *new;
	/**caso lista vuota
	 * lo inserisco come primo elemento*/
	if(inter==NULL){
		errno=EINVAL;
		return -1;
	}
	if(*l==NULL) {
		*l = malloc(sizeof(elem_t));
		(*l)->in = *inter;
		(*l)->next=NULL;
		return 0;
	}
	temp = *l;
	while(temp->next!=NULL && compIntervalli(inter,&(temp->next->in))>0){
	    temp = temp->next;
	}
	/**elemento gia inserito*/
	if(temp->next!=NULL && compIntervalli(inter,&(temp->next->in))==0){
	    /**Non è un errore,dato che l'elemento è gia presente l'aggiunta
	    si considera effettuata con successo*/
	    return 0;
	}
	/**elemento da inserire*/
	new=malloc(sizeof(elem_t));
	if(!new){
	    errno=ENOMEM;
	    return -1;
	}
	new->in=*inter;
	new->next=temp->next;
	temp->next=new;
	return 0;
}

/** rimuove un intervallo dato da una lista

 \param l puntatore al puntatore alla lista cui aggiungere l'intervallo
 \param inter intervallo da eliminare

 \retval -1 se si e' verificato un errore (l non viene modificata in questo caso )
 \retval 0 se tutto e' andato a buon fine
 */
int removeIntervallo(elem_t ** l, intervallo_t* inter) {
	elem_t *temp;
	if(inter==NULL){
		errno=EINVAL;
		return -1;
	}
	if(*l==NULL){
	    /**non ho niente da rimuovere*/
	    return 0;
	}
	else{
		temp=*l;
		if(compIntervalli(inter,&(temp->in))){
			return removeIntervallo(&(temp->next),inter);
		}
		else{
			/**ho individuato il nodo da eliminare*/
			elem_t *next=temp->next;
			*l = next;
			free(temp);
			return 0;
		}
	}
}

/** cerca nella lista un intervallo
 \param l la lista degli intervalli
 \param inter l'intervallo da cercare

 \retval NULL se l'intervallo non e' presente
 \retval p puntatore all'elemento della lista che contiene l'intervallo */
elem_t * findIntervallo(elem_t * l, intervallo_t* inter) {
	elem_t *temp;
	int cmp;
	if(inter == NULL){
		errno=EINVAL;
		return NULL;
	}
	temp = l;
	while (temp!=NULL) {
		cmp = compIntervalli(inter,&(temp->in));
		if(cmp==0) return temp;
		else temp = temp->next;
	}
	return NULL;
}

/** stampa una lista di intervalli con formato ctime/ctime_r
 \param l la lista degli intervalli da stampare
*/
void printListIntervallo(elem_t * l) {
	if(l!=NULL) {
		stampaIntervallo(&(l->in));
		printListIntervallo(l->next);
	}
}

/** dealloca tutta le lista degli intervalli
 \param l la lista degli intervalli da deallocare
*/
void deallocaListIntervallo(elem_t * l) {
	if(l!=NULL) {
		deallocaListIntervallo(l->next);
		free(l);
	}
}
