/* Simon puzzle door 
 *  The door has 3 window panels top center and bottom. each panel is approx 20x18 behind each panel are 13 WS2812B LED's 
 *  as well as aluminum window screening. the screen is used as a capacitive sensor antennae for each panel.
 *  when the door is idle, noise animations are displayed on the diffused glass panes 
 *  in a matrix when touch is detected the module switches to Simon mode and the user plays. 
 *  Once the final round is reached the MCU takes pin 13 low and congratulates the player with 
 *  a little tune and glitter rainbows..
 *  2016 Jon Bruno(SigmazGFX) for Klues Escape Room, Stroudsburg PA
 */
   
   
 
#include "Debounce.h"
#include "FastLED.h"
FASTLED_USING_NAMESPACE
#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LOCK_PIN    13 // Pin to control the lock

#define DATA_PIN   6
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
const uint8_t kMatrixWidth = 5;
const uint8_t kMatrixHeight = 15;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)

CRGB leds[NUM_LEDS];
#define BRIGHTNESS  64
#define LockPin 13

const int red_button = 2;      // Input pins for the buttons
const int blue_button = 3;
const int green_button = 4;

// Instantiate a Debounce object with a 20 millisecond debounce time
Debounce debounceR = Debounce( 20 , red_button );
Debounce debounceB = Debounce( 20 , blue_button );
Debounce debounceG = Debounce( 20 , green_button );

const int buzzer = 5;     // Output pin for the buzzer
long sequence[20];             // Array to hold sequence
int count = 0;                 // Sequence counter
long input = 5;                // Button indicator
int wait = 500;                // Variable delay as sequence gets longer

int gameWon = 0; //did they win the game
int playGame = 0; // game start switch
int maxRounds = 10; // number of game rounds (also the max number of lights in a pattern)

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
uint8_t colorLoop = 1;
uint8_t gHue = 0;


// Params for width and height

int topstart = 0;
int toplength = 13;
int centerstart = 13;
int centerlength = 13;
int bottomstart = 26;
int bottomlength = 13;

// This function will return the right 'led index number' for
// a given set of X and Y coordinates on your RGB Shades.
// This code, plus the supporting 80-byte table is much smaller
// and much faster than trying to calculate the pixel ID with code.
//5x15 array (75-LEDS) LAST_VISIBLE_LED 38
const uint8_t ArrayTable[] = {
  //--Top
  0,  74,  1, 73,  2,
  70,  4, 71,  3, 72,
  5,  69,  6, 68,  7,
  65,  9, 66,  8, 67,
  10, 64, 11, 63, 12,
  //--Center
  15, 61, 14, 62, 13,
  60, 16, 59, 17, 58,
  20, 56, 19, 57, 18,
  55, 21, 54, 22, 53,
  25, 51, 24, 52, 23,
  //--Bottom
  26, 50, 27, 49, 28,
  46, 30, 47, 29, 48,
  31, 45, 32, 44, 33,
  41, 35, 42, 34, 43,
  36, 40, 37, 39, 38
};

#define LAST_VISIBLE_LED 38
uint8_t XY( uint8_t x, uint8_t y)
{
  // any out of bounds address maps to the first hidden pixel
  if ( (x >= kMatrixWidth) || (y >= kMatrixHeight) ) {
    return (LAST_VISIBLE_LED + 1);
  }

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
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  //end LED setup------------------------------


  pinMode(red_button, INPUT);    // configure buttons on inputs
  digitalWrite(red_button, HIGH);// turn on pullup resistors
  pinMode(blue_button, INPUT);
  digitalWrite(blue_button, HIGH);
  pinMode(green_button, INPUT);
  digitalWrite(green_button, HIGH);
  pinMode(buzzer, OUTPUT);
  pinMode(LockPin, OUTPUT);
  digitalWrite(LockPin, HIGH);
  

  //-------------game stuff------------
  randomSeed(analogRead(5));     // random seed for sequence generation

  //-----------------------------------
}


void loop()
{

  debounceR.update();
  debounceB.update();
  debounceG.update();
  startButton();

  if (gameWon == 1) {
    gHue++;
    digitalWrite(LockPin, LOW);
    rainbowWithGlitter_2(10, 80);
    FastLED.setBrightness(255);
    FastLED.show();
    FastLED.delay(1000 / 120);
  }

  if (playGame == 0) {
    FastLED.setBrightness(128);
    // generate noise data
    fillnoise8();
    // convert the noise data to colors in the LED array
    // using the current palette
    mapNoiseToLEDsUsingPalette();
    FastLED.show();
    FastLED.delay(1000 / 30);
  }

  if (playGame == 1) {
    playSequence();  // play the sequence
    readSequence();  // read the sequence
    delay(1000);     // wait a sec
  }



}


