#ifndef _MICRORL_MISC_H_
#define _MICRORL_MISC_H_
#include <Arduino.h>
#include "config.h"
#include <string.h>
#include <stdlib.h>
extern "C" {
    #include "microrl.h"
}

extern microrl_t * prl;

// print to stream callback
void print (void * pThis, const char * str);

// execute callback
int execute (void * pThis, int argc, const char * const * argv);

// completion callback
char ** complet (void * pThis, int argc, const char * const * argv);

// ctrl+c callback
void sigint (void * pThis);

#endif /* _MICRORL_MISC_H_ */
