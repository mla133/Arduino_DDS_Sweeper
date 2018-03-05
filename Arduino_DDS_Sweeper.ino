#include <stdint.h>
#include <SPI.h>
#include <SoftwareSerial.h>

//Setup some items
#define W_CLK 8   // Pin 8 - connect to AD9850 module word load clock pin (CLK)
#define FQ_UD 9   // Pin 9 - connect to freq update pin (FQ)
#define DATA 10   // Pin 10 - connect to serial data load pin (DATA)
#define RESET 11  // Pin 11 - connect to reset pin (RST) 
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }
#define blinkLED(pin) {digitalWrite(pin,HIGH); delay(100); digitalWrite(pin,LOW); }
#define LED_PIN 13

uint32_t updateTime = 0;       // time for next update
double rx=7025000; // Starting frequency of VFO
double rx2=1; // variable to hold the updated frequency
int VSWR = 0;
int REV = 0;
int FWD = 0;

void setup()
{
    Serial.begin(9600);
    updateTime = millis(); // Next update time
}

void loop()
{  
  if (updateTime <= millis()) 
  {
    rx = SerialInput();
    if (rx != rx2)
    {  
          sendFrequency(rx);
          rx2 = rx;
    }

    updateTime = millis() + 500;

    FWD = analogRead(A2);
    REV = analogRead(A1);
    
    if(REV>=FWD)
    {
      // To avoid a divide by zero or negative VSWR then set to max 999
      VSWR = 100;
    }
    else
    {
        // Calculate VSWR
        VSWR = (FWD+REV)/(FWD-REV);
//        Serial.print("FREQ: ");
//        Serial.print(rx);
//        Serial.print(" FWD: ");
//        Serial.print(FWD);
//        Serial.print(" REV: ");
//        Serial.print(REV);
//        Serial.print(" VSWR: ");
//        Serial.println(VSWR);
//        blinkLED(LED_PIN);
     }
  }
}

char* numToAscii(double num)
{
// Takes a floating or double precision number in, and returns an ascii string with up to 1/1000
// precision
	char ascii[32];
	int frac;
	frac=(unsigned int) (num*1000)%1000;  //get three numbers to the right of decimal
	itoa((int)num,ascii,10);
	strcat(ascii,".");
	itoa(frac,&ascii[strlen(ascii)],10);  // put the frac after the decimal
	return ascii;
}

// frequency calc from datasheet page 8 = <sys clock> * <frequency tuning word>/2^32
void sendFrequency(double frequency) 
{  
  int32_t freq = frequency * 4294967295/125000000;  // note 125 MHz clock on 9850.  You can make 'slight' tuning variations here by adjusting the clock frequency.
  for (int b=0; b<4; b++, freq>>=8) {
    tfr_byte(freq & 0xFF);
  }
  tfr_byte(0x000);   // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
}

// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void tfr_byte(byte data)
{
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DATA, data & 0x01);
    pulseHigh(W_CLK);   //after each bit sent, CLK is pulsed high
  }
}

int SerialInput()
{
    char incomingByte;
    while (Serial.available() > 0) 
    {
      incomingByte = Serial.read();
      if (incomingByte == '\n') break;   // exit after newline char
      if (incomingByte == -1) continue;  // if no chars in buffer...Serial.read returns -1...
      rx *= 1000;
      rx += (incomingByte - 48);
    }
    return rx;
}
