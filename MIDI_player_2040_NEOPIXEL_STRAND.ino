#include <MD_MIDIFile.h>
#include <SdFat.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoPXL8.h>

#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiA);

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire1);

GFXcanvas1 canvas(128, 64);

#define debugME  0  // set to '1' to print Serial debugs
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_VOLUME 0x07

#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
#define SWITCH_PIN 26

#define NOTE_ON 0x90
#define NOTE_OFF 0x80

static volatile bool led_state[8];
static volatile int volume;

// #define LED_PIN 27
// #define LED_COUNT 8
// Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
int8_t pins[8] = { 16, 17, 18, 19, 20, 21, 22, 23 };
#define NUM_LED 50 // Longest strand length
#define MAX_PIXELS 150 // Use for final
#define MULTIPLES 19 // MAX_PIXELS / 8 (rounded up)
Adafruit_NeoPXL8 strip(NUM_LED, pins, NEO_GRB);
// Array size = Max pixels / 8 (will determine if round up)
int C_lights[19];
int D_lights[19];
int E_lights[19];
int F_lights[19];
int G_lights[19];
int A_lights[19];
int B_lights[19];
int X_lights[19];

static volatile bool C_state = 0;
static volatile bool D_state = 0;
static volatile bool E_state = 0;
static volatile bool F_state = 0;
static volatile bool G_state = 0;
static volatile bool A_state = 0;
static volatile bool B_state = 0;
static volatile bool X_state = 0;

static bool mode;

const int C_KEYS[20] = {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 1, 13, 25, 37, 49, 61, 73, 85, 97, 109};
const int D_KEYS[19] = {2, 14, 26, 38, 50, 62, 74, 86, 98, 110, 3, 15, 27, 39, 51, 63, 75, 87, 99};
const int E_KEYS[9] = {4, 16, 28, 40, 52, 64, 76, 88, 100};
const int F_KEYS[18] = {5, 17, 29, 41, 53, 65, 77, 89, 101, 6, 18, 30, 42, 54, 66, 78, 90, 102};
const int G_KEYS[18] = {7, 19, 31, 43, 55, 67, 79, 91, 103, 8, 20, 32, 44, 56, 68, 80, 92, 104};
const int A_KEYS[9] = {9, 21, 33, 45, 57, 69, 81, 93, 105};
const int B_KEYS[18] = {11, 23, 35, 47, 59, 71, 83, 95, 107, 10, 22, 34, 46, 58, 70, 82, 94, 106};
const int BLACK_KEYS[46] = 
{
  1, 3, 6, 8, 10, 13, 15, 18, 20, 22, 25, 27, 30, 32, 34, 37, 39, 42, 44, 46, 49, 51, 54, 56,
  58, 61, 63, 66, 68, 70, 73, 75, 78, 80, 82, 85, 87, 90, 92, 94, 97, 99, 102, 104, 106, 109
};

// Default colors
static int C_COLOR = strip.Color(60, 0, 0);
static int D_COLOR = strip.Color(60, 60, 0);
static int E_COLOR = strip.Color(40, 5, 60);
static int F_COLOR = strip.Color(0, 0, 60);
static int G_COLOR = strip.Color(0, 60, 60);
static int A_COLOR = strip.Color(60, 0, 60);
static int B_COLOR = strip.Color(0, 60, 0);
static int X_COLOR = strip.Color(20, 20, 20);

// Define to set random colors
// #define RANDOM

// set define SINGLE to catch all channels
// otherwise isolate one channel (usually an instrument) with MIDI_CHANNEL
#define SINGLE
const int MIDI_CHANNEL = 0;

const uint16_t WAIT_DELAY = 2000; // ms
// Music wing is 7; Adalogger is 10
const uint8_t SD_SELECT = 10;
// Pin numbers in templates must be constants.
const uint8_t SOFT_MISO_PIN = 8;
const uint8_t SOFT_MOSI_PIN = 15;
const uint8_t SOFT_SCK_PIN  = 14;

// #if SPI_DRIVER_SELECT == 2  // Must be set in SdFat/SdFatConfig.h

SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

#if ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_SELECT, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi)
#else  // ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_SELECT, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)
#endif  // ENABLE_DEDICATED_SPI

const char *tuneList[] = 
{  
  "MINUET.MID",
  "BOTH_SIDES.mid",
  "TWINKLE.MID",
  "ELISE.MID",
  "MOZART.MID",
  "FIRERAIN.MID",
  "FUGUEGM.MID",
  "PROWLER.MID",
  "rainsnip.mid",
  "FERNANDO.MID"
};

MD_MIDIFile SMF;
SdFat	SD;

int get_note_white(int note) {
  
  for (int i = 0; i < 20; i++) {
    if (note == C_KEYS[i]) return 0;
  }

  for (int i = 0; i < 19; i++) {
    if (note == D_KEYS[i]) return 1;
  }

  for (int i = 0; i < 9; i++) {
    if (note == E_KEYS[i]) return 2;
  }

  for (int i = 0; i < 18; i++) {
    if (note == F_KEYS[i]) return 3;
  }

  for (int i = 0; i < 18; i++) {
    if (note == G_KEYS[i]) return 4;
  }

  for (int i = 0; i < 9; i++) {
    if (note == A_KEYS[i]) return 5;
  }

  for (int i = 0; i < 18; i++) {
    if (note == B_KEYS[i]) return 6;
  }

  return 10;
}

bool get_note_black(int note) {
  for (int i = 0; i < 46; i++) {
    if (note == BLACK_KEYS[i]) return true;
  }
  
  return false;
}

void set_lights(int message, int channel, int note, int velocity) {
   #ifdef SINGLE
    if(message == NOTE_ON) {
  #else
    if(channel == MIDI_CHANNEL && message == NOTE_ON) {
  #endif
    if (debugME) {
      Serial.println(message);
      Serial.print(note);
      Serial.print(" | ");
      Serial.print(get_note_white(note));
      Serial.print(" | ");
      Serial.print(get_note_black(note));
      Serial.print(" | ");    
      Serial.println(velocity);
      if (velocity == 0) {
        Serial.println("OFF");
      }  
    }
    // NOTE_ON at 0 velocity is the same as NOTE_OFF
    // But both need to be handled to cover all cases
    if (velocity == 0) {
      led_state[get_note_white(note)] = 0;
      if (get_note_black(note)) led_state[7] = 0;
    }
    else {
      led_state[get_note_white(note)] = 1;
      if (get_note_black(note)) led_state[7] = 1;
      // volume = velocity;     
    }
  }
  #ifdef SINGLE
    if(message == NOTE_OFF) {
  #else
    if(channel == MIDI_CHANNEL && message == NOTE_OFF) {
  #endif
      led_state[get_note_white(note)] = 0;
      if (get_note_black(note)) led_state[7] = 0;
  }
}

void set_strip() {

  if (C_state != led_state[0]) {
    if (led_state[0]) {
     for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(C_lights[i], 60, 60, 20);
        strip.setPixelColor(C_lights[i], C_COLOR);        
        C_state = 1;
       }      
      }  
    else {
     for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(C_lights[i], 0);
        C_state = 0;
      }  
    }
  }

  if (D_state != led_state[1]) {
    if (led_state[1]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(D_lights[i], 20, 60, 5);
        strip.setPixelColor(D_lights[i], D_COLOR);
        D_state = 1;
      }
     }
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(D_lights[i], 0);
        D_state = 0;
      }
    }
  }

  if (E_state != led_state[2]) {
    if (led_state[2]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(E_lights[i], 40, 5, 60);
        strip.setPixelColor(E_lights[i], E_COLOR);
        E_state = 1;
      }
    } 
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(E_lights[i], 0);
        E_state = 0;
      } 
    }
  }
  
  if (F_state != led_state[3]) {
    if (led_state[3]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(F_lights[i], 60, 10, 5);
        strip.setPixelColor(F_lights[i], F_COLOR);
        F_state = 1;
      }
    } 
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(F_lights[i], 0);
        F_state = 0;
      }
    }
  }

  if (G_state != led_state[4]) {
    if (led_state[4]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(G_lights[i], 10, 40, 60);
        strip.setPixelColor(G_lights[i], G_COLOR);
        G_state = 1;
      }
    } 
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(G_lights[i], 0);
        G_state = 0;
      }
    }
  }
  
  if (A_state != led_state[5]) {
    if (led_state[5]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(A_lights[i], 60, 20, 10);
        strip.setPixelColor(A_lights[i], A_COLOR);
        A_state = 1;
      }
    } 
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(A_lights[i], 0);
        A_state = 0;
      }
    }
  }

  if (B_state != led_state[6]) {
    if (led_state[6]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(B_lights[i], 5, 60, 0);
        strip.setPixelColor(B_lights[i], B_COLOR);
        B_state = 1;
      }
    } 
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(B_lights[i], 0);
        B_state = 0;
      }
    }
  }

  if (X_state != led_state[7]) {
    if (led_state[7]) {
      for (int i = 0; i < MULTIPLES; i++) {
        // strip.setPixelColor(X_lights[i], 20, 20, 20);
        strip.setPixelColor(X_lights[i], X_COLOR);
        X_state = 1;
      }
    } 
    else {
      for (int i = 0; i < MULTIPLES; i++) {
        strip.setPixelColor(X_lights[i], 0);
        X_state = 0;
      }
    }
  }
}

