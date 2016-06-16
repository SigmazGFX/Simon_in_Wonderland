/* 3 panel cpacitive touch "Simon Says" Door game
    2016 -Sigmaz */

#include "FastLED.h"
FASTLED_USING_NAMESPACE
#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LOCK_PIN    13 // Pin to control the lock
#define DATA_PIN0   3  // Top Window
#define DATA_PIN1   4  // Middle Window
#define DATA_PIN2   5  // Bottom Window
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
const uint8_t kMatrixWidth = 5;
const uint8_t kMatrixHeight = 5;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
CRGB window0[NUM_LEDS];
CRGB window1[NUM_LEDS];
CRGB window2[NUM_LEDS];
#define BRIGHTNESS  64

#define Num_of_Panes 3 // number of buttons and/or LEDs Used
#define LockPin 13
#define SequenceLength 5 // max lenght of the pattern
#define NumRounds  5 // How many winning rounds until unlock
#define TonePin 12 // pin for the Piezo 

const byte LEDgroup[Num_of_Panes] = {1, 2, 3}; // flags for the 3 glass panes

const byte ButtonPin[Num_of_Panes] = {6, 7, 8}; //Pins that the buttons are on

const int Note[Num_of_Panes] = {165, 330, 659}; // tone per LED

byte ledList[SequenceLength]; // starting pattern
byte buttonState[Num_of_Panes] = {0, 0, 0}; // button state holder
byte lastButtonState[Num_of_Panes] = {0, 0, 0}; // store old button state for compare

byte Pat_count = 0; // number of LEDs in starting pattern
int Btemp = 0; // temporary button pin holder
unsigned int Speed = 300; // pattern display speed
byte count = 0; // cycle through button pins

unsigned long lastDebounceTime[Num_of_Panes]; // record last time button was pressed
unsigned long time; // record initial time for pattern blinks


int gameWon = 0; //did they win the game
int playGame = 0; // game start switch
bool PatternStatus = false; // toggle whether player has won or not

//=MATRIX DEFINES==========================================================================================
// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;
uint16_t speed = 20; // speed is set dynamically once we've started up
uint16_t scale = 30; // scale is set dynamically once we've started up
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];
CRGBPalette16 currentPalette( CRGB::Black );

CRGBPalette16 targetPalette( CRGB::Black );
uint8_t       colorLoop = 1;


// Params for width and height


// Pixel layout
//
//      0  1  2  3  4
//   +-----------------
// 0 |  0  .  1  .  2
// 1 |  .  4  .  3  .
// 2 |  5 .   6  .  7
// 3 |  .  9  .  8  .
// 4 | 10  . 11  . 12

// This function will return the right 'led index number' for
// a given set of X and Y coordinates on your RGB Shades.
// This code, plus the supporting 80-byte table is much smaller
// and much faster than trying to calculate the pixel ID with code.
#define LAST_VISIBLE_LED 12
uint8_t XY( uint8_t x, uint8_t y)
{
  // any out of bounds address maps to the first hidden pixel
  if ( (x >= kMatrixWidth) || (y >= kMatrixHeight) ) {
    return (LAST_VISIBLE_LED + 1);
  }

  const uint8_t ArrayTable[] = {
    0, 25, 1, 24, 2,
    21, 4, 22, 3, 23,
    5, 20, 6, 19, 7,
    16, 9, 17, 8, 18,
    10, 15, 11, 14, 12
  };

  uint8_t i = (y * kMatrixWidth) + x;
  uint8_t j = ArrayTable[i];
  return j;
}



void setup()
{
  delay(3000); // 3 second delay for recovery
  x = random16();
  y = random16();
  z = random16();

  //  Tell FastLED about the LED configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN0, COLOR_ORDER>(window0, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN1, COLOR_ORDER>(window1, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(window2, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  //end LED setup------------------------------

  pinMode(LockPin, OUTPUT);
  digitalWrite(LockPin, HIGH);
  randomSeed(analogRead(A4) / analogRead(A5)); // set a seed value upon start up
  Serial.begin(115200);
  for (int cnt = 0; cnt < Num_of_Panes; cnt++) // initialize LED pins and buttons
  {

    pinMode(ButtonPin[cnt], INPUT_PULLUP);

    ledList[cnt] = random(0, 3); // Random Number from 0 - 2.. could also use random(3)
    Pat_count++;
  }
}


