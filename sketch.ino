

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels 
#define SCREEN_HEIGHT 64 // OLED display height, in pixels 
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; ex3D for 128x64, 0x3C for 128x32

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35

#define DHTPIN 12

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0

//Global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;


unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0,1,2};
int alarm_minutes[] = {1,10,20};
bool alarm_triggered[] = {false, false,false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G ,A, B,C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time-Zone", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Set Alarm 3", "5- Disable Alarms"};

int current_timeZone = 0;
int max_timeZones = 5;
String timeZones[] = {"1- Canada", "2- Argentina", "3- Greenland", "4- Quatar", "5- SriLanka"};


//Declare objects
Adafruit_SSD1306 display (SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor; 

void setup() {

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_UP, INPUT);

  Serial.begin(9600);
  // put your setup code here, to run once:
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen -- 
  // the library initializes this with an Adafruit splash screen. 
  display.display();
  delay(2000); // Pause for 2 seconds
  
  // Clear the buffer 
  display.clearDisplay();
 

  print_line("Welcome  to Medibox!", 10, 20, 2);
  display.clearDisplay();

  dhtSensor.setup(DHTPIN,DHTesp::DHT22);
  
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0,0,2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0,0,2);
  // display.clearDisplay();

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}  
  
  void loop() {
    // put your main code here, to run repeatedly:
    update_time_with_check_alarm();
    

    if (digitalRead(PB_OK)==LOW){
      delay(200);
      go_to_menu();
    }
     check_temp_humid();
     delay(1000);
}

void print_line(String text, int column, int row, int text_size){
  // display.clearDisplay();
  display.setTextSize(text_size);               
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(column,row);              //(column, row)
  display.println(text); 
  
  display.display();
  // delay(2000);
}


// function to display the current time DD:HH:MM:SS 
void print_time_now(void) {
  display.clearDisplay();
  print_line(String(days), 0, 0, 2); 
  print_line(":", 20, 0, 2);
  print_line(String (hours), 30,0,2);
  print_line(":", 50, 0, 2);
  print_line(String(minutes), 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);
}

// function to automatically update the current time 
void update_time(void) {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3,"%H",&timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3,"%M",&timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3,"%S",&timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d",&timeinfo);
  days = atoi(timeDay);
}

// ring an alarm
void ring_alarm() {
  // Show message on display
  display.clearDisplay();
  print_line("Medicine Time!", 0, 0, 2);
  
  bool break_happened = false;
  
  // ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }  
        tone (BUZZER, notes[i]);
        delay(500);
        noTone (BUZZER);
        delay(2);
        digitalWrite(LED_1, HIGH); //Blink LED
    }    
  delay(200);
  digitalWrite(LED_1, LOW);
  display.clearDisplay();
  }
}

// function to automatically update the current time while checking for alarms
void update_time_with_check_alarm() {
  // update time
  display.clearDisplay();
  update_time();
  print_time_now();
  
  // check for alarms
  if (alarm_enabled) {
  // iterating through all alarms
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i]==hours && alarm_minutes[i]==minutes) {
        ring_alarm(); // call the ringing function
        alarm_triggered[i] = true;
      }
    }
  }
  
}



// function to wait for button press in the menu
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }

    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }

    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }  
    
    else if (digitalRead(PB_OK) == LOW ) {
      delay(200);
      return PB_OK;
    }

    update_time();
  }
}


// function to navigate through the menu 
void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    delay(1000);
  
    int pressed = wait_for_button_press();
  
    if (pressed == PB_UP) {
      current_mode += 1;
      current_mode %= max_modes; 
      delay(200);
    } 
      
    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
      current_mode = max_modes - 1;
      }
    }
  
    else if (pressed == PB_OK) {
      Serial.println(current_mode);
      delay(200);
      run_mode (current_mode);
    }  

    else if (pressed == PB_CANCEL) {
      delay(200);
      display.clearDisplay();
      break;
    }  
  }
}

// Below set time function is not used because we are using time zones
 
// void set_time() {
//   int temp_hour = hours;
//   while (true) {
//     display.clearDisplay();
//     print_line("Enter hour: " + String (temp_hour), 0, 0, 2);
    
//     int pressed = wait_for_button_press();
//     if (pressed == PB_UP) {
//       delay(200);
//       temp_hour += 1;
//       temp_hour = temp_hour % 24;
//     }  
//     else if (pressed==PB_DOWN) {
//       delay(200);
//       temp_hour -= 1;
    
//       if (temp_hour < 0) {
//         temp_hour = 23;
//     }
//   }
    
//     else if (pressed== PB_OK) {
//       delay(200);
//       hours = temp_hour;
//       break;
//     }
    
//     else if (pressed==PB_CANCEL) {
//       delay(200);
//       display.clearDisplay();
//       break;
//     }
//   }  

