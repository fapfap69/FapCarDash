/*
DsTemperature.cpp - 

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/

#include "DSTEMPERATURE.h"

// Class constructor
DSTEMPERATURE::DSTEMPERATURE(int BusAddr) 
	: devices((uint8_t)BusAddr)
{
	return;
}

int DSTEMPERATURE::getDevicesOnBus()
{

	int Counter;
	byte addrBuffer[8];

	devices.reset_search();

	while(devices.search(addrBuffer)) {
		if(Counter < MAXSENSORS) {

			sensors[Counter] = (Temperaturesensor *) malloc(sizeof(Temperaturesensor));		
			memcpy(sensors[Counter]->Address, addrBuffer, 8);
			sensors[Counter]->Status = -1;
			Counter++;
		}
	}
	devices.reset_search();

	NumOfDevices = Counter;
	return(Counter);
}

float DSTEMPERATURE::getTemperatureC(int deviceId)
{
	// is the Id valid
	if(deviceId >= NumOfDevices || deviceId < 0)
		return(-273.0);

	return(sensors[deviceId]->TemperatureC);
}

float DSTEMPERATURE::getTemperatureF(int deviceId)
{
	// is the Id valid
	if(deviceId >= NumOfDevices || deviceId < 0)
		return(-273.0);

	return(sensors[deviceId]->TemperatureF);
}

int DSTEMPERATURE::readDevice(int deviceId)
{

	byte sensorType;

	byte result = 0;
	byte i;
	byte dataBuffer[12];
	byte crc;
	unsigned int rawValue;

	// is the Id valid
	if(deviceId >= NumOfDevices || deviceId < 0)
		return(DSERR_BADDEVICEID);

	// distinguish the type
	switch (sensors[deviceId]->Address[0]) {
		case 0x10:
			sensorType = 1;  // or old DS1820
			break;
		case 0x28:
			sensorType = 0;  // Chip = DS18B20
			break;
		case 0x22:
			sensorType = 0; // Chip = DS1822
			break;
		default:
			sensorType = -1; // Unknown
			return(DSERR_UNKNOWNDEVICE);
	} 

	// Start the conversion
	devices.reset();
	devices.select(sensors[deviceId]->Address);
	devices.write(0x44,1);
  	delay(200); // ex 1000
	result = devices.reset();
	

	// Now read the raw data
	devices.select(sensors[deviceId]->Address);    
	devices.write(0xBE);
	for (i=0; i<9; i++) { 
		dataBuffer[i] = devices.read();
	}
	// check
	crc = OneWire::crc8(dataBuffer, 8);
	if (crc != dataBuffer[8]) {
		return(DSERR_BADCRCCHECK);
	}

	// Convert the row value
	rawValue = (dataBuffer[1] << 8) | dataBuffer[0]; //2bytes -> 1word
	if (sensorType) {
		rawValue = rawValue << 3; // 9 bit resolution default
		if (dataBuffer[7] == 0x10) { //  full 12 bit resolution
			rawValue = (rawValue & 0xFFF0) + 12 - dataBuffer[6];
  		}
	} else {      // default is 12 bit resolution, 750 ms conversion time
		byte cfg = (dataBuffer[4] & 0x60);
		if (cfg == 0x00) rawValue = rawValue << 3;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) rawValue = rawValue << 2; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) rawValue = rawValue << 1; // 11 bit res, 375 ms
	}


	// Store the vales
	sensors[deviceId]->TemperatureC = (float)rawValue / 16.0;
  	sensors[deviceId]->TemperatureF = sensors[deviceId]->TemperatureC * 1.8 + 32.0;
	
	return(DSOK);

}

void DSTEMPERATURE::readDevices() 
{
	byte i;
	for(i=0;i<NumOfDevices;i++) {
		readDevice(i);
	}
}
