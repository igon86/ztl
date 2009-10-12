/** 
\file ptree.c
\author	Andrea Lottarini

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include "ptree.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/**
Calcola il numero di permessi presenti in un albero

\param r puntatore alla radice dell'albero

\retval n numero di permessi presenti
*/
int sizeTree(nodo_t * r)
{
    int i;
    elem_t *temp;
    if (r == NULL) {
	return 0;
    }
    temp = r->lint;
    i = 0;
	/**scandisco la lista di intervalli associati al nodo*/
    while (temp != NULL) {
	i++;
	temp = temp->next;
    }
    return i + sizeTree(r->left) + sizeTree(r->right);
}

/**
Inserisce nell'albero r il sottoalbero binario add

\param r puntatore all'albero su cui effettura l'inserimento
\param add sottoalbero da inserire

\retval 0 se l'inserimento è andato a buon fine
\retval 1 altrimenti
*/
int addNodo(nodo_t ** r, nodo_t * add)
{
    if (add == NULL) {
	errno = EINVAL;
	return 1;
    }
    if ((*r) == NULL) {
	*r = add;
	return 0;
    } else {
	int discr = strcmp((*r)->targa, add->targa);
		/**il nuovo nodo non deve avere targa uguale a un nodo gia presente*/
	if (discr == 0) {
	    errno = EINVAL;
	    return 1;
	}
	if (discr < 0) {
	    return addNodo(&((*r)->right), add);
	} else
	    return addNodo(&((*r)->left), add);
    }

}

/**
Crea un nuovo nodo a partire da un permesso

\param pp puntatore al permesso da cui creare il nodo

\retval p puntatore al nuovo nodo
\retval NULL altrimenti
*/
nodo_t *creaNodo(permesso_t * pp)
{
    nodo_t *toadd;
    if (pp == NULL) {
	errno = EINVAL;
	return NULL;
    }
    toadd = malloc(sizeof(nodo_t));
    if (!toadd) {
	errno = ENOMEM;
	return NULL;
    }
    strcpy(toadd->targa, pp->targa);
    toadd->lint = malloc(sizeof(elem_t));
    if (!toadd->lint) {
	free(toadd);
	errno = ENOMEM;
	return NULL;
    }
    toadd->lint->next = NULL;
    toadd->lint->in = pp->in;
    toadd->left = NULL;
    toadd->right = NULL;
    return toadd;
}

/**
 Aggiunge un nuovo permesso all'albero mantenendolo ordinato rispetto
 alle targhe. Se la targa cui si riferisce il permesso e' gia'
 presente si aggiunge l'intervallo alla lista dei permessi di quella
 targa. Altrimenti si aggiunge un nuovo nodo mantenendo l'albero
 ordinato lessicograficamente rispetto alle targhe

 \param r puntatore alla radice dell'albero
 \param pp puntatore al permesso da inserire

 \retval 0 se l'inserzione e' andata a buon fine
 \retval 1 altrimenti
 */
int addPerm(nodo_t ** r, permesso_t * pp)
{
    if (pp == NULL) {
	errno = EINVAL;
	return 1;
    }
    if (*r == NULL) {
	if ((*r = creaNodo(pp)) == NULL)
	    return 1;
	else
	    return 0;
    } else {
	int discr = strcmp((*r)->targa, pp->targa);
		/**esiste gia un nodo per la targa.
		 * mi limito ad aggiornare la lista degli intervalli*/
	if (discr == 0) {
			/**devo controllare che la il nuovo permesso sia stato aggiunto*/
	    return addIntervallo(&((*r)->lint), &(pp->in));
	} else if (discr < 0) {
	    return addPerm(&((*r)->right), pp);
	} else
	    return addPerm(&((*r)->left), pp);
    }
}

/** Rimuove le informazioni relative ad un certo permesso dall'albero

 \param r puntatore alla radice dell'albero
 \param pp permesso da rimuovere

 \retval -1 se si e' verificato un errore o il permesso non e' presente
 \retval 0 se tutto OK
 */
int removePerm(permesso_t * pp, nodo_t ** r)
{
    int discr;
    int ret;
    if (r == NULL) {
	return -1;
    }
    discr = strcmp(pp->targa, (*r)->targa);
    if (discr == 0) {
	ret = removeIntervallo(&((*r)->lint), &(pp->in));
		/**se ho eliminato tutti i permessi (intervalli) devo eliminare anche il nodo relativo*/
	if (ret == 0 && (*r)->lint == NULL) {
	    nodo_t *elim = *r;
			/**assegno il figlio destro (del nodo eliminato) 
			al posto del nodo da eliminare*/
	    (*r) = elim->right;
			/**aggiungo il figlio sinistro all'albero ottenuto*/
	    addNodo(r, elim->left);
	    free(elim);
	}
	return ret;
    } else {
	if (discr < 0) {
	    return removePerm(pp, &((*r)->left));
	} else {
	    return removePerm(pp, &((*r)->right));
	}
    }
}

