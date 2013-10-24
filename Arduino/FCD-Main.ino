/* ------------------------------------------------------------
  Dash board
  v 1.0    04/02/2013  Auth: Antonio FRANCO
  
  
------------------------------------------------------------- */
#include <EEPROM.h>
#include <EEPROMAnything.h>
#include <OneWire.h>
#include <DSTEMPERATURE.h>
#include <DHTHUMIDITY.h>
#include <Wire.h>
#include <BAROMETER.h>
#include <THRAXISMAGN.h>
#include <THRAXISACCE.h>

#define VERSION 0.1
// ---------  Hardware Definition ------
#define EEPROM_TABLE_ADDRESS 0
#define EEPROM_VALIDITY_MARK 69

#define DSTEMP_INPUTPIN  34
#define DHT11_INPUTPIN 36

// ---- Costants -------
const int ciSerialBoudRate = 9600;
const boolean cbHasEcho = true;
const int ciMaxBufferLenght = 255;
const int ciSerialInputTimeOut = 2000;
const int caHeartBitLed = 13;
const int ciMainLoopDelay = 2000;  // 2 seconds


// define variables
DSTEMPERATURE  temperature(DSTEMP_INPUTPIN); 
int NumOfTempSensors;
DHTHUMIDITY huSensor;
BAROMETER barometro;
THRAXISMAGN compass;
THRAXISACCE clinometer;

// ------- Global Variables ----------------
char gcCommunicationBuffer[ciMaxBufferLenght+1];
boolean gbHeartBeat = false;
unsigned long acquisitionTime;

// ------- Table of Calibration Parameters ---
struct calibration_t
{
  byte ValidityFlag;
  int AltimeterBaseQuota;
  int CompassCorrectionAngle;
  float CompassCompesationX;
  float CompassCompesationY;
  float ClinometerRollBase;
  float ClinometerPitchBase;
  
} CalibrationParam;

// ---- Input Output definition -------------

 
// the setup routine runs once when you press reset:
void setup() {             

  // init the Serial Interface
  Serial.begin(ciSerialBoudRate);
  Serial.setTimeout(ciSerialInputTimeOut);
  // initialize I(/O direction
  pinMode(caHeartBitLed, OUTPUT);
  Wire.begin();

  // Start the configuration
  _splaschScreen();
  Serial.print("Init ...");
  // read the Claibration parameters table
  readCalibrationParameter();
 
  // Temperature setup
  NumOfTempSensors = temperature.getDevicesOnBus();

  // Umidity setup
  huSensor = DHTHUMIDITY();
  delay(500);
  huSensor.PowerOn(DHT11_INPUTPIN, DHT11);

  // Barometer setup
  barometro = BAROMETER();
  barometro.calibration();
  barometro.setBaseQuota(CalibrationParam.AltimeterBaseQuota);  // from EEPROM
  
  // Compass
  compass = THRAXISMAGN(); // instance of 3AXISMAGN 
  compass.setScale(0.88); //Set the scale of the compass.
  compass.setMeasurementMode(Measurement_SingleShot);  // Set the mode
  compass.setCorrectionAngle(CalibrationParam.CompassCorrectionAngle); // from EEPROM
  compass.setCompensation(CalibrationParam.CompassCompesationX, CalibrationParam.CompassCompesationY); // from EEPROM
    
  // clinometer
  clinometer = THRAXISACCE(); // instance of 3AXISACCE 
  clinometer.powerOn();
  clinometer.setRate(12.5);
  clinometer.setRangeSetting(2);
  clinometer.setAxisOffset(CalibrationParam.ClinometerRollBase, CalibrationParam.ClinometerPitchBase);
 
  Serial.println( "Done!");

}

// Main Loop routine runs forever:
void loop() {
  
  _createTheProcessImage();

  _exportTheProcessImage();

  _communicationLink();
  
  _loopManager();
  
}


// manage the loop delay
void _loopManager() 
{
  int iDelay = ciMainLoopDelay;
  while(iDelay > 0) {
    gbHeartBeat = !gbHeartBeat; digitalWrite(caHeartBitLed, gbHeartBeat);
    delay(500);
    iDelay -= 500; 
  }
  return;
}


/* ------------------------
  read the Inputs in the Process Image
  v.0.1
  ------------------------ */
void _createTheProcessImage() {

  unsigned long st,en;
  
//  st = millis();

  temperature.readDevices();
  huSensor.readHumidity();
  barometro.readPressure();
  compass.ReadMagnField();
  clinometer.readAccelerometer();
  
  en = millis();
//  Serial.print("Duration : "); Serial.println(en-st);
  acquisitionTime  = en;
  
  return;
}


/* ------------------------
  write the Outputs from the Process Image
  v.0.1
  ------------------------ */
void _exportTheProcessImage() {

  return;
}

/* ------------------------
  read the Serial input buffer
  v.0.1
  ------------------------ */
