
//  libraries
#include <WaziDev.h>
#include <xlpp.h>
#include <Base64.h>
#include <LCD_I2C.h>
#include <DHT.h>
// #include <Arduino.h>

// dht variable
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// lcd display
LCD_I2C lcd(0x27, 20, 4);

// relay
int compressorRelayPin = 8;  // cooling system
int nichromeRelayPin = 9;    // heating system

// fan
int condenserFanPin = 10;
int evaporatorFanPin = 11;
int nichromeFanPin = 12;

// buttons
int interruptStatusPin = 2;
int incrementButtonPin = 3;
int decrementButtonPin = 4;
int onOffButtonPin = 7;

volatile byte onOffButtonState = false;
volatile byte incrementButtonState = false;
volatile byte decrementButtonState = false;

/* global variables*/

bool coolingStatus = false;
bool heatingStatus = false;
bool systemStatus = false;

// threshold temperature
// for comparing the initial room temperature to know whether heating or cooling system is to be turned on
int thresholdTemp = 23;

// measured temperature and humidity
// measured by dht temperature sensor
volatile float measuredHumidity;
volatile float measuredTemp;

// user reference temperature
//  set by the sp[ecific user
volatile int referenceTempHigh = 0;
volatile int referenceTempLow = 0;

// default temperature for early adopters, i.e. dar es salaam region
// specified by the manufacturers or the producers of the system
int defaultTempHigh = 28;
int defaultTempLow = 16;

// actual temperature value to be used
volatile int highTempValue;
volatile int lowTempValue;

// lorawan variables
// Copy'n'paste the DevAddr (Device Address): 29011D01
unsigned char devAddr[4] = { 0x29, 0x01, 0x1D, 0x01 };

// Copy'n'paste the key to your Wazigate: 28158D3BBC31E6AF670D195B5AED5525
unsigned char appSkey[16] = { 0x28, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25 };

// Copy'n'paste the key to your Wazigate: 28158D3BBC31E6AF670D195B5AED5525
unsigned char nwkSkey[16] = { 0x28, 0x15, 0x8D, 0x3B, 0xBC, 0x31, 0xE6, 0xAF, 0x67, 0x0D, 0x19, 0x5B, 0x5A, 0xED, 0x55, 0x25 };

WaziDev wazidev;


void setup() {

  // initialize serial communication
  Serial.begin(38400);

  // wazidev initialization
  wazidev.setupLoRaWAN(devAddr, appSkey, nwkSkey);

  Serial.println();
  Serial.println("Wazidev Gateway Initialization Completed");

  // pinmode declaration
  pinMode(compressorRelayPin, OUTPUT);
  pinMode(nichromeRelayPin, OUTPUT);

  pinMode(condenserFanPin, OUTPUT);
  pinMode(evaporatorFanPin, OUTPUT);
  pinMode(nichromeFanPin, OUTPUT);

  // Set the button pins as input with internal pull-up resistors
  pinMode(onOffButtonPin, INPUT);
  pinMode(incrementButtonPin, INPUT);
  pinMode(decrementButtonPin, INPUT);
  pinMode(interruptStatusPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(incrementButtonPin), buttonInterrupt, CHANGE);

  // attachInterrupt(digitalPinToInterrupt(interruptStatusPin), handleButtonImplementation, CHANGING);

  Serial.println();
  Serial.println("Pinmode Initialization Completed");

  // initialize lcd
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome CATCS User..");
  lcd.setCursor(0, 2);
  lcd.print("System Configuration");
  delay(4000);

  // fetching the temperature values

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Operating Temp_Value");
  delay(2000);

  fetchTemperatureValue();

  dht.begin();

  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("System Config OK!!");

  delay(4000);
}

XLPP xlpp(120);

// --------------------------------------------------------------
// --------------------------------------------------------------
// --------------------------------------------------------------

void loop() {

  // system initiation and configuring which system to run depending on the current room temperature
  systemInitiation();

  //initiating continuous temperature monitoring
  continuousTemperatureMonitor();
}

// -------------------------------------------------------------- fetch temperature values for the enhanced comfortanility and convenience of the system

void fetchTemperatureValue() {

  // monitor and check whether a button was pressed
  // buttonMonitoring();

  // fetch the highest temperature value for user's comfortability during winter season
  if (referenceTempHigh == 0 && referenceTempLow == 0) {
    highTempValue = defaultTempHigh;
    lowTempValue = defaultTempLow;

    Serial.println();
    Serial.println("User's reference temperature not set!... ");

  } else {
    highTempValue = referenceTempHigh;
    lowTempValue = referenceTempLow;

    Serial.println();
    Serial.println("User's reference temperature already set!... ");
  }

  lcd.setCursor(0, 2);
  lcd.print("warm_temp: ");
  lcd.setCursor(12, 2);
  lcd.print(highTempValue);
  lcd.print("°C");
  lcd.setCursor(0, 3);
  lcd.print("cool_temp: ");
  lcd.setCursor(12, 3);
  lcd.print(lowTempValue);
  lcd.print("°C");

  delay(4000);
}

