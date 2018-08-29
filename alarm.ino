#include <LiquidCrystal_I2C.h>

#include <Adafruit_Sensor.h>

#include <DHT.h>
#include <DHT_U.h>  

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

#define PIN_BTN_INTERRUPT 2

#define TIME_PIN_CLK 3
#define TIME_PIN_DAT 4
#define TIME_PIN_RST 5

#define PIN_LCD_BACKLIGHT 6

#define NONE_A 7
#define NONE_B 8
#define PIN_LED_INDICATOR 9

#define PIN_WARM_LIGHT 10
#define PIN_COLD_LIGHT 11

#define PIN_BTN A0
#define PIN_LIGHT_DETECTOR A1
#define PIN_DHT A2
#define PIN_SWITCH A3
#define PIN_LCD_SDA A4
#define PIN_LCD_SCL A5

boolean isAlarmActive = false;
boolean isLightActive = true;

iarduino_RTC time(RTC_DS1302, TIME_PIN_RST, TIME_PIN_CLK, TIME_PIN_DAT);
LiquidCrystal_I2C lcd(0x3f, LCD_WIDTH, LCD_HEIGHT);
DHT dht(PIN_DHT, DHT11);

int temperature = 0;
int humidity = 0;

int brightness = 0;
int brightnessTarget = (int)(1024*0.24);

void setup() {
  pinMode(13, OUTPUT); // Onboard LED
  digitalWrite(13, LOW);
  
  pinMode(PIN_LED_INDICATOR, OUTPUT);
  
  pinMode(PIN_BTN_INTERRUPT, INPUT_PULLUP);

  pinMode(PIN_COLD_LIGHT, OUTPUT);
  pinMode(PIN_WARM_LIGHT, OUTPUT);

  pinMode(PIN_LIGHT_DETECTOR, INPUT_PULLUP);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  
  attachInterrupt(0, onButtonPress, RISING);

  
  lcd.init();
  lcd.backlight();

  if (Serial)
    Serial.begin(9600);
  
  lcd.createChar(1, degreeSymbol);
//  time.begin();
  dht.begin();
  updateHT();
}

void loop() {
  if (getLightIntensity() < brightnessTarget) {
    brightness += 3;
    if (brightness > 1023) brightness = 1023;
  }
  if (getLightIntensity() > brightnessTarget) {
    brightness -= 4;
    if (brightness < 0) brightness = 0;
  }

  if (Serial && Serial.available() > 0) {
    lcd.clear();
    lcd.print(Serial.readString());
  }
  analogWrite(PIN_LCD_BACKLIGHT, max(255 - getLightIntensity()/4, brightnessFilter(brightness)));
  
  if (millis()%10 == 0)
    updateHT();
  
  if (digitalRead(PIN_SWITCH) == HIGH) {
    displayDateTime();
    analogWrite(PIN_WARM_LIGHT, brightnessFilter(brightness));
    analogWrite(PIN_COLD_LIGHT, brightnessFilter(brightness));
    Serial.println(String(brightness) + " - " + String(brightnessFilter(brightness)) + " - " + String(getLightIntensity()));
  } else {
    analogWrite(PIN_COLD_LIGHT, 3);
    analogWrite(PIN_WARM_LIGHT, 3);
  }
}

void onButtonPress() {
  
}

int getLightIntensity() {
  return 1024 - analogRead(PIN_LIGHT_DETECTOR);
}

int brightnessFilter(int v) {
  return map((tanh(v/1024.0*2*PI - PI) + 1)/2.0*1024, 0, 1024, 0, 256);
}

void displayDateTime()
{
  lcd.home();
  //lcd.print(time.gettime("d/m/Y "));
  //lcd.print(time.gettime("D"));
  lcd.print("Light: " + String(map(getLightIntensity(), 0, 1024, 0, 100)) + "%");
  lcd.setCursor(0, 1);
  //lcd.print(time.gettime("h:i A"));
  lcd.setCursor(6, 1);
  lcd.print(humidity);
  lcd.print("%");
  lcd.setCursor(11, 1);
  lcd.print(temperature);
  lcd.print("\1C  ");
}

void updateHT()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}
