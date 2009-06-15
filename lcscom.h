/**  \file 
 *    \author lcs09
 *  \brief libreria di comunicazione server client
 *
 *  Libreria che definisce l'interfaccia di comunicazione fra client e server
 *  (canali di comunicazione realizzati con socket AF_UNIX) 
*/

#ifndef _LCSCOM_H
#define _LCSCOM_H

/* -= TIPI =- */

/** <H3>Messaggio</H3>
 * La struttura \c message_t rappresenta un messaggio tra client e server
 * - \c type rappresenta il tipo del messaggio
 * - \c length rappresenta la lunghezza in byte del campo buffer
 * - \c buffer e' il puntatore al messaggio
 *
 * <HR>
 */

typedef struct {
    char type;           /**< tipo del messaggio */
    unsigned int length; /**< lunghezza in byte */
    char* buffer;        /**< buffer messaggio */
} message_t; 

/** lunghezza buffer indirizzo AF_UNIX */
#define UNIX_PATH_MAX    108

/** fine dello stream su socket, connessione chiusa dal peer */
#define SEOF -2

/** tipo dei messaggi di errore */
#define MSG_ERROR          'E' 
/** tipo dei messaggi di OK (non c'e' infrazione) */
#define MSG_OK             '0' 
/** tipo dei messaggi di multa (c'e' infrazione) */
#define MSG_NO             'N' 
/** tipo dei messaggi di richiesta check */
#define MSG_CHECK          'C' 

#define SNAMETOOLONG -11 /**< Error Socket Path Too Long (exceeding UNIX_PATH_MAX) */


/** tipo descrittore canale di ascolto (server) */
typedef unsigned int serverChannel_t;

/** tipo descrittore del canale di comunicazione (server e client) */
typedef unsigned int channel_t;



/* -= FUNZIONI =- */
/** Crea un channel di ascolto AF_UNIX
 *  \param  path pathname del canale di ascolto da creare
 *  \return
 *     - s    il descrittore del channel di ascolto  (s>0)
 *     - -1   in casi di errore (sets errno)
 *     - SNAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 */
serverChannel_t createServerChannel(const char* path);

/** Distrugge un channel di ascolto
 *   \param s il channel di ascolto da distruggere
 *   \return
 *         -  0  se tutto ok, 
 *         - -1  se errore (sets errno)
 */
int closeSocket(serverChannel_t s);

/** accetta una connessione da parte di un client
 *   \param  s channel di ascolto su cui si vuole ricevere la connessione
 *   \return
 *       - c il descrittore del channel di trasmissione con il client 
 *       - -1 in casi di errore (sets errno)
 */
channel_t acceptConnection(serverChannel_t s);

/** legge un messaggio dal channel di trasmissione
 *  \param  sc  channel di trasmissione
 *  \param msg  struttura che conterra' il messagio letto 
 *		(deve essere allocata all'esterno della funzione,
 *		tranne il campo buffer)
 *  \return
 *     -  -1    se errore (errno settato),
 *     -  SEOF  se EOF sul socket  
 *     -  lung  lunghezza del buffer letto, se OK
 */
int receiveMessage(channel_t sc, message_t *msg);

/** scrive un messaggio sul channel di trasmissione
 *   \param  sc channel di trasmissione
 *   \param msg struttura che contiene il messaggio da scrivere 
 *   \return
 *     -  -1   se errore (sets errno) 
 *     -  SEOF se EOF sul socket
 *     -  n    il numero di caratteri inviati (se OK)
 */
int sendMessage(channel_t sc, message_t *msg);

/** Chiude un socket
 *   \param  sc il descrittore del channel da chiudere 
 *   \return
 *       - 0  se tutto ok
 *       - -1 se errore (sets errno)
*/
int closeConnection(channel_t sc);

/** crea un channel di trasmissione verso il server
 *   \param  path  nome del server socket
 *   \return
 *      -  c (c>0) il channel di trasmissione 
 *      - -1 in casi di errore (sets errno)
 *      - SNAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 */
channel_t openConnection(const char* path);

#endif
