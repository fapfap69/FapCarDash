/*
DhtHumidity.h - Header file for the SensorLib.

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/

#ifndef DHTHUMIDITY_DEF
#define DHTHUMIDITY_DEF

#include <Arduino.h>

// how many timing transitions we need to keep track of. 2 * number bits + extra
#define MAXTIMINGS 85

#define DHT11 11
#define DHT22 22
#define DHT21 21

struct Humiditysensor {

	uint8_t PinNumber;
	uint8_t Type;
		
	float TemperatureC;
	float TemperatureF;
	float Humidity;

	float DevPoint;


};

class DHTHUMIDITY {

	public:
		DHTHUMIDITY();
		void PowerOn(int, int);

		Humiditysensor readHumidity();

		float getTemperature(boolean);
		float getHumidity(void);
		float getDevPoint();
		float getDevPointFast();

		float convertCtoF(float);
		float convertCtoK(float);

	private:

		Humiditysensor sensor;
		boolean first;
		unsigned long lastReadTime;


		uint8_t data[6];
		boolean readSensor(void);


};

#endif
