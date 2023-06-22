#define I2C_TIMEOUT 1000
#define I2C_PULLUP 1

#ifdef __AVR_ATmega328P__
/* Corresponds to A4/A5 - the hardware I2C pins on Arduinos */
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
#define I2C_FASTMODE 1
#endif

#include <SoftI2CMaster.h>

#define I2C_7BITADDR 0x68 // DS1307
#define MEMLOC 0x0A
#define ADDRLEN 1

#include <Wire.h>
#include <ds3231.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

struct ts t;
#include "times.h"
#include "pump.h"
times new_time;
times self;

int pos_x = 0;
int pos_y = 0;
//minute,second,on 2 2 1
int adjust_duration[3][2];
int adjust_schedule[3][3];
bool forced;
unsigned char current_pump;

int ee_address = 0;
//int pump_address[3][]



pump pump1(7, times(), times(), times());
pump pump2(7, times(), times(), times());
pump pump3(7, times(), times(), times());

pump pumps[] = {pump1, pump2, pump3};

const unsigned char up = 2;
const unsigned char down = 3;
const unsigned char left = 4;
const unsigned char right = 5;
const unsigned char enter = 6;

unsigned char pressed = 0;
unsigned char t_menu = 0;
unsigned char num_press = 0;

unsigned char button[5] = {up, down, left, right, enter};
unsigned char choice[5] = {0};

struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int(*TickFct)(int);
};

const char tasksNum = 4;
task tasks[tasksNum];

String menu_items[4] = {"System Time", "Sprinkler Schedule", "Sprinkler Duration", "Toggle On/Off"};
bool button_input() {
  num_press = 0;
  for (int i = 0; i < 5; i++) {
    choice[i] = digitalRead(button[i]);
    num_press = (choice[i]) ? num_press + 1 : num_press;
    if (choice[i]) {
      pressed = i;
    }
    Serial.print(choice[i] + " ");
  }
  Serial.println();
  if (num_press == 0 || num_press > 1) {
    return false;
  }
  return true;
}

String arr_to_string(int s[]) {
  char buff[] = "00:00";
  sprintf(buff, "%02i:%02i", s[0], s[1]);
  return String(buff);
}

bool is_time() {
  for (int i = 0; i < 3; i++) {
    if (pumps[i].get_schedule().get_hour() == t.hour && pumps[i].get_schedule().get_minute() == t.min && t.sec == 0 && pumps[i].is_scheduled()) {
      pumps[i].active_on();
      Serial.print("active");
    }
    Serial.println();
  }
}
bool is_active() {
  for (int i = 0; i < 3; i++) {
    if (pumps[i].is_active()) {
      current_pump = i;
      return true;
    }
  }
  return false;
}



