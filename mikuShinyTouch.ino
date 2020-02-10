/*
 * mikuShinyTouch
 */

/*
 * References:
 * https://github.com/PaulStoffregen/USBHost_t36/blob/fb917d9597307fd6181ae939528a22e939e57706/examples/Serial/MIDI/InputFunctions/InputFunctions.ino
 * https://github.com/PaulStoffregen/WS2812Serial/blob/958edc97f6579b076ec0b41d8d45757b63d63c62/examples/BasicTest/BasicTest.ino
 */

#include <WS2812Serial.h>
#include <USBHost_t36.h>

/*
 * Timing/dimming configuration
 */
/*
 * Minimum tick is about 2500 usec?
 * https://github.com/PaulStoffregen/WS2812Serial/blob/39931d0a2d935318fdb37c2b454a1de6944f3823/WS2812Serial.cpp#L260
 */
const int     TICK_USEC     = 3000;
const double  DIM_PER_TICK  = 0.995;

/*
 * Keyboard-specific configuration
 * Configured for Roland DP603
 */
/* MIDI Note number */
#define NOTENUM_START (21)
#define NOTENUM_END   (108)
#define NUM_KEYS      (NOTENUM_END - NOTENUM_START + 1)
/* Positions of all keys in millimeter */
const double        KEYPOS[NUM_KEYS]    = {0.0, 14.799999999999997, 23.697863247863253, 47.39572649572649, 55.69572649572649, 71.09358974358975, 85.89358974358974, 94.791452991453, 118.48931623931625, 126.78931623931625, 142.1871794871795, 153.8871794871795, 165.88504273504276, 180.68504273504277, 189.58290598290603, 213.28076923076924, 221.58076923076925, 236.9786324786325, 251.7786324786325, 260.6764957264958, 284.374358974359, 292.674358974359, 308.07222222222225, 319.77222222222224, 331.7700854700855, 346.5700854700855, 355.46794871794873, 379.165811965812, 387.465811965812, 402.86367521367526, 417.6636752136752, 426.5615384615385, 450.25940170940174, 458.55940170940175, 473.957264957265, 485.657264957265, 497.6551282051283, 512.4551282051283, 521.3529914529915, 545.0508547008548, 553.3508547008547, 568.748717948718, 583.548717948718, 592.4465811965813, 616.1444444444445, 624.4444444444446, 639.8423076923077, 651.5423076923078, 663.540170940171, 678.340170940171, 687.2380341880342, 710.9358974358976, 719.2358974358975, 734.6337606837608, 749.4337606837609, 758.3316239316241, 782.0294871794873, 790.3294871794874, 805.7273504273505, 817.4273504273506, 829.4252136752139, 844.2252136752138, 853.1230769230771, 876.8209401709404, 885.1209401709403, 900.5188034188036, 915.3188034188037, 924.2166666666669, 947.9145299145301, 956.2145299145302, 971.6123931623933, 983.3123931623934, 995.3102564102567, 1010.1102564102566, 1019.0081196581199, 1042.705982905983, 1051.005982905983, 1066.4038461538464, 1081.2038461538464, 1090.1017094017095, 1113.7995726495728, 1122.0995726495728, 1137.4974358974362, 1149.1974358974362, 1161.1952991452993, 1175.9952991452994, 1184.8931623931626, 1208.5910256410257};
/* Velocity saturates around 110 in most of (my) key touches */
const unsigned int  VELOCITY_KEY_MAX    = 110;
const unsigned int  VELOCITY_PEDAL_MAX  = 127;

/*
 * LED tape specific configuration
 * Configured for WS2815 12V NeoPixel RGB Tape LED 60LED/m ( https://www.akiba-led.jp/product/1782 )
 */
#define NUM_LEDS  (60)
/* Leftmost key = key number 0 */
const unsigned int  KEYNUM_LEDSTART  = 8;
const unsigned int  KEYNUM_LEDEND    = 79;
/* in millimeter */
const double        INTERVAL_LEDS    = 16.6;

/*
 * Color configuration
 */
#define NUM_KEYTYPES  (2)
/* Hatsune Miku colors */
const int           COLOR[NUM_KEYTYPES] = {0x137a7f, 0xe12885};
const unsigned int  KEYTYPE[NUM_KEYS]   = {0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0};

/*
 * Variables
 */
double pedal = 0.0;
bool key[NUM_KEYS];
unsigned int key_presstime[NUM_KEYS];
double string[NUM_KEYS];

/*
 * Stuffs for WS2812Serial
 */
/*
 * No special reason for using pin 17.
 * See "Supported Pins & Serial Ports"
 * https://github.com/PaulStoffregen/WS2812Serial/blob/master/readme.md
 */
const int PIN_LEDTAPE = 17;
byte drawingMemory[NUM_LEDS*3];
DMAMEM byte displayMemory[NUM_LEDS*12];
WS2812Serial leds(NUM_LEDS, displayMemory, drawingMemory, PIN_LEDTAPE, WS2812_GRB);

/*
 * Stuffs for USBHost_t36
 */
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
/* No special reason for using big buffer */
MIDIDevice_BigBuffer midi1(myusb);

/*
 * Misc
 */
IntervalTimer myTimer;

void initVariables()
{
  for (unsigned int keyi = 0; keyi < NUM_KEYS; keyi++) {
    key[keyi] = false;
    string[keyi] = 0.0;
    key_presstime[keyi] = 0;
  }
}

