#include "PeristalticPump.h"

PeristalticPump::PeristalticPump(int pump1Pin, int pump2Pin, int pump3Pin, int pump4Pin, int pump5Pin) : pump1(pump1Pin), pump2(pump2Pin), pump3(pump3Pin), pump4(pump4Pin), pump5(pump5Pin)
{
}

PeristalticPump::~PeristalticPump()
{
}

void PeristalticPump::begin() {
  pinMode(pump1, OUTPUT);
  pinMode(pump2, OUTPUT);
  pinMode(pump3, OUTPUT);
  pinMode(pump4, OUTPUT);
  pinMode(pump5, OUTPUT);

  //Need to make sure all the Relays are off during initialisation
  digitalWrite(pump1, HIGH);
  digitalWrite(pump2, HIGH);
  digitalWrite(pump3, HIGH);
  digitalWrite(pump4, HIGH);
  digitalWrite(pump5, HIGH);
}

//if in calibration mode, then dosing would be disabled till calibration is exited\
so toggling pump on and off would not occure in any other function except those\
called in the calibration screen
void PeristalticPump::purgePumps(int pumpNum) {
  //toggle pump on and off
  //if pump is off then turn if on
  if (digitalRead(pumpIO(pumpNum)) == HIGH) {
    digitalWrite(pumpIO(pumpNum), LOW);
  }
  //if pump on then turn it off
  else if (digitalRead(pumpIO(pumpNum)) == LOW) {
    digitalWrite(pumpIO(pumpNum), HIGH);
  }
}

void PeristalticPump::setCalibrationTime(int pumpNum) {
  if (digitalRead(pumpIO(pumpNum)) == HIGH) {
    digitalWrite(pumpIO(pumpNum), LOW);
    startTime = millis();
  }
  else if (digitalRead(pumpIO(pumpNum)) == LOW) {
    digitalWrite(pumpIO(pumpNum), HIGH);
    endTime = millis();
    totalTime += (endTime - startTime);
  }
}

void PeristalticPump::setPumpCaliTimeOnStartUp(int pumpNum, long time) {
  if (pumpNum == 1) {
    pump1CaliTime = time;
    //Serial.print("SolA cali time:"); Serial.println(pump1CaliTime);
  }
  else if (pumpNum == 2) {
    pump2CaliTime = time;
    //Serial.print("SolB cali time:"); Serial.println(pump2CaliTime);
  }
  else if (pumpNum == 3) {
    pump3CaliTime = time;
    //Serial.print("SolC cali time:"); Serial.println(pump3CaliTime);
  }
  else if (pumpNum == 4) {
    pump4CaliTime = time;
    //Serial.print("pH up cali time:"); Serial.println(pump4CaliTime);
  }
  else if (pumpNum == 5) {
    pump5CaliTime = time;
    //Serial.print("pH down cali time:"); Serial.println(pump5CaliTime);
  }
}

void PeristalticPump::saveCalibrationTime(int pumpNum) {
  if (pumpNum == 1) {
    pump1CaliTime = totalTime;
    //Serial.print("SolA cali time:"); Serial.println(pump1CaliTime);
  }
  else if (pumpNum == 2) {
    pump2CaliTime = totalTime;
    //Serial.print("SolB cali time:"); Serial.println(pump2CaliTime);
  }
  else if (pumpNum == 3) {
    pump3CaliTime = totalTime;
    //Serial.print("SolC cali time:"); Serial.println(pump3CaliTime);
  }
  else if (pumpNum == 4) {
    pump4CaliTime = totalTime;
    //Serial.print("pH up cali time:"); Serial.println(pump4CaliTime);
  }
  else if (pumpNum == 5) {
    pump5CaliTime = totalTime;
    //Serial.print("pH down cali time:"); Serial.println(pump5CaliTime);
  }
  totalTime = 0;
}

//can add a boolean return type if necessary for menue operations
// pumps in used will stop off other pumps from operating if one pump is on!!!
bool PeristalticPump::runPumps(int pumpNum, long Duration) {
  //Find what pin is the pump conected to
  int pumpPin = pumpIO(pumpNum);
  if (digitalRead(pumpPin) == HIGH && Duration != 0) {
    restartPumpTimer(pumpNum);
    digitalWrite(pumpPin, LOW);
    return false;
  }
  else if (digitalRead(pumpPin) == LOW && (millis() - pumpStartTime(pumpNum)) > Duration) {

    digitalWrite(pumpPin, HIGH);
    return true;
  }
  else
    return false;
}

