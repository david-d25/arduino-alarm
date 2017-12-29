#include <Adafruit_Sensor.h>

#include <DHT.h>
#include <DHT_U.h>  

#include <LiquidCrystal.h>
#include <iarduino_RTC.h>

byte degreeSymbol[8] = {
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
};

#define LCD_WIDTH 16
#define LCD_HEIGHT 2

#define TIME_PIN_RST 5
#define TIME_PIN_DAT 4
#define TIME_PIN_CLK 3

#define LCD_PIN_RS 6
#define LCD_PIN_E 7
#define LCD_PIN_DB4 8
#define LCD_PIN_DB5 9
#define LCD_PIN_DB6 10
#define LCD_PIN_DB7 11

#define PIN_LIGHT 12
#define PIN_ALARMLED 13

#define PIN_BTN_INTERRUPT 2

#define PIN_DHT A0
#define PIN_BTN A1
#define PIN_LIGHT_DETECTOR A2
#define PIN_WARM_LIGHT A3
#define PIN_COLD_LIGHT A4

boolean isAlarmActive = false;
boolean isLightActive = true;

iarduino_RTC time(RTC_DS1302, TIME_PIN_RST, TIME_PIN_CLK, TIME_PIN_DAT);
LiquidCrystal lcd(LCD_PIN_RS, LCD_PIN_E, LCD_PIN_DB4, LCD_PIN_DB5, LCD_PIN_DB6, LCD_PIN_DB7);
DHT dht(PIN_DHT, DHT11);

int temperature = 0;
int humidity = 0;

void setup() {
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(PIN_ALARMLED, OUTPUT);
  
  pinMode(PIN_BTN_INTERRUPT, INPUT_PULLUP);
  
  attachInterrupt(0, onButtonPress, RISING);
  
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.createChar(1, degreeSymbol);
  time.begin();
  dht.begin();
  updateHT();
}

void loop() {
  if (millis()%5000==0)
  {
    updateHT();
  }
  if (millis()%1000==0)
  {
    displayDateTime();
    delay(1);
  }
  if (digitalRead(PIN_BTN) == 1)
  {
    turnLight(!isLightActive);
  }
}

void onButtonPress() {
 // turnLight(!isLightActive);
}

void displayDateTime()
{
  lcd.home();
  lcd.print(time.gettime("d/m/Y "));
  lcd.print(time.gettime("D"));
  lcd.setCursor(0, 1);
  lcd.print(time.gettime("h:i A"));
  lcd.setCursor(11, 1);
  lcd.print(temperature);
  lcd.print("\1C  ");
}

void updateHT()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void turnLight(boolean value)
{
  isLightActive = value;
  if (value) {
    analogWrite(PIN_LIGHT, 128);
  } else {
    analogWrite(PIN_LIGHT, 0);
  }
}

void turnAlarm(boolean active)
{
  isAlarmActive = active;
  if (isAlarmActive) {
    digitalWrite(PIN_ALARMLED, 1);
  } else {
    digitalWrite(PIN_ALARMLED, 0);
  }
}
