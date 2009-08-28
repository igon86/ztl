#ifndef MACRO_H_
#define MACRO_H_

#include <stdlib.h>
#include <errno.h>

#define ec_meno1(s,m) \
	if ( (s) == -1 ) { perror(m); exit(errno); }
	
#define ec_null(s,m) \
	if ( (s) == NULL ) { perror(m); exit(errno); }
	
#endif /*MACRO_H_*/
