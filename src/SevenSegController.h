#ifndef SevenSegController_h
#define SevenSegController_h

#include <Arduino.h>

#define _MUX_PERIOD   5000
#define _BLINK_PERIOD   10 
#define _NO_DIGITS       4

#define _BLINK_DIGIT     2
#define _ENABLE_DIGIT    1
#define _DISABLE_DIGIT   0

class SevenSegController
{
  public:
    SevenSegController(int muxPin0, int muxPin1, int muxPin2, int muxPin3, int colonPin, int degreePin, int latchPin, int dataPin, int clkPin);
    
    // write a single digit
    void writeDigit(byte digit, byte value);

    // control functions - single digits
    void disableDigit(byte digit);
    void enableDigit(byte digit);
    void enableDecimalPoint(byte digit);
    void disableDecimalPoint(byte digit);
    void enableColon();
    void disableColon();
    void enableDegreeSign();
    void disableDegreeSign();
    void enableBlink(byte digit);
    void disableBlink(byte digit);
    void setBrightness(byte brightness);
    
    // control functions - whole display
    void enableBlinkDisplay();
    void disableBlinkDisplay();
    void enableDisplay();   // display all digits
    void disableDisplay();  // disable complete display

    // high-level display modes
    void enableClockDisplay();
    void enableTempDisplay();
    void enableNumericDisplay();
    
    // function used to expose member interrupt function
    static inline void handle_interrupt();

  private:

    // pointer to handle timer1 interrupt
    static SevenSegController *active_object;

    volatile int _selectedDigit;
    byte _digitValues[_NO_DIGITS]; // stores the values to display for each digit
    byte _digitStatus[_NO_DIGITS]; // 0: disabled, 1: enabled, 2: blinking
    byte _showDecimal[_NO_DIGITS]; // 1: show, 0: do not show
    int _blinkCounter[_NO_DIGITS]; // used for timing blink pattern
    byte _brightness;  // define brightness from 0 to 255
    int _muxPins[4];
    int _colonPin;
    int _degreePin;
    int _latchPin;
    int _dataPin;
    int _clkPin;
    
    byte translateDigit(byte digit); // translates from binary to common anode segments
    void muxDisplay(void); // interrupt routine controlling display multiplexing
};

#endif
