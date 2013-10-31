/* ------------------------------------------

	redisClient.c
	
	Implements the Redis client counterpart 
	of the collector/dispatcher module
	
	First version : 0.1
	randomly generate data and sets ...

 	file : redisClient.h
 	ver 0.1 24/10/2013
 	auth : A.Franco


------------------------------------------- */




#include "hiredis.h"
#include "redisClient.h"

// global variables
redisContext *context;  // The connection context
redisReply *reply;  // the structure for the replays

struct sensorMeasure myMeasure[RC_MAXMEASUREDIM];


int main(void) {
    unsigned int j;
    bool bFlagRun = true;
    int iRunCounter = 0;
    
    struct timeval timeout = { TIMEOUT_SEC, TIMEOUT_USEC }; 
	
	// First of all try to connect to the server 
    context = redisConnectWithTimeout((char*) REDISERVER_HOSTNAME, REDISERVER_PORT, timeout);
    if (context->err) {
        printf("redisClient ::= Connection error: %s\n", context->errstr);
        printf("redisClient ::= Abort program !\n");
        exit(1);
    }

	// Now starts the main loop and fork two thread
	pid_t collectorPID;
    collectorPID = fork();

    if(collectorPID < 0)  { // fork fails
        printf("redisClient ::=  Fork failed, abort !\n");
        exit(1);
    }

    if(collectorPID == 0)  {// DB dispatcher
		while( bFlagRun ) {
			for(j=0; j<RC_MAXMEASUREDIM; j++) {
			printf(">%d=%d",j,myMeasure[j].type);
				if(myMeasure[j].type != RC_DATAINVALID) {	// Dispatch data
				    reply = redisCommand(context,"SET %s %s", myMeasure[j].name, myMeasure[j].stringValue);
				    myMeasure[j].type = RC_DATAINVALID;
				    freeReplyObject(reply);
				}
			}
			printf("\n");
			usleep( POLLING_DELAY_USEC );	
		
			// verify the SIGTERM in order to set bFlagRun = false
			
			
			// produce a heartbeat signal in order to flag the alive signal
			reply = redisCommand(context,"SET clientHBcounter %d", iRunCounter++);
		
	    	/* PING server */
    		reply = redisCommand(context,"PING");
    		if(strcmp( reply->str , "PONG") != 0) { // the server is bad
	        	printf("redisClient ::=  Server is sleeping or died !\n");	
    		}
    		freeReplyObject(reply);
		}
    }
    else { // Collector process
		while( bFlagRun ) {
	
			// Collect data	
			if( arduinoCollectData( myMeasure ) == RC_DATAVALID) {
			}

			// Sleep for a while
			usleep( POLLING_DELAY_USEC );	
		}
    }


    exit(0);
}


// obtains the new free slot in the array of measures
sensorMeasure * __searchFreeSlot(sensorMeasure *measureBuffer) {
	for(int j=0; j<RC_MAXMEASUREDIM; j++) {
		if(measureBuffer->type == RC_DATAINVALID) {	
			return(measureBuffer);
		}
		measureBuffer++;
	}
	return(NULL);
}

// ask for new measures
int arduinoCollectData(sensorMeasure *measureBuffer) {
	int iPos;
	sensorMeasure *freeSlot;
printf("entry\n");
	freeSlot = __searchFreeSlot(measureBuffer);
	if(freeSlot == NULL) { // there are not free slots
		return(RC_DATAINVALID);
	}	
	strcpy(freeSlot->name, "Temperature1");
	freeSlot->type = RC_FLOATTYPE;
	strcpy(freeSlot->stringValue, "15.4");
	freeSlot->floatValue = 15.4;
printf("eexit %s\n",freeSlot->name);	
		
	/*
	switch(measureBuffer->type) {
		case RC_INTTYPE:
			snprintf( measureBuffer->stringValue, 128, "%d", measureBuffer->intValue);
			break;
		case RC_FLOATTYPE:
			snprintf( measureBuffer->stringValue, 128, "%f", measureBuffer->floatValue);
			break;
		default:
			break;
	}			
	*/
	return(RC_DATAVALID);
	
}


