#include "Dosing.h"

Dosing::Dosing(const PeristalticPump &pumps, int upperFloatSwitchRes, int lowerFloatSwitchRes, int mixer, int fill, int drain)
:dosingPumps(pumps), upperFloatSwitchResevoir(upperFloatSwitchRes), lowerFloatSwitchResevoir(lowerFloatSwitchRes), magneticMixer(mixer), fillPump(fill), drainPump(drain)
{
  pinMode(upperFloatSwitchResevoir, INPUT);
  pinMode(lowerFloatSwitchResevoir, INPUT);
  pinMode(magneticMixer, OUTPUT);
  pinMode(fillPump, OUTPUT);
  pinMode(drainPump, OUTPUT);
  
  digitalWrite(magneticMixer, HIGH);
  digitalWrite(fillPump, HIGH);
  digitalWrite(drainPump, HIGH);
}

void Dosing::newRegiment(int solA, int solB, int solC) {

}

void Dosing::agitateSolution(int pumpIO, int onTime, int offTime, unsigned long &lastAgitateTime) {
  if (millis() - lastAgitateTime >= toggleDuration) {
    if (agitate == true) {
      digitalWrite(pumpIO, LOW);
      toggleDuration = onTime;
      agitate = false;  //Next toggle, do not agitate pump
    }
    else if (agitate == false) {
      digitalWrite(pumpIO, HIGH);
      toggleDuration = offTime;
      agitate = true;   //Next toggle, agitate pump
    }
    lastAgitateTime = millis();
  }
}

// reservoirLevel()
//0 = refilling: waiting for refill before starting dose\
//1 = continue: start dosing\
//2 = fault: do not dose
int Dosing::reservoirLevel() {
  if (firstRun == true) {   
    levelSwitchTimer = millis();    //Initialise Timer
    firstRun = false;
  }
  bool currentHighLevelState = digitalRead(upperFloatSwitchResevoir);
  bool currentLowLevelState = digitalRead(lowerFloatSwitchResevoir);

  if (currentHighLevelState == LOW && currentLowLevelState == LOW && (millis() - levelSwitchTimer) > 5000) {    // ensuer the reseriour level is stable for 5 seconds

    // at this point an error must have occured with passive filling, need to actively the fill tank
    
    return 1;
  }
  else if (currentHighLevelState == LOW && currentLowLevelState == HIGH) {    // sensor error: should not be possible
    //For now only fill from the fresh water tank to top level
    //this may cause an issue, as the system will always want to be filled to the top, find a better strategy
    //Trigger alarm
    return 2;
    //can change the return type to an integer so that this else state can trigger an alarm if entered\
    the alarm would be to indicate to the farmer that a porential problem has occured.
  }
  
  else if (currentHighLevelState == HIGH && currentLowLevelState == LOW) {    //Water level below high switch, do nothing
    //fill();
    //Serial.print("Timer: ");
    //levelSwitchTimer = millis();
    //Serial.println(levelSwitchTimer);
    return 0;
  }
  else if (currentHighLevelState == HIGH && currentLowLevelState == HIGH) {   //Re-filling reservoir(From empty)
    //Serial.print(": ");
    //fill();
    //Serial.print("Timer: ");
    //levelSwitchTimer = millis();
    //Serial.println(levelSwitchTimer);
    return 0;
  }
  /*for error handelling, if in this loop for more than 1 hour, trigger and ararm
  * this needs imperical testing*/
}

void Dosing::fill() {
  //digitalWrite(39, LOW);
  //digitalWrite(46, HIGH);
}

void Dosing::drain(int Draintime, int drainPin) {
  if (draining == false) {
    draining = true;
    digitalWrite(drainPin, HIGH);
    previousDrainTime = millis();
  }
  else if ((millis() - previousDrainTime > Draintime)) {
    digitalWrite(drainPin, LOW);
    draining = false;
    previousDrainTime = millis();
  }
}