// midiA callback functions
void handleNoteOn(byte channel, byte note, byte velocity)
{
  // display.println("NOTE_ON");
  // display.print(channel);
  // display.print(" | ");
  display.print(note);
  display.print(" | ");
  display.println(velocity);
  display.display();
  if (velocity == 0) {
      led_state[get_note_white(note)] = 0;
      if (get_note_black(note)) led_state[7] = 0;
    }
    else {
      led_state[get_note_white(note)] = 1;
      if (get_note_black(note)) led_state[7] = 1;
    }
}

void handleNoteOff(byte channel, byte note, byte velocity)
{
    // Do something when the note is released.
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
  led_state[get_note_white(note)] = 0;
  if (get_note_black(note)) led_state[7] = 0;
}

void midiCallback(midi_event *pev)
{
  uint8_t * foo;

  set_lights(pev->data[0], pev->channel, pev->data[1], pev->data[2]);
  
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
  {
    if (debugME)
    {
      Serial.println(pev->data[0] | pev->channel);
      Serial.println(pev->data[0]); 
      Serial.println(pev->channel);
      Serial.println(pev->data[1]);
      Serial.println(pev->size-1);
      Serial.write(&pev->data[1], pev->size-1);
    }
    foo = &pev->data[1];
    Serial1.write(pev->data[0] | pev->channel);
    Serial1.write(&pev->data[1], pev->size-1);
  }
  else
  {
    if (debugME)
    {
      Serial.write(pev->data, pev->size);
    }
    Serial1.write(pev->data, pev->size);
  }
}

void sysexCallback(sysex_event *pev)
{
  if (debugME)
  {
    Serial.println("\nS T");
    Serial.println(pev->track);
    Serial.println(": Data ");
    for (uint8_t i=0; i<pev->size; i++)
    {
      Serial.println(pev->data[i]);
      Serial.println(' ');
    }
  }
}

void midiSilence(void)
// Turn everything off on every channel.
{
  midi_event ev;

  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;
  for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);

  // Turn off lights 
  strip.clear();
  for (int i = 0; i < 8; i++) {
    led_state[i] = 0;     
  }
  C_state = 0;
  D_state = 0;
  E_state = 0;
  F_state = 0;
  G_state = 0;
  A_state = 0;
  B_state = 0;
  X_state = 0;  
}

void shuffleArray(int * array, int size)
{
  int last = 0;
  int temp = array[last];
  for (int i=0; i<size; i++)
  {
    int index = random(size);
    array[last] = array[index];
    last = index;
  }
  array[last] = temp;
}

