#pragma once

#include "arduino.h"

//The main program is in charge of controlling the lifecycle of each function\
in other words, if a function must be ran twice to toggle the pump on and then\
off. the main program should ensure this happens. Like wise if a function needs\
to be called continiously (e.g testNewCalibration) till it it complete the the\
main program should ensure this occurs

class PeristalticPump
{
public:
  PeristalticPump(int pump1Pin, int pump2Pin, int pump3Pin, int pump4Pin, int pump5Pin);
  ~PeristalticPump();

  void begin();

  void purgePumps(int pumpNum);
  bool runPumps(int pump, long time);
  void setCalibrationTime(int pumpNum);
  void saveCalibrationTime(int pumpNum);
  bool testNewCalibration(int pumpNum);
  bool testCurrentCalibration(int pumpNum);

  int pumpIO(int pumpNum);
  unsigned long pumpStartTime(int pumpNum);
  void restartPumpTimer(int pumpNum);

  void setFlowRate(int pumpNum, int flowRate);
  int getFlowRate(int pumpNum);

  long getPumpCalibrationTime(int pumpNum);
  void setPumpCalibrationTime(int pumpNum, long time);
  void setPumpCaliTimeOnStartUp(int pumpNum, long time);
  void reinitialiseCalibration() { totalTime = 0; };

private:
  unsigned long startTime = 0;
  unsigned long endTime = 0;
  unsigned long totalTime = 0;

  const int pump1;
  int pump1FlowRate = 0;
  long pump1CaliTime = 19625;         //calibration time for dispensing 25ml
  unsigned long pump1StartTime = 0;

  const int pump2;
  int pump2FlowRate = 0;
  long pump2CaliTime = 20000;         //calibration time for dispensing 25ml
  unsigned long pump2StartTime = 0;

  const int pump3;
  int pump3FlowRate = 0;
  long pump3CaliTime = 20000;         //calibration time for dispensing 25ml
  unsigned long pump3StartTime = 0;

  const int pump4;
  int pump4FlowRate = 0;
  long pump4CaliTime = 21100;         //calibration time for dispensing 25ml
  unsigned long pump4StartTime = 0;

  const int pump5;
  int pump5FlowRate = 0;
  long pump5CaliTime = 19950;         //calibration time for dispensing 25ml
  unsigned long pump5StartTime = 0;

  bool testRunning = false;
  unsigned long testStartTime;
};
