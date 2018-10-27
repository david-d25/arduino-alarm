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

#define LCD_WIDTH   16
#define LCD_HEIGHT  2

#define PIN_INTERRUPT 2

#define TIME_PIN_CLK 3
#define TIME_PIN_DAT 4
#define TIME_PIN_RST 5

#define PIN_LCD_BACKLIGHT 6

#define PIN_SPEAKER       7
#define PIN_CIRCUIT_CHECK 8
#define PIN_LED_INDICATOR 9

#define PIN_WARM_LIGHT 10
#define PIN_COLD_LIGHT 11

#define PIN_DHT             A0
#define PIN_LIGHT_DETECTOR  A1
#define PIN_BTN_SIGNAL      A2
#define PIN_SWITCH          A3
#define PIN_LCD_SDA         A4 // IMPORTANT! Do not move!
#define PIN_LCD_SCL         A5 // IMPORTANT! Do not move!

#define VIEW_STANDART 0
#define VIEW_SETTINGS 1

#define BTN_UP    1
#define BTN_RIGHT 2
#define BTN_DOWN  3
#define BTN_LEFT  4

#define BTN_UP_LONG     5
#define BTN_RIGHT_LONG  6
#define BTN_DOWN_LONG   7
#define BTN_LEFT_LONG   8

#define BTN_VALUE_UP    975
#define BTN_VALUE_RIGHT 990
#define BTN_VALUE_DOWN  980
#define BTN_VALUE_LEFT  998
#define BTN_VALUE_THRESHOLD 10
#define BTN_LONG_ITERATIONS 4000
#define BTN_MIN_ITERATIONS  500

bool isAlarmActive = false;

int interfaceBrightness = 0;
int warmLightBrightness = 0;
int coldLightBrightness = 0;
int brightnessTarget = (int)(1024 * 0.25);

unsigned int loopIteration = 0;

volatile unsigned long lastInterrupt = 0;
volatile byte lastPressedButton = 0;

iarduino_RTC time(RTC_DS1302, TIME_PIN_RST, TIME_PIN_CLK, TIME_PIN_DAT);
LiquidCrystal_I2C lcd(0x3f, LCD_WIDTH, LCD_HEIGHT);
DHT dht(PIN_DHT, DHT11);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PIN_LED_INDICATOR, OUTPUT);

  pinMode(PIN_COLD_LIGHT, OUTPUT);
  pinMode(PIN_WARM_LIGHT, OUTPUT);

  pinMode(PIN_BTN_SIGNAL, INPUT);

  pinMode(PIN_LIGHT_DETECTOR, INPUT_PULLUP);
  pinMode(PIN_SWITCH, INPUT_PULLUP);

  tone(PIN_SPEAKER, 500, 100);
  delay(200);
  tone(PIN_SPEAKER, 750, 100);
  delay(200);
  tone(PIN_SPEAKER, 1000, 100);

  attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), onInterrupt, RISING);

  digitalWrite(LED_BUILTIN, LOW);

  if (Serial)
    Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  lcd.createChar(1, degreeSymbol);
  time.begin();
  dht.begin();
}

