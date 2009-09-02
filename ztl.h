/**  
\file ztl.h
\author Strutture dati del client
\brief 
 
Strutture dati del client del programma ztl
	
Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
*/

typedef struct
{
	/** targa veicolo */
	char targa[LTARGA + 1];
 	/** ora del passaggio*/
 	time_t pass;

} permesso_t;

