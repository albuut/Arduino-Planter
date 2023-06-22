#include "times.h"
times::times() {
  hour = 0;
  minute = 0;
  second = 0;
}
void times::reset(){
  hour = 0;
  minute = 0;
  second = 0;
}
bool times::is_done(){
  return (hour == 0 && minute == 0 && second == 0) ? true : false;
}
void times::set_hour(unsigned char h) {
  hour = h;
}
void times::set_minute(unsigned char m) {
  minute = m;
}
void times::set_second(unsigned char s) {
  second = s;
}
void times::increment() {
  second++;
  if (second >= 60) {
    minute++;
    second -= 60;
  }
  if (minute >= 60) {
    hour++;
    minute -= 60;
  }
  if (hour >= 24) {
    hour -= 24;
  }
}
void times::decrement(){
  if(second == 0){
    if(minute == 0){
      if(hour == 0){
        //done
      }else{
        hour--;
        minute = 59;
        second = 59;
      }      
    }else{
      minute--;
      second = 59;
    }
  }else{
    second--;
  }
}
unsigned char times::get_hour() {
  return hour;
}
unsigned char times::get_minute() {
  return minute;
}
unsigned char times::get_second() {
  return second;
}
String times::to_string(){
  char buff[] = "00:00:00";
  sprintf(buff, "%02i:%02i:%02i", hour, minute, second);
  return String(buff);
}