void random_strip() {
  int temp[8];
  int MAX = MAX_PIXELS;

  int current = 0;
  int index = 0; 
  int OFFSET = 8;
  
  while (current < MAX_PIXELS - 1) {
    for (int i = 0; i < 8; i++) {
      temp[i] = current + i;
      // Serial.println(temp[i]);
    }

    shuffleArray(temp, 8);
    
    C_lights[index] = temp[0];
    D_lights[index] = temp[1];
    E_lights[index] = temp[2];
    F_lights[index] = temp[3];
    G_lights[index] = temp[4];
    A_lights[index] = temp[5];
    B_lights[index] = temp[6];
    X_lights[index] = temp[7];

    index++;
    current += OFFSET;    
  }
}

void random_color() {
  C_COLOR = strip.Color(random(60), random(60), random(60));
  D_COLOR = strip.Color(random(60), random(60), random(60));
  E_COLOR = strip.Color(random(60), random(60), random(60));
  F_COLOR = strip.Color(random(60), random(60), random(60));
  G_COLOR = strip.Color(random(60), random(60), random(60));
  A_COLOR = strip.Color(random(60), random(60), random(60));
  B_COLOR = strip.Color(random(60), random(60), random(60));
  X_COLOR = strip.Color(random(60), random(60), random(60));
}

void setup()
{
  Serial.begin(57600);
  SPI1.begin();
  Serial.println("CHECK SD CARD");
	if (debugME)
	{
    if (!SD.begin(SD_CONFIG))
    {
      Serial.println("\nSD init fail!");
      while (true) ;
    }
	}
	else
	{
  	!SD.begin(SD_CONFIG);
	}

  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // True for input mode. False for playback mode
  if (!digitalRead(SWITCH_PIN)) mode = true;

    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);

    // Serial.println("128x64 OLED FeatherWing test");
    delay(250); // wait for the OLED to power up
    display.begin(0x3C, true); // Address 0x3C default

    // Serial.println("OLED begun");

    display.display();
    delay(1000);

    // Clear the buffer.
    display.clearDisplay();
    display.display();

    display.setRotation(1);

    // text display tests
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE, 0);
    display.setCursor(0,0);

  if (mode) {
    display.println("INPUT MODE");
    display.display();
  
    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    midiA.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Do the same for NoteOffs
    midiA.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    #ifdef SINGLE
      midiA.begin(MIDI_CHANNEL_OMNI);
    #else
      midiA.begin(MIDI_CHANNEL);
    #endif      

    // Testing
    if (debugME) {
      Serial.print("C: ");
      for (int i = 0; i < 19; i++) {
      if (i == 18) {
        Serial.println(C_lights[i]);
      }
      else {
        Serial.print(C_lights[i]);
        Serial.print(", ");        
        }
      }
      Serial.print("D: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(D_lights[i]);
        }
        else {
          Serial.print(D_lights[i]);
          Serial.print(", ");        
        }
      }
      Serial.print("E: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(E_lights[i]);
        }
        else {
          Serial.print(E_lights[i]);
          Serial.print(", ");        
        }
      }
      Serial.print("F: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(F_lights[i]);
        }
        else {
          Serial.print(F_lights[i]);
          Serial.print(", ");        
        }
      }
      Serial.print("G: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(G_lights[i]);
        }
        else {
          Serial.print(G_lights[i]);
          Serial.print(", ");        
        }
      }
      Serial.print("A: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(A_lights[i]);
        }
        else {
          Serial.print(A_lights[i]);
          Serial.print(", ");        
        }
      }
      Serial.print("B: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(B_lights[i]);
        }
        else {
          Serial.print(B_lights[i]);
          Serial.print(", ");        
        }
      }
      Serial.print("X: ");
      for (int i = 0; i < 19; i++) {
        if (i == 18) {
          Serial.println(X_lights[i]);
        }
        else {
          Serial.print(X_lights[i]);
          Serial.print(", ");        
        }
      } 
    }
  }
  else {
    Serial1.begin(31250);
    Serial1.write(MIDI_CHAN_MSG | 0);
    Serial1.write(MIDI_CHAN_VOLUME);
    Serial1.write(127);

    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);
  }
}

