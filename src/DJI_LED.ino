/*

An Open Source Arduino based DJI Led project.

Program : DJI Led (Firmware)
Version : V1.1, Decembre 11th 2013
Author(s): Yves Gohy (diogeneyves@gmail.com)
Thanks to: Leandre Gohy (leandre.gohy@hexeo.be)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more detail on the MIT license, see <http://opensource.org/licenses/MIT>

*/

#include <Adafruit_NeoPixel.h>
#include <math.h>

#define ROTL(v, n) (v << n) | (v >> (sizeof(v) * 8 - n))
#define ROTR(v, n) (v >> n) | (v << (sizeof(v) * 8 - n))

#define PIN 9
#define RADIOIN1 10 // 'gaz' command
#define RADIOIN2 11 // animation selector
#define FADING_STEP 5
#define BRIGHTNESS 30 // brightness 0-255  

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(64, PIN);
word statesL = 0x303; // two arm define
word statesR = 0x303; // two arm define
//word statesL = 0xC63; // tree arm define
//word statesR = 0xC63; // tree arm define
//int radiomin; // minimum throtle

void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.setBrightness(BRIGHTNESS);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(RADIOIN1, INPUT);
  pinMode(RADIOIN2, INPUT);
  //radiomin = pulseIn(RADIOIN, HIGH, 25000); // store min throtle
}

word getAnimationSpeed(void) {
  static const int animationSpeeds[] = { 0, 0, 0, 0, 0, 0, 320, 160, 80, 40, 20, 10 }; // speeds of animation
  // Read the radio 'gaz' command
  unsigned long int duration = pulseIn(RADIOIN1, HIGH, 25000);
  // Determine the animation speed based on the previous read
  byte animationSpeedIndex = (byte)(ceil((float)duration / 200.0f));
  // Select the animation speed and limit the index in range 0 - 11
  return animationSpeeds[((animationSpeedIndex <= 11) ? animationSpeedIndex : 11)];
}

unsigned long int getAnimationColor(word animationSpeed) {
  static const unsigned long int animationColors[] = { 0x000000, 0x00FF00, 0xFF0000, 0xFF9900 }; // colors of animation
  static unsigned long int animationColor = 0x000000;
  // read Naza red led
  int sensorValue0 = analogRead(A2);
  // read Naza green led
  int sensorValue1 = analogRead(A3);
  byte animationColorIndex = (sensorValue0 > 500) + ((sensorValue1 > 500) << 1);
  
  if (animationColorIndex != 0 || animationSpeed == 0) {
    animationColor = animationColors[animationColorIndex];
  }
  
  return animationColor;
}

void playAnimation(word animationSpeed, unsigned long int animationColor) {
  static unsigned long int startTime = 0;
  if (millis() - startTime >= animationSpeed) {
    if (animationSpeed == 0) {
      playNoAnimation(animationColor);
    }
    else {
      if (pulseIn(RADIOIN2, HIGH, 25000) < 1500) {
        playAnimationRing(animationColor);
      }
      else {
        playAnimationFading(animationColor);
      }
    }
    startTime = millis();
  }
}

void playNoAnimation(unsigned long int animationColor) {
  for (byte i = 0; i < 16; i++) {
    pixels.setPixelColor(i, animationColor);
    pixels.setPixelColor(i+16, animationColor);
    pixels.setPixelColor(i+32, animationColor);
    pixels.setPixelColor(i+48, animationColor);
    pixels.setBrightness(BRIGHTNESS);
  }
  
  pixels.show();
}

void playAnimationRing(unsigned long int animationColor) {
  unsigned long int stateL = 0;
  unsigned long int stateR = 0;
  
  for (byte i = 0; i < 16; i++) {
    stateL = (statesL >> i) & 0x0001;
    stateR = (statesR >> i) & 0x0001;
    pixels.setPixelColor(i, stateR * animationColor);
    pixels.setPixelColor(i+16, stateL * animationColor);
    pixels.setPixelColor(i+32, stateR * animationColor);
    pixels.setPixelColor(i+48, stateL * animationColor);
  }
  
  statesL = ROTL(statesL, 1);
  statesR = ROTR(statesR, 1);
  
  pixels.show();
}

void playAnimationFading(unsigned long int animationColor) {
  static byte direction = 1; // start direction fading
  static word intensity = 0;

  for (byte i = 0; i < 64; i++) {
    pixels.setPixelColor(i, animationColor);
  }
  
  intensity += direction * FADING_STEP;
  
  if ( intensity >= 255 ) {
    direction = -1;
  }
  else if ( intensity <= 0 ) {
    direction = 1;
  }
  
  pixels.setBrightness(((intensity <= 255) ? intensity : 255));
  pixels.show();
}

void loop() {
  word animationSpeed = getAnimationSpeed();
  unsigned long int animationColor = getAnimationColor(animationSpeed);
  playAnimation(animationSpeed, animationColor);
}
