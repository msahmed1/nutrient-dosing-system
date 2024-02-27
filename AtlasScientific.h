#ifndef Atlas_Scientific_H
#define Atlas_Scientific_H

#include "arduino.h"
#include <SoftwareSerial.h>                  //Include the software serial library
//#include <SoftwareSerial/src/SoftwareSerial.h>

class AtlasScientific {
public:
  AtlasScientific(int baud_circuits, int NUM_CIRCUITS, int reading_delay);
  AtlasScientific();
  ~AtlasScientific();

  void begin();
  char* getChannelName(int channelNum);
  void do_sensor_readings();
  float getChannelReading(int channelNum);
  char* getContinuousChannelReading(int channelNum);
  // switch_channel(), request_reading() and open_channel() are called by do_sensor_readings
  void switch_channel();
  void request_reading();
  void open_channel();

  void setToContinuousReading(int channelNum);
  void setToAsynchronousReading(int channelNum);
  void pHCalibration(char cali[], long phValue);
  void ECCalibration(char cali[], long consentration);
  //char* checkSensorResponce();
  bool checkStampResponce();

  void changeECParameters(char Parameter[]);


private:
  const int RX = 11;
  const int TX = 10;
  SoftwareSerial sSerial;                 // RX, TX  - Name the software serial library sSerial
  // assigned to pins 10 and 11 for compatibilty with other arduino uno and mega boards
  const int s0 = 7;                   // Tentacle uses pin 7 for multiplexer control S0
  const int s1 = 6;                   // Tentacle uses pin 6 for multiplexer control S1
  const int enable_1 = 5;                 // Tentacle uses pin 5 to control pin E on shield 1

  int baud_circuits;
  const int NUM_CIRCUITS;                 // <-- CHANGE THIS | set how many UART circuits are attached to the Tentacle
  //String* channel_names;
  //char *channel_names[] = { "EC", "pH" };
  char channel_0[3] = "EC";               // <-- CHANGE THIS. A list of channel names (this list should have TOTAL_CIRCUITS entries)
  char channel_1[3] = "pH";               // <-- CHANGE THIS. A list of channel names (this list should have TOTAL_CIRCUITS entries)

  char sensordata[10];                  // A 10 byte character array to hold incoming data from the sensors (Do I need this many characters?)
  unsigned char sensor_bytes_received = 0;        // We need to know how many characters bytes have been received

  char contECreading[10];
  char contpHreading[10];

  float ECreading = 0.0;                 // Variable to hold channel 2 (EC) reading
  float prevECReading = -1.0;                // store previous channel 2 (EC) reading

  float pHreading = 0.0;                 // store channel 1 (pH) reading
  float prevpHReading = -1.0;                // store previous channel 1 (pH) reading

  int errorCounterEC;
  int errorCounterpH;

  //char readings[2][15];                 // an array of strings to hold the readings of each channel

  unsigned int channel = 0;               // INT pointer to hold the current position in the channel_ids/channel_names array
                              //unsigned int is used to exploite the roll over behaviour of this data type

  const unsigned int reading_delay;           // delay between each reading.
                              // low values give fast reading updates, <1 sec per circuit.
                              // high values give your Ardino more time for other stuff
  unsigned long next_reading_time;            // holds the time when the next reading should be ready from the circuit
  boolean request_pending = false;            // wether or not we're waiting for a reading

  char ECParameter[4] = "EC";
  bool AsynchronousReading = true;
  bool continuousReading = false;
};
#endif
