#include <Adafruit_Sensor.h>

#include <DHT.h>
#include <DHT_U.h>

#include <LiquidCrystal.h>
#include <iarduino_RTC.h>

byte degree[8] = {
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
};

#define RST 5
#define DAT 4
#define CLK 3

#define BTN_INTERRUPT_PIN 2

#define RW 6
#define E 7
#define DB4 8
#define DB5 9
#define DB6 10
#define DB7 11

#define LIGHT 12
#define ALARMLED 13

#define DHTPIN A0
#define BTNPIN A1

int h = 0;
int t = 0;
int mode = 0;

bool alarmActive = false;
bool lightActive = true;

iarduino_RTC time(RTC_DS1302, RST, CLK, DAT);
LiquidCrystal lcd(RW, E, DB4, DB5, DB6, DB7);
DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(9600);
  pinMode(A5, INPUT_PULLUP);
  pinMode(A4, OUTPUT);
  
  time.begin();
  lcd.begin(16, 2); //Размерность экрана
  lcd.createChar(1, degree);
  dht.begin();
  h = dht.readHumidity();
  t = dht.readTemperature();
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
  if (digitalRead(BTNPIN) == 1)
  {
    turnLight(!lightActive);
  }
}
void displayDateTime()
{
  lcd.home();
  lcd.print(time.gettime("d/m/Y "));
  lcd.print(time.gettime("D"));
  lcd.setCursor(0, 1);
  lcd.print(time.gettime("h:i A   "));
  lcd.print(t);
  lcd.print("\1C");
}
void updateHT()
{
  h = dht.readHumidity();
  t = dht.readTemperature();
}
void turnLight(bool value)
{
  lightActive = value;
  if (value) {
    analogWrite(LIGHT, 128);
  } else {
    analogWrite(LIGHT, 0);
  }
}
void turnAlarm(bool active)
{
  alarmActive = active;
  if (alarmActive) {
    digitalWrite(ALARMLED, 1);
  } else {
    digitalWrite(ALARMLED, 0);
  }
}
