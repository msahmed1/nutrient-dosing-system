//This code is intended to work with the following Equipment:
// 1. Arduino Mega
// 2. Atlas Scientific Whitelabs Tentacle Shield
// 3. Atlas Scientific pH probe
// 4. Atlas Scitentific EC probe
// 5. dalas temperature sensor
// 6. 3 monitary switches
// 7. 11 relay modules
// 8. 5 peristaltic pumps
// 9. 4 pumps
// 10. 5 magnetic mixers
// 11. 20X4 LCD screen
// 12. 2 DS18b20 temperature sensor
// 13. 2 float switches

// Pin Out
/* SCL: I2C
 * SDA: I2C
 * 5: MUX ENABLE 1
 * 6: MUX S1
 * 7: MUX S0
 * 10: TX
 * 11: RX

 * 32: Up Button
 * 30: Select Button
 * 28: Down Button

 * 26: Temperature sensor

 * 52: Lower level Float switch Reservoir
 * 53: Upper level Float switch Reservoir
 
 * 25: Magnetic mixer
 * 27: Agitator pump A/C
 * 37: pH Down    37
 * 35: pH Up    35
 * 33: Neutrient C  
 * 31: Neutrient B
 * 39: Neutrient A

 * 22: Fill pump
 * 24: Drain

 */

#include <MemoryFree.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "RTClib.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <avr/wdt.h>

#include "AtlasScientific.h"
#include "Dosing.h"
#include "PeristalticPump.h"

// Creating an instance of AtlasScientific class
// Parameters:
// 1. BaudRate
// 2. Number of sensors
// 3. Reading Delay
AtlasScientific probes(9600, 2, 500);

// Creating an instance of PeristalticPump class
// Parameters:
// 1. pump 1 pin
// 2. pump 2 pin
// 3. pump 3 pin
// 4. pump 4 pin
// 5. pump 5 pin
PeristalticPump pumps(37, 35, 33, 31, 29);

// Creating an instance of dosing class
// Parameters:
// 1. instance of PeristalticPump opbject
// 2. Lower float switch
// 3. Upper float switch
// 4. Magnetic mixer
// 5. fill pump
// 6. drain pump
Dosing dosing(pumps, 52, 53, 25, 22, 24);

LiquidCrystal_I2C lcd(0x27, 20, 4);   // Set the LCD address to 0x27 for a 20 chars and 4 line display

RTC_DS3231 rtc;

#define ONE_WIRE_BUS 26               // Data wire is plugged into port 8 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature. 

//pinout
//Buttons:
const int up = 32;
const int sel = 30;
const int down = 28;

//Relays
const int agitatorPump = 27;

//to deteermine rising and falling edge
boolean current_up = LOW;
boolean last_up = LOW;
boolean current_sel = LOW;
boolean last_sel = LOW;
boolean current_down = LOW;
boolean last_down = LOW;

//used in displaying different pages
uint8_t page_counter = 1;

//home page (case 1)
bool subpage_toggle = true;
unsigned long loopTime;
int cycleTime = 5000;

uint8_t currentDay;
int currentMonth;
int currentYear;
int previousDay = 0;

float idealpH = 0;
long idealEC = 0;

//System variables (case 2)
uint8_t subpage_2_counter = 0;
char tempUnit[2] = { "C" };        //Default is degrees celsius
char ECUnit[4][4] = { "EC","TDS","S","SG" };//Default is Siemens
uint8_t ECUnitIndex = 0;

//Regiment configuration(case 3)
uint8_t subpage_3_counter = 0;
uint8_t numOfRegiments = 2;
uint8_t regimentsDays = 1;

//set pump configuration variables (case 4)
uint8_t subpage_4_counter = 0;

//pH Calibratin (case 5)
uint8_t subpage_5_counter = 0;
float lowpHCalSol = 4;
float midpHCalSol = 7;
float highpHCalSol = 10;
uint8_t lastpHCaliDay = 0;
uint8_t lastpHCaliMonth = 0;
int lastpHCaliYear = 0;

//EC Calibration (case 6)
uint8_t subpage_6_counter = 0;
unsigned int lowECCalSol = 12880;
long highECCalSol = 80000;
uint8_t lastECCaliDay = 0;
uint8_t lastECCaliMonth = 0;
int lastECCaliYear = 0;

//set time and date variables (case 7)
uint8_t subpage_7_counter = 0;
int rtcCounter = 1;
boolean editRTC = false;
boolean last_editRTC = false;
uint8_t hour_ = 0;
uint8_t minute_ = 0;
uint8_t second_ = 0;
uint8_t day_ = 2;
uint8_t month_ = 12;
int year_ = 2018;

//regiment X config (case 8)
uint8_t subpage_8_counter = 0;
int increment = 1;

//configure Peristaltic pumps (case 9)
uint8_t subPagePumpCounter = 1;
bool testCurrentCali = false;
bool testNewCali = false;
uint8_t PumpNumber = 1;

//used in checking when its time to switch to next regiment
int elapsed_day = 0;
int regiment = 0;

// Agitator pump timer
unsigned long lastAgitateTime;

//Custom characters
byte upArrow[8] = {   //UP arrow
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000
};

byte downArrow[8] = {   //DOWN arrow
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000
};

byte leftArrow[8] = {   //LEFT arrow
  B00010,
  B00100,
  B01000,
  B11111,
  B01000,
  B00100,
  B00010,
  B00000
};

byte deg[8] = {   //Degreed symbol
  B00011,
  B00011,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

void setup() {
  pinMode_setup();    // Declare pin modes

  // initialize the LCD
  lcd.begin();
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);
  lcd.createChar(3, leftArrow);
  lcd.createChar(4, deg);
  lcd.backlight();

  //RTC lost power, set the time
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.begin(9600);                   // Set the hardware serial port to 9600

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  probes.begin();
  pumps.begin();

  // Start up DS18b20 the library
  sensors.begin();

  //The below code can be used to ensure that temperature sensor is connected (health monitoring)
  /*Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");*/

  EEPROM_setup();

  //Default values used during debugging
  dosing.setUpperpH(1, 6.4);
  dosing.setLowerpH(1, 6.1);
  dosing.setSolA(1, 1);
  dosing.setSolB(1, 1);
  dosing.setSolC(1, 1);
  dosing.setUpperEC(1, 1100);
  dosing.setLowerEC(1, 900);

  /*Serial.print("upperpH: ");
  Serial.println(dosing.getUpperpH(1));
  Serial.print("LowerpH: ");
  Serial.println(dosing.getLowerpH(1));
  Serial.print("solA: ");
  Serial.println(dosing.getSolA(1));
  Serial.print("solB: ");
  Serial.println(dosing.getSolB(1));
  Serial.print("solC: ");
  Serial.println(dosing.getSolC(1));
  Serial.print("upperEC: ");
  Serial.println(dosing.getUpperEC(1));
  Serial.print("LowerEC: ");
  Serial.println(dosing.getLowerEC(1));*/

  dosing.setUpperpH(2, 6.4);
  dosing.setLowerpH(2, 6.1);
  dosing.setSolA(2, 1.8);
  dosing.setSolB(2, 1.2);
  dosing.setSolC(2, 0.6);
  dosing.setUpperEC(2, 1825);
  dosing.setLowerEC(2, 1775);
  
  /*Serial.print("upperpH: ");
  Serial.println(dosing.getUpperpH(2));
  Serial.print("LowerpH: ");
  Serial.println(dosing.getLowerpH(2));
  Serial.print("solA: ");
  Serial.println(dosing.getSolA(2));
  Serial.print("solB: ");
  Serial.println(dosing.getSolB(2));
  Serial.print("solC: ");
  Serial.println(dosing.getSolC(2));
  Serial.print("upperEC: ");
  Serial.println(dosing.getUpperEC(2));
  Serial.print("LowerEC: ");
  Serial.println(dosing.getLowerEC(2));*/

  watchdogSetup();
}

boolean debounce(boolean last, int pin)
{
  boolean current = digitalRead(pin);
  if (last != current)
  {
    delay(20);
    current = digitalRead(pin);
  }
  return current;
}

