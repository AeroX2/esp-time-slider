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
    for (int8_t i = 0; i < 8; i++) {
      digitalWrite(_serial, (_mask >> i) & 1);
      delayMicroseconds(1);

      digitalWrite(_clock, HIGH);
      delayMicroseconds(1);
      digitalWrite(_clock, LOW);
      delayMicroseconds(1);
    }

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
class CustomAccelStepper : AccelStepper {
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

 private:
  ShiftRegister* _shiftRegister;
  bool _useMotor1;
};
