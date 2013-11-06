/* ------------------------------------------

	redisClient.c

	Implements the Redis client counterpart
	of the collector/dispatcher module

	First version : 0.2

 	file : redisClient.c
 	ver 0.1 24/10/2013
 	auth : A.Franco


------------------------------------------- */

#include "redisClient.h"
// constant
#define FCD_DEBUG 1

// global variables
static redisContext *context;  // The connection context
static redisReply *reply;  // the structure for the replays

static struct fixsource_t source;  // Source definition for gpsd
static struct gps_data_t gpsdata;  // Data structure for gpsd
static unsigned int flags = WATCH_ENABLE | WATCH_JSON; // base flags for streaming gpsd

static int uart0_filestream = -1;  // File handler fro the arduino UART connection
static unsigned char arduRxBuff[FCD_ARDU_RXBUFFERLEN]; // RX buffer
static unsigned char *arduPtrRxBuff = arduRxBuff; // RX buffer pointer
static int arduFSMState = FCD_ARDU_STATE_IDLE; // Arduino polling machine FSM Status

static bool bFlagRun = true;  // loop control flag


// --------  functions -----------------
// The logger
void fcdLog(char *msg) {
	if(FCD_DEBUG == 1)
		fprintf( stdout, "FCDcollector :: = %s\n",msg);
	return;
}
void fcdLogErr(char *msg, char *msgb) {
	fprintf( stderr, "FCDcollector Error = %s [%s]\n",msg, msgb);
	return;
}
// The splash screen
void fcdSplash(void) {
	printf("-------------------\n");
	printf("-   FapCarDash    -\n");
	printf("- data collector  -\n");
	printf("- ver.1.0 5/11/13 -\n");
	printf("-    A.Franco     -\n");
	printf("-------------------\n");
	return;
}

// The Hook handler for the OS Signals
static void _fcdCBSignalHandler(int signum)
{
    if (signum != SIGINT) {
    	fcdLog("Exiting, signal received"); //, signum));
    	bFlagRun = false;
	}
    return;
}


// ------ Arduino functions ----------------

// Open the Serial stream on the RaspPi UART
// pin8 UART0_TX and pin 10 UART0_RX
int arduinoOpenStream() {

	// First open the port
//  uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	uart0_filestream = open("/dev/ttyp2", O_RDWR | O_NOCTTY | O_NDELAY);		// the simulation port

	if(uart0_filestream == -1)	{ //ERROR - CAN'T OPEN SERIAL PORT
		fcdLogErr("Unable to open UART. Process abort!", "/dev/ttyp2");
		return(false);
	}

	// then configure the port
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;	// 9600,8bit,...
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

	// end
	return true;
}

// close the stream
int arduinoCloseStream() {
	close(uart0_filestream);
	return true;
}

// Write a command on the serial link
// the command is a string '\n' terminated
int arduinoWriteCommand(char *sCommand){

	int attempts = 0; // write attempts counter
	int commandLenght = strlen(sCommand);
	int bytesW = 0; // number of bytes written

	if (uart0_filestream != -1)	{
		while(attempts++ < FCD_ARDU_WRITEATTEMPT) { // loop for
			bytesW = write(uart0_filestream, sCommand, commandLenght);
			if (bytesW != commandLenght) { // there is an error
				fcdLogErr("UART0 error sending command on TX line.\n",NULL);
			} else {
				fcdLog("UART0 send command on TX line.");//,sCommand);
				return(true);
			}
		}
	}
	fcdLogErr("UART0 transmission aborted!",NULL);
	return(false);
}

