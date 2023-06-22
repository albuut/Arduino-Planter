#ifndef times_h
#define times_h
#include "Arduino.h"
class times {
  private:
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
  public:
    times();
    void reset();
    void turn_off();
    void turn_on();
    bool is_done();
    void set_hour(unsigned char h);
    void set_minute(unsigned char m);
    void set_second(unsigned char s);
    void increment();
    void decrement();
    unsigned char get_hour();
    unsigned char get_minute();
    unsigned char get_second();
    String to_string();
};
#endif