void pressPianoKey(unsigned int notenum, unsigned int velocity)
{
  int keyi = notenum - NOTENUM_START;

  key[keyi] = true;
  string[keyi] = (double)(velocity) / (double)(VELOCITY_KEY_MAX);
  if (string[keyi] > 1.0) {
    string[keyi] = 1.0;
  }

  key_presstime[keyi] = millis();
}

void releasePianoKey(unsigned int notenum, unsigned int velocity)
{
  int keyi = notenum - NOTENUM_START;

  key[keyi] = false;
}

void changePedalState(unsigned int velocity)
{
  pedal = (double)(velocity) / (double)(VELOCITY_PEDAL_MAX);
}

void updateStringState()
{
  for (unsigned int keyi = 0; keyi < NUM_KEYS; keyi++) {
    string[keyi] *= key[keyi] ? DIM_PER_TICK : DIM_PER_TICK * pedal;
  }
}

void setLedColors()
{
  double led[NUM_LEDS][NUM_KEYTYPES];
  unsigned int led_starttime[NUM_LEDS][NUM_KEYTYPES];

  for (unsigned int ledi = 0; ledi < NUM_LEDS; ledi++) {
    for (unsigned int kyti = 0; kyti < NUM_KEYTYPES; kyti++) {
      led[ledi][kyti] = 0.0;
      led_starttime[ledi][kyti] = 0;
    }
  }

  for (unsigned int keyi = KEYNUM_LEDSTART; keyi <= KEYNUM_LEDEND; keyi++) {
    double keypos = KEYPOS[keyi] - KEYPOS[KEYNUM_LEDSTART];
    int keytype = KEYTYPE[keyi];

    for (unsigned int ledi = 0; ledi < NUM_LEDS - 1; ledi++) {
      if ((ledi + 1) * INTERVAL_LEDS > keypos) {
        double vel_left = 1.0 - (keypos - ledi * INTERVAL_LEDS) / INTERVAL_LEDS;
        double vel_right = 1.0 - ((ledi + 1) * INTERVAL_LEDS - keypos) / INTERVAL_LEDS;

        led[ledi + 0][keytype] = min(1.0, led[ledi + 0][keytype] + vel_left  * string[keyi]);
        led[ledi + 1][keytype] = min(1.0, led[ledi + 1][keytype] + vel_right * string[keyi]);
        led_starttime[ledi + 0][keytype] = key_presstime[keyi];
        led_starttime[ledi + 1][keytype] = key_presstime[keyi];

        break;
      }
    }
  }

  for (unsigned int ledi = 0; ledi < NUM_LEDS; ledi++) {
    unsigned int lastpress_time = led_starttime[ledi][0];
    unsigned int lastpress_keyt = 0;

    for (unsigned int kyti = 0; kyti < NUM_KEYTYPES; kyti++) {
      if (led_starttime[ledi][kyti] > lastpress_time) {
        lastpress_time = led_starttime[ledi][kyti];
        lastpress_keyt = kyti;
      }
    }

    leds.setPixel(ledi, adjustColorBrightness(COLOR[lastpress_keyt], led[ledi][lastpress_keyt]));
  }
}

int adjustColorBrightness(int color, double velocity)
{
  byte blue  = (color & 0x0000FF) >>  0;
  byte green = (color & 0x00FF00) >>  8;
  byte red   = (color & 0xFF0000) >> 16;

  blue  *= velocity;
  green *= velocity;
  red   *= velocity;

  return red << 16 | green << 8 | blue << 0;
}

void turnOffAllLeds() {
  for (unsigned int ledi = 0; ledi < NUM_LEDS; ledi++) {
    leds.setPixel(ledi, 0x000000);
  }
}

void handleTimerInterrupt()
{
  myusb.Task();
  midi1.read();

  turnOffAllLeds();
  setLedColors();
  leds.show();

  updateStringState();
}

void setup() {
  Serial.begin(115200);

  // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
  // use too much power, Teensy at least completes USB enumeration, which
  // makes isolating the power issue easier.
  delay(1500);
  Serial.println("mikuShinyTouch");
  delay(10);
  myusb.begin();

  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandleNoteOff(myNoteOff);
  midi1.setHandleControlChange(myControlChange);

  initVariables();
  leds.begin();

  myTimer.begin(handleTimerInterrupt, TICK_USEC);
}

void loop() {
}


void myNoteOn(byte channel, byte note, byte velocity) {
//  Serial.print("Note On, ch=");
//  Serial.print(channel, DEC);
//  Serial.print(", note=");
//  Serial.print(note, DEC);
//  Serial.print(", velocity=");
//  Serial.println(velocity, DEC);

  if (channel == 1 && note >= NOTENUM_START && note <= NOTENUM_END) {
    pressPianoKey(note, velocity);
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {
//  Serial.print("Note Off, ch=");
//  Serial.print(channel, DEC);
//  Serial.print(", note=");
//  Serial.print(note, DEC);
//  Serial.print(", velocity=");
//  Serial.println(velocity, DEC);

  if (channel == 1 && note >= NOTENUM_START && note <= NOTENUM_END) {
    releasePianoKey(note, velocity);
  }
}

void myControlChange(byte channel, byte control, byte value) {
//  Serial.print("Control Change, ch=");
//  Serial.print(channel, DEC);
//  Serial.print(", control=");
//  Serial.print(control, DEC);
//  Serial.print(", value=");
//  Serial.println(value, DEC);

  if (channel == 1 && control == 64) {
    changePedalState(value);
  }
}
