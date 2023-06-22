#ifndef pump_h
#define pump_h
#include "times.h"
class pump {
  private:
    unsigned char pin;
    times schedule;
    times duration;
    times runtime;
    bool scheduled;
    bool active;
  public:
    pump(unsigned char pin, times s, times d, times r);
    void scheduled_on();
    void scheduled_off();

    void active_on();
    void active_off();

    bool is_scheduled();
    bool is_active();

    times get_schedule();
    times get_duration();
    times get_runtime();

    void set_schedule_hour(unsigned char h){
      schedule.set_hour(h);
    }
    void set_schedule_minute(unsigned char m){
      schedule.set_minute(m);
    }

    void set_duration_minute(unsigned char m){
      duration.set_minute(m);
    }
    void set_duration_second(unsigned char s){
      duration.set_second(s);
    }
    void tick_down();

    void force_on();
    void force_off();

    void start_pump(){
      runtime = duration;
    }

    void set_schedule(bool s){
      scheduled = s;
    }
};
#endif
