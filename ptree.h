/**
   \file
   \author lcs09

   \brief rappresentazione dei permessi come albero di ricerca
*/
#ifndef __PTREE__H
#define __PTREE__H

#include "intervals.h"
#include "ilist.h"

/**include aggiunto da me*/
#include <stdio.h>


/** Nodo dell'albero che rappresenta i permessi attivi in
    memoria principale**/
typedef struct nodo {
  /** targa veicolo */
  char targa[LTARGA + 1];
  /** lista degli intervalli dei permessi per quella targa */
  elem_t* lint;
  /** figlio destro */
  struct nodo* left;
  /** figlio sinistro */
  struct nodo* right;
} nodo_t;

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
int addPerm(nodo_t** r, permesso_t* pp);


/** Rimuove le informazioni relative ad un certo permesso dall'albero

 \param r puntatore alla radice dell'albero
 \param pp permesso da rimuovere

 \retval -1 se si e' verificato un errore o il permesso non e' presente
 \retval 0 se tutto OK
 */
int removePerm(permesso_t * pp, nodo_t** r);


/** Cerca il nodo dell'albero corrispondente ad un certo permesso
 \param r radice dell'albero
 \param pp permesso da cercare

 \retval p puntatore al nodo che contiene il permesso
 \retval NULL se il permesso non e' presente
 */
nodo_t* findPerm(permesso_t * pp, nodo_t* r);



/** Controlla se esiste un permesso per una certa targa ad un tempo dato

 \param r radice dell'albero

 \param targa la targa da cercare
 \param t tempo in cui deve essere attivo il permesso

 \retval 1 se al tempo t e' attivo un permesso per quella targa
 \retval 0 se non e' attivo alcun permesso
 */
int checkPerm(char targa[], time_t t, nodo_t* r);

/**
 * Legge il file che contiene l'archivio storico dei permessi e crea
 * l'albero di ricerca ordinato i permessi sono scritti in formato
 * stringa e separati da un '\n'

 \param ingresso il file di ingresso
 \param r il puntatore al puntatore alla radice dell'albero

 \retval n il numero dei permessi letti ed inseriti nell'albero se tutto e' andato a buon fine
 \retval -1 altrimenti
 */
int loadPerm(FILE* ingresso, nodo_t** r);

/**
 Scrive tutti i permessi nell'albero sul file 'uscita', i permessi
 sono prima convertiti in stringa e scritti nel file separati da '\n'
 i permessi sono scritti su file in ordine

 \param uscita il file su cui scrivere
 \param r il puntatore alla radice dell'albero

 \retval il numero di permessi registrati nel file
 \retval -1 se si e' verificato un errore
 */
int storePerm(FILE* uscita, nodo_t* r);

/** Stampa l'albero su stdout
    \param r radice dell'albero da stampare
 */
void printTree(nodo_t* r);

/** Dealloca l'albero

 \param r radice dell'albero da deallocare
*/
void freeTree(nodo_t* r);
#endif
