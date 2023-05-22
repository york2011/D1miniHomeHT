#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h> 
#include <RtcDS3231.h>
#include <DHT.h>
#include <DHT_U.h>
RtcDS3231<TwoWire> Rtc(Wire);

#define th_hour_record_count 6
// 設定時區
#define timezone 8

#define SCREEN_WIDTH 128 // OLED 寬度像素
#define SCREEN_HEIGHT 64 // OLED 高度像素

#define DHTPIN D3
#define DHTTYPE DHT22

#define LEDPIN D5
#define NUMPIXELS 16
#define upper 90               // 開燈上限值
#define lower 50                // 開燈下限值
Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 設定 DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);

byte currentSecond = 0, indexTHHourRecord = 0;

boolean lightMode = true; // 1: open, 0: close
int hue = 0; 

int th_record[4][th_hour_record_count] = {
    {999,999,999,999,999,999},  // minTemp
    {-400,-400,-400,-400,-400,-400},  // maxTemp
    {999,999,999,999,999,999},  // minHumi
    {-400,-400,-400,-400,-400,-400}   // maxHumi
  };

void setup ()
{
  Serial.begin(9600);
  dht.begin();
  
  Rtc.Begin();
  pixels.begin();

  if (!Rtc.GetIsRunning())
  {
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(1000);
  display.clearDisplay();
  display.display();

  
}

void loop ()
{
  char datesplitchar = '-';
  char timesplitchar = ':';
  char yearText[6], dateText[6], timeText[9], tempText[6], dhtTempText[22], dhtHumiText[22], lightSensorText[4];
  int lightSensorValue = 0;
  byte bgColor = BLACK, textColor = WHITE;

  float DHT22_TEMP, DHT22_HUMIDITY;
  int DHT22_TEMP_int, DHT22_HUMIDITY_int;
  sensors_event_t event;
  
  
  RtcDateTime now = Rtc.GetDateTime();
  now += (timezone * 60 * 60);
  if( currentSecond != now.Second() ) {
    currentSecond = now.Second();

    lightSensorValue = analogRead(A0);

    if( lightSensorValue > upper ) {
      closeLight();
    }
    if (lightSensorValue < lower) {
      openLight();
    }
    if (lightSensorValue >= lower && lightSensorValue <= upper) {
        if (lightMode) {
          openLight();
        }
        else {
          closeLight();
        }
    }

    

    int lastIndexTHHourRecord = indexTHHourRecord;
    int th_hour = 24 / th_hour_record_count;
    indexTHHourRecord = now.Hour() / ( th_hour );
    if (indexTHHourRecord != lastIndexTHHourRecord ) {
       th_record[0][indexTHHourRecord] = 999;
       th_record[1][indexTHHourRecord] = -400;
       th_record[2][indexTHHourRecord] = 999;
       th_record[3][indexTHHourRecord] = -400;
    }
    dht.temperature().getEvent(&event);
    DHT22_TEMP = event.temperature;
    dht.humidity().getEvent(&event);
    DHT22_HUMIDITY = event.relative_humidity;

    DHT22_TEMP_int = (int)(DHT22_TEMP * 10);
    DHT22_HUMIDITY_int = (int)(DHT22_HUMIDITY * 10);

    if ( DHT22_TEMP_int < th_record[0][indexTHHourRecord]) {
      th_record[0][indexTHHourRecord] = DHT22_TEMP_int;
    }
    if ( DHT22_TEMP_int > th_record[1][indexTHHourRecord]) {
      th_record[1][indexTHHourRecord] = DHT22_TEMP_int;
    }
    if ( DHT22_HUMIDITY_int < th_record[2][indexTHHourRecord]) {
      th_record[2][indexTHHourRecord] = DHT22_HUMIDITY_int;
    }
    if ( DHT22_HUMIDITY_int > th_record[3][indexTHHourRecord]) {
      th_record[3][indexTHHourRecord] = DHT22_HUMIDITY_int;
    }

    if (now.Second() % 2) {
      datesplitchar = '-';
      timesplitchar = ':';
    }
    else {
      datesplitchar = ' ';
      timesplitchar = ' ';
    }
        
  
    display.clearDisplay();
    snprintf_P(yearText, sizeof(yearText), PSTR(" %04u"), now.Year());
    snprintf_P(dateText, sizeof(dateText), PSTR("%02u%c%02u"), now.Month(), datesplitchar, now.Day());
    snprintf_P(timeText, sizeof(timeText), PSTR("%02u%c%02u%c%02u"), now.Hour(), timesplitchar, now.Minute(), timesplitchar, now.Second());

    snprintf_P(lightSensorText, sizeof(lightSensorText), PSTR("%03u"), lightSensorValue);

    snprintf_P(tempText, sizeof(tempText), PSTR("%02u.%01uC"), (int)DHT22_TEMP, (int)(DHT22_TEMP * 10) % 10);

    
    display.fillRect(0, 0, 128, 64, bgColor);
    display.setTextColor(textColor);
    display.setTextSize(1);
    display.setCursor(1, 1);
    display.print(yearText);
    display.setCursor(1, 9);
    display.print(dateText);
    
    display.setTextSize(2);
    display.setCursor(32, 1);
    display.print(timeText);

    display.setTextSize(3);
    display.setCursor(2, 18);
    display.print(tempText);
    
    display.setTextSize(2);
    display.setCursor(92, 18);
    display.print(lightSensorText);

    display.setTextSize(1);
    display.setCursor(1, 44);
    display.print("Temp: ");
    display.print(getTHRecordMin(0), 1);
    display.print(" ");
    display.print(DHT22_TEMP, 1);
    display.print("C ");
    display.print(getTHRecordMax(1), 1);
    display.setCursor(1, 54);
    display.print("Humi: ");
    display.print(getTHRecordMin(2), 1);
    display.print(" ");
    display.print(DHT22_HUMIDITY, 1);
    display.print("% ");
    display.print(getTHRecordMax(3), 1);

    snprintf_P(lightSensorText, sizeof(lightSensorText), PSTR("%03u"), hue);
    display.setTextSize(1);
    display.setCursor(100, 34);
    display.print(lightSensorText);
    
    display.invertDisplay( now.Minute() % 2 );
    display.display();

    Serial.print(yearText);
    Serial.print(" ");
    Serial.print(dateText);
    Serial.print(" ");
    Serial.print(timeText);
    Serial.println(" ");
    
    
  }
  delay(20);
  
}


float getTHRecordMin(int kind) {
  float rValue = 999;
  for (int i = 0 ; i < th_hour_record_count; i++) {
    if ( rValue > th_record[kind][i] ) {
      rValue = th_record[kind][i];
    }
  }
  return (rValue / 10);
}
float getTHRecordMax(int kind) {
  float rValue = -400;
  for (int i = 0 ; i < th_hour_record_count; i++) {
    if ( rValue < th_record[kind][i] ) {
      rValue = th_record[kind][i];
    }
  }
  return (rValue / 10);
}


void closeLight() {
  lightMode = false;
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void openLight() {
  lightMode = true;
  pixels.clear();
  pixels.fill(pixels.ColorHSV( hue * 256 ,255,255),0,0);
  hue++;
  if ( hue == 256 ) {
    hue = 0 ;
  }
  pixels.show();
}
