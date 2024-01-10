/*
MoveWithController.ino - Code to control the WidowX Robot Arm with a controller
Created by Lenin Silva, June, 2020

copied from:
https://github.com/LeninSG21/WidowX/blob/master/Arduino%20Library/Examples/MoveWithController/MoveWithController.ino
 
 MIT License
Copyright (c) 2021 LeninSG21
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include <WidowX.h>
#include <poses.h>
#include <ax12.h>

#define NUM_CHARS 6
#define USER_FRIENDLY 0
#define POINT_MOVEMENT 1
#define MOVE_TO_POINT  2

WidowX widow = WidowX();

byte buff[NUM_CHARS]; //buffer to save the message
char ln[200]; // debugging
uint8_t open_close, close, options;
uint8_t moveOption = USER_FRIENDLY;
int vx, vy, vz, vg, vq5;
float p[3];
long initial_time;

/*
    BUFFER MESSAGE

    byte index --> data
    0 --> speed in x, where MSb is sign
    1 --> speed in y, where MSb is sign
    2 --> speed in z, where MSb is sign
    3 --> speed for gamma
    4 --> speed for q5
    5 --> Sg<<7 | Sq5 <<6 | open_close[1..0] << 4 | options[3..0]
            Sg --> sign of gamma speed
            Sq5 --> sign of Q5 speed
            open_close: 0b00 || 0b11 --> void
                        0b01 --> open
                        0b10 --> close
            options: 1 --> rest
                    2 --> home
                    3 --> center / TO_POINT
                    4 --> relax
                    5 --> torque
                    6 --> moveOption = POINT_MOVEMENT
                    7 --> moveOption = USER_FRIENDLY
                    other --> void
*/

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("...Starting Robotic Arm...");
  delay(300);
  widow.init(0);
  delay(100);

  /*
   * Handshake with the sender.
   * Sends ok\n. Waits until the sender writes ok\n
   * Sends ok\n to indicate that it is ready to receive
  */
  
  widow.printState();
  Serial.println("ok");
  while (!Serial.available());
  while (readStringUntil('\n') != "ok")
  {
      while (!Serial.available());
  }
  delay(1000);
  widow.printState();
  Serial.println("ok");

}

void loop() {
  if(Serial.available())
  {
    readBytes();
    initial_time = millis();
    options = buff[5] & 0xF; //Get options bits 0b1111
    if (options == 0) //No given option (higher priority)
    {
        vx = buff[0] & 0x7F;
        if (buff[0] >> 7)
            vx = -vx;

        vy = buff[1] & 0x7F;
        if (buff[1] >> 7)
            vy = -vy;

        vz = buff[2] & 0x7F;
        if (buff[2] >> 7)
            vz = -vz;

        vg = buff[3];
        vq5 = buff[4];

        if (buff[5] >> 7) //Sg
            vg = -vg;
        if ((buff[5] >> 6) & 1) //Sq5
            vq5 = -vq5;
        open_close = (buff[5] >> 4) & 0b11;
        close = (buff[5] >> 5) & 1;
        delay(5);
        if (vq5 && moveOption != MOVE_TO_POINT)
            widow.moveServoWithSpeed(4, vq5, initial_time);
        if (vx || vy || vz || vg)
        {
          if(moveOption == POINT_MOVEMENT) {
            widow.movePointWithSpeed(vx, vy, vz, vg, initial_time);
          } else if(moveOption == MOVE_TO_POINT) {
            Serial.println("moveArmGamma");
            float fvx = 1.0 * vx;
            float fvy = 1.0 * vy;
            float fvz = 1.0 * vz;
            float fvg = 1.0 * vg;
            /*#######################
            # widow.moveServo2Angle(3, vg)
            # delay(3000);
            # widow.moveArmQ4(vx, vy, vz);
            # delay(5000);
            #######################*/
            delay(3000);
            widow.moveArmGammaController(fvx, fvy, fvz, fvg); 
            widow.openCloseGrip(close);
          } else if(moveOption == USER_FRIENDLY)
            widow.moveArmWithSpeed(vx, vy, vz, vg, initial_time);
        }
            
        if(moveOption != MOVE_TO_POINT) {
          switch (open_close)
          {
          case 1:
            widow.moveGrip(0);
            break;
          case 2:
            widow.moveGrip(1);
            break;
          default:
            break;
          }
        }
      delay(300);
    }
    else
    {
        switch (options)
        {
        case 1:
            widow.moveRest();
            delay(300);
            break;
        case 2:
            /* widow.moveCenter(); */
            widow.moveHome(); 
            delay(300);
            break;
        case 3:
            moveOption = MOVE_TO_POINT;
            /* widow.moveCenter(); */
            break;
        case 4:
            widow.relaxServos();
            break;
        case 5:
            widow.torqueServos();
            break;
        case 6:
            moveOption = POINT_MOVEMENT;
            break;
        case 7:
            moveOption = USER_FRIENDLY;
            break;
        default:
            break;
        }
    }
    if (0) {
      widow.getPoint(p);
      sprintf(ln, "point %d %d %d\n", int(p[0]),int(p[1]),int(p[2]));
      Serial.println(ln);
      delay(600);
    }
    widow.printState();
    Serial.println("ok");
    
  }

}

String readStringUntil(char endchar){
  char c = 0;
  String s = "";
  while(1){
   while(!Serial.available());
   c = Serial.read();
   if(c == endchar)
     break;
   s += c;
  }
  return s;
}

void readBytes(){
  for(int i = 0; i < NUM_CHARS; i++){
     while(!Serial.available());
     buff[i] = (byte) Serial.read(); 
  }
}
