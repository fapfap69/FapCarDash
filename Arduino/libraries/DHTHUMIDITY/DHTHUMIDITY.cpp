/*
DhtHumidity.cpph - Header file for the SensorLib.

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/

#include "DHTHUMIDITY.h"

// Class constructor
DHTHUMIDITY::DHTHUMIDITY() {

	first = true;
	return;
}

// Setup of lines
void DHTHUMIDITY::PowerOn(int pin, int type) {

	sensor.PinNumber = (uint8_t)pin;
	sensor.Type = (uint8_t)type;
	first = true;

	pinMode(sensor.PinNumber, INPUT);
	digitalWrite(sensor.PinNumber, HIGH);
	lastReadTime = 0;
	return;
}

// Interface functions
float DHTHUMIDITY::getTemperature(boolean Scale=false) {
	if(Scale) return(sensor.TemperatureC);
	else return(sensor.TemperatureF);
}

float DHTHUMIDITY::getHumidity(void) {
	return(sensor.Humidity);
}
	
float DHTHUMIDITY::getDevPoint() {

	float A0= 373.15/(273.15 + sensor.TemperatureC);
        float SUM = -7.90298 * (A0-1);
        SUM += 5.02808 * log10(A0);
        SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
        SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
        SUM += log10(1013.246);
        float VP = pow(10, SUM-3) * sensor.Humidity;
        float T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558-T);
}

float DHTHUMIDITY::getDevPointFast()
{
        double a = 17.271;
        double b = 237.7;
        double temp = (a * sensor.TemperatureC) / (b + sensor.TemperatureC) + log(sensor.Humidity/100);
        double Td = (b * temp) / (a - temp);
        return Td;
}


// The real aquire function	
Humiditysensor DHTHUMIDITY::readHumidity()
{
	float u,t;
	if (readSensor()) {
	    switch (sensor.Type) {
		case DHT11:
			u = data[0];
			t = data[2];
			break;
		case DHT22:
		case DHT21:
			u = data[0];
			u *= 256;
			u += data[1];
			u /= 10;
			t = data[2] & 0x7F;
			t *= 256;
			t += data[3];
			t /= 10;
			if (data[2] & 0x80) t *= -1;
			break;
		default:
			u = 0;
			t = 0;
			break;
	    }
	} else {
		Serial.print("Read fail");
		u = -1;
		t = -1;
	}

	sensor.TemperatureC = t;
	sensor.TemperatureF = convertCtoF(t);
	sensor.Humidity = u;

	return(sensor);
}

// Convert celsius to Farhenait
float DHTHUMIDITY::convertCtoF(float c) 
{
	return c * 1.8 + 32.0;
}

//Celsius to Kelvin conversion
float DHTHUMIDITY::convertCtoK(float c)
{
        return c + 273.15;
}

// read the sensor
boolean DHTHUMIDITY::readSensor(void) 
{

	uint8_t laststate = HIGH;
  	uint8_t counter = 0;
  	uint8_t j = 0, i;
	unsigned long currenttime;

	// pull the pin high and wait 250 milliseconds
	digitalWrite(sensor.PinNumber, HIGH);
	delay(250);

	currenttime = millis();
	if (currenttime < lastReadTime) {
		// ie there was a rollover
		lastReadTime = 0;
	}
	if (!first && ((currenttime - lastReadTime) < 2000)) {
		return true; 
  	}
	first = false;
	lastReadTime = millis();


	// Clear the buffer
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
  
	// low for ~20 milliseconds
	pinMode(sensor.PinNumber, OUTPUT);
	digitalWrite(sensor.PinNumber, LOW);
  	delay(20);
  	cli();
  	digitalWrite(sensor.PinNumber, HIGH);
	delayMicroseconds(40);
	pinMode(sensor.PinNumber, INPUT);

	// read 
	for ( i=0; i< MAXTIMINGS; i++) {
   		counter = 0;
		while (digitalRead(sensor.PinNumber) == laststate) {
			counter++;
			delayMicroseconds(1);
			if (counter == 255) {
				break;
			}
		}
		laststate = digitalRead(sensor.PinNumber);

 		if (counter == 255) break;

 		// ignore first 3 transitions
		if ((i >= 4) && (i%2 == 0)) {
			// shove each bit into the storage bytes
			data[j/8] <<= 1;
			if (counter > 6)
				data[j/8] |= 1;
			j++;
		}
	}

	sei();
	// check we read 40 bits and that the checksum matches
	if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
		return true;
	}
	return false;
}
