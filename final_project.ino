#include <LiquidCrystal.h>
#include <RTClib.h>
#include <DHT.h>
#include <Stepper.h>
#include "Arduino.h"
#include "uRTCLib.h"

int checkState();
void changeLED(int pin);

#define RDA 0x80
#define TBE 0x20  

// GPIO Pointers 
// port e (pins 2(pe4), 3(pe5), 5(pe3))
volatile unsigned char* port_e = (unsigned char*) 0x2E; 
volatile unsigned char* ddr_e  = (unsigned char*) 0x2D; 
volatile unsigned char* pin_e  = (unsigned char*) 0x2C;

//port b (pin 10(pb4), 11(pb5), 12(pb6), 13(pb7))
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23; 

//port f (pf0 = A0)
volatile unsigned char* port_f = (unsigned char*) 0x31;
volatile unsigned char* ddr_f  = (unsigned char*) 0x30;
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;

// Port G Registers
unsigned char* const pin_g = (unsigned char*) 0x32;
unsigned char* const port_g = (unsigned char*) 0x34;
unsigned char* const ddr_g = (unsigned char*) 0x33;

//UART pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//ADC pointers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// timer pointers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

// DHT setup    
#define DHTPIN 8  
#define DHTTYPE DHT11       
DHT dht(DHTPIN, DHTTYPE);

//lcd setup
const int rs = 31, en = 33, d4 = 37, d5 = 39, d6 = 41, d7 = 43;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//rtc setup
uRTCLib rtc(0x68);
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#define blueLED 10
#define greenLED 11
#define yellowLED 12
#define redLED 13

#define water_thr 100
#define temp_thr 80

int waterLevel = 0;
int water_lvl;
int i = 0;
float temp;

// 1 = running, 2 = idle, 3 = disabled, 4 = error
int curState = 2;
int prevState = 2;

//to handle the interrupt when the start/stop button is pressed
void handleButtonPress(){
  if (curState == 3){
      prevState = curState;
      curState = 2;
    } else {
      prevState = curState;
      curState = 3;
    }
}

void setup() {
  Serial.begin(9600);
  *ddr_b |= 0xf0; // set LED pins to output
  *ddr_e |= 0x04; // set Motor pin to output
  toggleMotor(0); // make sure motor starts off
  adc_init(); // initialize ADC
  dht.begin(); // initialize DHT
  lcd.begin(16, 2); // initiialize LCD
  URTCLIB_WIRE.begin();
  rtc.set(0, 56, 12, 5, 13, 1, 22);
  *ddr_e |= 0x28; //set motor pins to output
  *ddr_g |= 0x10;
  *pin_g &= 0xef; // disable CCW direction on motor
  *pin_e |= 0x20; // set speed of motor high
  attachInterrupt(digitalPinToInterrupt(2), handleButtonPress, RISING); //attach interrupt to start/stop button
}

void loop() {
  //curState = checkState();
  if (curState != prevState){
    stateChange();
  }
  switch(curState){
    case 1: // running
      changeLED(blueLED);
      // turn on fan
      toggleMotor(1);
      prevState = curState;
      curState = checkState(); 
        // monitor temp and when low enough switch to idle
        // monitor water and switch to error if too low
      displayTemp();
      break;
    case 2: // idle
      changeLED(greenLED);
      // turns fan off
      toggleMotor(0);
      prevState = curState;
      curState = checkState(); 
        // monitor temp and when high enough switch to running
        // monitor water and switch to error if too low
      displayTemp();
      break;
    case 3: // disabled
      changeLED(yellowLED);
      prevState = curState;
      // everything off, no monitoring
      toggleMotor(0);
      // interupt to monitor start button
        // switches to idle if pushed
      break;
    case 4: // error
      changeLED(redLED);
      displayErrMsg();
      prevState = curState;
      toggleMotor(0);
      // does nothing until stop or reset button is pushed
      break;
    }
    myDelay(1000);

  }

void changeLED(int pin){
  //turn LEDS off
  *pin_b |= 0x00;

  switch(pin){
    case blueLED:
      *pin_b |= 0x10;
      break;
    case greenLED:
      *pin_b |= 0x20;
      break;
    case yellowLED:
      *pin_b |= 0x40;
      break;
    case redLED:
      *pin_b |= 0x80;
      break;
  }

}