void rainbowWithGlitter_2( uint8_t stripeDensity, uint8_t chanceOfGlitter)
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  fill_rainbow( leds, NUM_LEDS, gHue, stripeDensity);
  addGlitter(chanceOfGlitter);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
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
      leds[XY(i, j)] = color;
    }
  }

  ihue += 1;
}



//-----------------------------------GAME SECTION---------------------------------

/*
  playtone function taken from Oomlout sample
  takes a tone variable that is half the period of desired frequency
  and a duration in milliseconds
*/
void playtone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(buzzer, HIGH);
    delayMicroseconds(tone);
    digitalWrite(buzzer, LOW);
    delayMicroseconds(tone);
  }
}


/*
  functions to flash LEDs and play corresponding tones
  very simple - turn LED on, play tone for .5s, turn LED off
*/
void flash_red() {
  FastLED.setBrightness(255);
  fill_solid(leds, (topstart, toplength), CRGB::Red);
  FastLED.show();
  playtone(2273, wait);            // low A
  FastLED.setBrightness(255);
  fill_solid(leds, (topstart, toplength), CRGB::Black);
  FastLED.show();
}

void flash_blue() {
  FastLED.setBrightness(255);
  fill_solid(leds, (centerstart, centerlength), CRGB::Blue);
  FastLED.show();
  playtone(1700, wait);            // D
  FastLED.setBrightness(255);
  fill_solid(leds, (centerstart, centerlength), CRGB::Black);
  FastLED.show();
}

void flash_green() {
  FastLED.setBrightness(255);
  fill_solid(leds, (bottomstart, bottomlength), CRGB::Green);
  FastLED.show();
  playtone(1275, wait);            // G
  FastLED.setBrightness(255);
  fill_solid(leds, (bottomstart, bottomlength), CRGB::Black);
  FastLED.show();
}


// a simple test function to flash all of the LEDs in turn
void runtest() {
  flash_red();
  flash_blue();
  flash_green();
}

/* a function to flash the LED corresponding to what is held
  in the sequence
*/
void squark(long led) {
  switch (led) {
    case 0:
      flash_red();
      break;
    case 1:
      flash_blue();
      break;
    case 2:
      flash_green();
      break;
  }
  delay(50);
}

// function to congratulate winning sequence
void congratulate() {
  playtone(1014, 250);               // play a jingle
  delay(25);
  playtone(1014, 250);
  delay(25);
  playtone(1014, 250);
  delay(25);
  playtone(956, 500);
  delay(25);
  playtone(1014, 250);
  delay(25);
  playtone(956, 500);
  delay(2000);
  resetCount();         // reset sequence
  gameWon = 1;

}

// function to reset after winning or losing
void resetCount() {
  count = 0;
  wait = 500;
}

// function to build and play the sequence
void playSequence() {
  FastLED.setBrightness(255);      //Clear LEDS so there isnt any left over color info from the idle animation.
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  sequence[count] = random(3);       // add a new value to sequence
  for (int i = 0; i < count; i++) {  // loop for sequence length
    squark(sequence[i]);             // flash/beep
  }
  wait = 500 - (count * 15);         // vary delay
  count++;                           // increment sequence length
}

// function to read sequence from player
void readSequence() {
  for (int i = 1; i < count; i++) {             // loop for sequence length
    while (input == 5) {                       // wait until button pressed
      if ((debounceR.read()) == true) {  // Red button
        input = 0;
      }
      if ((debounceB.read()) == true) {  // Blue button
        input = 1;
      }
      if ((debounceG.read()) == true) { // Green button
        input = 2;
      }

    }
    if (sequence[i - 1] == input) {            // was it the right button?
      squark(input);                           // flash/buzz
      if (i == maxRounds) {                    // check if the player has gotten past the final round
        congratulate();                        // congratulate the winner
      }
    }
    else {
      playtone(4545, 1000);                  // low tone for fail
      squark(sequence[i - 1]);               // double flash for the right colour
      squark(sequence[i - 1]);
      resetCount();                          // reset sequence
      playGame = 0;
    }
    input = 5;                                   // reset input
  }
}


void startButton()
{
  if ((debounceG.read()) == true) {
    playGame = 1;
  }
  if ((debounceR.read()) == true) {
    playGame = 1;
  }
  if ((debounceB.read()) == true) {
    playGame = 1;
  }
}

//
void confetti_2( uint8_t colorVariation, uint8_t fadeAmount)
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, fadeAmount);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(colorVariation), 200, 255);
}
