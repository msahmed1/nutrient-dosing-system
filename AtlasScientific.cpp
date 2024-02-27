#include "AtlasScientific.h"

AtlasScientific::AtlasScientific(int baudRateOfCircuits, int numberOfircuits, int readingDelay) : baud_circuits(baudRateOfCircuits), NUM_CIRCUITS(numberOfircuits), reading_delay(readingDelay), sSerial(RX, TX) {
}

// default constructor
AtlasScientific::AtlasScientific() : baud_circuits(9600), NUM_CIRCUITS(2), reading_delay(250), sSerial(RX, TX) {
  //baud_circuits = 9600;               // Set the soft serial port to 9600 (change if all your devices use another baudrate)
}

AtlasScientific::~AtlasScientific() {
  //delete[] readings;
}

// initialise stamps on tentical shield
void AtlasScientific::begin() {
  pinMode(s0, OUTPUT);                        // set the digital output pins for the serial multiplexer
  pinMode(s1, OUTPUT);
  pinMode(enable_1, OUTPUT);

  //readings = new String[NUM_CIRCUITS];
  sSerial.begin(baud_circuits);               // Set the soft serial port to 9600 (change if all your devices use another baudrate)

  //Need to set up some initialisation code here
  /*
     Need to set tentecal shield to continious mode
      to confirm I need to test them
  */
  // Can make the below for loop more robust by ensuring stamps enter continious reading mode
  // For now trust they enter continious reading mode
  for (int i = 0; i < NUM_CIRCUITS; i++) {
    channel = i;
    open_channel();
    sSerial.print(F("C,0\r"));
  }
}

// getChannelName()
// 1. Print Channel name
// change channelName to stampName
char* AtlasScientific::getChannelName(int channelNum) {
  if (channelNum == 0) {
    return channel_0;
  }
  else if (channelNum == 1) {
    return channel_1;
  }
};

// getChannelReading()
// 1. print reading for each stamp
float AtlasScientific::getChannelReading(int channelNum) {
  if (channelNum == 0) {
    return ECreading;
  }
  else if (channelNum == 1) {
    return pHreading;
  }
}

// getContinuousChannelReading()
// 1. print stamp reading continiously
char* AtlasScientific::getContinuousChannelReading(int channelNum) {
  if (channelNum == 0) {
    return contECreading;
  }
  else if (channelNum == 1) {
    return contpHreading;
  }
}

// do_sensor_readings()
// 1. process sensor readings for each stamp asynchronously
// 2. process sensor reading continiously
void AtlasScientific::do_sensor_readings() {
  if (AsynchronousReading == true) {
    if (request_pending) {                  // is a request pending?
      if (checkStampResponce() == true) {
        // update the readings array with this circuits data
        if (channel == 0) {
          float currentReading = atof(sensordata); // convert ASCII to long using atof()
          
          if (prevECReading == -1) {    // first run
            ECreading = currentReading;
            prevECReading = currentReading;
          }
          else if ((prevECReading*1.15) >= currentReading && (prevECReading*0.85) <= currentReading) {    // in range
            ECreading = currentReading;
            prevECReading = currentReading;
            errorCounterEC = 0;
          }
          else {    // out of range
            //ECreading = prevECReading;
            ECreading = currentReading;
            // use error counter to eventually diplay an error message on homescreen
            errorCounterEC++;
          }
        }
        else if (channel == 1) {
          float currentReading = atof(sensordata);

          if (prevpHReading == -1) {    // first run
            pHreading = currentReading;
            prevpHReading = currentReading;
          }
          else if ((prevpHReading*1.15) >= currentReading && (prevpHReading*0.85) <= currentReading) {  // in range
            pHreading = currentReading;
            prevpHReading = currentReading;
            errorCounterpH = 0;
          }
          else {    // out of range
            pHreading = currentReading;   //still print current reading, instead of previous
            errorCounterpH++;
          }
        }

        // un-comment to see the real update frequency of the readings / debug
        // need to edit the below code as it is legacy from the previous implementation
        //Serial.print(channel_names[channel]); Serial.print(" update:\t"); Serial.println(readings[channel]);

        memset(sensordata, 0, sizeof(sensordata));    // clear sensordata array;

        request_pending = false;            // toggle request_pending
      }
    }
    else {                          // no request is pending,
      if (reading_delay <= millis() - next_reading_time) {
        switch_channel();               // switch to the next channel
        request_reading();                // do the actual UART communication
        next_reading_time = millis();         // schedule the reading of the next sensor
      }
    }
  }
  else if (continuousReading == true) { // Continious reading used for calibation
    if (checkStampResponce() == true) {
      strcat(sensordata, "\0");
      if (channel == 0) {
        strcpy(contECreading, sensordata);    // update the readings array with this circuits data
      }
      else if (channel == 1) {
        strcpy(contpHreading, sensordata);
      }

      // un-comment to see the real update frequency of the readings / debug
      /*Serial.print(channel_names[channel]); Serial.print(" update:\t"); Serial.println(readings[channel]);*/

      memset(sensordata, 0, sizeof(sensordata));      // clear sensordata array;
    }
  }
}

