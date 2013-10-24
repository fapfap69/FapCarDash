/*
ThreeAxisAcce.h - Header file for the SensorLib.

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/

#ifndef BAROMETER_DEF
#define BAROMETER_DEF

#include <Arduino.h>
#include <Wire.h>

// Barometer based on BMP085

#define WRITEREGISTER_ADD 0xF4
#define TEMPERATUREREGISTER_ADD 0xF6

#define RequestTemperature 0x2E

struct Parameters {
	int ac1;
	int ac2;
	int ac3;
	unsigned int ac4;
	unsigned int ac5;
	unsigned int ac6;
	int b1;
	int b2;
	int mb;
	int mc;
	int md;
	long b5; 
};


class BAROMETER
{
  public:
	BAROMETER();
	double readPressure();
	void calibration();
	void setBaseQuota(int quota);
	int getBaseQuota();

	double getPressure();
	double getPressureSL();
	double getTemperature();
	double getAltitude();

	double pressure;
	double temperature;
	double altitude;
	double pressureSlm;
	
  protected:
	unsigned long readPressureUnc();
	double calculateTemperature();
	int readTemperatureUnc();
	int readIntFrom(byte address);
	void writeTo(byte address, byte val);
	void readFrom(byte address, int num, byte _buff[]);
	double calculateAltitude(double pressure);

  private:
	Parameters calibParam;
	unsigned long uncompensatedPress;
	int baseQuota;

};

#endif
