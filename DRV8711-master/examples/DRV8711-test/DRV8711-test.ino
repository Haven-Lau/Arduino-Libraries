#include <DRV8711.h>
#include <EEPROM.h>

#define DRV8711_EEPROM_ADDR 0x100 // leave 256 bytes untouched

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#  define CSpin    37
#  define STEPpin  36
#  define DIRpin   35
#  define RESETpin 34
#else
#  define CSpin    4
#  define STEPpin  5
#  define DIRpin   6
#  define RESETpin 7
#endif

DRV8711 stepper(CSpin) ;

void setup ()
{
  pinMode (RESETpin, OUTPUT) ;
  digitalWrite (RESETpin, HIGH) ;
  delay (10) ;
  digitalWrite (RESETpin, LOW) ;
  delay (1) ;
  
  Serial.begin (115200) ;
  Serial.print (pinSCLK) ;
  pinMode (STEPpin, OUTPUT) ;
  pinMode (DIRpin, OUTPUT) ;
  stepper.begin (60, 8, 2, 0, 1, 0) ;
  delay (10) ;
  Serial.println (stepper.get_status (), HEX) ;
}

int del = 800 ;
bool up = false ;
void loop ()
{
  //delay (2);
  while (del != 7)
  {
  delayMicroseconds (del) ;
  del -- ;
  if (del < 40) del = 40 ;
  do_step (0) ;
  }
  
  //Serial.println (stepper.get_status (), HEX) ;
  //stepper.clear_status () ;
}

void do_step (byte dir)
{
  digitalWrite (DIRpin, dir) ;
  digitalWrite (STEPpin, HIGH) ;
  //delayMicroseconds (2) ;
  digitalWrite (STEPpin, LOW) ;
}

