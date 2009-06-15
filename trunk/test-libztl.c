/**
   \file
   \author lcs09
   
   \brief Test iniziale primo frammento
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mcheck.h>

#include "intervals.h"
#include "ptree.h"

/** nome file di test */
#define FILETEST "./perm.txt"

/** esempi di permessi ben formati */
char* permessiok[] = {
  "CN445JK 12/12/2008-13:30 13/12/2008-14:30",
  "DD115PU 01/01/2009-00:00 30/06/2009-00:00",
  "CN445JK 10/02/2009-13:30 11/02/2009-14:30",
  "DD115PU 01/01/2008-00:00 30/08/2008-23:10",
  "AS105SU 13/01/2008-00:00 30/09/2008-20:15",
  "BU995PK 01/03/2009-10:00 01/03/2009-20:30",
  "KK876VB 01/02/2009-00:00 28/02/2009-22:00",
  "KK876VB 01/04/2009-00:00 30/04/2009-22:00",
  "DS009LU 01/01/2009-00:00 30/08/2009-23:10",
  "KC044ZZ 21/01/2009-09:00 22/01/2009-22:00",
  "AU663XZ 21/01/2009-09:00 22/01/2009-22:00",
  NULL
};

/** esempi di permessi con problemi */
char* permessibad[] = {
  "AA005PP 11/11/2009-15:30 13/02/2009-19:30", /* date non tornano */
  "CN44K 12/12/2008-13:30 13/12/2008-14:30",   /* corto */
  "CN4456K 12/12/2008-13:30 13/12/2008-14:30", /* alfanumerico */
  "CN445JK 12/15/2008-13:30 13/12/2008-14:30", /* data sbagliata */
  "CN445JK 12/12/b008-13:30 13/12/2008-14:30", /* anno sbagliato */
  "CN445JK 12/12/2008-14:30 12/12/2008-13:30", /* intervallo vuoto */
  NULL
};

/** orari per cui esiste/non esiste autorizzazione per una targa */
char* orariOkBad[] = {
  "CN445JK 12/12/2008-13:40 13/12/2008-18:30",
  "DD115PU 01/01/2009-01:00 30/06/2009-03:00",
  "DD115PU 01/01/2008-04:00 30/08/2008-23:50",
  "AS105SU 13/01/2008-01:00 30/09/2008-23:15",
  "BU995PK 01/03/2009-10:10 01/03/2009-22:30",
  "KK876VB 01/02/2009-20:00 28/02/2009-23:00",
   NULL
};

/** permessi con targhe non esistenti */
char* orariBadBad[] = {
  "CC445JK 12/12/2008-13:40 13/12/2008-18:30",
  "ZZ005JK 12/12/2008-13:40 13/12/2008-18:30",
   NULL
};