// Read the RX line terminated to '\n'
// for incomplete string store the chunk in the buffer
int arduinoReceiveResponse(char *ptrOut) {

	int lenOfBuffer=0;
	int bytesRead=0;

	if (uart0_filestream != -1) {  // the port is closed
		lenOfBuffer = FCD_ARDU_RXBUFFERLEN - (arduPtrRxBuff - arduRxBuff +1); // calculate the remained space

		if(lenOfBuffer < 2) { // the buffer is full then reset it
			arduPtrRxBuff = arduRxBuff;
			lenOfBuffer = FCD_ARDU_RXBUFFERLEN-1;
		}

		bytesRead = read(uart0_filestream, (void*)arduPtrRxBuff, lenOfBuffer);
		if (bytesRead < 0)	{ // seems an error but ...
			fcdLog("UART0 RX error");
			return(false);
		} else if (bytesRead == 0) {
			//No data...
		} else {
			//Data received
			fcdLog("UART0 RX receive bytes");
			fcdLog((char *)(arduPtrRxBuff));

			// Critichal : verify that the received string is '\n' terminated
			if((char)(*(arduPtrRxBuff + bytesRead - 1)) == '\n') { // It is good
				*(arduPtrRxBuff + bytesRead - 1) = '\0';  // convert it into a string
				strcpy(ptrOut, (char *)arduRxBuff);  // return the received string
				arduPtrRxBuff = arduRxBuff;  // reset the read buffer pointer
				return(true);
			}
			arduPtrRxBuff = arduPtrRxBuff + bytesRead + 1; // Move the pointer for the next chunk
			fcdLog("UART0 RX receive incomplete string");
		}
	}
	return(false);
}

// The Request/Response program for Arduino link
// it acts as a Finite State Machine
int arduinoCollectData() {

	char sBuffer[FCD_ARDU_RXBUFFERLEN];
    char *ptTok;

	switch(arduFSMState) {
	case FCD_ARDU_STATE_IDLE:
    	if(arduinoWriteCommand("V\n"))
    		arduFSMState = FCD_ARDU_STATE_WAITREPLACE;
    	break;

	case FCD_ARDU_STATE_WAITREPLACE:
		if(arduinoReceiveResponse(sBuffer)) { // We receive
	   		arduFSMState = FCD_ARDU_STATE_IDLE;
	    	ptTok = sBuffer;
	    	// Parse the received string and record into RedisDB the values
		    if(__recordParseValue(&ptTok, NULL) == RC_DATAINVALID) return(RC_DATAINVALID); // The time stamp
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
		}
		break;
	default:
		break;
	}
	return(RC_DATAVALID);
}

// -----------  Redis interface functions -------------------
//  records the float value into the DB
int __recordFloatValue(float fValue, char *sName) {
	// verify the syntax...
    if(sName == NULL) return(RC_DATAINVALID);
	reply = redisCommand(context,"SET %s %f", sName, fValue);
    freeReplyObject(reply);
    return(RC_DATAVALID);
}
//  records the int value into the DB
int __recordIntValue(int iValue, char *sName) {
	// verify the syntax...
    if(sName == NULL) return(RC_DATAINVALID);
	reply = redisCommand(context,"SET %s %d", sName, iValue);
    freeReplyObject(reply);
    return(RC_DATAVALID);
}
//  records the string value into the DB
int __recordStringValue(char *sValue, char *sName) {
	// verify the syntax...
    if(sName == NULL || sValue == NULL) return(RC_DATAINVALID);
	reply = redisCommand(context,"SET %s %s", sName, sValue);
    freeReplyObject(reply);
    return(RC_DATAVALID);
}
// parse the string delimited and records the value into the DB
int __recordParseValue(char **ptTok, const char *sName) {
	char sValue[45];
	char *ptStart = *ptTok;

    if(*ptTok != NULL) {
    	while(**ptTok != '\0' && **ptTok != FCD_ARDU_VALUESDELIMITER) (*ptTok)++; // search the delimiter
    	**ptTok = '\0'; // truncate the substring
    	(*ptTok)++;
    	strncpy(sValue,ptStart, 45);  // copy in the out buffer
    } else {
    	return(RC_DATAINVALID);
    }
    // if the name pointer is NULL skip the recording	
    if(sName != NULL) { // record into the DB
		reply = redisCommand(context,"SET %s %s", sName, sValue);
    	freeReplyObject(reply);
    }
    return(RC_DATAVALID);
}	 
    