int tempLight = false;
void loop() {
  while (isCircuitShorted()) {
    setLightsBrightness(0);
    lcd.clear();
    lcd.print("CIRCUIT SHORTED");
    tone(PIN_SPEAKER, 1000, 500);
    delay(1000);
  }

  if (lastPressedButton) {
    lcd.clear();
    if (lastPressedButton == BTN_UP)
      isAlarmActive = !isAlarmActive;
    if (lastPressedButton == BTN_RIGHT)
      interfaceBrightness = 1024;
    if (lastPressedButton == BTN_DOWN)
      tempLight = !tempLight;
    if (lastPressedButton == BTN_LEFT) {
      tone(PIN_COLD_LIGHT, 20, 3000);
    }
    lastPressedButton = 0;
  }

  if (tempLight) {
    if (interfaceBrightness < 1024) interfaceBrightness++;
    setLightsBrightness(brightnessFilter(interfaceBrightness));
    return;
  }
  if (getLightIntensity()/3 < brightnessTarget + (coldLightBrightness+warmLightBrightness)/11) {
    interfaceBrightness += 1;
    if (interfaceBrightness > 1023) interfaceBrightness = 1023;
  }
  if (getLightIntensity()/3 > brightnessTarget + (coldLightBrightness+warmLightBrightness)/11) {
    interfaceBrightness -= 1;
    if (interfaceBrightness < 0) interfaceBrightness = 0;
  }

  analogWrite(PIN_LCD_BACKLIGHT, getLightIntensity() / 4);
  analogWrite(PIN_LED_INDICATOR, isAlarmActive ? getLightIntensity()/4 : 0);
  
  if (digitalRead(PIN_SWITCH) == HIGH)
    setLightsBrightness(brightnessFilter(interfaceBrightness));
  else
    setLightsBrightness((sin(loopIteration/250.0)+1)*127.5, (cos(loopIteration/250.0)+1)*127.5);

  loopIteration++;
}

void onInterrupt() {
  if (millis() - lastInterrupt < 30) return;
  lastInterrupt = millis();
  unsigned short maxValue = 0;
  unsigned int iteration = 0;
  
  while (digitalRead(PIN_INTERRUPT) && ++iteration < BTN_LONG_ITERATIONS)
    maxValue = max(maxValue, analogRead(PIN_BTN_SIGNAL));

  if (iteration < BTN_MIN_ITERATIONS) return;
  
  lastPressedButton = getButtonByValue(maxValue, iteration >= BTN_LONG_ITERATIONS);
}

byte getButtonByValue(int value, bool longFlag) {
  byte result = 0;
  unsigned short minValue = 1024;
  if (abs(BTN_VALUE_UP    - value) < minValue) {minValue = abs(BTN_VALUE_UP     - value); result = BTN_UP;    }
  if (abs(BTN_VALUE_RIGHT - value) < minValue) {minValue = abs(BTN_VALUE_RIGHT  - value); result = BTN_RIGHT; }
  if (abs(BTN_VALUE_DOWN  - value) < minValue) {minValue = abs(BTN_VALUE_DOWN   - value); result = BTN_DOWN;  }
  if (abs(BTN_VALUE_LEFT  - value) < minValue) {minValue = abs(BTN_VALUE_LEFT   - value); result = BTN_LEFT;  }
  if (minValue > BTN_VALUE_THRESHOLD) return 0;
  if (result == BTN_UP    && longFlag) result = BTN_UP_LONG;
  if (result == BTN_RIGHT && longFlag) result = BTN_RIGHT_LONG;
  if (result == BTN_DOWN  && longFlag) result = BTN_DOWN_LONG;
  if (result == BTN_LEFT  && longFlag) result = BTN_LEFT_LONG;
  return result;
}

int getLightIntensity() {
  return 1024 - analogRead(PIN_LIGHT_DETECTOR);
}

int brightnessFilter(int v) {
  return map((tanh(v/1024.0*2*PI - PI) + 1) / 2.0 * 1024, 0, 1024, 0, 256);
}

void setLightsBrightness(byte cold, byte warm) {
  coldLightBrightness = cold;
  warmLightBrightness = warm;
  analogWrite(PIN_COLD_LIGHT, cold);
  analogWrite(PIN_WARM_LIGHT, warm);
}
void setLightsBrightness(byte brightness) {
  setLightsBrightness(brightness, brightness);
}

bool isCircuitShorted() {
  return digitalRead(PIN_CIRCUIT_CHECK) == 0;
}

//void displayDateTime()
//{
//  lcd.home();
//  lcd.print(time.gettime("d/m/Y "));
//  lcd.print(time.gettime("D"));
//  lcd.setCursor(0, 1);
//  lcd.print(time.gettime("h:i A "));
//  lcd.setCursor(11, 1);
//  lcd.print(temperature);
//  lcd.print("\1C  ");
//}
