int lightMOSFETpin = 19; // pin 19 controls the light MOSFET
int photoDiodePin = 18; // pin 18 is the photo diode
int pirPin = 17; // pin 17 is the PIR sensor
int irPin = 16; // pin 16 is the IR sensor
int batteryIGBT0 = 15; // pin 15 is the IGBT for battery 0, which is for running the light & (normally) the esp32
int batteryIGBT1 = 14; // pin 14 is the IGBT for battery 1, which is for running the esp32 when the main battery is low
int batteryVoltagePin0 = 13; // pin 13 is the voltage divider for battery 0
int batteryVoltagePin1 = 12; // pin 12 is the voltage divider for battery 1


int SOC0 = 0; // state of charge of battery 0
int SOC1 = 0; // state of charge of battery 1

int ledBrightness = 0; // brightness of the LED
bool lightState = 0; // state of the light, 0 is off, 1 is on

int inferredDaylightLength = 0; // total time in minutes the photodiode has been on in the previous 24 hours.

int photoDiodeOnTime24Hrs = 0; // total time in minutes the photodiode has been on in the previous 24 hours.
int photoDiodeOffTime24Hrs = 0;

long long lastMS; // variable to infer Timing of functions
int inferredDayLengths[28]; // array to store the inferred day lengths for the last 28 days
int lastMonthInfDayLengths[28]; // array to store the inferred day lengths for the last 28 days
int currentDay = 0; // variable to store the current day
int prevDay = 0; //for logDayLen();
void switchESPBattery(bool battery);
void sunlightStatus();
void lightControl(bool state, int brightness);
void getStateOfCharge();
void switchESPBattery(bool battery);
void voltageRead(int battery);
void Controller();


//logs time photodiode is on by adding 1 to a variable every minute
void logPhotoDiodeOnandOffTime() {
  if(millis() - lastMS >= 60000) {
    photoDiodeOnTime24Hrs++;
    lastMS = millis();
  }
  else {
    photoDiodeOffTime24Hrs++;
  }
}

void logDayLen() //logs the previous days length
  if(currentDay > prevDay) {
    inferredDayLengths[prevDay] = inferredDaylightLength;
    prevDay = currentDay;
  }

//a function that infers the current date from the photodiode on and off time.
void inferAndLogDaylightLength() {
  if (currentDay < 29) {
    if (photoDiodeOnTime24Hrs >= 1440) {
      inferredDaylightLength = photoDiodeOnTime24Hrs;
      inferredDayLengths[currentDay] = inferredDaylightLength;
      currentDay++;
      inferredDaylightLength = 0;
      photoDiodeOffTime24Hrs = 0;
      photoDiodeOnTime24Hrs = 0;
    }
    else {
      inferredDaylightLength = photoDiodeOnTime24Hrs;
    }
  }
  else {
    lastMonthInfDayLengths = inferredDayLengths;
    for(i = 0; i < 28; i++) {
      inferredDayLengths[i] = 0;
    }
        currentDay = 0;

  }
}

void voltageRead(int battery) {
    if (battery == 0) {
        int voltage = analogRead(batteryVoltagePin0) + 1000;
        return voltage;
    }
    else if (battery == 1) {
        int voltage = analogRead(batteryVoltagePin1) + 1000;
        return voltage;
    }
}

void getStateOfCharge() {
  int soc[2];
  for(int i = 0; i < 2; i++) {
    int voltage = voltageRead(i);
    if (voltage >= 4200) {
      soc[i] = 100;
    }
    else if (voltage >= 4000) {
      soc[i] = (int)ceil((voltageRead - 4000) * 0.1 + 75);
    }
    else if (voltage >= 3800) {
      soc[i] = (int)ceil((voltage - 3800) * 0.2 + 50);
    }
    else if (voltage >= 3600) {
      soc[i] = (int)ceil((voltage - 3600) * 0.3 + 25);
    }
    else if (voltage >= 3400) {
      soc[i] = (int)ceil((voltage - 3400) * 0.4 + 10);
    }
    else {
      soc[i] = 0;
    }
  }
  SOC0 = soc[0];
  SOC1 = soc[1];
}

void switchESPBattery(bool battery) { //controls the battery the ESP32 is connected to.
    if (battery == 0 ) {
        digitalWrite(batteryIGBT0, HIGH);
        digitalWrite(batteryIGBT1, LOW);
    }
    else {
        digitalWrite(batteryIGBT0, LOW);
        digitalWrite(batteryIGBT1, HIGH);
    }
}

void batterySwitch() { //switches the battery the ESP32 is connected to if the battery is low.
    if (SOC0 < SOC1)
      switchESPBattery(1);
    else
      switchESPBattery(0);
}

void sunlightStatus() {
    averagePhotoDiodeValue = analogRead(photoDiodePin);
    if (averagePhotoDiodeValue > 100) {
        sunshineState = 1;
    }
    else {
        sunshineState = 0;
    }
}


void lightControl(bool state, int brightness) {
    if (state == 1) {
        analogWrite(lightMOSFETpin, brightness);
        lightState = 1;
    }
    else {
        digitalWrite(lightMOSFETpin, 0);
        lightState = 0;
    }
}

void determineLEDBrightness() {// changes value of ledBrightness based on SOC0
    if (SOC0 >= 75) {
        ledBrightness = 255;
    }
    else if (SOC0 >= 50) {
        ledBrightness = 200;
    }
    else if (SOC0 >= 25) {
        ledBrightness = 150;
    }
    else if (SOC0 >= 10) {
        ledBrightness = 100;
    }
    else {
        ledBrightness = 0;
    }
}

void Controller() {
  getStateOfCharge();
  batterySwitch(); // we need a small capacitor the hold an interim voltage?
  sunlightStatus();
  determineLEDBrightness();
  if (sunshineState == 0) {
    lightControl(1, ledBrightness);
  }
  else {
    lightControl(0, 0);
  }
  inferAndLogDaylightLength();
  logPhotoDiodeOnandOffTime();
  logDayLen();
}

void setup() {
    pinMode(lightMOSFETpin, OUTPUT);
    pinMode(photoDiodePin, INPUT);
    pinMode(pirPin, INPUT);
    pinMode(irPin, INPUT);
}


void loop() {
  Controller();
}
