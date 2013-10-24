/*
ThreeAxisAcce.cpp - File for the SensorLib.

Author A.Franco
Date 31/01/2013
Ver. 0.1

 Cabling for i2c using Sparkfun breakout with an Arduino Mega / Mega ADK:
 Arduino <-> Breakout board
 Gnd      -  GND
 3.3v     -  VCC
 3.3v     -  CS
 20       -  SDA
 21       -  SLC

*/

#include "Arduino.h"
#include "THRAXISACCE.h"
#include <Wire.h>

#define ADXL345_DEVICE (0x53)    // ADXL345 device address

// Class constructor
THRAXISACCE::THRAXISACCE() {
	status = ADXL345_OK;
	errorCode = ADXL345_NO_ERROR;
	
	accelero = Accelerometer();

	// set the default gains
	accelero.Xgain = 0.00376390;
	accelero.Ygain = 0.00376009;
	accelero.Zgain = 0.00349265;

	return;
}
// -----------------------  Power section ------------
// Switch On the Chip
void THRAXISACCE::powerOn() {
	//Turning on the ADXL345
	writeTo(ADXL345_POWER_CTL, 0);      
	writeTo(ADXL345_POWER_CTL, 16);
	writeTo(ADXL345_POWER_CTL, 8); 
	return;
}

bool THRAXISACCE::isLowPower(){ 
	return getRegisterBit(ADXL345_BW_RATE, 4); 
}
void THRAXISACCE::setLowPower(bool state) {  
	setRegisterBit(ADXL345_BW_RATE, 4, state); 
}
double THRAXISACCE::getRate(){
	byte buffer;
	readFrom(ADXL345_BW_RATE, 1, &buffer);
	buffer &= B00001111;
	return (pow(2,((int) buffer)-6)) * 6.25;
}
void THRAXISACCE::setRate(double rate){
	byte readbuffer;
	byte writebuffer;
	int v = (int) (rate / 6.25);
	int r = 0;
	while(v >>= 1)	{
		r++;
	}
	if(r <= 9) { 
		readFrom(ADXL345_BW_RATE, 1, &readbuffer);
		writebuffer = (byte) (r + 6) | (readbuffer & B11110000);
		writeTo(ADXL345_BW_RATE, writebuffer);
	}
	return;
}

// ---------------- Accelerator section --------------
Accelerometer THRAXISACCE::readAccelerometer() {

	byte buffer[6];

	readFrom(ADXL345_DATAX0, 6, buffer); 
	
	// converting two bytes in to one int
	accelero.Xraw = (((int)buffer[1]) << 8) | buffer[0];   
	accelero.Yraw = (((int)buffer[3]) << 8) | buffer[2];   
	accelero.Zraw = (((int)buffer[5]) << 8) | buffer[4];   

	// convert raw value into eng. values
	accelero.Xint = accelero.Xraw * accelero.Xgain;
	accelero.Yint = accelero.Yraw * accelero.Ygain;
	accelero.Zint = accelero.Zraw * accelero.Zgain;

	// calculate roll & pitch
	calculateRollPitch();

	return(accelero);
}

void THRAXISACCE::calculateRollPitch() {

	double x_val, y_val, z_val;
	double x2, y2, z2; 

	x_val = (double)(accelero.Xint);
	y_val = (double)(accelero.Yint);
	z_val = (double)(accelero.Zint);

	x2 = (x_val*x_val);
	y2 = (y_val*y_val);
	z2 = (z_val*z_val);

	accelero.pitch = atan2( x_val , sqrt(y2+z2)) * 57.2957795133 - accelero.pitchBase;
	accelero.roll = atan2( y_val , sqrt(x2+z2)) * 57.2957795133 - accelero.rollBase;
	return;

}



void THRAXISACCE::setOffset(){

	// first reset the Offset
	accelero.rollBase = 0.0;
	accelero.pitchBase = 0.0;
	// calculate the assolute
	calculateRollPitch();
	// set the new offset
	accelero.rollBase = accelero.roll;
	accelero.pitchBase = accelero.pitch;
	return;
}

void THRAXISACCE::setAxisOffset(double rollOffset, double pitchOffset)
{
	accelero.rollBase = rollOffset;
	accelero.pitchBase = pitchOffset;
	return;
}

void THRAXISACCE::getAxisOffset(double* rollOffset, double* pitchOffset)
{
	*rollOffset = accelero.rollBase;
	*pitchOffset = accelero.pitchBase;
	return;
}

double THRAXISACCE::getRoll(void) {
	return(accelero.roll);
}

double THRAXISACCE::getPitch(void) {
	return(accelero.pitch);
}

// -------------- Gain section -------------------
void THRAXISACCE::setAxisGains(double Xgain, double Ygain, double Zgain)
{
	accelero.Xgain = Xgain;
	accelero.Ygain = Ygain;
	accelero.Zgain = Zgain;
	return;
}
void THRAXISACCE::getAxisGains(double* Xgain, double* Ygain, double* Zgain)
{
	*Xgain = accelero.Xgain;
	*Ygain = accelero.Ygain;
	*Zgain = accelero.Zgain;
	return;
}
byte THRAXISACCE::getRangeSetting(void) {
	byte buffer;
	readFrom(ADXL345_DATA_FORMAT, 1, &buffer);
	return(buffer & B00000011);
}
void THRAXISACCE::setRangeSetting(int val) {
	byte writebuffer;
	byte readbuffer;
	
	switch (val) {
		case 2:  
			writebuffer = B00000000; 
			break;
		case 4:  
			writebuffer = B00000001; 
			break;
		case 8:  
			writebuffer = B00000010; 
			break;
		case 16: 
			writebuffer = B00000011; 
			break;
		default: 
			writebuffer = B00000000;
	}
	readFrom(ADXL345_DATA_FORMAT, 1, &readbuffer);
	writebuffer |= (readbuffer & B11101100);
	writeTo(ADXL345_DATA_FORMAT, writebuffer);
	return;
}

// ----  Private functions -----------------------------------------
// Writes val to address register on device
void THRAXISACCE::writeTo(byte address, byte val) {
	Wire.beginTransmission(ADXL345_DEVICE);
	Wire.write(address);
	Wire.write(val);
	Wire.endTransmission();
	return;
}

// Reads num bytes starting from address register on device in to _buff array
void THRAXISACCE::readFrom(byte address, int num, byte _buff[]) {
	Wire.beginTransmission(ADXL345_DEVICE); 
	Wire.write(address);
	Wire.endTransmission();

	delay(6);

	Wire.beginTransmission(ADXL345_DEVICE);
	Wire.requestFrom(ADXL345_DEVICE, num);
	
	int i = 0;
	while(Wire.available())	{ 
		_buff[i] = Wire.read();
		i++;
	}
	if(i != num){
		status = ADXL345_ERROR;
		errorCode = ADXL345_READ_ERROR;
	}
	Wire.endTransmission();
	return;
}
// set bit of a register
void THRAXISACCE::setRegisterBit(byte regAdress, int bitPos, bool value) {
	byte buffer;
	readFrom(regAdress, 1, &buffer);
	if (value) {
		buffer |= (1 << bitPos);
	} else {
		buffer &= ~(1 << bitPos);
	}
	writeTo(regAdress, buffer);
	return;  
}
// get a gegister bit
bool THRAXISACCE::getRegisterBit(byte regAdress, int bitPos) {
	byte buffer;
	readFrom(regAdress, 1, &buffer);
	return ((buffer >> bitPos) & 1);
}