int Dosing::ECMonitor(long ECReading, int regiment) {
  if ((millis() - previousECMillis > 3000) && ECSamples < 20) {   // Average 20 EC sensors readings taken every 3 seconds
    totalEC = totalEC + ECReading;
    previousECMillis = millis();
    ECSamples++;
    regulateEC = 0;
    return 0;
  }
  else if (ECSamples >= 20 && regulateEC == 0) {                                                        // Calculate EC error once sampling has completed
    if ((totalEC / ECSamples) < regi[regiment].lowerEC) {                                               // Consentration low, start dosing
      long error = 1 - ((totalEC / ECSamples) - baselineEC) / (regi[regiment].upperEC - baselineEC);
      long flowRateFactorEC = error * 0.6;                                                              //0.6 is a correction factor, to be tuned based on the systems response rate
      if (flowRateFactorEC < 0) {
        flowRateFactorEC = flowRateFactorEC * -1;
      }
      SolADosingTime = regi[regiment].solA * flowRateFactorEC*dosingPumps.getPumpCalibrationTime(1) / 25 * reserviorSize;
      SolBDosingTime = regi[regiment].solB * flowRateFactorEC*dosingPumps.getPumpCalibrationTime(2) / 25 * reserviorSize;
      SolCDosingTime = regi[regiment].solC * flowRateFactorEC*dosingPumps.getPumpCalibrationTime(3) / 25 * reserviorSize;
      regulateEC = 1;
      totalEC = 0;
    }
    else if ((totalEC / ECSamples) > regi[regiment].upperEC) {              // Consentration to high, start diluting
      long flowRateFactorEC = (((totalEC / ECSamples) - (regi[regiment].lowerEC))*reserviorSize) / regi[regiment].lowerEC;
      drainPumpTimer = millis();                                            // initialise drain pump timer
      ECDrainTime = flowRateFactorEC * drainPumpCalibrationTime;            // Initialising drain time
      ECFillTime = flowRateFactorEC * fillPumpCalibrationTime;              // Initialising fill time
      StartDrain = true;
      digitalWrite(drainPump, LOW);                                                // Turn on drain pump
      regulateEC = 2;
      totalEC = 0;
    }
    else if (((totalEC / ECSamples) > regi[regiment].lowerEC) && ((totalEC / ECSamples) < regi[regiment].upperEC)) {    // If everything is within range then return true
      ECSamples = 0;
      regulateEC = 0;
      totalEC = 0;
      return 2;
    }
  }
  else if (regulateEC == 1) {    // add nutrients to until pump on time has elapsed then reset to timer to 0
    if (dosingPumps.runPumps(1, SolADosingTime) == true) {
      SolADosingTime = 0;
    }
    if (dosingPumps.runPumps(2, SolBDosingTime) == true) {
      SolBDosingTime = 0;
    }
    if (dosingPumps.runPumps(3, SolCDosingTime) == true) {
      SolCDosingTime = 0;
    }
  }
  else if (regulateEC == 2) {
    if ((millis() - drainPumpTimer > ECDrainTime) && StartDrain == true) {    // When drain pump on time hase elapsed exicute the following
      digitalWrite(drainPump, HIGH);             // Turn off drain pump
      ECDrainTime = 0;                    // Re-initialise ECDranTime
      digitalWrite(fillPump, LOW);              // Turn on fill pump
      fillPumpTimer = millis();           // Initialise fillPumpTimer
      StartDrain = false;                 // Switch drain to equal false to start filling
    }
    else if ((millis() - fillPumpTimer > ECFillTime) && StartDrain == false) {    // When drain pump on time hase elapsed exicute the following
      digitalWrite(fillPump, HIGH);             // Turn off fill pump
      ECFillTime = 0;                 // Re-initialise ECFillTime
    }
  }
  
  if (SolADosingTime == 0 && SolBDosingTime == 0 && SolCDosingTime == 0 && ECDrainTime == 0 && ECFillTime == 0 && regulateEC != 0) {  // reinisialise parameters when dosing complete
    ECSamples = 0;
    regulateEC = 0;
    return 1;
  }
  else  // Default response
    return 0;
}