void loop()
{
  if (mode) {
    midiA.read();    
  }
  else {
    static enum { S_IDLE, S_PLAYING, S_END, S_WAIT_BETWEEN } state = S_IDLE;
    static uint16_t currTune = ARRAY_SIZE(tuneList);
    static uint32_t timeStart;

    static bool A_pressed;
    static bool B_pressed;
    static bool set_music = false;
    
    // PLAYER LOOP

    switch (state)
    {
    case S_IDLE:    // now idle, set up the next tune
      {
        int err;
        
        if (!set_music) {
          currTune++;
          if (currTune >= ARRAY_SIZE(tuneList))
            currTune = 0;     
        }

        
        err = SMF.load(tuneList[currTune]);
        if (err == MD_MIDIFile::E_OK)
        {
          display.print("CURRENT: ");
          display.print("(");        
          display.print(currTune + 1);
          display.print("/");  
          display.print(ARRAY_SIZE(tuneList));
          display.println(")");  
          display.setCursor(display.getCursorX(), display.getCursorY() + 5);
          display.println(SMF.getFilename());
          display.println("------------");
          display.display();
          set_music = false;
          state = S_PLAYING;
        }
      }
      break;

    case S_PLAYING: // play the file 
      if (!SMF.isEOF())
      {
        if (SMF.getNextEvent())
        {
          if (debugME)
          {
            Serial.println("playing");
          }
        }
      }
      else
        state = S_END;
      break;

    case S_END:   // done with this one
      SMF.close();
      midiSilence();
      display.clearDisplay();
      display.setCursor(0,0);
      #ifdef RANDOM
       random_color();
      #endif
      timeStart = millis();
      state = S_WAIT_BETWEEN;
      break;

    case S_WAIT_BETWEEN:    // signal finished with a dignified pause
      if (millis() - timeStart >= WAIT_DELAY)
        state = S_IDLE;
      break;

    default:
      state = S_IDLE;
      break;
    }

    // BUTTON PRESSES
    if(!digitalRead(BUTTON_A)) {
      int text_x = display.getCursorX();
      int text_y = display.getCursorY();
      
      if (!A_pressed) {
        display.drawBitmap(text_x, text_y, canvas.getBuffer(), 128, 10, 0, 0);
        A_pressed = true;
        currTune--;
        if (currTune >= ARRAY_SIZE(tuneList))
          currTune = ARRAY_SIZE(tuneList) - 1;
        display.print("NEXT: ");
        display.println(tuneList[currTune]);
        display.setCursor(text_x, text_y);   
      }
    }
    if(digitalRead(BUTTON_A)) A_pressed = false;

    
    if(!digitalRead(BUTTON_B)) {
      int text_x = display.getCursorX();
      int text_y = display.getCursorY();
      
      if (!B_pressed) {
        display.drawBitmap(text_x, text_y, canvas.getBuffer(), 128, 10, 0, 0);
        B_pressed = true;
        currTune++;
        if (currTune >= ARRAY_SIZE(tuneList))
          currTune = 0;
        display.print("NEXT: ");
        display.println(tuneList[currTune]);
        display.setCursor(text_x, text_y);  
      }
    }
    if(digitalRead(BUTTON_B)) B_pressed = false;

    if(!digitalRead(BUTTON_C)) {
      set_music = true;
      state = S_END;    
    }
    // delay(10);
    display.display();
  }
}

void setup1() {
  strip.begin();
  strip.setBrightness(128);
  strip.show(); // Initialize all pixels to 'off'

  randomSeed(analogRead(28));
  random_strip();
  #ifdef RANDOM
    random_color();
  #endif
}

void loop1() {
  // put your main code here, to run repeatedly:

  // OLD_METHOD for LED
  // for (int i = 0; i < 8; i++) {
  //   if (led_state[i]) {
  //     digitalWrite(led_pins[i], HIGH);
  //     // analogWrite(led_pins[i], volume);
  //   }
  //   else {
  //     digitalWrite(led_pins[i], LOW);
  //     // analogWrite(led_pins[i], 0);
  //   }
  // }
  set_strip();
  strip.show();
}