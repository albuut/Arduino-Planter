#include "pump.h"
pump::pump(unsigned char pin, times s, times d, times r){
  this->pin = pin;
  duration = d;
  runtime = r;
  scheduled = false;
  active = false;
}
void pump::scheduled_on(){
  scheduled = true;  
}
void pump::scheduled_off(){
  scheduled = false;
}
void pump::active_on(){
  active = true;
}
void pump::active_off(){
  active = false;  
}
bool pump::is_scheduled(){
  return scheduled;
}
bool pump::is_active(){
  return active;
}
times pump::get_duration(){
  return duration;
}
times pump::get_runtime(){
  return runtime;
}
times pump::get_schedule(){
  return schedule;
}
void pump::tick_down(){
  if(runtime.is_done()){
    active = false;
  }else{
    runtime.decrement();
  }
}
