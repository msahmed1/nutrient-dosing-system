#ifndef Dosing_H
#define Dosing_H

#include "arduino.h"
#include "PeristalticPump.h"
#include "AtlasScientific.h"

//The dosing class should hold and manipulate regiments and hold the resivour size\
it should also calculat the duration that the pumps need to be on for and exicute\
the command

class Dosing {
public:
  Dosing(const PeristalticPump &pumps, int upperFloatSwitchRes, int lowerFloatSwitchRes, int mixer, int fill, int drain);
  ~Dosing() {};

  void newRegiment(int solA, int solB, int solC);

  void enable_dosing();
  void disable_dosing();
  void checkNutrientBalace(long ECReading, long pHReading, int regiment);

  void agitateSolution(int pumpIO, int onTime, int offTime, unsigned long &lastAgitateTime);
  void drain(int Draintime, int drainPin);
  int reservoirLevel();
  void fill();
  int ECMonitor(long ECReading, int regiment);
  int pHMonitor(long pHReading, int regiment);

  int getReserviorSize() { return reserviorSize; };

  void setReserviorSize(int volume) { reserviorSize = volume; };
  void setBaselineEC(long ecValue) { baselineEC = ecValue; };

  void setSolA(int regiment, long value) { regi[regiment].solA = value; };
  long getSolA(int regiment) { return regi[regiment].solA; };

  void setSolB(int regiment, long value) { regi[regiment].solB = value; };
  long getSolB(int regiment) { return regi[regiment].solB; };

  void setSolC(int regiment, long value) { regi[regiment].solC = value; };
  long getSolC(int regiment) { return regi[regiment].solC; };

  void setUpperEC(int regiment, long value) { regi[regiment].upperEC = value; };
  long getUpperEC(int regiment) { return regi[regiment].upperEC; };

  void setLowerEC(int regiment, long value) { regi[regiment].lowerEC = value; };
  long getLowerEC(int regiment) { return regi[regiment].lowerEC; };

  void setUpperpH(int regiment, float value) { regi[regiment].upperpH = value; };
  float getUpperpH(int regiment) { return regi[regiment].upperpH; };

  void setLowerpH(int regiment, float value) { regi[regiment].lowerpH = value; };
  float getLowerpH(int regiment) { return regi[regiment].lowerpH; };

private:
  PeristalticPump dosingPumps;

  //Float switches
  const int upperFloatSwitchResevoir;
  const int lowerFloatSwitchResevoir;
  //const int lowerFloatSwitchSumpTank;
  const int magneticMixer;
  const int fillPump;
  const int drainPump;

  struct regiment {
    long solA = 0, solB = 0, solC = 0;
    long upperEC = 0, lowerEC = 0;
    float upperpH = 0, lowerpH = 0;
  } regi[14];

  // Used in calling EC and pH control functions periodicaly and in sequence
  unsigned long ECCounter = millis();
  bool ECchecked;
  unsigned long phCounter = millis();
  long ECCheckTime = 10000;

  bool StartDrain = false;
  unsigned long drainPumpTimer = 0.0;
  unsigned long fillPumpTimer = 0.0;

  int reserviorSize = 75;     //In liters

  bool agitate = false;
  int toggleDuration = 0;

  bool draining = false;
  unsigned long previousDrainTime = 0.0;

  bool firstRun = true;
  bool startDosing = true;
  unsigned long levelSwitchTimer = 0.0;

  int ECSamples = 0;
  long totalEC = 0;
  unsigned long previousECMillis = 0.0;
  long SolADosingTime = 0;
  long SolBDosingTime = 0;
  long SolCDosingTime = 0;
  long ECDrainTime = 0;
  long ECFillTime = 0;
  int regulateEC;
  long baselineEC = 540;
  long drainPumpCalibrationTime = 15000;
  long fillPumpCalibrationTime = 15000;

  int pHSamples = 0;
  long totalpH = 0;
  unsigned long previouspHMillis = 0.0;
  long phDosingTime;
  int regulatepH;
};
#endif