// checkStampResponce()
// 1. Serial read bytes from stamp
// 2. Stop reading when * is recived
bool AtlasScientific::checkStampResponce() {
  if (sSerial.available() > 0) {          // If data has been transmitted from an Atlas Scientific device
    sensor_bytes_received = sSerial.readBytesUntil(13, sensordata, 30); //we read the data sent from the Atlas Scientific device until we see a <CR>. We also count how many character have been received
    //Serial.println("SensorData[0]: "); Serial.println(sensordata[0]);
    sensordata[sensor_bytes_received] = 0;    // we add a 0 to the spot in the array just after the last character we received. This will stop us from transmitting incorrect data that may have been left in the buffer

    if (sensordata[0] != '*') {
      return true;
    }
    //Serial.println("SensorData: "); for (int i = 0; i <= 30; i++) { Serial.print(sensordata[i]); }Serial.print("\n");
    //Serial.print("sensordata: "); Serial.println(sensordata);
    //Serial.print("sensor_bytes_received: "); Serial.println(sensor_bytes_received);
  }
  return false;
}

// switch_channel()
// 1. used to asyncrinously switch between stamps
void AtlasScientific::switch_channel() {
  channel = (channel + 1) % NUM_CIRCUITS;     // switch to the next channel (increase current channel by 1, and roll over if we're at the last channel using the % modulo operator)
  open_channel();                             // configure the multiplexer for the new channel - we "hot swap" the circuit connected to the softSerial pins
  sSerial.flush();                            // clear out everything that is in the buffer already
}

// request_reading()
// 1.Request a reading from the current channel asyncrynously
void AtlasScientific::request_reading() {
  if (request_pending != true) {
    //Serial.println("request_reading = true");
    request_pending = true;
    sSerial.print(F("r\r"));                     // <CR> carriage return to terminate message
  }
}

// open_channel()
// 1. Open a channel via the Tentacle serial multiplexer
void AtlasScientific::open_channel() {
  switch (channel) {
  case 0:                                  // if channel==0 then we open channel 0
    digitalWrite(enable_1, LOW);           // setting enable_1 to low activates primary channels: 0,1,2,3
    digitalWrite(s0, LOW);                 // S0 and S1 control what channel opens
    digitalWrite(s1, LOW);                 // S0 and S1 control what channel opens
    break;

  case 1:
    digitalWrite(enable_1, LOW);
    digitalWrite(s0, HIGH);
    digitalWrite(s1, LOW);
    break;

  case 2:
    digitalWrite(enable_1, LOW);
    digitalWrite(s0, LOW);
    digitalWrite(s1, HIGH);
    break;

  case 3:
    digitalWrite(enable_1, LOW);
    digitalWrite(s0, HIGH);
    digitalWrite(s1, HIGH);
    break;

  default:
    //you can disable serial communication (UART 0-3) by setting enable_1 = HIGH
    digitalWrite(enable_1, HIGH);
    break;
  }
}