int main (void) {
  int i,cont;
  permesso_t* pr;
  char r[LRECORD+1];
  nodo_t* ptree=NULL;

  mtrace();

  /* test conversione permessi ok */
  for(i=0;permessiok[i]!=NULL;i++) {
    if ( (pr = convertiRecord(permessiok[i])) == NULL ) {
      fprintf(stderr,"convertiRecord: Errore fatale! %s\n",permessiok[i]);
      exit(EXIT_FAILURE);
    }
    stampaPermesso(pr);
    convertiPermesso(pr,r);
    if ( strcmp(permessiok[i],r) != 0 )  {
      fprintf(stderr,"convertiPermesso: Errore fatale! %s\n",permessiok[i]);
      free(pr);
      exit(EXIT_FAILURE);
    }
    free(pr);
  }

  cont = i;

  /* test conversione permessi mal formattati */
  for(i=0;permessibad[i]!=NULL;i++) {
    if ( (pr = convertiRecord(permessibad[i])) != NULL ) {
      fprintf(stderr,"convertiRecord: Errore fatale! %s\n",permessibad[i]);
      exit(EXIT_FAILURE);
    }
  }
  
  printf("Test Albero Permessi!!\n");
  /* test creazione albero permessi */
  for(i=0;permessiok[i]!=NULL;i++) {
    nodo_t * p;
    
    if ( (pr = convertiRecord(permessiok[i])) == NULL ) {
      fprintf(stderr,"convertiRecord: Errore fatale! %s\n",permessiok[i]);
      exit(EXIT_FAILURE);
    }
    
    if ( addPerm(&ptree,pr) != 0 ) {
      fprintf(stderr,"AddPerm: Errore fatale! %s\n",permessiok[i]);
      free(pr);
      exit(EXIT_FAILURE);
    }
    
    if ( ( ( p = findPerm(pr,ptree) ) == NULL ) || (strcmp(p->targa,pr->targa) != 0 ) ) {
      fprintf(stderr,"FindPerm: Errore fatale! %s\n",permessiok[i]);
      free(pr);
      exit(EXIT_FAILURE);
    }
    free(pr);
  }
  
  /* stampa albero risultante */
  printTree(ptree);

  /* test funzione di check con orari dentro/fuori 
     l'intervallo permesso */
  for(i=0;orariOkBad[i]!=NULL;i++) {

    if ( (pr = convertiRecord(orariOkBad[i])) == NULL ) {
      fprintf(stderr,"convertiRecord: Errore fatale! %s\n",orariOkBad[i]);
      exit(EXIT_FAILURE);
    }
    
    if ( ! checkPerm(pr->targa, pr->in.inizio,ptree) ){
      fprintf(stderr,"checkPerm: Errore fatale! Good %s\n",orariOkBad[i]);
      free(pr);
      exit(EXIT_FAILURE);
    }
    
    if ( checkPerm(pr->targa, pr->in.fine,ptree) ){
      fprintf(stderr,"checkPerm: Errore fatale! Bad %s\n",orariOkBad[i]);
      free(pr);
      exit(EXIT_FAILURE);
    }
    
    free(pr);
    
  }
  
  /*  test funzione di check con targhe non esistenti */
  for(i=0;orariBadBad[i]!=NULL;i++) {
    if ( (pr = convertiRecord(orariBadBad[i])) == NULL ) {
      fprintf(stderr,"convertiRecord: Errore fatale! %s\n",orariBadBad[i]);
      exit(EXIT_FAILURE);
    }

    if ( checkPerm(pr->targa, pr->in.fine,ptree) ){
      fprintf(stderr,"checkPerm: Errore fatale! Bad %s\n",orariBadBad[i]);
      free(pr);
      exit(EXIT_FAILURE);
    }
    free(pr);
  }

  {
    FILE* fsa;

    printf("Registro i permessi.\n");
    fsa=fopen(FILETEST,"w");
    if ( ( fsa == NULL ) || ( cont != storePerm(fsa,ptree) ) ) {
      fprintf(stderr,"storePerm: Errore fatale!\n");
      exit(EXIT_FAILURE);
    }
    printf("Registrati %d permessi.\n",cont);
    fclose(fsa);
  }

  /* test rimozione dei permessi dall'albero */
  for(i=0;permessiok[i]!=NULL;i++) {
    nodo_t * p;
    
    if ( (pr = convertiRecord(permessiok[i])) == NULL ) {
      fprintf(stderr,"convertiRecord: Errore fatale! %s\n",permessiok[i]);
      exit(EXIT_FAILURE);
    }
    
    if ( removePerm(pr,&ptree) != 0 ) {
      fprintf(stderr,"removePerm: Errore fatale! %s\n",permessiok[i]);
      exit(EXIT_FAILURE);
    }
    if ( ( ( p = findPerm(pr,ptree) ) != NULL ) ) {
      fprintf(stderr,"findPerm: Errore fatale! %s\n",permessiok[i]);
      exit(EXIT_FAILURE);
    }
    
    free(pr);
  }
  


  {
    FILE* fsa;
    
    printf("Rileggo i permessi.\n");
    fsa=fopen(FILETEST,"r");
    if ( cont != loadPerm(fsa,&ptree) ) {
      fprintf(stderr,"loadPerm: Errore fatale!\n");
      exit(EXIT_FAILURE);
    }
    
    printf("Letti %d permessi.\n",cont);
    fclose(fsa);
  }
  
  freeTree(ptree);
  printf("Test Superato!!\n");
  return EXIT_SUCCESS;
    
}
