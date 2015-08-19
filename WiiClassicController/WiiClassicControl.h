#ifndef WiiClassicControl_h
#define WiiClassicControl_h
/*
  Creator: Andrew Mascolo 
  This library is a fixed version of the original library created by Tim Hirzel.
*/

#include <Arduino.h>
#include<Wire.h>

#define VERSION 1.01
/*
Version 1.01:
  Fixed button defines for X, Y, A and B.
  They were causing errors with my other sketches and libraries.
*/
#define USE_MONITOR false

/* BBBBYTE 5*/
#define UD 0x01
#define LD 0x02
#define ZR 0x04
#define BX  0x08
#define BA  0x10
#define BY  0x20
#define BB  0x40
#define ZL 0x80

/* BBBBYTE 4*/
#define RB 0x02
#define START 0x04
#define HOME 0x08
#define SELECT 0x10
#define LB 0x20
#define DD 0x40
#define RD 0x80

class WiiClassicControl
{
  public:
    void begin(bool Vcc, bool Gnd)
	{
	    if(Vcc)
			pinMode(A3, INPUT);
		if(Gnd)
        {
			pinMode(A2, OUTPUT);
			digitalWrite(A2, LOW);
		}
		
		Wire.begin();
		Wire.beginTransmission(0x52);      // transmit to device @ BBAddress 0x52
		Wire.write(0x40);           // write memory address
		Wire.write(0x00);           // write memory address
		Wire.endTransmission();
		count = 0;
		
		for(byte i = 0; i < 6; i++)
		    data[i] = 0x00;
		
		time = millis();
	}
	
	#if USE_MONITOR
	void RawData()
	{
		Wire.requestFrom (0x52, 6); // request data from controller
		if(Wire.available ()) 
		{
			for(count = 0; count < 6; count++)
			{
				Serial.print(Wire.read(), BIN);
				Serial.print("|");
			}
			Serial.println();
			Zero();	
		}
	}
	#endif
	
	void CollectData()
	{
	    Wire.requestFrom (0x52, 6); // request data from controller
		if(Wire.available ()) 
		{
		    for(count = 0; count < 6; count++)
				data[count] = Wire.read();
			Zero();
		}
	}
	
	boolean leftShoulderPressed() {
		 return GetButton(4,LB);
	}

	boolean rightShoulderPressed() {
		 return GetButton(4,RB);
	 }

	boolean lzPressed() {
		 return GetButton(5,ZL);
	 }

	boolean rzPressed() {
		 return GetButton(5,ZR);
	 }

	boolean leftDPressed() {
		 return GetButton(5,LD);
	 }

	boolean rightDPressed() {
		 return GetButton(4,RD);
	 }

	boolean upDPressed() {
		 return GetButton(5,UD);
	 }

	boolean downDPressed() {
		 return GetButton(4,DD);
	 }

	boolean selectPressed() {
		 return GetButton(4,SELECT);
	 }

	boolean homePressed() {
		 return GetButton(4,HOME);
	 }

	boolean startPressed() {
		 return GetButton(4,START);
	 }

	boolean xPressed() {
		 return GetButton(5,BX);
	 }

	boolean yPressed() {
		 return GetButton(5,BY);
	 }

	boolean aPressed() {
		 return GetButton(5,BA);
	 }

	boolean bPressed() {
		 return GetButton(5,BB);
	 }

	byte leftStickX() {
		 return  (GetStick(0, 0x3F)>>1)*8;
	}

	byte leftStickY() {
		 return  (GetStick(1, 0x3F)>>1)*8;       
	}

	byte rightStickX() {
		 return ((GetStick(0, 0xC0)>>3) | (GetStick(1, 0xC0)>>5) | (GetStick(2, 0xC0)>>7))*8;
	}

	byte rightStickY() {
		 return GetStick(2, 0x3F)*8; 
	}
	
	#if USE_MONITOR
	void ShowData(byte col)
	{
		Serial.print(data[col],BIN);
		Serial.print(" | ");
	}
    #endif
	
	private:
	
		boolean GetButton(byte col, byte D) 
		{
			return (~data[col] & D);            
		}
	    
		int GetStick(byte col, byte mask) 
		{
			return (data[col] & mask);            
		}
		
		void Zero()
		{
			Wire.beginTransmission(0x52);   // transmit to device 0x52
			Wire.write(0x00);           	// writes one byte
			Wire.endTransmission();    		// stop transmitting
		} 
	
		int 	count;
		byte 	data[6];
		long 	time;
};
#endif
