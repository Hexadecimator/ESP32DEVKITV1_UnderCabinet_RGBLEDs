#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <FastLED.h>

FASTLED_USING_NAMESPACE

#define DATA_PIN    4
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    33
CRGB leds[NUM_LEDS];

#define BRIGHTNESS         170
#define FRAMES_PER_SECOND  120

typedef void (*SimplePatternList[])();

bool pattern_running = true;
bool sequencing = true;

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

static uint8_t hue = 0;

String valor;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void juggle() 
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) 
  {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void rainbowslide1()
{	
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    //Serial.println("in for1");
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  //Serial.print("x");

  // Now go in the other direction.  
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    //Serial.println("in for2");
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}

void clearpatterns()
{
  Serial.println("clearing and turning off");
  
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    // Turn our current led on to white, then show the leds
    leds[whiteLed] = CRGB::White;

    // Show the leds (only one of which is set to white, from above)
    FastLED.show();

    // Wait a little bit
    delay(100);

    // Turn our current led back to black for the next loop around
    leds[whiteLed] = CRGB::Black;
  }

  FastLED.show();
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

// List of patterns to cycle through.  Each is defined as a separate functions.
SimplePatternList gPatterns = { rainbowslide1, rainbowWithGlitter, sinelon, bpm, juggle, confetti };

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        valor = "";
        for (int i = 0; i < value.length(); i++){
          //Serial.print(value[i]); // Presenta value.
          valor = valor + value[i];
        }

        Serial.print("***packet = ");
        Serial.println(valor + "***"); // Presenta valor.

        if      (valor == "<1>") { sequencing = false; pattern_running = true; gCurrentPatternNumber = 0; }
        else if (valor == "<2>") { sequencing = false; pattern_running = true; gCurrentPatternNumber = 1; }
        else if (valor == "<3>") { sequencing = false; pattern_running = true; gCurrentPatternNumber = 2; }
        else if (valor == "<4>") { sequencing = false; pattern_running = true; gCurrentPatternNumber = 3; }
        else if (valor == "<5>") { sequencing = false; pattern_running = true; gCurrentPatternNumber = 4; }
        else if (valor == "<6>") { sequencing = false; pattern_running = true; gCurrentPatternNumber = 5; }
        else if (valor == "<C>") 
        {
          pattern_running = false;
          sequencing = false;
          clearpatterns();
        }
        else if (valor == "<R>")
        {
          if(pattern_running)
          {
            clearpatterns();
          }
          
          pattern_running = true;
          sequencing = true;
        }

        valor = "";
      }
    }
};

void setup() 
{
  delay(2000); // 2 second delay for recovery
  Serial.begin(9600);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  BLEDevice::init("CABINET01");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  if (sequencing)
  {
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
  }
}

void loop() 
{
  // put your main code here, to run repeatedly:
  if(pattern_running)
  {
    gPatterns[gCurrentPatternNumber]();
    FastLED.show();
  }
  
  FastLED.delay(1000/240); // don't let it refresh too fast
  EVERY_N_MILLISECONDS(10) { gHue++; }
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}