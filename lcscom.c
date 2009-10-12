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

#define TESTREAD(R,DIM) \
	if ( R == -1 ) return -1; \
	if ( R != DIM ) return SEOF;

#define TESTARG(A) \
	if ( !A ) { errno = EINVAL; return -1;}

#define TESTWRITE(W,DIM,B) \
	if ( W != DIM ) {return -1;} B +=W;

serverChannel_t createServerChannel(const char *path)
{
    serverChannel_t sck;
    struct sockaddr_un sa;

    TESTARG(path);

    /* controllo che il nome della socket non sia maggiore di UNIX_PATH_MAX */
    if (strlen(path) > UNIX_PATH_MAX) {
	errno = EINVAL;
	return SNAMETOOLONG;
    }

    /* inizializzo la struct indirizzo */
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if ((sck = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	return -1;
    }

    /* binding della socket */
    if (bind(sck, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	close(sck);
	return -1;
    }

    /* imposto il limite di connessioni */
    if (listen(sck, SOMAXCONN) == -1) {
	close(sck);
	return -1;
    }

    return sck;
}

int closeSocket(serverChannel_t s)
{
    unlink("./tmp/permsock");
    return close(s);
}

channel_t acceptConnection(serverChannel_t s)
{
    return accept(s, NULL, 0);
}



int receiveMessage(channel_t sc, message_t * msg)
{
    int red;

    TESTARG(msg);

    /* leggo il tipo del messaggio */
    red = read(sc, &(msg->type), 1);

    TESTREAD(red, 1);

    /* leggo la lunghezza del buffer */
    red = read(sc, &(msg->length), sizeof(int));

    TESTREAD(red, sizeof(int));

    if (msg->length > 0) {
	/* alloco la memoria per il corpo del messaggio */
	printf("Allocata la memoria bislacca\n");
	if (!(msg->buffer = (char *) malloc(msg->length))) {
	    errno = ENOMEM;
	    return -1;
	}

	red = read(sc, (msg->buffer), msg->length);

	TESTREAD(red, msg->length);
    }

    return red;
}



int sendMessage(channel_t sc, message_t * msg)
{
    int bytes, written;

    TESTARG(msg);

    bytes = written = 0;

    /*scrittura in ordine di tipo del messaggio, dimensione e corpo del messaggio stesso */
    if ((written = write(sc, &(msg->type), 1)) < 1) {
	return -1;
    }
    bytes = bytes + written;

    if ((written = write(sc, &(msg->length), sizeof(int))) < sizeof(int)) {
	return -1;
    }
    bytes = bytes + written;

    if ((written = write(sc, msg->buffer, msg->length)) < msg->length) {
	return -1;
    }
    bytes = bytes + written;

    return bytes;
}


int closeConnection(channel_t sc)
{
    return close(sc);
}

channel_t openConnection(const char *path)
{
    channel_t sck;
    struct sockaddr_un sa;

    TESTARG(path);

    if (strlen(path) > UNIX_PATH_MAX) {
	errno = EINVAL;
	return SNAMETOOLONG;
    }

    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);


    sck = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connect(sck, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	close(sck);
	return -1;
    }

    return sck;
}