// --------  GPSD interface functions -------------------

// Open the stream with the GPSd socket
int gpsdOpenStream() {

	// ... to verify what version
	gpsd_source_spec(NULL, &source); // GPSD_SERVERPORT, &source);
	// gpsd_source_spec(FCD_GPSD_SERVERPORT, &source);
	//
	// Open the stream
    if (gps_open(source.server, source.port, &gpsdata) != 0) {
    	fcdLogErr("No GPSD daemon running or network !",(char *)gps_errstr(errno));
		return(false);
    }
    // If the device is defined then add a flag
    if (source.device != NULL) flags |= WATCH_DEVICE;
    // Finally connect the device to the stream
    (void)gps_stream(&gpsdata, flags, source.device);
	return true;
}

// Close the stream
int gpsdCloseStream() {
	(void)gps_stream(&gpsdata, WATCH_DISABLE, NULL);
    (void)gps_close(&gpsdata);
	return true;
}

// This is the main polling function
int GPScollectData() {

	char timeStamp[30];

	if (gps_waiting(&gpsdata, FCD_GPSWAITINGTIMEOUT)) {  // Are there data ?
        if (gps_read(&gpsdata) != -1) { // Read data
            if(gpsdata.online != 0) { // GPS not online
	            if(gpsdata.status != STATUS_NO_FIX) { // We have a fix point
					__recordStringValue("FIXED", "GPSstatus"); // set into DB
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
	return(RC_DATAVALID);
}

// -------------------------------------
// -------- main program ---------------
int main(void) {

    //   catch all interesting signals
    (void)signal(SIGTERM, _fcdCBSignalHandler);
    (void)signal(SIGQUIT, _fcdCBSignalHandler);
    (void)signal(SIGINT, _fcdCBSignalHandler);


	// First of all try to connect to the Redis DB server
    struct timeval timeout = { TIMEOUT_SEC, TIMEOUT_USEC };
   	context = redisConnectWithTimeout((char*) REDISERVER_HOSTNAME, REDISERVER_PORT, timeout);
   	if (context->err) {
   		fcdLogErr("Redis DB server connection error:",context->errstr);
   		fcdLogErr("Abort program !", NULL);
       	exit(1);
   	}

	// variables
    int iRunCounter = 0;  // HeartBeat variable

    // Polling time cycle duration variables
    clock_t tStartRread;
    clock_t tEndRead;
    long lDelay;
    float fClocsPerMicroSeconds = CLOCKS_PER_SEC / 1000000.0;


    // Init phase Open & Init streams
    if (!arduinoOpenStream()) {
    	fcdLogErr("Arduino not connected. Abort!", NULL);
       	exit(1);
    }
    if (!gpsdOpenStream()) {
    	fcdLogErr("GPSD not present !", NULL);
    }

    // loop ....
	while( bFlagRun ) {

		// timing
    	tStartRread = clock();

		// Collect data from arduino
		arduinoCollectData();

    	// Collect data from GPS
   		GPScollectData();

		// calculate the delay ...
		tEndRead = clock();
		lDelay = POLLING_DELAY_USEC - ((float)(tEndRead - tStartRread) / fClocsPerMicroSeconds);
		if(lDelay > 0) usleep( lDelay );

		// produce a heartbeat signal in order to flag the alive signal
		reply = redisCommand(context,"SET clientHBcounter %d", iRunCounter++);

	    // PING the Redis DB server
    	reply = redisCommand(context,"PING");
    	if(strcmp( reply->str , "PONG") != 0) { // the server is bad
    		fcdLogErr("Redis DB server is sleeping or died !", NULL);
    	}
    	freeReplyObject(reply);

    }  // end of main loop

	// Disarm the system
	arduinoCloseStream();
	gpsdCloseStream();

	fcdLog("Bye bye !");
    exit(0);
}