//  int temp_minute = minutes;
//   while (true) {
//     display.clearDisplay();
//     print_line("Enter minute: " + String (temp_minute), 0, 0, 2);
    
//     int pressed = wait_for_button_press();
//     if (pressed == PB_UP) {
//       delay(200);
//       temp_minute += 1;
//       temp_minute = temp_minute % 60;
//     }  
//     else if (pressed==PB_DOWN) {
//       delay(200);
//       temp_minute -= 1;
    
//       if (temp_minute < 0) {
//         temp_minute = 59;
//     }
//   }
    
//     else if (pressed== PB_OK) {
//       delay(200);
//       minutes = temp_minute;
//       break;
//     }
    
//     else if (pressed==PB_CANCEL) {
//       delay(200);
//       display.clearDisplay();
//       break;
//     }
//   }

//   display.clearDisplay();
//   print_line("Time is set", 0, 0, 2);
//   delay(1000);
// }  

//Function to set time using time zones using a sub menu
void set_timeZone() {
  while (digitalRead(PB_CANCEL) == HIGH) {
  display.clearDisplay();
  print_line(timeZones[current_timeZone], 0, 0, 2);
  delay(1000);

  int pressed = wait_for_button_press();

  if (pressed == PB_UP) {
    current_timeZone += 1;
    current_timeZone = current_timeZone % max_timeZones; 
    delay(200);
  } 
    
  else if (pressed == PB_DOWN) {
    delay(200);
    current_timeZone -= 1;
    if (current_timeZone < 0) {
    current_timeZone = max_timeZones - 1;
    }
  }

  else if (pressed == PB_OK) {
  Serial.println(current_timeZone);
  delay(200);
  user_timeZone(current_timeZone);
  break;
  }   

  else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  delay(1000);
} 

//5 time zones given in the sub menu
void user_timeZone(int timeZone){

  int NEW_OFFSET;

  if (timeZone == 0) {
      NEW_OFFSET = -5*60*60;
  }               
  else if (timeZone == 1) {
      NEW_OFFSET = -3*60*60 ;
  }    
  else if (timeZone == 2) {
      NEW_OFFSET = 0;  
  }         
  else if (timeZone == 3) {
      NEW_OFFSET = 3*60*60 ;
  }    
  else if (timeZone == 4) {
      NEW_OFFSET = 5*60*60 + 30*60;  
  }  

  configTime(NEW_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

}



void set_alarm(int alarm) {
  int temp_hour =alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String (temp_hour), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }  
    else if (pressed==PB_DOWN) {
      delay(200);
      temp_hour -= 1;
    
      if (temp_hour < 0) {
        temp_hour = 23;
    }
  }
    
    else if (pressed== PB_OK) {
      delay(200);
     alarm_triggered[alarm]= false;
     alarm_hours[alarm] = temp_hour;
      break;
    }
    
    else if (pressed==PB_CANCEL) {
      delay(200);
      break;
    }
  }  

 int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String (temp_minute), 0, 0, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }  
    else if (pressed==PB_DOWN) {
      delay(200);
      temp_minute -= 1;
    
      if (temp_minute < 0) {
        temp_minute = 59;
    }
  }
    
    else if (pressed== PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }
    
    else if (pressed==PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}  

void run_mode(int mode) {
  if (mode == 0) {
    set_timeZone();
  }
  else if (mode ==1 ||mode == 2 ||mode == 3 ){
    set_alarm(mode-1);
  }
  else if (mode == 4 ){
    alarm_enabled = false;
  }
}

void check_temp_humid(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  print_line("Temp = "+ String(data.temperature), 1, 40, 0);
  print_line("Humid = "+ String(data.humidity), 1, 50, 0);

  if (data.temperature > 32){
    display.clearDisplay();
    print_line("HIGH TEMPERATURE",0,40,1);

    digitalWrite(LED_1, HIGH);
    tone (BUZZER, 349);

    delay(2000);

  }
  else if (data.temperature < 26){
    display.clearDisplay();
    print_line("LOW TEMPERATURE",0,40,1);

    digitalWrite(LED_1, HIGH);
    tone (BUZZER, 349);

    delay(2000);
  }

  if (data.humidity > 80){
    display.clearDisplay();
    print_line("HIGH HUMIDITY",0,50,1);

    digitalWrite(LED_1, HIGH);
    tone (BUZZER, 349);

    delay(2000);
  }
  else if (data.humidity < 60){
    display.clearDisplay();
    print_line("LOW HUMIDITY",0,50,1);

    digitalWrite(LED_1, HIGH);
    tone (BUZZER, 349);

    delay(2000);
  }
  if ((data.temperature >= 26) && (data.temperature <= 32) && (data.humidity <= 80) && (data.humidity >= 60)) {
    digitalWrite(LED_1, LOW);
    noTone (BUZZER);
  }
  
}
