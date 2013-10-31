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


#include "redisClient.h"

// global variables
static redisContext *context;  // The connection context
static redisReply *reply;  // the structure for the replays

//typedef double timestamp_t;

static struct fixsource_t source;
static struct gps_data_t gpsdata;

static char sBuffer[256];
static bool bFlagRun = true;


int main(void) {


    /* catch all interesting signals */
    (void)signal(SIGTERM, quit_handler);
    (void)signal(SIGQUIT, quit_handler);
    (void)signal(SIGINT, quit_handler);


	// First of all try to connect to the server
    struct timeval timeout = { TIMEOUT_SEC, TIMEOUT_USEC };
   	context = redisConnectWithTimeout((char*) REDISERVER_HOSTNAME, REDISERVER_PORT, timeout);
   	if (context->err) {
    	printf("redisClient ::= Connection error: %s\n", context->errstr);
       	printf("redisClient ::= Abort program !\n");
       	exit(1);
   	}

	// Manager Status and ...
    int iRunCounter = 0;
//    int iManagerStatus = 0;

	// String buffers         
        
    // Writing cycle duration
    clock_t tStartRread;
    clock_t tEndRead;
    long lDelay;
    float fClocsPerMicroSeconds = CLOCKS_PER_SEC / 1000000.0;
    	
    // loop ....
	while( bFlagRun ) {
    
    	tStartRread = clock();
            
		// Collect data from arduino
		if( arduinoCollectData() == RC_DATAVALID) {
		}
    
    	// Collect data from GPS
   		if(GPScollectData()  == RC_DATAVALID) {
   		}
 
		// calculate the delay ...
		tEndRead = clock();
		lDelay = POLLING_DELAY_USEC - ((float)(tEndRead - tStartRread) / fClocsPerMicroSeconds);
		if(lDelay > 0) usleep( lDelay );

		// produce a heartbeat signal in order to flag the alive signal
		reply = redisCommand(context,"SET clientHBcounter %d", iRunCounter++);

	    /* PING server */
    	reply = redisCommand(context,"PING");
    	if(strcmp( reply->str , "PONG") != 0) { // the server is bad
	       	printf("redisClient ::=  Server is sleeping or died !\n");
    	}
    	freeReplyObject(reply);
    }  // end of main loop 
    exit(0);
}


// Quitter ....
static void quit_handler(int signum)
{
    if (signum != SIGINT)
	printf("exiting, signal %d received", signum);
    bFlagRun = false;
    return;
}


// ask for new measures
int arduinoCollectData() {

    char *ptTok;
	
	// read the string from arduino 
	strcpy( sBuffer, "12.001545,15.254201,85.2587,18.158425,25.2587,36.3698,1005.258711,980.258701,25.2587,18.2589,36.2587,1254.1258,32.2587,-2.2586,34.2540,25.21,5.36");

    ptTok = strtok(sBuffer, ",");
	if(__recordParseValue(&ptTok, "Temp1") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, "Temp2") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, "Humid") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // The temp sensor of Higrometer
	if(__recordParseValue(&ptTok, "Devpoint") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // Dev.Point fast
	if(__recordParseValue(&ptTok, "Pressure") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, "PressureSL") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, "Altitude") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // Temperature of barometric sensor
	if(__recordParseValue(&ptTok, "Compass") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // "MagIntensity"
	if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // Declination
	if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // Inclination
	if(__recordParseValue(&ptTok, "GeoNorth") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, "Roll") == RC_DATAINVALID) return(RC_DATAINVALID);
	if(__recordParseValue(&ptTok, "Pitch") == RC_DATAINVALID) return(RC_DATAINVALID);
//	if(__recordValue(ptTok, "Batt1V") == RC_DATAINVALID) return(RC_DATAINVALID);
//	if(__recordValue(ptTok, "Batt1A") == RC_DATAINVALID) return(RC_DATAINVALID);
//	if(__recordValue(ptTok, "Batt2V") == RC_DATAINVALID) return(RC_DATAINVALID);
//	if(__recordValue(ptTok, "Batt2A") == RC_DATAINVALID) return(RC_DATAINVALID);

	return(RC_DATAVALID);

}

