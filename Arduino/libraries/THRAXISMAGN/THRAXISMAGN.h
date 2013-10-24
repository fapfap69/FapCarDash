/*
ThreeAxisMagn.h - Header file for the SensorLib.

Author A.Franco
Date 31/01/2013
Ver. 0.1

*/

#ifndef THRAXISMAGN_DEF
#define THRAXISMAGN_DEF

#include <Arduino.h>
#include <Wire.h>

// 3-Axis Magnetometer based on HMC5883L 

#define HMC5883L_Address 0x1E

#define ConfigurationRegisterA 0x00
#define ConfigurationRegisterB 0x01
#define ModeRegister 0x02
#define DataRegisterBegin 0x03

#define Measurement_Continuous 0x00
#define Measurement_SingleShot 0x01
#define Measurement_Idle 0x03

#define RAD2DEG 57.29577951
#define DEG2RAD 0.017453293

struct Magnetometer
{
	int XRaw;
	int YRaw;
	int ZRaw;
	double XInt;
	double YInt;
	double ZInt;
	double Intensity;
	double Inclination;
	double Declination;

	double XComp;
	double YComp;
};


class THRAXISMAGN
{
	public:
	  THRAXISMAGN();

	  Magnetometer ReadMagnField();
  
	  void setMeasurementMode(uint8_t mode);
	  uint8_t getMeasurementMode(void);

	  void setScale(double gauss);
	  double getScale();

	  double getIntensity();
	  double getInclinationDeg();
	  double getInclinationRad();
	  double getDeclinationDeg();
	  double getDeclinationRad();
	  double getHeadingMag();
	  double getHeadingGeo();

	  void compensateMagnField();
	  void getCompensation(double *, double *);
	  void setCompensation(double, double);

	  void setCorrectionAngle(int);
	  int getCorrectionAngle();

	protected:
	  void Write(int , int);
	  uint8_t* Read(int , int );
	  double getHeading(double);

	private:
	  double Scale;
	  Magnetometer Sensor;
	  int MeasureMode;
	  double CorrectionAngle;

};
#endif