int Dosing::pHMonitor(long pHReading, int regiment) {
  if (((millis() - previouspHMillis) > 3000) && pHSamples < 20) {
    totalpH = totalpH + pHReading;

    previouspHMillis = millis();

    pHSamples++;

    //Serial.print("pHSamples: "); Serial.print(pHSamples); Serial.print(", totalpH: "); Serial.println(totalpH);

    regulatepH = 0;
    return 0;
  }
  else if (pHSamples >= 20 && regulatepH == 0) {
    // Can removed the top level if function
    if ((totalpH / pHSamples) < regi[regiment].lowerpH) {
      long flowRateFactorpH = ((regi[regiment].upperpH + regi[regiment].lowerpH) / 2 - (totalpH / pHSamples))*0.4;          //0.4 is a correction factor (it can be changed if necessary)
      phDosingTime = flowRateFactorpH * dosingPumps.getPumpCalibrationTime(4) / 25 * reserviorSize;   //Serial.print("phDosingTime (Increase): "); Serial.println(phDosingTime);

      totalpH = 0;

      if (phDosingTime < 0) {
        pHSamples = 0;
        regulatepH = 0;
      }
      else regulatepH = 1;

    }
    else if ((totalpH / pHSamples) > regi[regiment].upperpH) {
      long flowRateFactorpH = ((regi[regiment].upperpH + regi[regiment].lowerpH) / 2 - (totalpH / pHSamples))*0.4;

      if (flowRateFactorpH < 0) {
        flowRateFactorpH = flowRateFactorpH * -1;
      }

      phDosingTime = flowRateFactorpH * dosingPumps.getPumpCalibrationTime(5) / 25 * reserviorSize;   //Serial.print("phDosingTime (Decrease): "); Serial.println(phDosingTime);
      
      totalpH = 0;

      if (phDosingTime < 0) {
        pHSamples = 0;
        regulatepH = 0;
      }
      else regulatepH = 2;
    }
    else if ((totalpH / pHSamples) > regi[regiment].lowerpH && (totalpH / pHSamples) < regi[regiment].upperpH) {
      //Serial.print("In range");
      regulatepH = 0;
      totalpH = 0;
      pHSamples = 0;
      return 2;
    }
  }
  else if (regulatepH == 1) {
    if (dosingPumps.runPumps(4, phDosingTime) == true) {
      phDosingTime = 0;
    }
  }
  else if (regulatepH == 2) {
    if (dosingPumps.runPumps(5, phDosingTime) == true) {
      phDosingTime = 0;
    }
  }

  if (phDosingTime == 0 && regulatepH != 0) {
    pHSamples = 0;
    regulatepH = 0;
    return 1;
  }
  else
    return 0;
}

// enable_dosing()
// 1. enable dosing and initialise timers 
void Dosing::enable_dosing() {
  startDosing = true;
  ECCounter = millis();
  phCounter = millis();
}

// disable_dosing()
// 1. Set all variables to default values
void Dosing::disable_dosing() {
  // turn all pumps off
  for (int i = 0; i <= 5; i++) {
    dosingPumps.runPumps(i, 0);
  }

  // restart EC dosing function variables
  totalEC = 0;
  ECSamples = 0;
  regulateEC = 0;

  // reinitialise pump timers
  SolADosingTime = 0;
  SolBDosingTime = 0;
  SolCDosingTime = 0;
  ECDrainTime = 0;
  ECFillTime = 0;

  // restart pH dosing function variables
  totalpH = 0;
  pHSamples = 0;
  regulatepH = 0;

  // reinitialise pump timers
  phDosingTime = 0;

  startDosing = false;

  // turn off magnetic mixers
  digitalWrite(magneticMixer, HIGH);

  // turn off pumps
  digitalWrite(fillPump, HIGH);
  digitalWrite(drainPump, HIGH);
}

// checkNutrientBalace()
// 1. Dosing Requested - initialise timers
// 2. Dose EC - keep checking EC till in range
// 3. Dose pH - keep checking pH till in range
void Dosing::checkNutrientBalace(long ECReading, long pHReading, int regiment) {
  if (startDosing = true) {
    if ((millis() - ECCounter) > ECCheckTime) {
      ECCounter = millis();
      ECCheckTime = 60000;
    }
    else if ((millis() - ECCounter) > ECCheckTime && ECchecked == false) {
      digitalWrite(magneticMixer, LOW);   //start mixing nutrient + pH solution
      int reset;
      reset = ECMonitor(ECReading, regiment);
      if (reset > 0) {
        if (reset == 1) {
          ECCheckTime = 90000;    // wait 1.5 minutes before checking EC again
          ECCounter = millis();
        }
        else if (reset == 2) {
          ECchecked = true;
          phCounter = millis();
        }
      }
    }
    else if (millis() - phCounter > 120000 && ECchecked == true) {
      int reset;
      reset = pHMonitor(pHReading, regiment);
      if (reset > 0) {
        if (reset == 1) {
          phCounter = millis();
        }
        else if (reset == 2) {
          ECchecked = false;
          ECCheckTime = 900000; //re-check EC and pH every 15 minutes
          ECCounter = millis();
          firstRun = true;  //needed for checking reservior level
          digitalWrite(magneticMixer, HIGH);    // stop mixing nutrient + pH solution
        }
      }
    }
  }
}