//  records the float value into the DB
int __recordFloatValue(float fValue, char *sName) {
	// verify the syntax...
    if(sName == NULL) return(RC_DATAINVALID);

	reply = redisCommand(context,"SET %s %f", sName, fValue);
    freeReplyObject(reply);
    return(RC_DATAVALID);
}
//  records the float value into the DB
int __recordIntValue(int iValue, char *sName) {
	// verify the syntax...
    if(sName == NULL) return(RC_DATAINVALID);

	reply = redisCommand(context,"SET %s %d", sName, iValue);
    freeReplyObject(reply);
    return(RC_DATAVALID);
}
//  records the vale into the DB
int __recordStringValue(char *sValue, char *sName) {

	// verify the syntax...
    if(sName == NULL || sValue == NULL) return(RC_DATAINVALID);

	reply = redisCommand(context,"SET %s %s", sName, sValue);
    freeReplyObject(reply);
    return(RC_DATAVALID);
}
// parse the comma separated string and records the vale into the DB
int __recordParseValue(char **ptTok, char *sName) {
	char sValue[45];
	
	// verify the syntax...
    if(*ptTok != NULL)
    	strcpy(sValue,*ptTok);
    else
    	return(RC_DATAINVALID);
    // if the name pointer is NULL skip the recording	
    if(sName != NULL) { // record into the DB
		reply = redisCommand(context,"SET %s %s", sName, sValue);
    	freeReplyObject(reply);
    }
    *ptTok = strtok(NULL, ",");
    return(RC_DATAVALID);
}	 
    

int GPScollectData() {

	char timeStamp[30];
	unsigned int flags = WATCH_ENABLE;
    
	// if specified 
	gpsd_source_spec(GPSD_SERVERPORT, &source);

    if (gps_open(source.server, source.port, &gpsdata) != 0) {
		fprintf(stderr, "GPScollectData ::= no gpsd running or network error: %d, %s\n", errno, gps_errstr(errno));
		return(RC_DATAINVALID);
    }

    if (source.device != NULL) flags |= WATCH_DEVICE;

    (void)gps_stream(&gpsdata, flags, source.device);

	if (gps_waiting(&gpsdata, 500)) {
        if (gps_read(&gpsdata) != -1) {
            /* Display data from the GPS receiver. */
            if(gpsdata.online != 0) { // GPS not online
	            if(gpsdata.status != STATUS_NO_FIX) { // We have a fix point
					__recordStringValue("FIXED", "GPSstatus");
				    // gpsdata.satellites_used
					if( gpsdata.fix.mode ==  MODE_2D || gpsdata.fix.mode ==  MODE_3D) {
						unix_to_iso8601(gpsdata.fix.time, timeStamp, sizeof(timeStamp));
						__recordStringValue(timeStamp, "GPStime");
						__recordFloatValue(gpsdata.fix.latitude, "GPSlatit");
						__recordFloatValue(gpsdata.fix.longitude, "GPSlongi");
					}
					if( gpsdata.fix.mode ==  MODE_3D) {
						__recordFloatValue(gpsdata.fix.altitude, "GPSaltit");
						__recordFloatValue(gpsdata.fix.climb, "GPSclimb");
						__recordFloatValue(gpsdata.fix.speed, "GPSspeed");
						__recordFloatValue(gpsdata.fix.track, "GPSlatit");
						__recordFloatValue(gpsdata.fix.latitude, "GPStrack");
					}
				} else {
					__recordStringValue("FIXING", "GPSstatus");
				}
			} else {
				__recordStringValue("OFFLINE", "GPSstatus");
			}
        } else {
			__recordStringValue("NODATA", "GPSstatus");
        }
    } else {
		__recordStringValue("NODEVICE", "GPSstatus");
    }

	(void)gps_stream(&gpsdata, WATCH_DISABLE, NULL);
    (void)gps_close(&gpsdata);

	return(RC_DATAVALID);

}