void  _communicationLink() {
  boolean bExec = true;
  int iNumOfChars = 0;
  int iNumOfReadChars = 0;
  
  iNumOfChars = Serial.available();
  bExec = iNumOfChars > 0 ? true : false;
  
  while(bExec) {
    iNumOfReadChars = Serial.readBytesUntil('\n', gcCommunicationBuffer, ciMaxBufferLenght);
    if(iNumOfReadChars > 0) {
      _decodeCommunicationString(gcCommunicationBuffer);   
    } else {
      Serial.println("Hello !");
    }
    iNumOfChars = Serial.available();
    bExec = iNumOfChars > 0 ? true : false;  // we need to add the TimeSlice control
  }
  memset(gcCommunicationBuffer,0,sizeof(gcCommunicationBuffer));
  return;
}


void _decodeCommunicationString(char* cInputBuffer) {

  int n;
  double a,b;
  
  if(cbHasEcho) Serial.println(cInputBuffer);
   
  switch(toupper(cInputBuffer[0])) {
    case '?':
      Serial.println("HELP : (D)ump, (L)ist, (V)alue, (Z)ero clinometer, (C)ompensate compass, (A)ltitude set, (G)eo north correction"); 
      break;
    case 'Z':
      clinometer.setOffset();
      Serial.print("Clinometer offset setted!");
      clinometer.getAxisOffset(&a, &b);
      CalibrationParam.ClinometerRollBase = a;
      CalibrationParam.ClinometerPitchBase = b;
      storeEEPROMParameter();
      break;
    case 'C':
      Serial.print("Start magnetometer compensation...");
      compass.compensateMagnField();
      Serial.print("... magnetometer compensation DONE !");
      compass.getCompensation(&a, &b);
      CalibrationParam.CompassCompesationX = a;
      CalibrationParam.CompassCompesationY = b;
      storeEEPROMParameter();
      break;
    case 'A':
      n = atoi(cInputBuffer+1);
      barometro.setBaseQuota(n);
      Serial.print("Set base quota for altimeter at (m) :"); Serial.println(n); 
      CalibrationParam.AltimeterBaseQuota = n;
      storeEEPROMParameter();
      break;
    case 'G':
      n = atoi(cInputBuffer+1);
      compass.setCorrectionAngle(n);
      Serial.print("Set compass correction (o) :"); Serial.println(n); 
      CalibrationParam.CompassCorrectionAngle = n;
      storeEEPROMParameter();
      break;
    case 'L':
      _dumpSensorsDefinition();
      break;
    case 'D':
      _dumpSensorsValues();
      break;
    case 'V':
      _dumpValues();
      break;
    case 'X':
      n = atoi(cInputBuffer+1);
      Serial.print("Sensor :");
      Serial.println(n); 
      break;
    default:
      Serial.println("Sorry !");
      break;
  }
  return;
}

// read from structure e dump values
void _dumpSensorsValues()
{
  double x,y;
  byte i;

  Serial.println("-----------------------------------");
  Serial.print("Time stamp (sec) : "); Serial.println(acquisitionTime);
  for(i=0;i<NumOfTempSensors;i++) {
    Serial.print("DS Device ID=");
    Serial.print(i);
    Serial.print("  temperature (oC) = ");
    Serial.println(temperature.getTemperatureC(i));
  }
  
  if(huSensor.getHumidity() == -1) {
    Serial.println("DHT Device Error !"); 
  } else {
    Serial.print("Humidity (%) = ");
    Serial.println(huSensor.getHumidity(), 2);
    Serial.print("  Temperature (oC) = ");
    Serial.println(huSensor.getTemperature(false), 2);
    Serial.print("  Dew Point (oC) = ");
    Serial.println(huSensor.getDevPoint());
    Serial.print("  Dew PointFast (oC) = ");
    Serial.println(huSensor.getDevPointFast());
  }
  
  Serial.print("Pressure (hPas) = "); Serial.println(barometro.getPressure(), 1);
  Serial.print("  Pressure sl (hPas) = "); Serial.println(barometro.getPressureSL(), 1);
  Serial.print("  Altitude sl (m) = "); Serial.println(barometro.getAltitude(), 1);
  Serial.print("  Temperature (oC) = "); Serial.println(barometro.getTemperature(), 1);
  Serial.print("  - Base quota (m) = "); Serial.println(barometro.getBaseQuota());

  Serial.print("Heading  (o) = "); Serial.println(compass.getHeadingMag(), 1);
  Serial.print("  Field intensity (Guss) = "); Serial.println(compass.getIntensity(), 3);
  Serial.print("  Declination (o) = "); Serial.println(compass.getDeclinationDeg(), 1); 
  Serial.print("  Inclination (o) = "); Serial.println(compass.getInclinationDeg(), 1);
  Serial.print("  Geo heading (o) = "); Serial.println(compass.getHeadingGeo(), 1);
  Serial.print("  - Geo correction (o) = "); Serial.println(compass.getCorrectionAngle());
  compass.getCompensation(&x, &y);
  Serial.print("  - Magnetic Compensation (mGauss) X = "); Serial.print(x, 3); Serial.print(" Y = "); Serial.println(y, 3);
  Serial.print("Roll (o) = "); Serial.println(clinometer.getRoll(), 1); 
  Serial.print("Pitch (o) = "); Serial.println(clinometer.getPitch(), 1); 
  clinometer.getAxisOffset(&x, &y);
  Serial.print("  - Clinometer offset (o) Roll = "); Serial.print(x, 1);  Serial.print(" Pitch = "); Serial.println(y, 1);
  Serial.println("-----------------------------------");

  return;

}

