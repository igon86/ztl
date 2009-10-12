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

static ssize_t Read(int fd, const void *buf, size_t nbyte)
{
    ssize_t nread = 0, n;
    printf("READ MODIFICATA\n");
    do {
	if ((n = read(fd, &((char *) buf)[nread], nbyte - nread)) == -1) {
	    /* "EINTR is not really an error */
	    if (errno == EINTR) {
		continue;
	    }
	    /* ERROR */
	    else {
		return -1;
	    }
	}
	/* EOF */
	if (n == 0) {
	    return nread;
	}
	nread = nread + n;
    } while (nread < nbyte);

    return nread;
}

static ssize_t Write(int fd, const void *buf, size_t nbyte)
{
    int nwritten = 0, n;
    printf("Write MOdificata\n");
    do {
	if ((n =
	     write(fd, &((const char *) buf)[nwritten],
		   nbyte - nwritten)) == -1) {
	    if (errno == EINTR) {
		continue;
	    } else {
		return -1;
	    }
	}
	nwritten = nwritten + n;
    } while (nwritten < nbyte);
    return nwritten;
}

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
    red = Read(sc, &(msg->type), 1);

    TESTREAD(red, 1);

    /* leggo la lunghezza del buffer */
    red = Read(sc, &(msg->length), sizeof(int));

    TESTREAD(red, sizeof(int));

    if (msg->length > 0) {
	/* alloco la memoria per il corpo del messaggio */
	if (!(msg->buffer = (char *) malloc(msg->length))) {
	    errno = ENOMEM;
	    return -1;
	}

	red = Read(sc, (msg->buffer), msg->length);

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
    if ((written = Write(sc, &(msg->type), 1)) < 1) {
	return -1;
    }
    bytes = bytes + written;

    if ((written = Write(sc, &(msg->length), sizeof(int))) < sizeof(int)) {
	return -1;
    }
    bytes = bytes + written;

    if ((written = Write(sc, msg->buffer, msg->length)) < msg->length) {
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