/** Cerca il nodo dell'albero corrispondente ad un certo permesso
 \param r radice dell'albero
 \param pp permesso da cercare

 \retval p puntatore al nodo che contiene il permesso
 \retval NULL se il permesso non e' presente
 */
nodo_t *findPerm(permesso_t * pp, nodo_t * r)
{
    int discr;
    if (r == NULL)
	return NULL;
    discr = strcmp(pp->targa, r->targa);
    if (discr == 0) {
	if (findIntervallo(r->lint, &(pp->in)))
	    return r;
	else
	    return NULL;
    } else {
	if (discr < 0) {
	    return findPerm(pp, r->left);
	} else {
	    return findPerm(pp, r->right);
	}
    }
}

/** Controlla se esiste un permesso per una certa targa ad un tempo dato
 *
 \param r radice dell'albero

 \param targa la targa da cercare
 \param t tempo in cui deve essere attivo il permesso

 \retval 1 se al tempo t e' attivo un permesso per quella targa
 \retval 0 se non e' attivo alcun permesso
 */
int checkPerm(char targa[], time_t t, nodo_t * r)
{

    int discr;

    if (r == NULL) {
	return 0;
    }

    discr = strncmp(targa, r->targa, LTARGA);

    if (discr == 0) {
	return checkTime(r->lint, t);
    } else if (discr < 0) {
	return checkPerm(targa, t, r->left);
    } else {
	return checkPerm(targa, t, r->right);
    }
}

/**
 * Legge il file che contiene l'archivio storico dei permessi e crea
 * l'albero di ricerca ordinato.
 * i permessi sono scritti in formato stringa e separati da un '\n'

 \param ingresso il file di ingresso
 \param r il puntatore al puntatore alla radice dell'albero

 \retval n il numero dei permessi letti ed inseriti nell'albero se tutto e' andato a buon fine
 \retval -1 altrimenti
 */
int loadPerm(FILE * ingresso, nodo_t ** r)
{
    permesso_t *pt;
    char buf[50];
    int prima, dopo;

    prima = sizeTree(*r);

    while (fgets(buf, 50, ingresso)) {
	pt = convertiRecord(buf);
	if (addPerm(r, pt)) {
	    free(pt);
	    errno = EINVAL;
	    return -1;
	}
	free(pt);
    }
    dopo = sizeTree(*r);
    return dopo - prima;
}

/**
 Scrive tutti i permessi nell'albero sul file 'uscita', i permessi
 sono prima convertiti in stringa e scritti nel file separati da '\n'
 i permessi sono scritti su file in ordine

 \param uscita il file su cui scrivere
 \param r il puntatore alla radice dell'albero

 \retval il numero di permessi registrati nel file
 \retval -1 se si e' verificato un errore
 */
int storePerm(FILE * uscita, nodo_t * r)
{
    int i, j;
    elem_t *temp;
    permesso_t pt;
    char out[LRECORD + 2];
    if (r == NULL) {
	return 0;
    }
    j = storePerm(uscita, r->left);
    temp = r->lint;
    i = 0;
	/**scandisco la lista di intervalli associati al nodo, li salvo come permessi, quindi in record
	e li scrivo su file*/
    while (temp != NULL) {
	i++;
	strcpy(pt.targa, r->targa);
	pt.in = temp->in;
	convertiPermesso(&pt, out);
	out[LRECORD] = '\n';
	out[LRECORD + 1] = '\0';
	fprintf(uscita, "%s", out);
	temp = temp->next;
    }
    return i + j + storePerm(uscita, r->right);
}

/** Stampa l'albero su stdout
 \param r radice dell'albero da stampare
 */
void printTree(nodo_t * r)
{
    if (r != NULL) {
	printf("TARGA: %s\n", r->targa);
	printListIntervallo(r->lint);
	printTree(r->left);
	printTree(r->right);
    }
}

/** Dealloca l'albero

 \param r radice dell'albero da deallocare
 */
void freeTree(nodo_t * r)
{
    if (r != NULL) {
	if (r->left != NULL)
	    freeTree(r->left);
	if (r->right != NULL)
	    freeTree(r->right);
	deallocaListIntervallo(r->lint);
	free(r);
    }
}