int PeristalticPump::pumpIO(int pumpNum) {
  if (pumpNum == 1) {
    return pump1;
  }
  else if (pumpNum == 2) {
    return pump2;
  }
  else if (pumpNum == 3) {
    return pump3;
  }
  else if (pumpNum == 4) {
    return pump4;
  }
  else if (pumpNum == 5) {
    return pump5;
  }
}

unsigned long PeristalticPump::pumpStartTime(int pumpNum) {
  if (pumpNum == 1) {
    return (pump1StartTime);
  }
  else if (pumpNum == 2) {
    return (pump2StartTime);
  }
  else if (pumpNum == 3) {
    return (pump3StartTime);
  }
  else if (pumpNum == 4) {
    return (pump4StartTime);
  }
  else if (pumpNum == 5) {
    return (pump5StartTime);
  }
}

void PeristalticPump::restartPumpTimer(int pumpNum) {
  if (pumpNum == 1) {
    pump1StartTime = millis();
  }
  else if (pumpNum == 2) {
    pump2StartTime = millis();
  }
  else if (pumpNum == 3) {
    pump3StartTime = millis();
  }
  else if (pumpNum == 4) {
    pump4StartTime = millis();
  }
  else if (pumpNum == 5) {
    pump5StartTime = millis();
  }
}

bool PeristalticPump::testCurrentCalibration(int pumpNum) {
  int pumpPin = pumpIO(pumpNum);

  if (digitalRead(pumpPin) == HIGH) {
    restartPumpTimer(pumpNum);
    digitalWrite(pumpPin, LOW);

    return true;
  }
  else if (millis() - pumpStartTime(pumpNum) > getPumpCalibrationTime(pumpNum)) {
    digitalWrite(pumpPin, HIGH);

    return false;
  }
}

//the return type is used in the main code to stop the user from terminating the timer before it restarts i.e. stops them from changing screens
bool PeristalticPump::testNewCalibration(int pumpNum) {
  //initialise start time when set is ran for the first time
  if (testRunning == false) {
    testStartTime = millis();
    testRunning = true;
  }
  //return true when test is running
  if (millis() - testStartTime < totalTime) {
    digitalWrite(pumpIO(pumpNum), LOW);
    return true;
  }
  //return false when test is completed
  else if (millis() - testStartTime >= totalTime) {
    digitalWrite(pumpIO(pumpNum), HIGH);
    testRunning = false;
    return false;
  }
}

void PeristalticPump::setFlowRate(int pumpNum, int flowRate) {
  if (pumpNum == 1) {
    pump1FlowRate = flowRate;
  }
  else if (pumpNum == 2) {
    pump2FlowRate = flowRate;
  }
  else if (pumpNum == 3) {
    pump3FlowRate = flowRate;
  }
  else if (pumpNum == 4) {
    pump4FlowRate = flowRate;
  }
  else if (pumpNum == 5) {
    pump5FlowRate = flowRate;
  }
}

int PeristalticPump::getFlowRate(int pumpNum) {
  if (pumpNum == 1) {
    return pump1FlowRate;
  }
  else if (pumpNum == 2) {
    return pump2FlowRate;
  }
  else if (pumpNum == 3) {
    return pump3FlowRate;
  }
  else if (pumpNum == 4) {
    return pump4FlowRate;
  }
  else if (pumpNum == 5) {
    return pump5FlowRate;
  }
}

long PeristalticPump::getPumpCalibrationTime(int pumpNum) {
  if (pumpNum == 1) {
    return pump1CaliTime;
  }
  else if (pumpNum == 2) {
    return pump2CaliTime;
  }
  else if (pumpNum == 3) {
    return pump3CaliTime;
  }
  else if (pumpNum == 4) {
    return pump4CaliTime;
  }
  else if (pumpNum == 5) {
    return pump5CaliTime;
  }
}

void PeristalticPump::setPumpCalibrationTime(int pumpNum, long time) {
  if (pumpNum == 1) {
    pump1CaliTime = time;
  }
  else if (pumpNum == 2) {
    pump2CaliTime = time;
  }
  else if (pumpNum == 3) {
    pump3CaliTime = time;
  }
  else if (pumpNum == 4) {
    pump4CaliTime = time;
  }
  else if (pumpNum == 5) {
    pump5CaliTime = time;
  }
}
