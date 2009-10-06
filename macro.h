#ifndef MACRO_H_
#define MACRO_H_

#include <stdlib.h>
#include <errno.h>

#define ec_meno1(s,m) \
	if ( (s) == -1 ) { perror(m); exit(errno); }

#define ec_meno1_c(s,m,c) \
	if ( (s) == -1 ) { perror(m); c; }
	
#define ec_null(s,m) \
	if ( (s) == NULL ) { perror(m); exit(errno); }

#define ec_non0(s,m) \
	if ( (s) != 0 ) { perror(m); exit(errno); }
	
#endif /*MACRO_H_*/
