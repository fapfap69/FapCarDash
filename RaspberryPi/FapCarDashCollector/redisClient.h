/* ----------------------

	Header file for the redisClient.c
	module
	

 	file : redisClient.h
 	ver 0.1 24/10/2013
 	auth : A.Franco
 	
----------------------- */

#ifndef __REDISCLIENT_H

#define __REDISCLIENT_H
#include <stdio.h> /* for size_t */
#include <stdarg.h> /* for va_list */
#include <sys/time.h> /* for struct timeval */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

// -- constants ---
#define REDISERVER_HOSTNAME "127.0.0.1"
#define REDISERVER_PORT 6379
#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 500000

#define POLLING_DELAY_USEC 5000000


typedef struct sensorMeasure {
    char name[32];
    int type;
    int intValue;
    float floatValue;
    char stringValue[128];   
} sensorMeasure;

#define RC_MAXMEASUREDIM 20

#define RC_DATAVALID 1
#define RC_DATAINVALID 0


#define RC_FLOATTYPE 2
#define RC_INTTYPE 1
#define RC_SRE

// functions prototype
int arduinoCollectData(sensorMeasure *);
sensorMeasure * __searchFreeSlot(sensorMeasure *);



#endif






// ============ EOF =====================