void loop() {
  current_up = debounce(last_up, up);
  current_sel = debounce(last_sel, sel);
  current_down = debounce(last_down, down);

  DateTime now = rtc.now();

  //If we are not in a sub menue then we can start scrolling through the differenc screens
  if (subpage_2_counter == 0 && subpage_3_counter == 0 && subpage_4_counter == 0 && subpage_5_counter == 0 && subpage_6_counter == 0 && subpage_8_counter == 0 && subPagePumpCounter == 1) {
    
    if (last_up == LOW && current_up == HIGH) {   //Page Up
      lcd.clear();                                //Clear lcd to print new data
      if (page_counter < 6) {                     //cycle through pages
        page_counter++;                           //Page up
      }
      else {
        page_counter = 1;
      }
    }
    last_up = current_up;

    if (last_down == LOW && current_down == HIGH) {
      lcd.clear();                              //Clear lcd to print new data
      if (page_counter > 1) {                   //cycle through pages
        page_counter--;                         //Page down
      }
      else {
        page_counter = 6;
      }
    }
    last_down = current_down;                   //update last state

    //Only balances nutrients if user is not editing the subpages
    dosing.checkNutrientBalace(probes.getChannelReading(0), probes.getChannelReading(1), regiment);
  }

  probes.do_sensor_readings();    //get readings from pH and EC probes

  switch (page_counter) {
  //home page
  case 1:
    //Update RTC values
    currentDay = now.day();
    currentMonth = now.month();
    currentYear = now.year();

    //Toggel cetween two screens
    if ((millis() - loopTime) >= cycleTime) {
      if (subpage_toggle == true) {
        subpage_toggle = false;
        cycleTime = 5000;   //Display for 5 seconds
      }
      else {
        subpage_toggle = true;
        cycleTime = 10000;  //Display for 10 seconds
      }
      loopTime = millis();
      lcd.clear();
    }

    if (subpage_toggle == true) {
      lcd.setCursor(1, 0);
      lcd.print("NAME  VAL.  TARGET");

      lcd.setCursor(2, 1);
      lcd.print(probes.getChannelName(0));

      lcd.setCursor(7, 1);
      lcd.print(probes.getChannelReading(0)); //not printing anything after the decimal point

      lcd.setCursor(13, 1);
      idealEC = (dosing.getUpperEC(regiment) + dosing.getLowerEC(regiment)) / 2;  //Calculate Ideal EC
      
      /*Serial.println("dosing.getUpperEC(regiment)");
      Serial.println(dosing.getUpperEC(regiment));
      Serial.println("dosing.getLowerEC(regiment)");
      Serial.println(dosing.getLowerEC(regiment));
      Serial.println("idealEC");
      Serial.println(idealEC);*/

      if (idealEC == 0) {
        lcd.print("(XXXX)");
      }
      else {
        lcd.print("(");
        lcd.print(idealEC);
        lcd.print(")");
      }

      lcd.setCursor(2, 2);
      lcd.print(probes.getChannelName(1));

      lcd.setCursor(7, 2);
      lcd.print(probes.getChannelReading(1));

      lcd.setCursor(13, 2);
      idealpH = (dosing.getUpperpH(regiment) + dosing.getLowerpH(regiment)) / 2;  //calculate ideal pH
      
      Serial.println("dosing.getUpperpH(regiment)");
      Serial.println(dosing.getUpperpH(regiment));
      Serial.println("dosing.getLowerpH(regiment)");
      Serial.println(dosing.getLowerpH(regiment));
      Serial.println("idealpH");
      Serial.println(idealpH);

      if (idealpH == 0) {
        lcd.print("(X.XX)");
      }
      else {
        lcd.print("(");
        lcd.print(idealpH);
        lcd.print(")");
      }

      //Print Resivour Temperature
      lcd.setCursor(1, 3);
      lcd.print("Temp");
      lcd.setCursor(7, 3);
      sensors.requestTemperatures(); // Send the command to get temperatures
      
      long avgTempC = 0;

      // Get reading from each sensor and calculate the average
      for (int i = 0; i < 2; i++) {
        avgTempC += sensors.getTempCByIndex(i)/2;
      }

      if (tempUnit[0] == 'C') {
        lcd.print(avgTempC);
      }
      else {
        lcd.print(DallasTemperature::toFahrenheit(avgTempC));
      }

      lcd.setCursor(12, 3);
      lcd.write(4);       // print degrees symbol
      lcd.print(tempUnit[0]);   //print unit lable
    }

    if (subpage_toggle == false) {
      lcd.setCursor(4, 0);
      lcd.print("HOME  SCREEN");

      lcd.setCursor(0, 1);
      lcd.print("SYS. STATUS:");
      lcd.setCursor(13, 1);
      lcd.print("RUNNING");

      lcd.setCursor(0, 2);
      lcd.print("TIME:");

      lcd.setCursor(6, 2);
      if (now.hour() < 10) {
        lcd.print("0");
      }
      lcd.print(now.hour(), DEC);

      lcd.print(':');

      if (now.minute() < 10) {
        lcd.print("0");
      }
      lcd.print(now.minute(), DEC);

      lcd.print(':');

      if (now.second() < 10) {
        lcd.print("0");
      }
      lcd.print(now.second(), DEC);

      lcd.setCursor(0, 3);
      lcd.print("DATE:");

      lcd.setCursor(6, 3);
      if (currentDay < 10) {
        lcd.print("0");
      }
      lcd.print(currentDay, DEC);

      lcd.print('/');

      if (currentMonth < 10) {
        lcd.print("0");
      }
      lcd.print(currentMonth, DEC);

      lcd.print('/');

      lcd.print(currentYear, DEC);
    }
    break;
    
  case 2:
    // pring heading
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM CONFIGERATION");

    //when we select edit,  change test to "Next"
    lcd.setCursor(7, 3);
    if (subpage_2_counter == 0) {
      lcd.print("(Edit)");
    }
    else {
      lcd.print("(Next)");
    }

    if (subpage_2_counter != 0) {
      lcd.setCursor(19, 1);
      lcd.write(0);
      lcd.setCursor(19, 2);
      lcd.write(1);
    }

    if (subpage_2_counter >= 4) {
      lcd.setCursor(0, 3);
      lcd.print("(OK)");
    }
    else {
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(") ");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
    }

    //Scrolling
    if (last_sel == LOW && current_sel == HIGH) {
      //clear screen content
      lcd.clear();

      // Save reservoir size to eeprom
      EEPROM.put(1, dosing.getReserviorSize());
      EEPROM.update(3, tempUnit[0]);
      EEPROM.update(4, ECUnitIndex);

      if (subpage_2_counter < 6) {
        dosing.disable_dosing();
        subpage_2_counter++;
      }
      else {
        subpage_2_counter = 1;
      }
    }

    last_sel = current_sel;

    if (subpage_2_counter == 0) {
      lcd.setCursor(0, 1);
      lcd.print("Res. Size: ");
      if (dosing.getReserviorSize() < 1000) {
        lcd.print(" ");
      }
      if (dosing.getReserviorSize() < 100) {
        lcd.print(" ");
      }
      if (dosing.getReserviorSize() < 10) {
        lcd.print(" ");
      }
      lcd.print(dosing.getReserviorSize());
      lcd.print("L");
      lcd.setCursor(0, 2);
      lcd.print("Temp. unit: ");
      lcd.write(4);
      lcd.print(tempUnit[0]);
    }

    if (subpage_2_counter == 1) {
      // place down arrow on editing value
      lcd.setCursor(18, 1);

      lcd.write(3);
      lcd.setCursor(0, 1);
      lcd.print("Res. Size: ");
      if (dosing.getReserviorSize() < 1000) {
        lcd.print(" ");
      }
      if (dosing.getReserviorSize() < 100) {
        lcd.print(" ");
      }
      if (dosing.getReserviorSize() < 10) {
        lcd.print(" ");
      }
      lcd.print(dosing.getReserviorSize());
      lcd.print("L");

      lcd.setCursor(0, 2);
      lcd.print("Temp. unit: ");
      lcd.write(4);
      lcd.print(tempUnit[0]);

      if (last_up == LOW && current_up == HIGH) {
        if (dosing.getReserviorSize() < 1000) dosing.setReserviorSize(dosing.getReserviorSize() + 10);
        else dosing.setReserviorSize(0);
      }
      if (last_down == LOW && current_down == HIGH) {
        if (dosing.getReserviorSize() > 0) dosing.setReserviorSize(dosing.getReserviorSize() - 10);
        else dosing.setReserviorSize(1000);
      }
    }

    if (subpage_2_counter == 2) {
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("Res. Size: ");
      if (dosing.getReserviorSize() < 1000) {
        lcd.print(" ");
      }
      if (dosing.getReserviorSize() < 100) {
        lcd.print(" ");
      }
      if (dosing.getReserviorSize() < 10) {
        lcd.print(" ");
      }
      lcd.print(dosing.getReserviorSize());
      lcd.print("L");
      // place down arrow on editing value

      lcd.setCursor(0, 2);
      lcd.print("Temp. unit: ");
      lcd.write(4);
      lcd.print(tempUnit[0]);

      if (last_up == LOW && current_up == HIGH) {
        if (tempUnit[0] == 'C') {
          strcpy(tempUnit, "F");
        }
        else {
          strcpy(tempUnit, "C");
        }
      }

      if (last_down == LOW && current_down == HIGH) {
        if (tempUnit[0] == 'C') {
          strcpy(tempUnit, "F");
        }
        else {
          strcpy(tempUnit, "C");
        }
      }
    }

    if (subpage_2_counter == 3) {
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("Temp. unit: ");
      lcd.write(4);
      lcd.print(tempUnit[0]);

      lcd.setCursor(0, 2);
      lcd.print("EC Units: ");
      lcd.print(ECUnit[ECUnitIndex]);

      if (last_up == LOW && current_up == HIGH) {
        //must lear lcd or trailing characters will remain
        lcd.clear();
        if (ECUnitIndex < 3) {
          ++ECUnitIndex;
        }
        else {
          ECUnitIndex = 0;
        }
        probes.changeECParameters(ECUnit[ECUnitIndex]);
      }
      if (last_down == LOW && current_down == HIGH) {
        lcd.clear();
        if (ECUnitIndex > 0) {
          ECUnitIndex--;
        }
        else {
          ECUnitIndex = 3;
        }
        probes.changeECParameters(ECUnit[ECUnitIndex]);
      }
    }

    if (subpage_2_counter == 4) {
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("EC Units: ");
      lcd.print(ECUnit[ECUnitIndex]);

      lcd.setCursor(0, 2);
      lcd.print("Set feed water EC");

      if (last_up == LOW && current_up == HIGH) {
        subpage_2_counter = 7;
        lcd.clear();
      }
    }

    if (subpage_2_counter == 5) {
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("Set feed water EC");

      lcd.setCursor(0, 2);
      lcd.print("Set date and time");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 7;
        lcd.clear();
      }
    }

    if (subpage_2_counter == 6) {
      lcd.setCursor(0, 1);
      lcd.print("Set date and time");

      lcd.setCursor(0, 2);
      lcd.print("Back");

      lcd.setCursor(18, 2);
      lcd.write(3);
      if (last_up == LOW && current_up == HIGH) {
        subpage_2_counter = 0;
        dosing.enable_dosing();
        lcd.clear();
      }
    }

    if (subpage_2_counter == 7) {
      lcd.setCursor(0, 1);
      lcd.print("Save feed water EC");

      lcd.setCursor(0, 2);
      lcd.print("reading as: ");
      lcd.print(probes.getChannelReading(0));

      lcd.setCursor(14, 3);
      lcd.print("(BACK)");

      if (last_down == LOW && current_down == HIGH) {
        subpage_2_counter = 4;
        lcd.clear();
      }

      if (last_up == LOW && current_up == HIGH) {
        dosing.setBaselineEC(probes.getChannelReading(0));
        EEPROM.put(5, probes.getChannelReading(0));
        lcd.clear();
      }
    }
    break;

  case 3:
    lcd.setCursor(2, 0);
    lcd.print("REGIMENT SETTING");

    //pring arrows only when editing
    if (subpage_3_counter != 0) {
      lcd.setCursor(19, 1);
      lcd.write(0);
      lcd.setCursor(19, 2);
      lcd.write(1);
    }
    // if on back menue option then replace up arrow
    if (subpage_3_counter == 3 || subpage_3_counter == 4) {
      lcd.setCursor(0, 3);
      lcd.print("(OK)");
    }
    else {
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(")");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
    }
    lcd.setCursor(7, 3);
    //when we select a page to edit then we want to change text to "Next"
    if (subpage_3_counter == 0) {
      lcd.print("(Edit)");
    }
    else {
      lcd.print("(Next)");
    }

    //Scrolling
    if (last_sel == LOW && current_sel == HIGH) {
      dosing.disable_dosing();
      //clear screen content
      lcd.clear();

      if (subpage_3_counter < 4) {
        subpage_3_counter++;
      }
      else {
        subpage_3_counter = 1;
      }
    }

    last_sel = current_sel;

    if (subpage_3_counter == 0) {
      lcd.setCursor(0, 1);
      lcd.print("No. REGIMENTS: ");
      if (numOfRegiments < 10);
      {
        lcd.print("0");
      }
      lcd.print(numOfRegiments);

      lcd.setCursor(0, 2);
      lcd.print("REGIMENT DAYS: ");
      if (regimentsDays < 10)
      {
        lcd.print("0");
      }
      lcd.print(regimentsDays);
    }

    if (subpage_3_counter == 1) {
      lcd.setCursor(17, 1);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("No. REGIMENTS:");
      lcd.setCursor(15, 1);
      if (numOfRegiments < 10)
      {
        lcd.print("0");
      }
      lcd.print(numOfRegiments);

      if (last_up == LOW && current_up == HIGH) {
        if (numOfRegiments < 21) numOfRegiments++;
        else numOfRegiments = 1;
      }
      if (last_down == LOW && current_down == HIGH) {
        if (numOfRegiments > 1) numOfRegiments--;
        else numOfRegiments = 21;
      }

      lcd.setCursor(0, 2);
      lcd.print("REGIMENT DAYS: ");
      if (regimentsDays < 10)
      {
        lcd.print("0");
      }
      lcd.print(regimentsDays);
    }

    if (subpage_3_counter == 2) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("No. REGIMENTS:");
      lcd.setCursor(15, 1);
      if (numOfRegiments < 10)
      {
        lcd.print("0");
      }
      lcd.print(numOfRegiments);

      lcd.setCursor(0, 2);
      lcd.print("REGIMENT DAYS: ");
      if (regimentsDays < 10)
      {
        lcd.print("0");
      }
      lcd.print(regimentsDays);

      if (last_up == LOW && current_up == HIGH) {
        if (regimentsDays < 14) regimentsDays++;
        else regimentsDays = 1;
      }
      if (last_down == LOW && current_down == HIGH) {
        if (regimentsDays > 1) regimentsDays--;
        else regimentsDays = 14;
      }
    }

    if (subpage_3_counter == 3) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("REGIMENT DAYS: ");
      if (regimentsDays < 10)
      {
        lcd.print("0");
      }
      lcd.print(regimentsDays);
      lcd.setCursor(0, 2);
      lcd.print("CONFIG. REGIMENT");

      if (last_up == LOW && current_up == HIGH) {
        subpage_3_counter = 0;
        page_counter = 8;
        lcd.clear();
      }
      increment = 1;
    }

    if (subpage_3_counter == 4) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("CONFIG. REGIMENT");

      lcd.setCursor(0, 2);
      lcd.print("Back");

      if (last_up == LOW && current_up == HIGH) {
        subpage_3_counter = 0;
        lcd.setCursor(17, 2);
        lcd.print(" ");
        EEPROM.update(7, numOfRegiments);
        EEPROM.update(8, regimentsDays);
        dosing.enable_dosing();
      }
    }
    break;

  case 4:
    lcd.setCursor(1, 0);
    lcd.print("PUMP CONFIGURATION");

    //pring arrows only when editing
    if (subpage_4_counter != 0 && subpage_4_counter != 6) {
      lcd.setCursor(19, 1);
      lcd.write(0);
      lcd.setCursor(19, 2);
      lcd.write(1);
      lcd.setCursor(0, 3);
      lcd.print("(OK)");
    }
    else if (subpage_4_counter == 6) {
      lcd.setCursor(0, 3);
      lcd.print("(Purge)");
    }
    else if (subpage_4_counter == 0) {
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(") ");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
    }
    lcd.setCursor(7, 3);
    //when we select a page to edit then we want to change test to "Next"
    if (subpage_4_counter == 0) {
      lcd.print("(Edit)");
    }
    else {
      lcd.print("(Next)");
    }

    //Scrolling
    if (last_sel == LOW && current_sel == HIGH) {
      dosing.disable_dosing();
      //clear screen content
      lcd.clear();
      if (subpage_4_counter < 7) {
        subpage_4_counter++;
      }
      else {
        subpage_4_counter = 1;
      }
    }

    last_sel = current_sel;

    if (subpage_4_counter == 0) {
      lcd.setCursor(0, 1);
      lcd.print("pH UP PUMP");
      lcd.setCursor(0, 2);
      lcd.print("pH DOWN PUMP");
    }

    if (subpage_4_counter == 1) {
      lcd.setCursor(17, 1);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("pH UP PUMP");

      lcd.setCursor(0, 2);
      lcd.print("pH DOWN PUMP");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 9;
        PumpNumber = 4;
        lcd.clear();
      }
    }

    if (subpage_4_counter == 2) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("pH UP PUMP ");
      lcd.setCursor(0, 2);
      lcd.print("pH DOWN PUMP ");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 9;
        PumpNumber = 5;
        lcd.clear();
      }
    }

    if (subpage_4_counter == 3) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("pH DOWN PUMP");
      lcd.setCursor(0, 2);
      lcd.print("SOL. A PUMP");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 9;
        PumpNumber = 1;
        lcd.clear();
      }
    }

    if (subpage_4_counter == 4) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL. A PUMP");
      lcd.setCursor(0, 2);
      lcd.print("SOL. B PUMP");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 9;
        PumpNumber = 2;
        lcd.clear();
      }
    }

    if (subpage_4_counter == 5) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL. B PUMP");
      lcd.setCursor(0, 2);
      lcd.print("SOL. C PUMP");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 9;
        PumpNumber = 3;
        lcd.clear();
      }
    }

    if (subpage_4_counter == 6) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL. C PUMP");
      lcd.setCursor(0, 2);
      lcd.print("AGITATOR PUMP");


      if (last_up == LOW && current_up == HIGH) {
        digitalWrite(agitatorPump, LOW);
        last_up = current_up;
      }
      if (last_up == HIGH && current_up == LOW) {
        digitalWrite(agitatorPump, HIGH);
        last_up = current_up;
      }
    }

    if (subpage_4_counter == 7) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("AGITATOR PUMP");
      lcd.setCursor(0, 2);
      lcd.print("BACK");

      if (last_up == LOW && current_up == HIGH) {
        subpage_4_counter = 0;
        dosing.enable_dosing();
        lcd.clear();
      }
    }
    break;
  case 5:
    //when we select a page to edit then we want to change text to "Next"
    lcd.setCursor(0, 3);
    lcd.print("(");
    lcd.write(0);
    lcd.print(") ");
    lcd.setCursor(17, 3);
    lcd.print("(");
    lcd.write(1);
    lcd.print(")");

    // Scrolling, sends comand when transitioning to next page
    if (last_sel == LOW && current_sel == HIGH) {
      dosing.disable_dosing();
      //clear screen content
      lcd.clear();
      if (subpage_5_counter == 0) {
        subpage_5_counter = 1;
        probes.setToContinuousReading(1);
      }
      else if (subpage_5_counter == 1) {
        subpage_5_counter = 2;
        probes.pHCalibration("mid", midpHCalSol);
      }
      else if (subpage_5_counter == 2) {
        subpage_5_counter = 3;
        probes.pHCalibration("low", lowpHCalSol);
      }
      else if (subpage_5_counter == 3) {
        subpage_5_counter = 0;
        probes.pHCalibration("high", highpHCalSol);

        // set date of calibration
        //Serial.print("Saving pH calibration Date as: ");
        lastpHCaliDay = currentDay;
        EEPROM.update(343, lastpHCaliDay);
        //Serial.print(lastpHCaliDay);
        //Serial.print("/");
        lastpHCaliMonth = currentMonth;
        EEPROM.update(344, lastpHCaliMonth);
        //Serial.print(lastpHCaliMonth);
        //Serial.print("/");
        lastpHCaliYear = currentYear;
        EEPROM.update(345, lastpHCaliYear);
        //Serial.print(lastpHCaliYear);
        //Serial.print("/");
        subpage_5_counter = 0;

        // return to asynchronous mode
        probes.setToAsynchronousReading(1);
        dosing.enable_dosing();
      }
    }

    last_sel = current_sel;
    
    if (subpage_5_counter == 0) {   // main screen
      lcd.setCursor(0, 0);
      lcd.print("pH PROBE CALIBRATION");
      lcd.setCursor(0, 1);
      lcd.print(" LAST CALIBRATED:");
      lcd.setCursor(10, 2);
      if (lastpHCaliDay < 10)
      {
        lcd.print("0");
      }
      lcd.print(lastpHCaliDay);
      lcd.print('/');
      if (lastpHCaliMonth < 10)
      {
        lcd.print("0");
      }
      lcd.print(lastpHCaliMonth);
      lcd.print('/');
      lcd.print(lastpHCaliYear);
      lcd.setCursor(4, 3);
      lcd.print("(CALIBRATE)");
    }

    if (subpage_5_counter == 1) {   // fist calibration value
      lcd.setCursor(1, 0);
      lcd.print("MID pH CALIBRATION");
      lcd.setCursor(0, 1);
      lcd.print("CONSENTRATION:");
      if (midpHCalSol < 10) {
        lcd.print("0");
      }
      lcd.print(midpHCalSol);
      lcd.setCursor(18, 1);
      lcd.write(3);

      lcd.setCursor(0, 2);
      lcd.print("pH reading:");

      lcd.print(probes.getContinuousChannelReading(1));

      lcd.setCursor(7, 3);
      lcd.print("(Next)");

      if (last_up == LOW && current_up == HIGH) {
        if (midpHCalSol < 9.9) {
          midpHCalSol = midpHCalSol + 0.1;
        }
        else midpHCalSol = 4.1;
      }

      if (last_down == LOW && current_down == HIGH) {
        if (midpHCalSol > 4.1) {
          midpHCalSol = midpHCalSol - 0.1;
        }
        else midpHCalSol = 9.9;
      }
    }

    if (subpage_5_counter == 2) {
      lcd.setCursor(1, 0);
      lcd.print("LOW pH CALIBRATION");
      lcd.setCursor(0, 1);
      lcd.print("CONSENTRATION:");
      if (lowpHCalSol < 10) {
        lcd.print("0");
      }
      lcd.print(lowpHCalSol);
      lcd.setCursor(18, 1);
      lcd.write(3);

      lcd.setCursor(0, 2);
      lcd.print("pH reading:");

      lcd.print(probes.getContinuousChannelReading(1));

      lcd.setCursor(4, 3);
      lcd.print("(CALIBRATE)");

      if (last_up == LOW && current_up == HIGH) {
        if (lowpHCalSol < midpHCalSol - 0.1) {
          lowpHCalSol = lowpHCalSol + 0.1;
        }
        else lowpHCalSol = 0;
      }

      if (last_down == LOW && current_down == HIGH) {
        if (lowpHCalSol > 00) {
          lowpHCalSol = lowpHCalSol - 0.1;
        }
        else lowpHCalSol = midpHCalSol - 0.1;
      }
    }

    if (subpage_5_counter == 3) {
      lcd.setCursor(0, 0);
      lcd.print("HIGH pH CALIBRATION");
      lcd.setCursor(0, 1);
      lcd.print("CONSENTRATION:");
      if (highpHCalSol < 10) {
        lcd.print("0");
      }
      lcd.print(highpHCalSol);
      lcd.setCursor(18, 1);
      lcd.write(3);

      lcd.setCursor(4, 3);
      lcd.print("(CALIBRATE)");

      lcd.setCursor(0, 2);
      lcd.print("pH reading:");

      lcd.print(probes.getContinuousChannelReading(1));

      if (last_up == LOW && current_up == HIGH) {
        if (highpHCalSol < 14) {
          highpHCalSol = highpHCalSol + 0.1;
        }
        else highpHCalSol = midpHCalSol + 0.1;
      }

      if (last_down == LOW && current_down == HIGH) {
        if (highpHCalSol > (midpHCalSol + 0.1)) {
          highpHCalSol = highpHCalSol - 0.1;
        }
        else highpHCalSol = 14;
      }
    }

    break;
  case 6:
    //when we select a page to edit then we want to change text to "Next"
    if (subpage_6_counter != 1) {
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(") ");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
    }

    //Scrolling
    if (last_sel == LOW && current_sel == HIGH) {
      dosing.disable_dosing();
      //clear screen content
      lcd.clear();
      if (subpage_6_counter == 0) {
        probes.setToContinuousReading(0);
        subpage_6_counter = 1;
      }
      else if (subpage_6_counter == 1) {
        probes.ECCalibration("dry", 0);
        subpage_6_counter = 2;
      }
      else if (subpage_6_counter == 2) {
        probes.ECCalibration("low", lowECCalSol);
        subpage_6_counter = 3;
      }
      else if (subpage_6_counter == 3) {
        probes.ECCalibration("high", highECCalSol);
        probes.setToAsynchronousReading(0);
        // set date of calibration
        subpage_6_counter = 0;

        //Serial.print("Saving EC calibration Date as: ");
        lastECCaliDay = currentDay;
        EEPROM.update(347, lastECCaliDay);
        //Serial.print(lastECCaliDay);
        //Serial.print("/");
        lastECCaliMonth = currentMonth;
        EEPROM.update(348, lastECCaliMonth);
        //Serial.print(lastECCaliMonth);
        //Serial.print("/");
        lastECCaliYear = currentYear;
        EEPROM.update(349, lastECCaliYear);
        //Serial.print(lastECCaliYear);
        //Serial.print("/");
        dosing.enable_dosing();

      }
    }

    if (subpage_6_counter == 0) {
      lcd.setCursor(0, 0);
      lcd.print("EC PROBE CALIBRATION");

      lcd.setCursor(0, 1);
      lcd.print(" LAST CALIBRATED:");

      lcd.setCursor(10, 2);
      if (lastECCaliDay < 10) lcd.print("0");
      lcd.print(lastECCaliDay);

      lcd.print('/');

      if (lastECCaliMonth < 10) lcd.print("0");
      lcd.print(lastECCaliMonth);

      lcd.print('/');

      lcd.print(lastECCaliYear);

      lcd.setCursor(4, 3);
      lcd.print("RE-CALIBRATE");
    }

    if (subpage_6_counter == 1) {
      lcd.setCursor(1, 0);
      lcd.print("DRY EC CALIBRATION");

      lcd.setCursor(0, 1);
      lcd.print("CONSENTRATION: 000");

      lcd.setCursor(0, 2);
      lcd.print("EC reading: ");
      lcd.print(probes.getContinuousChannelReading(0));

      lcd.setCursor(4, 3);
      lcd.print("(CALIBRATE)");
    }

    if (subpage_6_counter == 2) {
      lcd.setCursor(1, 0);
      lcd.print("LOW EC CALIBRATION");

      lcd.setCursor(0, 1);
      lcd.print("CONSENTRATION:");
      if (lowECCalSol < 1000) lcd.print("0");
      if (lowECCalSol == 0) lcd.print("000");
      lcd.print(lowECCalSol);

      lcd.setCursor(19, 1);
      lcd.write(3);

      lcd.setCursor(0, 2);
      lcd.print("EC reading: ");
      lcd.print(probes.getContinuousChannelReading(0));

      lcd.setCursor(4, 3);
      lcd.print("(CALIBRATE)");

      //+-10 chosen based on calibration solution
      if (last_up == LOW && current_up == HIGH) {
        if (lowECCalSol < 20000) {
          lowECCalSol = lowECCalSol + 10;
        }
        else lowECCalSol = 8000;
      }
      if (last_down == LOW && current_down == HIGH) {
        if (lowECCalSol > 8000) {
          lowECCalSol = lowECCalSol - 10;
        }
        else lowECCalSol = 20000;
      }
    }

    if (subpage_6_counter == 3) {
      lcd.setCursor(1, 0);
      lcd.print("HIGH EC CALIBRATION");

      lcd.setCursor(0, 1);
      lcd.print("CONSENTRATION:");
      lcd.print(highECCalSol, 0);

      lcd.setCursor(19, 1);
      lcd.write(3);

      lcd.setCursor(0, 2);
      lcd.print("EC reading: ");
      lcd.print(probes.getContinuousChannelReading(0));

      lcd.setCursor(4, 3);
      lcd.print("(CALIBRATE)");

      //+-200 chosen based on calibration solution
      if (last_up == LOW && current_up == HIGH) {
        if (highECCalSol < 99800) {
          highECCalSol = highECCalSol + 200;
        }
        else highECCalSol = 50000;
      }
      if (last_down == LOW && current_down == HIGH) {
        if (highECCalSol > 50000) {
          highECCalSol = highECCalSol - 200;
        }
        else highECCalSol = 99800;
      }
    }
    break;
  case 7:
    lcd.setCursor(1, 0);
    lcd.print("SET TIME AND DATE");

    if (subpage_7_counter == 0) {
      lcd.setCursor(0, 3);
      lcd.print("(BACK)");
      lcd.setCursor(7, 3);
      lcd.print("(Edit)");;
    }
    else if (subpage_7_counter == 3) {
      lcd.setCursor(0, 3);
      lcd.print("(Yes)");
      lcd.setCursor(7, 3);
      lcd.print("(Next)");
      lcd.setCursor(16, 3);
      lcd.print("(No)");
    }
    else {
      lcd.setCursor(19, 1);
      lcd.write(0);
      lcd.setCursor(19, 2);
      lcd.write(1);
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(") ");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
      lcd.setCursor(7, 3);
      lcd.print("(Next)");
    }

    //scrolling
    if (last_sel == LOW && current_sel == HIGH && editRTC == false) {
      //clear screen content
      lcd.clear();
      if (subpage_7_counter < 3) {
        subpage_7_counter++;
      }
      else {
        subpage_7_counter = 1;
      }
    }

    if (last_sel == LOW && current_sel == HIGH && editRTC == true) {
      //clear screen content
      lcd.clear();
      if (rtcCounter < 4) {
        rtcCounter++;
      }
    }

    last_sel = current_sel;

    if (last_editRTC == true && editRTC == false)
    {
      subpage_7_counter++;
      //clear screen content
      lcd.clear();
    }

    last_editRTC = editRTC;

    if (subpage_7_counter == 0) {
      lcd.setCursor(0, 1);
      lcd.print("TIME:");
      lcd.setCursor(0, 2);
      lcd.print("DATE:");
      lcd.setCursor(6, 1);
      if (now.hour() < 10)
      {
        lcd.print("0");
      }
      lcd.print(now.hour(), DEC);
      lcd.print(':');
      if (now.minute() < 10)
      {
        lcd.print("0");
      }
      lcd.print(now.minute(), DEC);
      lcd.print(':');
      if (now.second() < 10)
      {
        lcd.print("0");
      }
      lcd.print(now.second(), DEC);

      lcd.setCursor(6, 2);
      if (now.day() < 10) lcd.print("0");
      lcd.print(now.day());
      lcd.print("/");
      lcd.print(now.month());
      if (now.month() < 10) lcd.print("0");
      lcd.print("/");
      lcd.print(now.year());

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 2;
        lcd.clear();
      }
    }

    // add code to incrimenting and decrementing date and time
    if (subpage_7_counter == 1) {
      editRTC = true;

      lcd.setCursor(0, 1);
      lcd.print("Time: ");

      if (hour_ < 10) lcd.print("0");
      lcd.print(hour_, DEC);
      if (rtcCounter == 1) lcd.write(3);
      else lcd.print(':');

      if (minute_ < 10)  lcd.print("0");
      lcd.print(minute_, DEC);
      if (rtcCounter == 2) lcd.write(3);
      else lcd.print(':');

      if (second_ < 10) lcd.print("0");
      lcd.print(second_, DEC);
      if (rtcCounter == 3) lcd.write(3);

      lcd.setCursor(0, 2);
      lcd.print("DATE: ");

      lcd.setCursor(6, 2);
      if (now.day() < 10) lcd.print("0");
      lcd.print(now.day());
      lcd.print("/");

      if (now.month() < 10) lcd.print("0");
      lcd.print(now.month());
      lcd.print("/");

      lcd.print(now.year());

      if (rtcCounter == 1) {
        if (last_up == LOW && current_up == HIGH) {
          if (hour_ < 23) hour_++;
          else hour_ = 0;
        }
        if (last_down == LOW && current_down == HIGH) {
          if (hour_ > 0) hour_--;
          else hour_ = 23;
        }
      }

      if (rtcCounter == 2) {
        if (last_up == LOW && current_up == HIGH) {
          if (minute_ < 59) minute_++;
          else minute_ = 0;
        }
        if (last_down == LOW && current_down == HIGH) {
          if (minute_ > 0) minute_--;
          else minute_ = 59;
        }
      }

      if (rtcCounter == 3) {
        if (last_up == LOW && current_up == HIGH) {
          if (second_ < 59) second_++;
          else second_ = 0;
        }
        if (last_down == LOW && current_down == HIGH) {
          if (second_ > 0) second_--;
          else second_ = 59;
        }
      }

      if (rtcCounter == 4) {
        editRTC = false;
        rtcCounter = 1;
      }
    }

    if (subpage_7_counter == 2) {
      editRTC = true;
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("Time: ");

      if (hour_ < 10) lcd.print("0");
      lcd.print(hour_, DEC);
      lcd.print(':');

      if (minute_ < 10) lcd.print("0");
      lcd.print(minute_, DEC);
      lcd.print(':');

      if (second_ < 10) lcd.print("0");
      lcd.print(second_, DEC);

      lcd.setCursor(0, 2);
      lcd.print("DATE: ");

      if (day_ < 10)  lcd.print("0");
      lcd.print(day_, DEC);
      if (rtcCounter == 1) lcd.write(3);
      else lcd.print('/');

      if (month_ < 10) lcd.print("0");
      lcd.print(month_, DEC);
      if (rtcCounter == 2) lcd.write(3);
      else lcd.print('/');

      lcd.print(year_, DEC);
      if (rtcCounter == 3) lcd.write(3);

      if ((rtcCounter == 1)) {
        if (last_up == LOW && current_up == HIGH) {
          if (day_ < 31) day_++;
          else day_ = 1;
        }
        if (last_down == LOW && current_down == HIGH) {
          if (day_ > 1) day_--;
          else day_ = 31;
        }
      }

      if (rtcCounter == 2) {
        if (last_up == LOW && current_up == HIGH) {
          if (month_ < 12) month_++;
          else month_ = 1;
        }
        if (last_down == LOW && current_down == HIGH) {
          if (month_ > 1) month_--;
          else month_ = 12;
        }
      }

      if (rtcCounter == 3) {
        if (last_up == LOW && current_up == HIGH) {
          if (year_ >= 2018) year_++;
        }
        if (last_down == LOW && current_down == HIGH) {
          if (year_ > 2018) year_--;
        }
      }

      if (rtcCounter == 4) {
        editRTC = false;
        rtcCounter = 1;
      }
    }

    if (subpage_7_counter == 3) {
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("DATE:");

      if (day_ < 10) lcd.print("0");
      lcd.print(day_, DEC);
      lcd.print('/');

      if (month_ < 10) lcd.print("0");
      lcd.print(month_, DEC);

      lcd.print('/' + String(year_));

      lcd.setCursor(0, 2);
      lcd.print("Save");

      if (last_up == LOW && current_up == HIGH) {
        subpage_7_counter = 0;
        page_counter = 2;
        rtc.adjust(DateTime(year_, month_, day_, hour_, minute_, second_));
        lcd.clear();
      }
      if (last_down == LOW && current_down == HIGH) {
        subpage_7_counter = 0;
        page_counter = 2;
        lcd.clear();
      }
    }
    break;

  case 8:
    lcd.setCursor(1, 0);
    lcd.print("REGIMENT ");
    lcd.print(increment);
    lcd.print(" CONFIG.");
    if (subpage_8_counter != 0) {
      lcd.setCursor(19, 1);
      lcd.write(0);
      lcd.setCursor(19, 2);
      lcd.write(1);
    }

    if (subpage_8_counter == 7 || subpage_8_counter == 8) {
      lcd.setCursor(0, 3);
      lcd.print("(OK)");
      lcd.setCursor(17, 3);
      lcd.print("   ");
    }
    else {
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(") ");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
    }
    lcd.setCursor(7, 3);
    lcd.print("(NEXT)");

    //Scrolling
    if (last_sel == LOW && current_sel == HIGH) {
      //clear screen content
      lcd.clear();
      if (subpage_8_counter < 8) {
        subpage_8_counter++;
      }
      else {
        subpage_8_counter = 0;
      }
    }

    last_sel = current_sel;

    if (subpage_8_counter == 0) {
      lcd.setCursor(17, 1);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL A.: ");
      if (dosing.getSolA(increment) < 100) lcd.print("0");
      if (dosing.getSolA(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolA(increment));
      lcd.print("ml");

      lcd.setCursor(0, 2);
      lcd.print("SOL B.: ");
      if (dosing.getSolB(increment) < 100) lcd.print("0");
      if (dosing.getSolB(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolB(increment));
      lcd.print("ml");

      if (current_up == HIGH) {
        if (dosing.getSolA(increment) < 500) dosing.setSolA(increment, (dosing.getSolA(increment) + 1));
        else dosing.setSolA(increment, 0);
      }
      if (current_down == HIGH) {
        if (dosing.getSolA(increment) > 0) dosing.setSolA(increment, (dosing.getSolA(increment) - 1));
        else dosing.setSolA(increment, 500);
      }
    }

    if (subpage_8_counter == 1) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL A.: ");
      if (dosing.getSolA(increment) < 100) lcd.print("0");
      if (dosing.getSolA(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolA(increment));
      lcd.print("ml");

      lcd.setCursor(0, 2);
      lcd.print("SOL B.: ");
      if (dosing.getSolB(increment) < 100) lcd.print("0");
      if (dosing.getSolB(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolB(increment));
      lcd.print("ml");

      if (current_up == HIGH) {
        if (dosing.getSolB(increment) < 500) dosing.setSolB(increment, (dosing.getSolB(increment) + 1));
        else dosing.setSolB(increment, 0);
      }
      if (current_down == HIGH) {
        if (dosing.getSolB(increment) > 0) dosing.setSolB(increment, (dosing.getSolB(increment) - 1));
        else dosing.setSolB(increment, 500);
      }
    }

    if (subpage_8_counter == 2) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL B.: ");
      if (dosing.getSolB(increment) < 100) lcd.print("0");
      if (dosing.getSolB(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolB(increment));
      lcd.print("ml");

      lcd.setCursor(0, 2);
      lcd.print("SOL C.: ");
      if (dosing.getSolC(increment) < 100) lcd.print("0");
      if (dosing.getSolC(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolC(increment));
      lcd.print("ml");

      if (last_up == LOW && current_up == HIGH) {
        if (dosing.getSolC(increment) < 500) dosing.setSolC(increment, (dosing.getSolC(increment) + 1));
        else dosing.setSolC(increment, 0);
      }
      if (last_down == LOW && current_down == HIGH) {
        if (dosing.getSolC(increment) > 0) dosing.setSolC(increment, (dosing.getSolC(increment) - 1));
        else dosing.setSolC(increment, 500);
      }
    }

    if (subpage_8_counter == 3) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("SOL C.: ");
      if (dosing.getSolC(increment) < 100) lcd.print("0");
      if (dosing.getSolC(increment) < 10) lcd.print("0");
      lcd.print(dosing.getSolC(increment));
      lcd.print("ml");

      lcd.setCursor(0, 2);
      lcd.print("EC upper: ");
      if (dosing.getUpperEC(increment) < 1000) lcd.print("0");
      if (dosing.getUpperEC(increment) < 100) lcd.print("0");
      if (dosing.getUpperEC(increment) < 10) lcd.print("0");
      lcd.print(dosing.getUpperEC(increment));
      lcd.print("ppm");

      if (last_up == LOW && current_up == HIGH) {
        if (dosing.getUpperEC(increment) < 9990) dosing.setUpperEC(increment, (dosing.getUpperEC(increment) + 10));
        else dosing.setUpperEC(increment, 0);
      }
      if (last_down == LOW && current_down == HIGH) {
        if (dosing.getUpperEC(increment) > 0) dosing.setUpperEC(increment, (dosing.getUpperEC(increment) - 10));
        else dosing.setUpperEC(increment, 9990);
      }
    }

    if (subpage_8_counter == 4) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("EC upper: ");
      if (dosing.getUpperEC(increment) < 1000) lcd.print("0");
      if (dosing.getUpperEC(increment) < 100) lcd.print("0");
      if (dosing.getUpperEC(increment) < 10) lcd.print("0");
      lcd.print(dosing.getUpperEC(increment));
      lcd.print("ppm");

      lcd.setCursor(0, 2);
      lcd.print("EC lower: ");
      if (dosing.getLowerEC(increment) < 1000) lcd.print("0");
      if (dosing.getLowerEC(increment) < 100) lcd.print("0");
      if (dosing.getLowerEC(increment) < 10) lcd.print("0");
      lcd.print(dosing.getLowerEC(increment));
      lcd.print("ppm");

      if (last_up == LOW && current_up == HIGH) {
        if (dosing.getLowerEC(increment) < 9990) dosing.setLowerEC(increment, (dosing.getLowerEC(increment) + 10));
        else dosing.setLowerEC(increment, 0);
      }
      if (last_down == LOW && current_down == HIGH) {
        if (dosing.getLowerEC(increment) > 0) dosing.setLowerEC(increment, (dosing.getLowerEC(increment) - 10));
        else dosing.setLowerEC(increment, 9990);
      }
    }

    if (subpage_8_counter == 5) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("EC lower: ");
      if (dosing.getLowerpH(increment) < 1000) lcd.print("0");
      if (dosing.getLowerpH(increment) < 100) lcd.print("0");
      if (dosing.getLowerpH(increment) < 10) lcd.print("0");
      lcd.print(dosing.getLowerpH(increment));
      lcd.print("ppm");

      lcd.setCursor(0, 2);
      lcd.print("UPPER pH: ");
      if (dosing.getUpperpH(increment)) lcd.print("0");
      lcd.print(dosing.getUpperpH(increment));

      if (last_up == LOW && current_up == HIGH) {
        if (dosing.getUpperpH(increment) < 14) dosing.setUpperpH(increment, (dosing.getUpperpH(increment) + 0.1));
        else dosing.setUpperpH(increment, 0);
      }
      if (last_down == LOW && current_down == HIGH) {
        if (dosing.getUpperpH(increment) > 0) dosing.setUpperpH(increment, (dosing.getUpperpH(increment) - 0.1));
        else {
          dosing.setUpperpH(increment, 14);
        }
      }
    }

    if (subpage_8_counter == 6) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("UPPER pH: ");
      if (dosing.getUpperpH(increment) < 10) lcd.print("0");
      lcd.print(dosing.getUpperpH(increment));

      lcd.setCursor(0, 2);
      lcd.print("LOWER pH: ");
      if (dosing.getLowerpH(increment) < 10) lcd.print("0");
      lcd.print(dosing.getLowerpH(increment));

      if (last_up == LOW && current_up == HIGH) {
        if (dosing.getLowerpH(increment) < (dosing.getUpperpH(increment) - 0.1)) dosing.setLowerpH(increment, (dosing.getLowerpH(increment) + 0.1));
        else dosing.setLowerpH(increment, 0);
      }
      if (last_down == LOW && current_down == HIGH) {
        if (dosing.getLowerpH(increment) > 0.1) dosing.setLowerpH(increment, (dosing.getLowerpH(increment) - 0.1));
        else if (dosing.getUpperpH(increment) != 0) dosing.setLowerpH(increment, (dosing.getUpperpH(increment) - 0.1));
      }
    }

    if (subpage_8_counter == 7) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("LOWER pH: ");
      if (dosing.getLowerpH(increment) < 10) lcd.print("0");
      lcd.print(dosing.getLowerpH(increment));

      if (numOfRegiments == increment) {
        lcd.setCursor(0, 2);
        lcd.print("Exit");

        if (last_up == LOW && current_up == HIGH) {
          page_counter = 3;
          subpage_8_counter = 0;
        }
      }
      else {
        lcd.setCursor(0, 2);
        lcd.print("Next Reg = ");
        lcd.print(numOfRegiments - increment + 1);
        lcd.print(")");

        if (last_up == LOW && current_up == HIGH) {
          increment++;
          subpage_8_counter = 0;
          lcd.clear();
        }
      }
    }

    if (subpage_8_counter == 8) {
      lcd.setCursor(17, 2);
      lcd.write(3);

      //if on the last regiment
      if (numOfRegiments == increment) {
        lcd.setCursor(0, 1);
        lcd.print("LOWER pH: ");
        if (dosing.getLowerpH(increment) < 10) lcd.print("0");
        lcd.print(dosing.getLowerpH(increment));
      }
      else {
        lcd.setCursor(0, 1);
        lcd.print("Next (rmd. = ");
        lcd.print(numOfRegiments - increment);
        lcd.print(")");
      }

      lcd.setCursor(0, 2);
      lcd.print("Exit");

      if (last_up == LOW && current_up == HIGH) {
        page_counter = 3;
        subpage_8_counter = 0;
        //if any remaining regiments are not set then they will carry forward the last set regiment configuration
        for (int i = (increment + 1); i <= numOfRegiments; i++) {
          dosing.setSolA(i, dosing.getSolA(increment));
          dosing.setSolB(i, dosing.getSolB(increment));
          dosing.setSolC(i, dosing.getSolC(increment));
          dosing.setUpperEC(i, dosing.getUpperEC(increment));
          dosing.setLowerEC(i, dosing.getLowerEC(increment));
          dosing.setUpperpH(i, dosing.getUpperpH(increment));
          dosing.setLowerpH(i, dosing.getLowerpH(increment));
        }
        //save to EEPROM
        for (int i = 0; i < numOfRegiments; i++) {
          int baseNum = 9;
          EEPROM.put(baseNum + (24 * i), dosing.getSolA(i + 1));
          EEPROM.put(baseNum + (24 * i) + 4, dosing.getSolB(i + 1));
          EEPROM.put(baseNum + (24 * i) + 8, dosing.getSolC(i + 1));
          EEPROM.put(baseNum + (24 * i) + 12, dosing.getUpperEC(i + 1));
          EEPROM.put(baseNum + (24 * i) + 14, dosing.getLowerEC(i + 1));
          EEPROM.put(baseNum + (24 * i) + 16, dosing.getUpperpH(i + 1));
          EEPROM.put(baseNum + (24 * i) + 20, dosing.getLowerpH(i + 1));
        }
      }
    }
    break;
  case 9:
    lcd.setCursor(0, 0);
    if (PumpNumber == 1) {
      lcd.print("Sol A. PUMP CONFIG.");
    }
    else if (PumpNumber == 2) {
      lcd.print("Sol B. PUMP CONFIG.");
    }
    else if (PumpNumber == 3) {
      lcd.print("Sol C. PUMP CONFIG.");
    }
    else if (PumpNumber == 4) {
      lcd.print("pH Up PUMP CONFIG.");
    }
    else if (PumpNumber == 5) {
      lcd.print("pH DOWN PUMP CONFIG.");
    }

    if (subPagePumpCounter == 1) {
      lcd.setCursor(0, 3);
      lcd.print("(");
      lcd.write(0);
      lcd.print(") ");
      lcd.setCursor(17, 3);
      lcd.print("(");
      lcd.write(1);
      lcd.print(")");
    }
    else if (subPagePumpCounter == 2) {
      lcd.setCursor(0, 3);
      lcd.print("(PURGE)");
    }
    else if (subPagePumpCounter == 3) {
      lcd.setCursor(0, 3);
      lcd.print("(CALI.)");
    }
    else if (subPagePumpCounter == 4) {
      lcd.setCursor(0, 3);
      lcd.print("(RUN)");
    }
    else if (subPagePumpCounter == 5) {
      lcd.setCursor(0, 3);
      lcd.print("(OK)");
    }
    else if (subPagePumpCounter == 6) {
      lcd.setCursor(0, 3);
      lcd.print("(BACK)");
      lcd.setCursor(7, 3);
      lcd.print("(Test)");
      lcd.setCursor(13, 3);
      lcd.print("(CALI.)");
    }
    else if (subPagePumpCounter == 7) {
      lcd.setCursor(0, 3);
      lcd.print("(EXIT)");
      lcd.setCursor(7, 3);
      lcd.print("(RUN)");
      lcd.setCursor(14, 3);
      lcd.print("(SAVE)");
    }
    if (subPagePumpCounter < 6) {
      lcd.setCursor(19, 1);
      lcd.write(0);
      lcd.setCursor(19, 2);
      lcd.write(1);
      lcd.setCursor(7, 3);
      lcd.print("(NEXT)");
    }

    //Scrolling
    if (last_sel == LOW && current_sel == HIGH) {
      if (subPagePumpCounter < 5) {
        //if test is running do nothing
        if (testCurrentCali != true) {
          subPagePumpCounter++;
        }
      }
      else if (subPagePumpCounter == 6) {
        subPagePumpCounter = 7;
      }
      else if (subPagePumpCounter == 7) {
        testNewCali = true;
      }
      else {
        subPagePumpCounter = 1;
      }
      //clear screen content
      lcd.clear();
    }

    last_sel = current_sel;

    if ((subPagePumpCounter == 1)) {
      lcd.setCursor(18, 1);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("MAX FLOW: ");

      if (pumps.getFlowRate(PumpNumber) < 100) lcd.print(" ");
      if (pumps.getFlowRate(PumpNumber) < 10) lcd.print(" ");

      //lcd.print(solAFlowRate);
      lcd.print(pumps.getFlowRate(PumpNumber));
      lcd.print("mL/hr");

      lcd.setCursor(0, 2);
      lcd.print("PURGE PUMP");

      if (current_up == HIGH) {
        pumps.setFlowRate(PumpNumber, pumps.getFlowRate(PumpNumber) + 1);
        if (pumps.getFlowRate(PumpNumber) > 999) {
          pumps.setFlowRate(PumpNumber, 0);
        }
      }
      if (current_down == HIGH) {
        pumps.setFlowRate(PumpNumber, pumps.getFlowRate(PumpNumber) - 1);
        if (pumps.getFlowRate(PumpNumber) < 0) {
          pumps.setFlowRate(PumpNumber, 999);
        }
      }
    }

    if ((subPagePumpCounter == 2)) {
      lcd.setCursor(18, 2);
      lcd.write(3);
      lcd.setCursor(0, 1);
      lcd.print("MAX FLOW: ");

      if (pumps.getFlowRate(PumpNumber) < 100) lcd.print(" ");
      if (pumps.getFlowRate(PumpNumber) < 10) lcd.print(" ");
      lcd.print(pumps.getFlowRate(PumpNumber));
      lcd.print("mL/hr");

      lcd.setCursor(0, 2);
      lcd.print("PURGE PUMP");

      if (last_up == LOW && current_up == HIGH) {

        pumps.purgePumps(PumpNumber);
      }
      if (last_up == HIGH && current_up == LOW) {
        pumps.purgePumps(PumpNumber);
      }
      last_up = current_up;           //used to detect rising and falling edge
    }

    if ((subPagePumpCounter == 3)) {
      lcd.setCursor(18, 2);
      lcd.write(3);

      lcd.setCursor(0, 1);
      lcd.print("PURGE PUMP");

      lcd.setCursor(0, 2);
      lcd.print("CALIBRATE PUMP");

      if (current_up == HIGH) {
        subPagePumpCounter = 6;
        lcd.clear();
      }
    }

    if ((subPagePumpCounter == 4)) {
      lcd.setCursor(0, 1);
      lcd.print("CALIBRATE PUMP");
      lcd.setCursor(0, 2);
      lcd.print("TEST CALIBRATION");

      lcd.setCursor(18, 2);
      lcd.write(3);

      if (last_up == LOW && current_up == HIGH) {
        testCurrentCali = true;
      }
      last_up = current_up;           //used to detect rising and falling edge
    }

    if ((subPagePumpCounter == 5)) {
      lcd.setCursor(0, 1);
      lcd.print("TEST CALIBRATION");
      lcd.setCursor(0, 2);
      lcd.print("BACK");

      lcd.setCursor(18, 2);
      lcd.write(3);

      if (current_up == HIGH) {
        subPagePumpCounter = 1;
        page_counter = 4;
        lcd.clear();
      }
    }

    if ((subPagePumpCounter == 6)) {
      lcd.setCursor(1, 1);
      lcd.print("Hold down (CALI.)");
      lcd.setCursor(0, 2);
      lcd.print("until 25ml dispenced");

      if (last_down == LOW && current_down == HIGH) {
        pumps.setCalibrationTime(PumpNumber);
      }
      if (last_down == HIGH && current_down == LOW) {
        pumps.setCalibrationTime(PumpNumber);
      }
      last_down = current_down;           //used to detect rising and falling edge
      if (last_up == LOW && current_up == HIGH) {
        subPagePumpCounter = 3;
        pumps.reinitialiseCalibration();
        lcd.clear();
      }
      last_up = current_up;
    }

    if ((subPagePumpCounter == 7)) {
      lcd.setCursor(0, 1);
      lcd.print("Run to test if pump");
      lcd.setCursor(0, 2);
      lcd.print("dispences 25ml");

      // if a "testNewCali" is running this screen can't be eited till its completion
      if (current_down == HIGH && testNewCali == false) {
        pumps.saveCalibrationTime(PumpNumber);
        EEPROM.put(322 + (PumpNumber - 1) * 4, pumps.getPumpCalibrationTime(PumpNumber));
        subPagePumpCounter = 3;
        lcd.clear();
      }
      if (current_up == HIGH && testNewCali == false) {
        subPagePumpCounter = 3;
        lcd.clear();
        pumps.reinitialiseCalibration();
      }
    }

    if (testNewCali == true) {
      testNewCali = pumps.testNewCalibration(PumpNumber);
    }

    if (testCurrentCali == true) {
      testCurrentCali = pumps.testCurrentCalibration(PumpNumber);
    }
    break;
    case 10:
      //aggitator pump configuration
      break;
  }

  if (previousDay != currentDay) {          // If the stored day is not the same as the current day
    //Serial.print("Regiment(Before): ");
    //Serial.println(regiment); 
    previousDay = currentDay;         // Re-initialise the day
    elapsed_day++;                // Add 1 to the day counter
    //Serial.print("elapsed_day: ");
    //Serial.println(elapsed_day);
    regiment = elapsed_day / regimentsDays; // Change to next regiment when regiment days is exceeded
    //Serial.print("Regiment(After): ");
    //Serial.println(regiment);
    if (regiment > numOfRegiments) regiment = 1;
  }

  //agitate the resivour every 1 minutes
  dosing.agitateSolution(agitatorPump, 20000, 5000, lastAgitateTime);

  //Reset watchdog timer
  wdt_reset();
}