// -------------------------------------------------------------- system initialization function

void systemInitiation() {
  //  measure the temperature and humidity of a room
  measuredHumidity = dht.readHumidity();
  measuredTemp = dht.readTemperature();

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("room_temp: ");
  lcd.setCursor(12, 1);
  lcd.print(measuredTemp);
  lcd.setCursor(16, 1);
  lcd.print("°C");
  lcd.setCursor(0, 2);
  lcd.print("room_humid: ");
  lcd.setCursor(13, 2);
  lcd.print(measuredHumidity);
  lcd.setCursor(15, 2);
  lcd.print("%");

  delay(5000);

  //  compare the room temperature and humidity of a room to the preset threshold value
  if (measuredTemp > thresholdTemp) {

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("AC System Initiated.");
    delay(4000);

    initiatingCoolingSystem();

    systemStatus = true;
  } else {

    lcd.setCursor(2, 1);
    lcd.print("Heater Initiated");
    delay(3000);

    initiatingHeatingSystem();

    systemStatus = true;
  }
}

// -------------------------------------------------------------- initiate cooling system

void initiatingCoolingSystem() {

  // start operating the fans
  digitalWrite(condenserFanPin, HIGH);   // start the condenser fan pin
  digitalWrite(evaporatorFanPin, HIGH);  // start the evaporator fan

  // send high signal to a compressor
  digitalWrite(compressorRelayPin, LOW);
  digitalWrite(nichromeRelayPin, HIGH);

  // update the system status to indicate the cooling system is in action

  coolingStatus = true;
  heatingStatus = false;
}

// -------------------------------------------------------------- initiate heating system

void initiatingHeatingSystem() {

  // send a high signal to heating system relay part
  digitalWrite(compressorRelayPin, HIGH);
  digitalWrite(nichromeRelayPin, LOW);

  // start the operation of the fan
  digitalWrite(nichromeFanPin, HIGH);

  // update the system status to indicate the heating system is in action

  coolingStatus = false;
  heatingStatus = true;
  //
}

// -------------------------------------------------------------- system monitoring function

void continuousTemperatureMonitor() {
  // monitoring changes in room temperature when cooling system is in action
  while (coolingStatus) {

    // measure the current room temperatures and humidity
    measuredHumidity = dht.readHumidity();
    measuredTemp = dht.readTemperature();

    // check whether the temperature required is met
    while (measuredTemp > lowTempValue) {

      // measure the current room temperatures and humidity
      measuredHumidity = dht.readHumidity();
      measuredTemp = dht.readTemperature();

      // send data to wazi gateway
      sendDataToGateway(measuredTemp, measuredHumidity);

      // keep on using the ac system
      digitalWrite(compressorRelayPin, LOW);
      digitalWrite(nichromeRelayPin, HIGH);

      // keep on using the fan
      digitalWrite(condenserFanPin, HIGH);
      digitalWrite(evaporatorFanPin, HIGH);

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("room_temp: ");
      lcd.print(measuredTemp);
      lcd.print("°C");
      lcd.setCursor(0, 2);
      lcd.print("cool_temp: ");
      lcd.print(lowTempValue);
      lcd.print("°C");

      delay(10000);
    }

    if (measuredTemp <= lowTempValue) {

      //switch off cooling system
      digitalWrite(compressorRelayPin, HIGH);
      digitalWrite(nichromeRelayPin, HIGH);

      // but keep fan on
      digitalWrite(condenserFanPin, HIGH);
      digitalWrite(evaporatorFanPin, HIGH);

      // measure the current room temperatures and humidity
      float newMeasuredHumidity = dht.readHumidity();
      float newMeasuredTemp = dht.readTemperature();

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("room_temp: ");
      lcd.print(newMeasuredTemp);
      lcd.print("°C");
      lcd.setCursor(0, 2);
      lcd.print("cool_temp: ");
      lcd.print(lowTempValue);
      lcd.print("°C");

      if (newMeasuredTemp > lowTempValue) {
        initiatingCoolingSystem();
      }

      delay(10000);
    }
  }

  // monitoring changes in room temperature when heating system is in action
  while (heatingStatus) {

    // measure the current room temperatures and humidity
    measuredHumidity = dht.readHumidity();
    measuredTemp = dht.readTemperature();

    // check whether the temperature required is met
    while (measuredTemp < highTempValue) {

      // measure the current room temperatures and humidity
      measuredHumidity = dht.readHumidity();
      measuredTemp = dht.readTemperature();

      // send data to wazi gateway
      sendDataToGateway(measuredTemp, measuredHumidity);

      // keep on using the heating system
      digitalWrite(compressorRelayPin, HIGH);
      digitalWrite(nichromeRelayPin, LOW);

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("room_temp: ");
      lcd.print(measuredTemp);
      lcd.print("°C");
      lcd.setCursor(0, 2);
      lcd.print("cool_temp: ");
      lcd.print(highTempValue);
      lcd.print("°C");

      delay(10000);

      // continue the operation of the heating fan
      digitalWrite(nichromeFanPin, HIGH);
    }

    if (measuredTemp >= highTempValue) {

      // switching off heating system
      digitalWrite(compressorRelayPin, HIGH);
      digitalWrite(nichromeRelayPin, HIGH);

      //  switch fan off
      digitalWrite(nichromeFanPin, LOW);

      // measure the current room temperatures and humidity
      float newMeasuredHumidity = dht.readHumidity();
      float newMeasuredTemp = dht.readTemperature();

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("room_temp: ");
      lcd.print(newMeasuredTemp);
      lcd.print("°C");
      lcd.setCursor(0, 2);
      lcd.print("cool_temp: ");
      lcd.print(highTempValue);
      lcd.print("°C");

      // // send data to wazi gateway
      // sendDataToGateway(newMeasuredTemp, newMeasuredHumidity);

      if (newMeasuredTemp < lowTempValue) {
        initiatingHeatingSystem();
      }

      delay(10000);
    }
  }
}

