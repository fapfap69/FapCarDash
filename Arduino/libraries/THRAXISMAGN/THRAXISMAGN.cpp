/*
ThreeAxisMagn.cpp - Library definition.

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/


#include <Arduino.h> 
#include <Wire.h>
#include "THRAXISMAGN.h"

#define MAGCOMPENSATION_TIMEOUT 300;  // seconds (5 minutes)

// Class constructor
THRAXISMAGN::THRAXISMAGN()
{
	// reset the scale
	Scale = 0.92;
	MeasureMode = Measurement_SingleShot;
	Sensor = Magnetometer();
	CorrectionAngle = 0.052; // Bari correction factor
	return;
}


Magnetometer THRAXISMAGN::ReadMagnField()
{
	// Read the magnetic sensor
	if(MeasureMode == Measurement_SingleShot) setMeasurementMode(Measurement_SingleShot);

	uint8_t* buffer = Read(DataRegisterBegin, 6);

	Sensor.XRaw = (buffer[0] << 8) | buffer[1];
	Sensor.ZRaw = (buffer[2] << 8) | buffer[3];
	Sensor.YRaw = (buffer[4] << 8) | buffer[5];
	Sensor.XInt = Sensor.XRaw * Scale;
	Sensor.ZInt = Sensor.ZRaw * Scale;
	Sensor.YInt = Sensor.YRaw * Scale;
	
	double X2,Y2,Z2;
	X2 = Sensor.XInt * Sensor.XInt;
	Y2 = Sensor.YInt * Sensor.YInt;
	Z2 = Sensor.ZInt * Sensor.ZInt;

	Sensor.Intensity = sqrt(X2+Y2+Z2);
	Sensor.Inclination = asin(Sensor.ZInt / Sensor.Intensity);
	Sensor.Declination = atan2(Sensor.YInt, Sensor.XInt);

	return Sensor;
}

// return the intensity of the Magnetic field in Gauss
double THRAXISMAGN::getIntensity(void) 
{
	return(Sensor.Intensity / 1000.0);
}

double THRAXISMAGN::getInclinationRad(void)
{
	return(Sensor.Inclination);
}

// Return the inclination in Degrees
double THRAXISMAGN::getInclinationDeg(void)
{
	return(Sensor.Inclination * RAD2DEG);
}

// Return the Heading in degrees
double THRAXISMAGN::getHeading(double head) 
{
 	// Correct for when signs are reversed.
	if(head < 0) head += 2*PI;
	else if(head > 2*PI) head -= 2*PI;
	return(head * RAD2DEG);
}
// Return the magnetic North
double THRAXISMAGN::getHeadingMag(void) 
{
	return(getHeading(Sensor.Declination));
}
// Return the geographic North
double THRAXISMAGN::getHeadingGeo(void) 
{
	return(getHeading(Sensor.Declination + CorrectionAngle));
}

// Return the Declination in Radians
double THRAXISMAGN::getDeclinationRad(void)
{
	return( Sensor.Declination);
}

// Return the Declination in Degrees
double THRAXISMAGN::getDeclinationDeg(void)
{
	return( Sensor.Declination * RAD2DEG);
}


// Set the sensibility scale
void THRAXISMAGN::setScale(double gauss)
{
	uint8_t value = 0x00;

	if(gauss == 0.88) {
		value = 0x00;
		Scale = 0.73;
	} else if(gauss == 1.3) {
		value = 0x01;
		Scale = 0.92;
	} else if(gauss == 1.9) {
		value = 0x02;
		Scale = 1.22;
	} else if(gauss == 2.5) {
		value = 0x03;
		Scale = 1.52;
	} else if(gauss == 4.0) {
		value = 0x04;
		Scale = 2.27;
	} else if(gauss == 4.7) {
		value = 0x05;
		Scale = 2.56;
	} else if(gauss == 5.6) {
		value = 0x06;
		Scale = 3.03;
	} else if(gauss == 8.1) {
		value = 0x07;
		Scale = 4.35;
	} else {
		return;
	}
	
	// Arrange the value is in the top 3 bits of the register.
	value = value << 5;
	// and write 
	Write(ConfigurationRegisterB, value);
	return;
}

// Get the sensibility scale
double THRAXISMAGN::getScale(void)
{
	return(Scale);
}

// Set the measurement mode : Continuos / One Shot
void THRAXISMAGN::setMeasurementMode(uint8_t mode)
{
	Write(ModeRegister, mode);
	MeasureMode = mode;
	return;
}

// Set the measurement mode : Continuos / One Shot
uint8_t THRAXISMAGN::getMeasurementMode(void)
{
	return(MeasureMode);
}

// Write the register
void THRAXISMAGN::Write(int address, int data)
{
	Wire.beginTransmission(HMC5883L_Address);
	Wire.write(address);
	Wire.write(data);
	Wire.endTransmission();
	return;
}

// Read the Data Buffer
uint8_t* THRAXISMAGN::Read(int address, int length)
{
	Wire.beginTransmission(HMC5883L_Address);
	Wire.write(address);
	Wire.endTransmission();
  
	Wire.beginTransmission(HMC5883L_Address);
	Wire.requestFrom(HMC5883L_Address, length);

	uint8_t buffer[length];
	if(Wire.available() == length)  {
		for(uint8_t i = 0; i < length; i++) {
			buffer[i] = Wire.read();
		}
	}
	Wire.endTransmission();
	return buffer;
}

// Magnetic compensation
void THRAXISMAGN::compensateMagnField() {

	bool bFlagExit = true;
	bool bIncrease = false;
	int iCatched = 0;
	int iTimeCounter = 0;
	double X,Y,R, XM,YM,RM, Xm,Ym,Rm, Rp;

	ReadMagnField();

	Xm = XM = Sensor.XInt;
	Ym = YM = Sensor.YInt;
	Rm = RM = sqrt(X*X + Y*Y);
	Rp = Rm;

	while(bFlagExit) {

		ReadMagnField();
		X = Sensor.XInt;
		Y = Sensor.YInt;
		R = sqrt(X*X + Y*Y);

		//Detect a local minimum or maximum 
		if(R > Rp) {
			if(bIncrease == false) iCatched++;
			bIncrease = true;
		} else {
			if(bIncrease == true) iCatched++;
			bIncrease = false;
		}
		Rp = R;		
		// Calculate the absolute Minimum/Maximum
		if(R > RM) {
			XM = X;
			YM = Y;
			RM = R;
		}
		if(R < Rm) {
			Xm = X;
			Ym = Y;
			Rm = R;
		}
		delay(1000);
		iTimeCounter++;

		// Verify the exit of compesation
		if(iCatched >= 5) {
			// Done calculate the compensation
			bFlagExit = false;
		}
		if(iTimeCounter >= MAGCOMPENSATION_TIMEOUT) {
			// Time out !
			bFlagExit = false;
		}

	}

	Sensor.XComp = (XM - Xm) / 2.0;
	Sensor.YComp = (YM - Ym) / 2.0;
	return;

}

void THRAXISMAGN::setCompensation(double Xc, double Yc) {

	Sensor.XComp = Xc;
	Sensor.YComp = Yc;

	return;
}

void THRAXISMAGN::getCompensation(double *Xc, double *Yc) {

	*Xc = Sensor.XComp;
	*Yc = Sensor.YComp;
	
	return;
}

// Geographic correction
void THRAXISMAGN::setCorrectionAngle(int newAngle) 
{
	CorrectionAngle = (double)newAngle * DEG2RAD;
	return;
}

int THRAXISMAGN::getCorrectionAngle() 
{
	return((int)(CorrectionAngle * RAD2DEG));
}


