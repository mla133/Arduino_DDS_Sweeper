#include <stdint.h>
#include <TFTv2.h>
#include <SPI.h>
#include <SoftwareSerial.h>

//Setup some items
#define W_CLK 8   // Pin 8 - connect to AD9850 module word load clock pin (CLK)
#define FQ_UD 9   // Pin 9 - connect to freq update pin (FQ)
#define DATA 10   // Pin 10 - connect to serial data load pin (DATA)
#define RESET 11  // Pin 11 - connect to reset pin (RST) 
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }
#define TX_PIN 3
#define LED_PIN 13

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = 120, osy = 120; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update
int_fast32_t rx=7025000; // Starting frequency of VFO
int_fast32_t rx2=1; // variable to hold the updated frequency
int_fast32_t increment = 10000; // starting VFO update increment in HZ

const char *splash =
" _  __   _____   ____    ______  __    __   _____ \r\n"
"| |/ /  / ____| |___ \\  |  ____| \\ \\  / /  / ____|\r\n"
"| ' /  | |        __) | | |__     \\ \\/ /  | (___  \r\n"
"|  <   | |       |__ <  |  __|     \\  /    \\___ \\ \r\n"
"| . \\  | |____   ___) | | |____    |  |    ____) |\r\n"
"|_|\\_\\  \\_____| |____/  |______|   |__|   |_____/ \r\n";

SoftwareSerial lcd = SoftwareSerial(255, TX_PIN);

int old_analog =  -999; // Value last displayed
int old_digital = -999; // Value last displayed
int VSWR = 0;
int REV = 0;
int FWD = 0;

void setup()
{
    TFT_BL_ON;                                      // turn on the background light
    Tft.TFTinit();                                  // init TFT library
    Serial.begin(9600);
    Serial.print(splash);
    analogMeter(); // Draw analogue meter
    digitalMeter(); // Draw digital meter
    updateTime = millis(); // Next update time
}

void loop()
{
  if (updateTime <= millis()) {

//    rx = SerialInput();

//    if (rx != rx2)
//    {  
//          sendFrequency(rx);
//          rx2 = rx;
//    }

    updateTime = millis() + 500;

    // Read the forawrd and reverse voltages [map to 0-100 scale for plotNeedle()]
    //REV = map(analogRead(A9),0,1023,0,100);
    //FWD = map(analogRead(A8),0,1023,0,100);
    REV = random(25,75);
    FWD = random(25,75);
    
    if(REV>=FWD){
      // To avoid a divide by zero or negative VSWR then set to max 999
      VSWR = 100;
    }else{
      // Calculate VSWR
      VSWR = (FWD+REV)/(FWD-REV);
    }
    plotNeedle(VSWR, 8); // Update analogue meter, 8ms delay per needle increment    
    showDigital(map(VSWR,0,100,0,5),8);
  }
}

// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  // Meter outline
  Tft.fillRectangle(0, 0, 239, 126, GRAY1);
  Tft.fillRectangle(5, 3, 230, 119, WHITE);
  
  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;
    
    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 120;
    uint16_t y0 = sy * (100 + tl) + 140;
    uint16_t x1 = sx * 100 + 120;
    uint16_t y1 = sy * 100 + 140;
    
    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120;
    int y2 = sy2 * (100 + tl) + 140;
    int x3 = sx2 * 100 + 120;
    int y3 = sy2 * 100 + 140;
    
    // Green zone limits
    if (i >= -50 && i < 0) {
      Tft.fillTriangle(x0, y0, x1, y1, x2, y2, GREEN);
      Tft.fillTriangle(x1, y1, x2, y2, x3, y3, GREEN);
    }
    
    // Yellow zone limits
    if (i >= 0 && i < 25) {
      Tft.fillTriangle(x0, y0, x1, y1, x2, y2, YELLOW);
     Tft.fillTriangle(x1, y1, x2, y2, x3, y3, YELLOW);
    }

    // Red zone limits
    if (i >= 25 && i < 50) {
      Tft.fillTriangle(x0, y0, x1, y1, x2, y2, RED);
      Tft.fillTriangle(x1, y1, x2, y2, x3, y3, RED);
    }
    
    // Short scale tick length
    if (i % 25 != 0) tl = 8;
    
    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 120;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120;
    y1 = sy * 100 + 140;
    
    // Draw tick
    Tft.drawLine(x0, y0, x1, y1, BLACK);
    
    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 120;
      y0 = sy * (100 + tl + 10) + 140;
      switch (i / 25) {
        case -2: Tft.drawString("1", x0, y0 - 12, 1,BLACK); break;
        case -1: Tft.drawString("1.5", x0, y0 - 9, 1, BLACK); break;
        case 0: Tft.drawString("2:1", x0-8, y0 - 10, 1, BLACK); break;
        case 1: Tft.drawString("3", x0, y0 - 9, 1, BLACK); break;
        case 2: Tft.drawString("5", x0, y0 - 12, 1, BLACK); break;
      }
    }
    
    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120;
    y0 = sy * 100 + 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) Tft.drawLine(x0, y0, x1, y1, BLACK);
  }
  
  Tft.drawString("SWR", 5 + 230 - 40, 119 - 20, 2, BLACK); // Units at bottom right
  Tft.drawRectangle(5, 3, 230, 119, BLACK); // Draw bezel line
  
  plotNeedle(0,0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  char buf[8]; dtostrf(value, 4, 0, buf);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle util new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;
    
    if (ms_delay == 0) old_analog = value; // Update immediately id delay is 0
    
    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle 
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg+90) * 0.0174532925);
    
    // Erase old needle image
    Tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, WHITE);
    Tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, WHITE);
    Tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, WHITE);
    
    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;
    
    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    Tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, RED);
    Tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, BRIGHT_RED);
    Tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, RED);
    
    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;
    
    // Wait before next update
    delay(ms_delay);
  }
}

// #########################################################################
// Draw 3 digit digital meter with faint 7 segment image
// #########################################################################
void digitalMeter()
{
  int xpos = 118, ypos = 134; // was 134
  
  Tft.fillRectangle(xpos - 52, ypos - 5, 2 * 54, 59, GRAY1);
  Tft.fillRectangle(xpos - 49, ypos - 2, 2 * 51, 53, BLACK);
  Tft.drawString("888", xpos - 48, ypos+1, 5, BLACK);
}

// #########################################################################
// Update digital meter reading
// #########################################################################
void showDigital(float value, byte ms_delay)
{
  if (value==old_digital) return; // return if no change to prevent flicker
  if (value < 0) value = 0; //Constrain lower limit to 0
  if (value > 999) value = 999; //Constrain upper limit to 999
  
  int xpos = 118, ypos = 134+1; // Position with location tweak
  Tft.fillRectangle(xpos - 47, ypos, xpos+100, ypos+100, BLACK); //Erase old value
  
  // Nb. 32 pixels wide +2 gap per digit
  if (ms_delay == 0) old_digital = value; // Update immediately if delay is 0
  
  // Update with new value
  if (value < 10) Tft.drawFloat(value, xpos-54, ypos, 5, RED);
  else if (value < 100) Tft.drawFloat(value, xpos - 14, ypos, 5, RED);
  //else Tft.drawNumber((long int) numToAscii(value), xpos - 47, ypos, 5, RED);
  else Tft.drawFloat(value, xpos-14, ypos, 5, RED);
  old_digital = value;
  
    // Slow display down slightly as it approaches new postion
    if (abs(old_digital - value) < 10) ms_delay += ms_delay / 5;
    
    // Wait before next update
    delay(ms_delay);
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

void setCursor(uint8_t row, uint8_t col)
{
    int position;
    if(row==0) position = 128;
    else if (row==1) position = 148;
    
    position = position + col;
    lcd.write(position);    
}
