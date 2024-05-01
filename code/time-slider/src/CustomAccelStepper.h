#include <AccelStepper.h>

class ShiftRegister {
 public:
  ShiftRegister(int8_t serial, int8_t clock, int8_t rclock) {
    _serial = serial;
    _clock = clock;
    _rclock = rclock;
  }

  void init() {
    pinMode(_serial, OUTPUT);
    pinMode(_clock, OUTPUT);
    pinMode(_rclock, OUTPUT);
  }

  void step() {
    //Serial.println(_mask, BIN);
    shiftOut(_serial, _clock, LSBFIRST, _mask);

    digitalWrite(_rclock, HIGH);
    delayMicroseconds(1);
    digitalWrite(_rclock, LOW);
    delayMicroseconds(1);
  }

  void stepMotor1(int8_t mask) {
    _mask = (_mask & 0b11110000) | (mask & 0b00001111);
    step();
  }

  void stepMotor2(int8_t mask) {
    _mask = ((mask << 4) & 0b11110000) | (_mask & 0b00001111);
    step();
  }

 private:
  int8_t _serial;
  int8_t _clock;
  int8_t _rclock;

  int8_t _mask = 0;
};

// SER, SRCK, RCK
class CustomAccelStepper : public AccelStepper {
 public:
  CustomAccelStepper(ShiftRegister* shiftRegister, bool useMotor1)
      : AccelStepper(HALF4WIRE, -1, -1, -1, -1) {
    _shiftRegister = shiftRegister;
    _useMotor1 = useMotor1;
  }

  void setOutputPins(uint8_t mask) override {
    if (_useMotor1) {
      _shiftRegister->stepMotor1(mask);
    } else {
      _shiftRegister->stepMotor2(mask);
    }
  }

  // blue pink yellow orange -> 
  void step4(long step) override {
    switch (step & 0x3) {
      case 0:  // 1010
        setOutputPins(0b0110);
        break;
      case 1:  // 0110
        setOutputPins(0b0101);
        break;
      case 2:  // 0101
        setOutputPins(0b1001);
        break;
      case 3:  // 1001
        setOutputPins(0b1010);
        break;
    }
  }

  // blue pink yellow orange -> 
  void step8(long step) override {
    switch (step & 0x7) {
      case 0:  // 1100
        setOutputPins(0b0011);
        break;
      case 1:  // 0100
        setOutputPins(0b0010);
        break;
      case 2:  // 0110
        setOutputPins(0b0110);
        break;
      case 3:  // 0010
        setOutputPins(0b0100);
        break;
      case 4:  // 0011
        setOutputPins(0b1100);
        break;
      case 5:  // 0001
        setOutputPins(0b1000);
        break;
      case 6:  // 1001
        setOutputPins(0b1001);
        break;
      case 7:  // 1000
        setOutputPins(0b0001);
        break;
    }
  }

 private:
  ShiftRegister* _shiftRegister;
  bool _useMotor1;
};