// ----------------------------------------------------------- send measured sensor data to the wazigate

void sendDataToGateway(float t, float h) {

  // 1
  // Create xlpp payload.
  xlpp.reset();
  xlpp.addTemperature(1, t);       // adding temperature value to the payload
  xlpp.addRelativeHumidity(2, h);  // adding relative humidity to the payload

  // 2.
  // Send paload with LoRaWAN.
  serialPrintf("LoRaWAN send ... ");
  uint8_t e = wazidev.sendLoRaWAN(xlpp.buf, xlpp.len);
  if (e != 0) {
    serialPrintf("Err %d\n", e);
    delay(3000);
    return;
  }
  serialPrintf("OK\n");

  // 3.
  // Receive LoRaWAN message (waiting for 6 seconds only).
  serialPrintf("LoRa receive ... ");
  uint8_t offs = 0;
  long startSend = millis();
  e = wazidev.receiveLoRaWAN(xlpp.buf, &xlpp.offset, &xlpp.len, 6000);
  long endSend = millis();
  if (e != 0) {
    if (e == ERR_LORA_TIMEOUT) {
      serialPrintf("nothing received\n");
    } else {
      serialPrintf("Err %d\n", e);
    }
    delay(3000);
    return;
  }
  serialPrintf("OK\n");

  serialPrintf("Payload: ");
  char payload[100];
  base64_decode(payload, xlpp.getBuffer(), xlpp.len);
  serialPrintf(payload);
  serialPrintf("\n");

  delay(1000);
}

// -------------------------------------------------------------- button interrupt function

void buttonInterrupt() {

  // increment button is pressed
  if (digitalRead(incrementButtonPin) == HIGH) {

    digitalWrite(compressorRelayPin, HIGH);

    // for cooling system
    if (coolingStatus) {

      // check whether the referenceTempLow is within the range of operating temperature
      if (lowTempValue >= 12 && lowTempValue < 20) {
        lowTempValue = lowTempValue + 1;
      }

      // updating the target temperature value
      referenceTempLow = lowTempValue;
    }

    // for heating system
    if (heatingStatus) {

      // check whether the referenceTempHigh is within the range of operating temperature
      if (highTempValue >= 25 && highTempValue < 33) {
        highTempValue = highTempValue + 1;
      }

      // updating the target temperature value
      referenceTempHigh = highTempValue;
    }
  }

  // decrement button is pressed
  else if (digitalRead(decrementButtonPin) == HIGH) {

    digitalWrite(compressorRelayPin, HIGH);

    // for cooling system
    if (coolingStatus) {

      // check whether the referenceTempLow is within the range of operating temperature
      if (lowTempValue > 12 && lowTempValue < 20) {
        lowTempValue = lowTempValue - 1;
      }

      // updating the target temperature value
      referenceTempLow = lowTempValue;
    }

    // for heating system
    if (heatingStatus) {

      // check whether the referenceTempHigh is within the range of operating temperature
      if (highTempValue >= 25 && highTempValue <= 33) {
        highTempValue = highTempValue - 1;
      }

      // updating the target temperature value
      referenceTempHigh = highTempValue;
    }
  }

  // when onOffButton is pressed
  else if (digitalRead(onOffButtonPin) == HIGH) {

    // digitalWrite(compressorRelayPin, HIGH);

    // check if any system is running
    if (systemStatus) {

      // turn off all relays
      digitalWrite(compressorRelayPin, HIGH);
      digitalWrite(nichromeRelayPin, HIGH);

      // turn off all the fans
      digitalWrite(condenserFanPin, LOW);
      digitalWrite(evaporatorFanPin, LOW);
      digitalWrite(nichromeFanPin, LOW);

    } else {

      // initialize the system
      systemInitiation();
    }
  }
}
