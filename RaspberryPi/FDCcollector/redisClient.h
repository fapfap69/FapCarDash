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
#include <time.h>
#include <signal.h>
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#include "Includes/hiredis.h"

#include "Includes/gps.h"
#include "Includes/gpsd_config.h"
#include "Includes/gpsdclient.h"
// #include "revision.h"




// -- constants ---
#define REDISERVER_HOSTNAME "127.0.0.1"
#define REDISERVER_PORT 6379
#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 500000

#define POLLING_DELAY_USEC 1000000


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


#define RC_FLOATTYPE "2"
#define RC_INTTYPE "1"
#define RC_STRINGTYPE "3"


// GPSD definitions DEFAULT_GPSD_PORT
#define FCD_GPSD_SERVERPORT "localhost:2947"
#define FCD_GPSWAITINGTIMEOUT 500


#define FCD_ARDU_WRITEATTEMPT 3
#define FCD_ARDU_RXBUFFERLEN 256
#define FCD_ARDU_VALUESDELIMITER ','
#define FCD_ARDU_STATE_IDLE 0
#define FCD_ARDU_STATE_WAITREPLACE 1


// functions prototype
int arduinoOpenStream(void);
int arduinoCloseStream(void);
int arduinoCollectData(void);
int arduinoWriteCommand(char *);
int arduinoReceiveResponse(char *);

int gpsdOpenStream(void);
int gpsdCloseStream(void);
int GPScollectData(void);

int __recordIntValue(int, char *);
int __recordFloatValue(float, char *);
int __recordStringValue(char *, char *);
int __recordParseValue(char **, const char *);
void __executeCommand(char *);

static void _fcdCBSignalHandler(int);
void fcdLog(char *, char *, int);
void fcdLogErr(char *, char *);
void fcdSplash(void);

#endif


// ============ EOF =====================