enum e_menu {M_START, M_FOR, M_WAIT, M_SCHE, M_SYS, M_SPR, M_DOWN, M_OFF, M_IDLE};
int menu_prev = M_START;
int menu_next = M_START;
int TickFct_Menu(int state) {
  menu_prev = state;
  switch (state) {
    case M_START:
      state = M_IDLE;
      menu_next = M_IDLE;
      break;
    case M_FOR:
      if (button_input() && pressed == 4) {
        if (t_menu == 3) {
          state = M_DOWN;
          menu_next = M_WAIT;
          t_menu = 0;
          forced = false;
        }
        t_menu++;
      } else {
        t_menu = 0;
        state = M_FOR;
      }
      break;
    case M_WAIT:
      if (button_input()) {
        if (pressed == 0) {
          //System Time
          state = M_DOWN;
          menu_next = M_SYS;
          new_time.set_hour(self.get_hour());
          new_time.set_minute(self.get_minute());
        } else if (pressed == 1) {
          //Sprinkler Schedule
          state = M_DOWN;
          menu_next = M_SCHE;
          pos_x = -1;
          pos_y = 0;
          for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
              switch (j) {
                case 0:
                  adjust_schedule[i][j] = pumps[i].get_schedule().get_hour();
                  break;
                case 1:
                  adjust_schedule[i][j] = pumps[i].get_schedule().get_minute();
                  break;
                case 2:
                  if (pumps[i].is_scheduled()) {
                    adjust_schedule[i][j] = 1;
                  } else {
                    adjust_schedule[i][j] = 0;
                  }
                  break;
              }
            }
          }
          lcd.cursor();
        } else if (pressed == 3) {
          //Forced Toggle
          if (t_menu == 3) {
            state = M_DOWN;
            menu_next = M_FOR;
            forced = true;
          } else {
            t_menu++;
            state = M_WAIT;
          }
        } else if (pressed == 2) {
          state = M_DOWN;
          menu_next = M_SPR;
          pos_x = -1;
          pos_y = 0;
          ee_address = 0;
          for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 2; j++) {
              switch (j) {
                case 0:
                  adjust_duration[i][j] = pumps[i].get_duration().get_minute();
                  break;
                case 1:
                  adjust_duration[i][j] = pumps[i].get_duration().get_second();
                  break;
              }
            }
          }
          lcd.cursor();
        } else if (pressed == 4) {
          state = M_DOWN;
          menu_next = M_IDLE;
        }
      } else {
        t_menu = 0;
        state = M_WAIT;
        menu_next = M_WAIT;
      }
      break;
    case M_SCHE:
      if (button_input() && pressed == 4) {
        state = M_DOWN;
        menu_next = M_WAIT;
        ee_address = 0;
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            switch (j) {
              case 0:
                pumps[i].set_schedule_hour(adjust_schedule[i][j]);
                break;
              case 1:
                pumps[i].set_schedule_minute(adjust_schedule[i][j]);
                break;
              case 2:
                if (adjust_schedule[i][j]) {
                  pumps[i].scheduled_on();
                } else {
                  pumps[i].scheduled_off();
                }
                break;
            }
          }
          EEPROM.put(ee_address,pumps[i]);
          ee_address += sizeof(pumps[i]);
        }
        lcd.noCursor();
      } else {
        state = M_SCHE;
      }
      break;
    case M_SYS:
      if (button_input() && pressed == 4) {
        state = M_DOWN;
        menu_next = M_WAIT;
        t.hour = new_time.get_hour();
        t.min = new_time.get_minute();
        t.sec = 0;
        DS3231_set(t);
      } else {
        state = M_SYS;
      }
      break;
    case M_SPR:
      if (button_input() && pressed == 4) {
        state = M_DOWN;
        menu_next = M_WAIT;
        ee_address = 0;
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 2; j++) {
            switch (j) {
              case 0:
                pumps[i].set_duration_minute(adjust_duration[i][j]);
                break;
              case 1:
                pumps[i].set_duration_second(adjust_duration[i][j]);
                break;
            }
          }
          EEPROM.put(ee_address,pumps[i]);
          ee_address += sizeof(pumps[i]);
        }
        lcd.noCursor();
      } else {
        state = M_SPR;
      }
      break;
    case M_DOWN:
      if (!button_input() && num_press == 0) {
        state = menu_next;
      } else {
        state = M_DOWN;
      }
      break;
    case M_OFF:
      if (!is_active()) {
        state = M_IDLE;
        menu_next = M_IDLE;
      } else {
        state = M_OFF;
        menu_next = M_OFF;
      }
      break;
    case M_IDLE:
      if (button_input() && pressed == 4) {
        state = M_DOWN;
        menu_next = M_WAIT;
      } else {
        state = M_IDLE;
      }
      break;
  }
  switch (state) {
    case M_START: break;
    case M_FOR: break;
    case M_WAIT: break;
    case M_SYS:
      if (button_input()) {
        if (pressed == 0) {
          if (new_time.get_hour() < 23) {
            new_time.set_hour(new_time.get_hour() + 1);
          }
        } else if (pressed == 1) {
          if (new_time.get_hour() > 0) {
            new_time.set_hour(new_time.get_hour() - 1);
          }
        } else if (pressed == 2) {
          if (new_time.get_minute() < 59) {
            new_time.set_minute(new_time.get_minute() + 1);
          }
        } else if (pressed == 3) {
          if (new_time.get_minute() > 0) {
            new_time.set_minute(new_time.get_minute() - 1);
          }
        }
      }
      break;
    case M_SCHE:
      if (button_input()) {
        if (pressed == 0) {
          if (pos_x == -1) {
            pos_y = (pos_y < 2) ? pos_y + 1 : pos_y;
          } else {
            if (pos_x == 0) {
              adjust_schedule[pos_y][pos_x] = (adjust_schedule[pos_y][pos_x] < 23) ? adjust_schedule[pos_y][pos_x] + 1 : 23;
            } else if (pos_x == 1) {
              adjust_schedule[pos_y][pos_x] = (adjust_schedule[pos_y][pos_x] < 59) ? adjust_schedule[pos_y][pos_x] + 1 : 59;
            } else if (pos_x == 2) {
              adjust_schedule[pos_y][pos_x] = 1;
            }
          }
        } else if (pressed == 1) {
          if (pos_x == -1) {
            pos_y = (pos_y > 0) ? pos_y - 1 : pos_y;
          } else {
            if (pos_x == 0) {
              adjust_schedule[pos_y][pos_x] = (adjust_schedule[pos_y][pos_x] > 0) ? adjust_schedule[pos_y][pos_x] - 1 : 0;
            } else if (pos_x == 1) {
              adjust_schedule[pos_y][pos_x] = (adjust_schedule[pos_y][pos_x] > 0) ? adjust_schedule[pos_y][pos_x] - 1 : 0;
            } else if (pos_x == 2) {
              adjust_schedule[pos_y][pos_x] = 0;
            }
          }
        } else if (pressed == 2) {
          pos_x = (pos_x > -1) ? pos_x - 1 : pos_x;
        } else if (pressed == 3) {
          pos_x = (pos_x < 2) ? pos_x + 1 : pos_x;
        }
      }
      break;
    case M_SPR:
      if (button_input()) {
        if (pressed == 0) {
          if (pos_x == -1) {
            pos_y = (pos_y < 2) ? pos_y + 1 : pos_y;
          } else {
            if (pos_x == 0) {
              adjust_duration[pos_y][pos_x] = (adjust_duration[pos_y][pos_x] < 9) ? adjust_duration[pos_y][pos_x] + 1 : 9;
            } else if (pos_x == 1) {
              adjust_duration[pos_y][pos_x] = (adjust_duration[pos_y][pos_x] < 59) ? adjust_duration[pos_y][pos_x] + 1 : 59;
            }
          }
        } else if (pressed == 1) {
          if (pos_x == -1) {
            pos_y = (pos_y > 0) ? pos_y - 1 : pos_y;
          } else {
            if (pos_x == 0) {
              adjust_duration[pos_y][pos_x] = (adjust_duration[pos_y][pos_x] > 0) ? adjust_duration[pos_y][pos_x] - 1 : 0;
            } else if (pos_x == 1) {
              adjust_duration[pos_y][pos_x] = (adjust_duration[pos_y][pos_x] > 0) ? adjust_duration[pos_y][pos_x] - 1 : 0;
            }
          }
        } else if (pressed == 2) {
          pos_x = (pos_x > -1) ? pos_x - 1 : pos_x;
        } else if (pressed == 3) {
          pos_x = (pos_x < 1) ? pos_x + 1 : pos_x;
        }
      }
      break;
  }
  return state;
}

