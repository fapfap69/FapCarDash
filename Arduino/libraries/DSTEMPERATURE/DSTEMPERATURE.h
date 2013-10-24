/*
DsTemperature.h - Header file for the SensorLib.

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/

#ifndef DSTEMPERATURE_DEF
#define DSTEMPERATURE_DEF

#include <Arduino.h>
#include <OneWire.h>

#define DSOK 0
#define DSERR_BADDEVICEID 1
#define DSERR_NOMOREADDRESS 2
#define DSERR_BADCRCCHECK 3
#define DSERR_UNKNOWNDEVICE 4

#define MAXSENSORS 10



struct Temperaturesensor {

	int Status;
	byte Type;
	byte Address[8];
	float TemperatureC;
	float TemperatureF;

};

class DSTEMPERATURE {

	public:
		DSTEMPERATURE(int);
		int getDevicesOnBus();
		int readDevice(int);
		void readDevices();
		float getTemperatureC(int);
		float getTemperatureF(int);

	private:

		OneWire  devices;
		Temperaturesensor* sensors[MAXSENSORS];
		int NumOfDevices;
};

#endif
