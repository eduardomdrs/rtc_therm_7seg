#include "SevenSegController.h"
#include "TimerOne.h"


SevenSegController *SevenSegController::active_object = 0;


SevenSegController::SevenSegController(int muxPin0, int muxPin1, int muxPin2, int muxPin3, int colonPin, int degreePin, int latchPin, int dataPin, int clkPin)
{
    active_object = this;

    _muxPins[0] = muxPin0;
    _muxPins[1] = muxPin1;
    _muxPins[2] = muxPin2;
    _muxPins[3] = muxPin3;

    for (int i = 0; i < _NO_DIGITS; ++i)
    {
      _digitStatus [i] = 0;
      _digitStatus [0] = _ENABLE_DIGIT;
      _blinkCounter[0] = 0;
      _showDecimal [0] = 0;
    }

    _colonPin  = colonPin;
    _degreePin = degreePin;
    _latchPin  = latchPin;
    _dataPin   = dataPin;
    _clkPin    = clkPin;
    _selectedDigit = 0;
    
    pinMode(_muxPins[0], OUTPUT);
    pinMode(_muxPins[1], OUTPUT);
    pinMode(_muxPins[2], OUTPUT);
    pinMode(_muxPins[3], OUTPUT);
    pinMode(_colonPin , OUTPUT);
    pinMode(_degreePin, OUTPUT);
    pinMode(_latchPin , OUTPUT);
    pinMode(_dataPin  , OUTPUT);
    pinMode(_clkPin   , OUTPUT);

    Timer1.initialize(_MUX_PERIOD);
    Timer1.attachInterrupt(handle_interrupt);
}

void SevenSegController::writeDigit(byte digit, byte value)
{
  noInterrupts();
  _digitValues[digit] = value;
  interrupts();
}

void SevenSegController::disableDigit(byte digit)
{
  noInterrupts();
  _digitStatus[digit] = _DISABLE_DIGIT;
  interrupts();
}

void SevenSegController::enableDigit(byte digit)
{
  noInterrupts();
  _digitStatus[digit] = _ENABLE_DIGIT;
  interrupts();
}

void SevenSegController::enableDecimalPoint(byte digit)
{
  _showDecimal[digit] = 0xFE;
}

void SevenSegController::disableDecimalPoint(byte digit)
{
  _showDecimal[digit] = 0xFF;
}

void SevenSegController::enableBlink(byte digit)
{
  noInterrupts();
  _digitStatus[digit] = _BLINK_DIGIT;
  interrupts();
}

void SevenSegController::disableBlink(byte digit)
{
  enableDigit(digit);
}

void SevenSegController::enableDegreeSign()
{
  digitalWrite(_degreePin, HIGH);
}

void SevenSegController::disableDegreeSign()
{
  digitalWrite(_degreePin, LOW);
}

void SevenSegController::enableColon()
{
  digitalWrite(_colonPin, HIGH);
}

void SevenSegController::disableColon()
{
  digitalWrite(_colonPin, LOW);
}

void SevenSegController::enableBlinkDisplay()
{
  noInterrupts();
  int i = 0;
  for (int i = 0; i < _NO_DIGITS; ++i)
    _digitStatus[_BLINK_DIGIT];
  interrupts();
}

void SevenSegController::disableBlinkDisplay()
{
  enableDisplay();
}

void SevenSegController::enableDisplay()
{
  noInterrupts();
  int i = 0;
  for (int i = 0; i < _NO_DIGITS; ++i)
    _digitStatus[_ENABLE_DIGIT];
  interrupts();
}

void SevenSegController::disableDisplay()
{
  noInterrupts();
  int i = 0;
  for (int i = 0; i < _NO_DIGITS; ++i)
    _digitStatus[_DISABLE_DIGIT];
  interrupts(); 
}

void SevenSegController::enableClockDisplay()
{
  disableDegreeSign();

  enableDigit(0);
  enableDigit(1);
  enableDigit(2);
  enableDigit(3);

  disableDecimalPoint(0);
  disableDecimalPoint(1);
  disableDecimalPoint(2);
  disableDecimalPoint(3);

  enableColon();
}

void SevenSegController::enableTempDisplay()
{
  disableColon();
  
  enableDigit(0);
  enableDigit(1);
  enableDigit(2);
  disableDigit(3);

  disableDecimalPoint(0);
  disableDecimalPoint(2);
  disableDecimalPoint(3);
  enableDecimalPoint(1);

  enableDegreeSign();
}

// ------------------------------ //
//   Interrupt code
// ------------------------------ //


void SevenSegController::handle_interrupt()
{
  active_object->muxDisplay();
}

void SevenSegController::muxDisplay(void)
{
    byte value = ( (translateDigit(_digitValues[_selectedDigit])) & _showDecimal[_selectedDigit] );
    digitalWrite(_latchPin, LOW);
    shiftOut(_dataPin, _clkPin, LSBFIRST, value);

    digitalWrite(_muxPins[0], LOW);
    digitalWrite(_muxPins[1], LOW);
    digitalWrite(_muxPins[2], LOW);
    digitalWrite(_muxPins[3], LOW);

    if (_digitStatus[_selectedDigit] == 1)
    {

      digitalWrite(_muxPins[_selectedDigit], HIGH);

    } else if (_digitStatus[_selectedDigit] == 2)
    {
      if (_blinkCounter[_selectedDigit] < 2 * _BLINK_PERIOD)
      {
        if (_blinkCounter[_selectedDigit] < _BLINK_PERIOD)
        {
          digitalWrite(_muxPins[_selectedDigit], HIGH);
        }
        _blinkCounter[_selectedDigit]++;

      } else {
        _blinkCounter[_selectedDigit] = 0;
      }
    }

    digitalWrite(_latchPin, HIGH);

    _selectedDigit++;
    if (_selectedDigit == 4)
      _selectedDigit = 0;
}


byte SevenSegController::translateDigit(byte digit)
{
  switch (digit)
  {
    case 0:
      return B00000011;
      break;
    case 1:
      return B10011111;
      break;
    case 2:
      return B00100101;
      break;
    case 3:
      return B00001101;
      break;
    case 4:
      return B10011001;
      break;
    case 5:
      return B01001001;
      break;
    case 6:
      return B01000001;
      break;
    case 7:
      return B00011111;
      break;
    case 8:
      return B00000001;
      break;
    case 9:
      return B00001001;
      break;
    default:
      return B11111110;
      break;
  }
}