uint8_t gHue = 0; // rotating "base color" used by many of the patterns



void loop()
{

  startGame();

  if (gameWon == 1) {
    digitalWrite(LockPin, LOW);
    rainbowWithGlitter_2(10, 80);
    FastLED.setBrightness(255);
    FastLED.show();
    FastLED.delay(1000 / 120);
  }

  if (playGame == 0) {
    FastLED.setBrightness(64);
    // generate noise data
    fillnoise8();
    // convert the noise data to colors in the LED array
    // using the current palette
    mapNoiseToLEDsUsingPalette();
  }

  if (playGame == 1) {
    showPattern(); // display the LED pattern
    VerifyButtons(); // wait for button presses and check to see button presses match LEDs that were lit
  }
}

void showPattern()
{
  for (byte pattern = 0; pattern < Pat_count; pattern++) // cycle through LED pattern and blink them
  {
    time = millis();
    while (millis() - time < Speed)
    {
      //digitalWrite(ledList[ pattern ], HIGH);

      //FASTLED implementation?
      if (ledList[pattern] == 0) { // Im not sure if I use (ledList[pattern]) or just pattern to select the window..

        FastLED.setBrightness(255);
        fill_solid(window0, NUM_LEDS, CRGB::Yellow);
        FastLED.show();

      }
      if (ledList[pattern] == 1) {  // Im not sure if I use (ledList[pattern]) or just pattern to select the window..

        FastLED.setBrightness(255);
        fill_solid(window1, NUM_LEDS, CRGB::Green);
        FastLED.show();

      }
      if (ledList[pattern] == 2) {  // Im not sure if I use (ledList[pattern]) or just pattern to select the window..

        FastLED.setBrightness(255);
        fill_solid(window2, NUM_LEDS, CRGB::Blue);
        FastLED.show();

      }

      tone(TonePin, Note[ledList[pattern] - 6]);
    }

    time = millis();
    while (millis() - time < Speed)
    {
      noTone(TonePin);


      FastLED.setBrightness(255);
      fill_solid(window0, NUM_LEDS, CRGB::Black);
      fill_solid(window1, NUM_LEDS, CRGB::Black);
      fill_solid(window2, NUM_LEDS, CRGB::Black);
      FastLED.show();

    }

  }
}


void VerifyButtons()
{
  static byte i;
  i = 0;

  while (i < Pat_count)
  {
    Btemp = Getbutton(); // get a value from Getbutton function
    if (Btemp != -1) // filter out no button press "0"
    {
      tone(TonePin, Note[Btemp - 2]);

      //Serial.println(Btemp); // debug purpose
      if (ledList[i] == (Btemp + Num_of_Panes)) // compare button pressed to LED lit
      {
        i++; // each correct button press, increments ledList to check the next LED
        PatternStatus = true;
      }
      else // If at any time a button was incorrect, set PatternStatus to false to show losing message then get out of while loop
      {
        PatternStatus = false;
        FastLED.setBrightness(255);
        fill_solid(window0, NUM_LEDS, CRGB::Red);
        fill_solid(window1, NUM_LEDS, CRGB::Red);
        fill_solid(window2, NUM_LEDS, CRGB::Red);
        FastLED.show();
        tone(TonePin, 90);
        delay(500);
        tone(TonePin, 90);
        delay(500);
        int playGame = 0;
        noTone(TonePin);
        break;
      }
    }
    else
    {
      noTone(TonePin);
      //Serial.println(F("Times Up"));
      FastLED.setBrightness(255);
      fill_solid(window0, NUM_LEDS, CRGB::Red);
      fill_solid(window1, NUM_LEDS, CRGB::Red);
      fill_solid(window2, NUM_LEDS, CRGB::Red);
      FastLED.show();
      tone(TonePin, 90);
      delay(500);
      tone(TonePin, 90);
      delay(500);
      int playGame = 0;
      noTone(TonePin);
      PatternStatus = false;
      break;
    }
  }
  if (PatternStatus == true) // button pattern matched all shown LEDs
  {
    Speed -= 9; // increase pattern speed, by lowering Speed value
    Serial.println(F("Pattern was Good"));
    updatePattern();
  }
  else Serial.println(F("Player Lost"));

}