// read from structure e dump values
void _dumpSensorsDefinition()
{

  byte i;

  Serial.println("-----------------------------------");
  Serial.println("Clock\tTime stamp\tmsec");
  for(i=0;i<NumOfTempSensors;i++) {
    Serial.print("Temperature Device ID=");
    Serial.print(i);
    Serial.println("\ttemperature\tCelsius dgr");
  }
  
  Serial.println("DHT Device\tHumidity\t%");
  Serial.println("DHT Device\tTemperature\tCelsius dgr");
  Serial.println("DHT Device\tDew Point\tCelsius dgr");
  Serial.println("DHT Device\tDew PointFast\tCelsius dgr");
  Serial.println("Barometer\tPressure\thPas");
  Serial.println("Barometer\tPressure sl\thPas");
  Serial.println("Barometer\tAltitude sl\tm");
  Serial.println("Barometer\tTemperature\tCelsius dgr");
  Serial.println("Compass\tMagneting heading\tdgr");
  Serial.println("Compass\tField intensity\tGauss");
  Serial.println("Compass\tDeclination\tdgr");
  Serial.println("Compass\tInclination\tdgr");
  Serial.println("Compass\tGeografic heading\tdgr");
  Serial.println("Clinometer\tRoll\tdgr");
  Serial.println("Clinometer\tPitch\tdgr");

  return;

}

// read from structure e dump values
void _dumpValues()
{

  byte i;
  Serial.print(acquisitionTime); Serial.print(",");
  for(i=0;i<NumOfTempSensors;i++) {
    Serial.print(temperature.getTemperatureC(i));
    Serial.print(",");
  }
  
  Serial.print(huSensor.getHumidity(), 2);
  Serial.print(",");
  Serial.print(huSensor.getTemperature(false), 2);
  Serial.print(",");
  Serial.print(huSensor.getDevPoint());
  Serial.print(",");
  Serial.print(huSensor.getDevPointFast());
  Serial.print(",");
  
  Serial.print(barometro.getPressure(), 1);
  Serial.print(",");
  Serial.print(barometro.getPressureSL(), 1);
  Serial.print(",");
  Serial.print(barometro.getAltitude(), 1);
  Serial.print(",");
  Serial.print(barometro.getTemperature(), 1);
  Serial.print(",");
  Serial.print(compass.getHeadingMag(), 1);
  Serial.print(",");
  Serial.print(compass.getIntensity(), 3);
  Serial.print(",");
  Serial.print(compass.getDeclinationDeg(), 1); 
  Serial.print(",");
  Serial.print(compass.getInclinationDeg(), 1);
  Serial.print(",");
  Serial.print(compass.getHeadingGeo(), 1);
  Serial.print(",");
  Serial.print(clinometer.getRoll(), 1); 
  Serial.print(",");
  Serial.print(clinometer.getPitch(), 1); 
  Serial.println();

  return;

}

void _splaschScreen()
{
  Serial.println("------------------");
  Serial.println("!   ARDU DASH    !");
  Serial.println("! Auth:A.Franco  !");
  Serial.print("! Vers.: "); Serial.print(VERSION);  Serial.println("    !");
  Serial.println("------------------");
  return;

}


// EEPROM management function
void readCalibrationParameter()
{
  EEPROM_readAnything(EEPROM_TABLE_ADDRESS, CalibrationParam);

  if(CalibrationParam.ValidityFlag != EEPROM_VALIDITY_MARK) {
    CalibrationParam.ValidityFlag = EEPROM_VALIDITY_MARK;
    CalibrationParam.AltimeterBaseQuota = 0;
    CalibrationParam.CompassCorrectionAngle = 0;
    CalibrationParam.CompassCompesationX = 0;
    CalibrationParam.CompassCompesationY = 0;
    CalibrationParam.ClinometerRollBase = 0;
    CalibrationParam.ClinometerPitchBase = 0;
    EEPROM_writeAnything(EEPROM_TABLE_ADDRESS, CalibrationParam);
    
  }
  return;
}

void storeEEPROMParameter()
{
  EEPROM_writeAnything(EEPROM_TABLE_ADDRESS, CalibrationParam);
  return;
}


