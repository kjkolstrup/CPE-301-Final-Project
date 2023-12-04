#include "Arduino.h"
#include "uRTCLib.h"

#define RDA 0x80
#define TBE 0x20  

// uRTCLib rtc;
uRTCLib rtc(0x68);
//UART pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  Serial.begin(9600);
  delay(3000); // wait for console opening

  URTCLIB_WIRE.begin();
  U0init(9600);

  rtc.set(0, 56, 12, 5, 13, 1, 22);
}

void loop() {

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
  
  delay(10000000);
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