/*void printData() {
  if (millis() >= next_serial_time) {                // is it time for the next serial communication?
    Serial.println("-------------");
    for (int i = 0; i < NUM_CIRCUITS; i++) {         // loop through all the sensors
      Serial.print(channel_names[i]);                // print channel name
      Serial.print(":\t");
      Serial.println(readings[i]);                   // print the actual reading
    }
    Serial.println("-");
    next_serial_time = millis() + send_readings_every;
  }
}*/

//need to add which channel to send the command to
//Need to add calibation soltion value, as user can choose
void AtlasScientific::pHCalibration(char cali[], long phValue) {

  char *calibrationCommand = (char *)malloc(15);
  char *calibrationValue = (char *)malloc(6);
  strcpy(calibrationCommand, "Cal,");
  strcat(calibrationCommand, cali);
  strcat(calibrationCommand, ",");
  dtostrf(phValue, 3, 2, calibrationValue);
  strcat(calibrationCommand, calibrationValue);

  sSerial.print(calibrationCommand);                  // Send the command from the computer to the Atlas Scientific device using the softserial port
  sSerial.print(F("\r"));                            // <CR> carriage return to terminate message

  free(calibrationCommand);
  free(calibrationValue);
}

// Overloaded function, to reduce complexity in the main code
void AtlasScientific::ECCalibration(char cali[], long consentration) {

  if (cali == "dry") {
    //send low calibration comand
    sSerial.print(F("Cal,dry\r"));            // Send the command from the computer to the Atlas Scientific device using the softserial port\
                            <CR> carriage return to terminate message
  }
  else {
    char *calibrationCommand = (char *)malloc(16);
    char *calibrationValue = (char *)malloc(9);
    strcpy(calibrationCommand, "Cal,");
    strcat(calibrationCommand, cali);
    strcat(calibrationCommand, ",");
    //sprintf(calibrationValue, "%f", consentration);
    dtostrf(consentration, 5, 0, calibrationValue);
    strcat(calibrationCommand, calibrationValue);

    //send mid calibration comand
    sSerial.print(calibrationCommand);
    sSerial.print(F("\r"));

    free(calibrationCommand);
    free(calibrationValue);
  }
}

// setToContinuousReading()
// 1. sets stamp to periodically read the sensor data
void AtlasScientific::setToContinuousReading(int channelNum) {
  channel = channelNum;
  open_channel();
  sSerial.print(F("C,1\r"));
  /*I can increase robustnes of detecting whether the stamps are
  * in continious or asynchronous mode by sending "C,?" and
  * analysing the resonse or each stamp*/
  //the following are used in do_sensor_reading()

  continuousReading = true;
  AsynchronousReading = false;
}

//setToAsynchronousReading()
// 1. Send comand to stamp to read sensor response only when requested
void AtlasScientific::setToAsynchronousReading(int channelNum) {
  channel = channelNum;
  open_channel();
  sSerial.print(F("C,0\r"));

  //the following are used in do_sensor_reading()
  continuousReading = false;
  AsynchronousReading = true;
}

/*char* checkSensorResponce() {
  if (sSerial.available() > 0) {                 // If data has been transmitted from an Atlas Scientific device
    sensor_bytes_received = sSerial.readBytesUntil(13, sensordata, 30); //we read the data sent from the Atlas Scientific device until we see a <CR>. We also count how many character have been received
    sensordata[sensor_bytes_received] = 0;       // we add a 0 to the spot in the array just after the last character we received. This will stop us from transmitting incorrect data that may have been left in the buffer
  }

  return sensordata;
}*/

void AtlasScientific::changeECParameters(char Parameter[]) {
  channel = 0;
  open_channel();

  char *cmd = (char *)malloc(8);
  strcpy(cmd, "0,");
  strcat(cmd, ECParameter);
  strcat(cmd, ",0");

  //Send comand to disable previous EC units
  sSerial.print(cmd);
  sSerial.print(F("\r"));

  strcpy(cmd, "1,");
  strcat(cmd, Parameter);
  strcat(cmd, ",1");

  sSerial.print(cmd);
  sSerial.print(F("\r"));

  //Free SRAM memory
  free(cmd);

  strcpy(ECParameter, Parameter);
}
