// DHT setup
#include "DHT.h"      
#define DHTPIN 8  
#define DHTTYPE DHT11       
DHT dht(DHTPIN, DHTTYPE);
// end DHT setup

//lcd setup
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// end lcd setup

void setup() {

  dht.begin();
  lcd.begin(16, 2);

}

void loop() {
	// Wait a few seconds between measurements.
  delay(5000);

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  lcd.setCursor(0,0);
  lcd.print("Humidity: ");
  lcd.setCursor(10,0);
  lcd.print(h);
  lcd.setCursor(14,0);
  lcd.print("%");

  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.setCursor(6,1);
  lcd.print(f);
  lcd.setCursor(11,1);
  lcd.print(char(223));
  lcd.print("F");  
}