void pinMode_setup() {
  pinMode(up, INPUT);
  pinMode(sel, INPUT);
  pinMode(down, INPUT);

  pinMode(agitatorPump, OUTPUT);
  //Need to make sure all the Relays are off during initialisation
  digitalWrite(agitatorPump, HIGH);
}

void EEPROM_setup() {
  // read data from EEPROM
  Serial.print("Crash report: ");
  Serial.println(EEPROM.read(0));
  if (EEPROM.read(0) == 1) {
    EEPROM.write(0, 0);
  }

  EEPROM.put(1, dosing.getReserviorSize());
  int resSize;
  EEPROM.get(1, resSize);
  dosing.setReserviorSize(resSize);

  EEPROM.update(3, 'C');
  tempUnit[0] = EEPROM.read(3);

  EEPROM.update(4, 0);
  ECUnitIndex = EEPROM.read(4);

  EEPROM.put(5, 540);
  long baselineEC = 0.0;
  EEPROM.get(5, baselineEC);
  dosing.setBaselineEC(baselineEC);

  EEPROM.write(7, 1);
  numOfRegiments = EEPROM.read(7);

  EEPROM.write(8, 1);
  regimentsDays = EEPROM.read(8);

  int baseNum = 9;
  for (int i = 0; i == 14; i++) {
    long solA = 0, solB = 0, solC = 0;
    int upperEC = 0, lowerEC = 0;
    long upperpH = 0, lowerpH = 0;

    EEPROM.get(baseNum, solA);
    dosing.setSolA(i, solA);
    EEPROM.get(baseNum + 4, solB);
    dosing.setSolB(i, solB);
    EEPROM.get(baseNum + 8, solC);
    dosing.setSolC(i, solC);
    EEPROM.get(baseNum + 12, upperEC);
    dosing.setUpperEC(i, upperEC);
    EEPROM.get(baseNum + 14, lowerEC);
    dosing.setLowerEC(i, lowerEC);
    EEPROM.get(baseNum + 16, upperpH);
    dosing.setUpperpH(i, upperpH);
    EEPROM.get(baseNum + 20, lowerpH);
    dosing.setLowerpH(i, lowerpH);

    baseNum += 24;
  }

  baseNum = 321;
  long pumpTime = 0;

  EEPROM.put(baseNum + 4, 19625);
  EEPROM.get(baseNum + 4, pumpTime);
  pumps.setPumpCaliTimeOnStartUp(1, pumpTime);

  EEPROM.put(baseNum + 8, 20000);
  EEPROM.get(baseNum + 8, pumpTime);
  pumps.setPumpCaliTimeOnStartUp(2, pumpTime);

  EEPROM.put(baseNum + 12, 20000);
  EEPROM.get(baseNum + 12, pumpTime);
  pumps.setPumpCaliTimeOnStartUp(3, pumpTime);

  EEPROM.put(baseNum + 16, 21100);
  EEPROM.get(baseNum + 16, pumpTime);
  pumps.setPumpCaliTimeOnStartUp(4, pumpTime);

  EEPROM.put(baseNum + 20, 19950);
  EEPROM.get(baseNum + 20, pumpTime);
  pumps.setPumpCaliTimeOnStartUp(5, pumpTime);

  lastpHCaliDay = EEPROM.read(343);
  lastpHCaliMonth = EEPROM.read(344);
  lastpHCaliYear = EEPROM.read(345);

  lastECCaliDay = EEPROM.read(347);
  lastECCaliMonth = EEPROM.read(348);
  lastECCaliYear = EEPROM.read(349);
}

//
void watchdogSetup(void) {
  cli(); // disable all interrupts 
  wdt_reset(); // reset the WDT timer 
  /*   WDTCSR conﬁgurations:
  WDIE  = 1: Interrupt Enable
  WDE   = 1 :Reset Enable
  WDP3  WDP2  WDP1  WDP0  Time-Out(ms)
  0   0   0   0   16
  0   0   0   1   32
  0   0   1   0   64
  0   0   1   1   125
  0   1   0   0   250
  0   1   0   1   500
  0   1   1   0   1000
  0   1   1   1   2000
  1   0   0   0   4000
  1   0   0   1   8000
  */
  // Enter Watchdog Conﬁguration mode:
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // Set Watchdog settings: 
  WDTCSR = (1 << WDIE) | (1 << WDE) | (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0); sei(); \
}

ISR(WDT_vect) // Watchdog timer interrupt.
{
  //store and event in EEPROM
  EEPROM.write(0, 1);
}