void updatePattern() // if player has entered buttons correctly add a new LED to ledList
{
  if (Pat_count > NumRounds)
    int gameWon = 1 ; //show gamewon rainbow animation and unlock the door.
  else
    Pat_count++;
  ledList[Pat_count - 1] = random(5, 7); // Im not sure what this value is used for or why they set Pat_count - 1 to 6?
}

int Getbutton()
{
  static unsigned long timeOut = 2000, countDown;
  countDown = millis();

  while (millis() - countDown <= timeOut)
  {
    for (count = 0; count < Num_of_Panes; count++) // loop through all the buttons
    {
      buttonState[count] = digitalRead(ButtonPin[count]); // read button states

      if (buttonState[count] != lastButtonState[count]) // check to see if the state has changed from last press

      {
        if (buttonState[count] == LOW)
        {
          lastDebounceTime[count] = millis();// record the time of the last press
          lastButtonState[count] = buttonState[count]; // update old button state for next checking
        }
      }
      if ((millis() - lastDebounceTime[count]) > 50UL)
      {
        if (buttonState[count] == LOW)
        {
          return ButtonPin[count];
        }
      }
    }
  }
  return -1; // return button pin pressed
}


void rainbowWithGlitter_2( uint8_t stripeDensity, uint8_t chanceOfGlitter)
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  fill_rainbow( window0, NUM_LEDS, gHue, stripeDensity);
  fill_rainbow( window1, NUM_LEDS, gHue, stripeDensity);
  fill_rainbow( window2, NUM_LEDS, gHue, stripeDensity);
  addGlitter(chanceOfGlitter);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    window0[ random16(NUM_LEDS) ] += CRGB::White;
    window1[ random16(NUM_LEDS) ] += CRGB::White;
    window2[ random16(NUM_LEDS) ] += CRGB::White;
  }
}


// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if ( speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }

  for (int i = 0; i < MAX_DIMENSION; i++) {
    int ioffset = scale * i;
    for (int j = 0; j < MAX_DIMENSION; j++) {
      int joffset = scale * j;

      uint8_t data = inoise8(x + ioffset, y + joffset, z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if ( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }

  z += speed;

  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue = 0;

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri =   noise[i][j];

      // if this palette is a 'loop', add a slowly-changing base value
      if ( colorLoop) {
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if ( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }
      currentPalette = CloudColors_p;
      CRGB color = ColorFromPalette( currentPalette, index, bri);
      window0[XY(i, j)] = color;
      window1[XY(i, j)] = color;
      window2[XY(i, j)] = color;
    }
  }

  ihue += 1;
}



void startGame()
{
  for (count = 0; count < Num_of_Panes; count++) // loop through all the buttons

  {
    buttonState[count] = digitalRead(ButtonPin[count]); // read button states

    if (buttonState[count] != lastButtonState[count]) // check to see if the state has changed from last press

    {
      if (buttonState[count] == LOW)
      {
        lastDebounceTime[count] = millis();// record the time of the last press
        lastButtonState[count] = buttonState[count]; // update old button state for next checking
      }
    }

    if ((millis() - lastDebounceTime[count]) > 50UL)
    {
      if (buttonState[count] == LOW)
      {
        int playGame = 1;
      }
    }

  }
}

//
//void confetti_2( uint8_t colorVariation, uint8_t fadeAmount)
//{
//  // random colored speckles that blink in and fade smoothly
//  fadeToBlackBy( window0, NUM_LEDS, fadeAmount);
//  fadeToBlackBy( window1, NUM_LEDS, fadeAmount);
//  fadeToBlackBy( window2, NUM_LEDS, fadeAmount);
//  int pos = random16(NUM_LEDS);
//  window0[pos] += CHSV( gHue + random8(colorVariation), 200, 255);
//  window1[pos] += CHSV( gHue + random8(colorVariation), 200, 255);
//  window2[pos] += CHSV( gHue + random8(colorVariation), 200, 255);
//}
