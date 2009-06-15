/** 
\file lcscom.c
\author	Andrea Lottarini
\brief libreria di comunicazione server client
 
 Libreria che definisce l'interfaccia di comunicazione fra client e server
(canali di comunicazione realizzati con socket AF_UNIX) 

Si dichiara che il contenuto di questo file e` in ogni sua parte opera
originale dell'autore.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lcscom.h"

#define ERROR(E,RET) { errno = E; return RET; }
#define LEGGI(FIELD,E,DIM) if ( ( size = read ( sc, E(msg->FIELD), DIM ) ) == -1 ) return -1; if ( size != DIM ) return SEOF;
#define SCRIVI(FIELD,DIM) if ( ( written = write ( sc, FIELD, DIM ) ) < DIM ) return -1; bytes += written;

#define UNIX_PATH_MAX 108

serverChannel_t createServerChannel ( const char* path ) {
	serverChannel_t sck;
	struct sockaddr_un sa;
	
	if ( ! path )
		ERROR(EINVAL,-1)
				
	/*controllo che il nome della socket non ecceda UNIX_PATH_MAX. */
	if ( strlen( path ) > UNIX_PATH_MAX )
		ERROR(EINVAL,SOCKNAMETOOLONG)
	
	/*inizializzo, in modo conforme alle specifiche, la struttura indirizzo. */
	strncpy ( sa.sun_path, path, UNIX_PATH_MAX );
	sa.sun_family = AF_UNIX;
	
	if ( (sck = socket ( AF_UNIX, SOCK_STREAM, 0)) == -1 ) 
		return -1; /*errore creazione socket. errno gia' settato. */
	
	if ( bind ( sck,(struct sockaddr *)&sa, sizeof(sa) ) == - 1) {
		close ( sck );
		return -1;
	}
	/*accetto un massimo di SOMAXCONN connessioni. */
	if ( listen ( sck, SOMAXCONN ) == -1 ) {
		close ( sck );
		return -1;
	}
	
	return sck;
}

int closeSocket ( serverChannel_t s ) {
	unlink ( TMP SKTNAME );
	return close ( s );
}

channel_t acceptConnection ( serverChannel_t s ) {
	return accept ( s, NULL, 0 );
}



int receiveMessage ( channel_t sc, message_t *msg ) {
	int size;
	
	if ( ! msg )
		ERROR(EINVAL,-1)
				
	LEGGI( type, &, 1 ) 			/*leggo il tipo del messaggio. */
	LEGGI( length, &, sizeof(int) )		/*leggo la lunghezza del buffer. */
				
	if ( ! (msg->buffer = (char*)malloc ( msg->length )) )
		ERROR(ENOMEM,-1)
	
	LEGGI( buffer, ,msg->length )

	return size;
}



int sendMessage ( channel_t sc, message_t *msg ) {
	int bytes, written;

	if ( ! msg )
		ERROR(EINVAL,-1)
	
	bytes = written = 0;				
	
	SCRIVI ( &(msg->type), 1 )			/*scrivo il tipo del messagio. */
	SCRIVI ( &(msg->length), sizeof(int) )		/*scrivo la lunghezza del buffer. */
	SCRIVI ( msg->buffer, msg->length )		/*scrivo i caratteri del buffer. */

	return bytes;
}


int closeConnection ( channel_t sc ) {
	return close ( sc );
}

channel_t openConnection ( const char* path ) {
	channel_t sck;
	struct sockaddr_un sa;
	
	if ( ! path )
		ERROR(EINVAL,-1)
				
	/*controllo che il nome della socket non ecceda UNIX_PATH_MAX. */
	if ( strlen( path ) > UNIX_PATH_MAX )
		ERROR(EINVAL,SOCKNAMETOOLONG)
	
	/*inizializzo, in modo conforme alle specifiche, la struttura indirizzo. */
	strncpy ( sa.sun_path, path, UNIX_PATH_MAX );
	sa.sun_family = AF_UNIX;
	
	sck = socket ( AF_UNIX, SOCK_STREAM, 0 );
	if ( connect ( sck, (struct sockaddr*) &sa, sizeof(sa) ) == -1 ) {
		close ( sck );
		return -1;
	}
	
	return sck;
}