enum e_time {T_START, T_TICK};
int TickFct_Time(int state) {
  switch (state) {
    case T_START:
      state = T_TICK;
      break;
    case T_TICK:
      state = T_TICK;
      break;
  }
  switch (state) {
    case T_START: break;
    case T_TICK:
      DS3231_get(&t);
      self.set_hour(t.hour);
      self.set_minute(t.min);
      self.set_second(t.sec);
      break;
  }
  return state;
}

enum e_display {D_START, D_FOR, D_WAIT, D_SCHE, D_SYS, D_SPR, D_PUMP, D_IDLE};
int TickFct_Display(int state) {
  switch (menu_next) {
    case M_START: 
      state = D_WAIT;
      break;
    case M_FOR:
      state = D_FOR;
      break;
    case M_WAIT:
      state = D_WAIT;
      break;
    case M_SCHE:
      state = D_SCHE;
      break;
    case M_SYS:
      state = D_SYS;
      break;
    case M_SPR:
      state = D_SPR;
      break;
    case M_OFF:
      state = D_PUMP;
      break;
    case M_IDLE:
      state = D_IDLE;
      break;
  }
  unsigned char sched = 1;
  switch (state) {
    case D_START: break;
    case D_FOR:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water Pump On");
      lcd.setCursor(0, 2);
      lcd.print("Hold Enter to turn");
      lcd.setCursor(0, 3);
      lcd.print("Water Pump off.");
      break;
    case D_WAIT:
      lcd.clear();
      for (int i = 0; i < 4; i++) {
        lcd.setCursor(0, i);
        lcd.print(menu_items[i]);
      }
      break;
    case D_SCHE:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sprinkler Schedule");
      for (int i = 0; i < 3; i++) {
        lcd.setCursor(0, i + 1);
        lcd.print(String(i + 1) + ")");
        lcd.print(arr_to_string(adjust_schedule[i]));
        lcd.setCursor(11, i + 1);
        if (adjust_schedule[i][2]) {
          lcd.print("ON");
        } else {
          lcd.print("OFF");
        }
      }
      if (pos_x == -1) {
        lcd.setCursor(0, pos_y + 1);
      } else if (pos_x == 0) {
        lcd.setCursor(3, pos_y + 1);
      } else if (pos_x == 1) {
        lcd.setCursor(6, pos_y + 1);
      } else if (pos_x == 2) {
        lcd.setCursor(11, pos_y + 1);
      }
      break;
    case D_SYS:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("System Time");
      lcd.setCursor(0, 1);
      lcd.print(new_time.to_string());
      lcd.setCursor(0, 3);
      lcd.print("Press Enter to Save");
      break;
    case D_SPR:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sprinkler Duration");
      for (int i = 0; i < 3; i++) {
        lcd.setCursor(0, i + 1);
        lcd.print(String(i + 1) + ")");
        lcd.print(arr_to_string(adjust_duration[i]));
      }
      if (pos_x == -1) {
        lcd.setCursor(0, pos_y + 1);
      } else if (pos_x == 0) {
        lcd.setCursor(3, pos_y + 1);
      } else if (pos_x == 1) {
        lcd.setCursor(6, pos_y + 1);
      }
      Serial.println(adjust_duration[0][1]);
      break;
    case D_PUMP:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pumping Water");
      lcd.setCursor(0, 1);
      lcd.print("Duration: " + pumps[current_pump].get_duration().to_string());
      lcd.setCursor(0, 2);
      lcd.print("Runtime: " + pumps[current_pump].get_runtime().to_string());
      break;
    case D_IDLE:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time: " + self.to_string());
      for (int i = 0; i < 3; i++) {
        lcd.setCursor(0, sched);
        if (pumps[i].is_scheduled()) {
          lcd.print(String(sched) + ") " + pumps[i].get_schedule().to_string());
          sched++;
        }
      }
      break;
  }

  return state;
}