void toggleMotor(bool mode){
  if (!mode){
    *pin_e &= 0xf7;
  } else {
    *pin_e |= 0x08;
  }
}

int checkState(){
  water_lvl = readWaterLevelADC(); 
  temp = dht.readTemperature(true);
  if(water_lvl <= water_thr ){
    return 4;
  } else if(temp <= temp_thr){
    return 2;
  } else if(temp > temp_thr){
    return 1;
  }
}

void stateChange(){
  Serial.println("state changed at: ");
  myDelay(50);
  displayTime();
  myDelay(2000);
}

//UART functions
void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

// ADC functions
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

int readWaterLevelADC(){
  int data;
  //turn on
  *pin_f |= 0x01;

  //read data
  myDelay(10);						      // wait 10 milliseconds
	waterLevel = adc_read(0);		  // Read the analog value form sensor
  
  //turn off
  *pin_f &= 0xFE;

  // repeate for funsies
  //turn on
  *pin_f |= 0x01;
  
  //read data
  myDelay(10);						      // wait 10 milliseconds
	waterLevel = adc_read(0);		  // Read the analog value form sensor
  
  //turn off
  *pin_f &= 0xFE;

  return waterLevel;
}

// replaces delay
void myDelay(unsigned int milliseconds) {
  int freq = (1/milliseconds)*1000;
  // calc period
  double period = 1.0/double(freq);
  // 50% duty cycle
  double half_period = period/ 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  // start the timer
  * myTCCR1A = 0x0;
  * myTCCR1B |= 0b00000001;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8;   // 0b 0000 0000
  // reset TOV           
  *myTIFR1 |= 0x01;
}

int prevMs = -600000;
int curMs = 0;

void displayTemp(){
  curMs = millis();
  //if ( (curMs - prevMs) >= 60000 ){
    prevMs = curMs;
    float h = dht.readHumidity();
    float f = dht.readTemperature(true);
    lcd.clear();

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
  //}
}

void displayErrMsg(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ERROR: ");
  lcd.setCursor(0,1);
  lcd.print("Water Level Low");
}

void displayTime() {
  U0putchar(' ');
  rtc.refresh();
  int year = rtc.year();
  int month = rtc.month();
  int day = rtc.day();
  int hour = rtc.hour();
  int minute = rtc.minute();
  int second = rtc.second();
  char numbers[10] = {'0','1','2','3','4','5','6','7','8','9'};
  int onesYear = year % 10;
  int tensYear = year / 10 % 10;
  int onesMonth = month % 10;
  int tensMonth = month / 10 % 10;
  int onesDay = day % 10;
  int tensDay = day / 10 % 10;
  int onesHour = hour % 10;
  int tensHour = hour / 10 % 10;
  int onesMinute = minute % 10;
  int tensMinute = minute / 10 % 10;
  int onesSecond = second % 10;
  int tensSecond = second / 10 % 10;
  
  U0putchar('M');
  U0putchar(':');
  U0putchar(numbers[tensMonth]);
  U0putchar(numbers[onesMonth]);
  U0putchar(' ');
  U0putchar('D');
  U0putchar(':');
  U0putchar(numbers[tensDay]);
  U0putchar(numbers[onesDay]);
  U0putchar(' ');
  U0putchar('Y');
  U0putchar(numbers[tensYear]);
  U0putchar(numbers[onesYear]);
  U0putchar(' ');
  
  U0putchar('H');
  U0putchar(':');
  U0putchar(numbers[tensHour]);
  U0putchar(numbers[onesHour]);
  U0putchar(' ');
  U0putchar('M');
  U0putchar(':');
  U0putchar(numbers[tensMinute]);
  U0putchar(numbers[onesMinute]);
  U0putchar(' ');
  U0putchar('S');
  U0putchar(':');
  U0putchar(numbers[tensSecond]);
  U0putchar(numbers[onesSecond]);
  U0putchar(' ');
  U0putchar('\n');
}