enum e_pump {P_START, P_WAIT, P_ON};
int TickFct_Pump (int state) {
  switch (state) {
    case P_START:
      state = P_WAIT;
      break;
    case P_WAIT:
      if (is_active()) {
        state = P_ON;
        digitalWrite(7, HIGH);
        pumps[current_pump].start_pump();
        //set menu to M_OFF;
        tasks[0].state = M_OFF;
      } else if (forced) {
        digitalWrite(7, HIGH);
        state = P_ON;
      } else {
        is_time();
        state = P_WAIT;
      }
      break;
    case P_ON:
      if (forced) {
        state = P_ON;
      } else {
        if (is_active()) {
          pumps[current_pump].tick_down();
        } else {
          state = P_WAIT;
          digitalWrite(7, LOW);
        }
      }
      break;
  }
  switch (state) {
    case P_START: break;
    case P_WAIT: break;
    case P_ON: break;
  }
  return state;
}
void setup() {
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);

  lcd.init();
  lcd.backlight();

  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(left, INPUT);
  pinMode(right, INPUT);
  pinMode(enter, INPUT);

  pinMode(7,OUTPUT);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      switch (j) {
        case 0:
          adjust_duration[i][j] = pumps[i].get_duration().get_minute();
          break;
        case 1:
          adjust_duration[i][j] = pumps[i].get_duration().get_second();
          break;
      }
    }
  }

  read_eeprom();



  Serial.begin(9600);

  unsigned char i = 0;
  tasks[i].state = M_START;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &TickFct_Menu;
  i++;
  tasks[i].state = T_START;
  tasks[i].period = 1000;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &TickFct_Time;
  i++;
  tasks[i].state = D_START;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &TickFct_Display;
  i++;
  tasks[i].state = P_START;
  tasks[i].period = 1000;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &TickFct_Pump;

}

void loop() {
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) {
    if ( (millis() - tasks[i].elapsedTime) >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis(); // Last time this task was ran
    }
  }
  //Serial.println(tasks[0].state);
  delay(100);
}


//EEPROM
void read_eeprom(){
  ee_address = 0;
  for(int i = 0; i < 3; i++){
    EEPROM.get(ee_address, pumps[i]);
    ee_address += sizeof(pumps[i]);
  }
}
