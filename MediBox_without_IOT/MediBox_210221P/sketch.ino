// import libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <time.h>



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 // in pixel
#define OLED_RESET -1 // reset pin
#define SCREEN_ADDRESS 0x3C  //I2c adress

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12
#define LED_2 16
#define LED_3 17

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0

//create object of oled display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor; // create object for DHT22 sensor

// global variable
int days=0;
int hours=0;
int minutes=0;
int seconds=0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3; // can set a any number of alarms by change this value
int alarm_hours[] = {0,1,3}; // setted for only 3 alarms
int alarm_minutes[] = {1,10,3};// setted for only 3 alarms
bool alarm_triggered[] = {false,false, false}; //considering if alarm is ringged or not

//buzzer ringing notes 
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode= 0;//current option
int max_mode = 5; // maximum number of options
String modes[] = {"1-Set Time Zone", "2-Set Alarm 1", "3-Set Alarm 2","4-Set Alarm 3", "5-Disable Alarm"};
long utc_offset_sec = 0; 
int offset_hours = utc_offset_sec / 3600; // set initial value of the offset hours to zero (global variable)
int offset_minutes = (utc_offset_sec % 3600) / 60; // set the intial value of the offset minutes to zero (global variable)



void setup() {
  // put your setup code here, to run once:
  //Pinmode() is a function used to set a specific pin as either an input or an output
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1,OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(LED_2,OUTPUT);
  pinMode(LED_3,OUTPUT);

  //initialize the sensor
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);


  Serial.begin(115200);
  // initializing OLED display
  if(! display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Turn on the oled display
  display.display();
  delay(500);
  
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI",0,0,2);
    
  }

  display.clearDisplay();
  print_line("Connected to WIFI",0,0,2);

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);//configTime() function call sets up the Arduino to synchronize its time with an NTP server.
  
  
  
  // Clear the display
  display.clearDisplay();

  // custom the display
  print_line("Welcome to the Medibox", 10,20 ,2);
  delay(500);
  display.clearDisplay();
}




void loop() {
  
  // put your main code here, to run repeatedly:
  // this speeds up the simulation
  update_time_with_check_alarm();  

  //checking the state of the button
  if (digitalRead(PB_OK)== LOW){
    delay(200);
    go_to_menu();
  }

  check_temp();
}

// Respobile of this function is print on OLED display
void print_line(String text, int column, int row, int text_size){

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text); //set the row value and column value

  display.display();
}

//printing days,hours,minutes and the days on OLED display
void print_time_now(void){
  display.clearDisplay();
  delay(100);
  print_line(String(days),0,0,2);
  print_line(":",20,0,2);
  print_line(String(hours),30,0,2);
  print_line(":",50,0,2);
  print_line(String(minutes),60,0,2);
  print_line(":",80,0,2);
  print_line(String(seconds),90,0,2);
}

/// function ensures that the global variables representing the current time are synchronized with the actual local time

void update_time(){
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
  strftime(timeDay,3,"%D",&timeinfo);
  days = atoi(timeDay);

}


// Function responsible for how alrm ringing(buzzer) until user turn off the alarm
void ring_alarm(){ 
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);

  digitalWrite(LED_1, HIGH); //LED bulb light up while alarm is ringing
  
  bool break_happened = false;

  ////RING THE BUZZER .....................................
  
  while (break_happened == false && digitalRead(PB_CANCEL)== HIGH){ //check previously alarm off or not
    for(int i =0 ; i<n_notes ; i++){
      if (digitalRead(PB_CANCEL) == LOW){ // check is user set off the alarm during ringing
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]); // buzzer ringing ,(one note for oen itteration)
      delay(500);
      noTone(BUZZER);
      delay(2);
    
    }

  }
  digitalWrite(LED_1, LOW); // of the LED after set off the alarm
  display.clearDisplay();
}



//Alarm checking with time
// this function is responsible for ringing the alarm when time reaches to the alarm time
//And also this function check if the alarms are dissable or not before it ringing
void update_time_with_check_alarm(){
  update_time(); // retrieves the current time 
  print_time_now(); // display the current time
  
  if (alarm_enabled == true){  // checking weather alarm disable or not
    for(int i=0; i < n_alarms ; i++){
      if (alarm_triggered[i] == false && alarm_hours[i] ==hours && alarm_minutes[i] ==minutes ){ // checking conditions  for alarm ring
        ring_alarm();
        alarm_triggered[i] =true;
      }
    }
  }
}


// The responsible of this function is to inform about user press button to the system.
int wait_for_button_press(){
  while (true){ // while loop add for check continuously whether button is pressed or not
    if (digitalRead(PB_UP)==LOW){
      delay(200);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN)==LOW){
      delay(200);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK)==LOW){
      delay(200);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL)==LOW){
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}


//menu function
//defining menu behaviour
//function manages the navigation through a menu system by handling button presses and updating the current mode accordingly.
void go_to_menu(){
  while(digitalRead(PB_CANCEL)== HIGH){ //running until PB_CANCEL button pressed
    display.clearDisplay();
    print_line(modes[current_mode],0,0,2);

    int pressed = wait_for_button_press();
    
    if(pressed == PB_UP){  //If (PB_UP) is pressed, it increments the current_mode variable
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_mode;
    }

    else if(pressed == PB_DOWN){ // If (PB_DOWN) is pressed, it decrements the current_mode variable 
      delay(200);
      current_mode -= 1;
      if(current_mode < 0 ){
        current_mode = max_mode -1;
      }
    }

    else if(pressed == PB_OK){ // If (PB_OK) is pressed, it calls the run_mode function with the current mode
      delay(200);
      //Serial.print(current_mode);
      run_mode(current_mode);
    }

    else if(pressed == PB_CANCEL){ //break function when PB_CANCEL button is pressed
      delay(200);
      break;
    }

  }  
}


//set the time of medi box......
// set_time function divided into set_time_hours and set_time_minutes.Because cannot get two return values using only set_time()
// responsible of this function is get the UTC offset hours and minutes through the user, store those values in offste_hour and offset_minute global variables
int  set_time_hour(){  //get the UTC hours which user input to the medibox
  
  int temp_hour = offset_hours ; 

  while(true){
    display.clearDisplay();
    print_line("Enter offset hours: "+String(temp_hour),0,0,2); // display the value of offset hours

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 15; //increase upto 14, beacuse international UTC hours < +14

    }

    else if(pressed == PB_DOWN){
      delay(200);
      temp_hour -= 1;
      temp_hour = temp_hour % 13; // decrease upto 12, because international UTC hours > -12
      
    }


    else if(pressed == PB_OK){
      delay(200);
      // store user setting hours in offset_hours is essential because time zone can be change....
      //....  Even if the time zone option is clicked by mistake and nothing is changed and the cancel button is pressed at the same time and exited.
      offset_hours = temp_hour; 
      break;
    }

    else if(pressed == PB_CANCEL){ // if PB_CANCEL pressed then loop break
      delay(200);
      break;
    }
    

  }
  return temp_hour;
}

//get the UTC minutes which user input to the medibox (simillar to the set_time_hour function)
int set_time_minute(){
  int temp_minute = offset_minutes ;
  while(true){
    display.clearDisplay();
    print_line("Enter offset minutes: "+ String(temp_minute),0,0,2);

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;

    }

    else if(pressed == PB_DOWN){
      delay(200);
      temp_minute -= 1;
      if(temp_minute < 0 ){
        temp_minute = 59;
      }
    }


    else if(pressed == PB_OK){
      delay(200);
      // store user setting minutes in offset_minutes is essential because time zone can be change....
      //....  Even if the time zone option is clicked by mistake and nothing is changed and the cancel button is pressed at the same time and exited.
      offset_minutes = temp_minute;   
      break;
    }

    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }
    
  }
  return temp_minute;
}  
  
  



//set the alarm
// This function resposible for set the alrm hours and minutes 
void set_alarm(int alarm){
  // when disable option select , then all alarm cancled.if user wants to set alarm again after that alarm_enabled should equals to true otherwise alarm doesn't set
  alarm_enabled = true; 

  int temp_hour = alarm_hours[alarm];

  while(true){
    display.clearDisplay();
    print_line("Enter hour: "+String(temp_hour),0,0,2);

    int pressed = wait_for_button_press();

    if(pressed == PB_UP){ //if PB_UP button pressed alarm hours increase one by one
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24; //maximum value of alarm hour is 23

    }

    else if(pressed == PB_DOWN){
      delay(200);
      temp_hour -= 1; //if PB_DOWN button pressed alarm hours decrease one by one
      if(temp_hour < 0 ){ // alarm hours cannot be negetive ,minimum value is 0 and maximum value is 23
        temp_hour = 23;
      }
    }


    else if(pressed == PB_OK){
      delay(200);
      alarm_hours[alarm] = temp_hour;//set alarm hour store in alarm hours array
      break;
    }

    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }

  }

  int temp_minute = alarm_minutes[alarm];
  while(true){
    display.clearDisplay();
    print_line("Enter minute: "+String(temp_minute),0,0,2);

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){ //if PB_UP button pressed alarm minutes increase one by one
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60; // maximum value of minute is XX:59 (XX means hours)

    }

    else if(pressed == PB_DOWN){  //if PB_DOWN button pressed alarm minutes dicrease one by one
      delay(200);
      temp_minute -= 1;
      if(temp_minute < 0 ){
        temp_minute = 59; 
      }
    }


    else if(pressed == PB_OK){
      delay(200);
      alarm_minutes[alarm] = temp_minute; //set alarm minute store in alarm minutes array
      alarm_triggered[alarm] = false;
      break;
    }

    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
  display.clearDisplay();
  print_line("Alarm is set",0,0,2);
  delay(1000);
}  


// set the time zone .......................................................
//This function is responsible for show the time according to the user input time zone
void Set_Timezone_And_Configuration() {
  
  
  
  int temp_hour = set_time_hour(); //store the user input hours of the time zone
  int temp_minute = set_time_minute(); // store the user input minutes of the time zone

  

  utc_offset_sec = temp_hour * 3600 + temp_minute * 60; //This line calculates the UTC offset in seconds
  // configtime function  sets up the ESP8266 to fetch time data from an NTP server, using the calculated UTC offset
  configTime(utc_offset_sec, UTC_OFFSET_DST, NTP_SERVER); 
  display.clearDisplay();
  print_line("Timezone is setting to " + String(temp_hour) + ":" + String(temp_minute), 0, 0, 2); //print the time zone 
  delay(1000);
}


// this function responsible for coordinating different operational modes of the system, handling configuration settings, and managing alarms
void run_mode(int mode){
  if(mode == 0){
    Set_Timezone_And_Configuration(); // Because frist option shold be select time zone
  }

  else if (mode == 1 || mode == 2|| mode == 3){ // this mode is for select the alarm and set the time for the alarm.
    set_alarm(mode-1);
  }

  else if(mode == 4){
    alarm_enabled = false; // when alarm_enable = flase ,alarm dosent ring because conditions doesn't satisfy in update_time_with_check_alarm() function
    display.clearDisplay();
    print_line("All Alrms Are Disabled",4,0,2); // show the message when user select dissable all alarm function
    delay(500);
  }
}



// create a temperature and huidity checking  function
// This function is responsible for checking temperature and humidity and inform user if those condition are not in better ranges
// There is message show in the OLED display and turn on the LED light up , If conditions are bad
void check_temp(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if(data.temperature>32){ // set what happen temperature greater than it maximum value
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 40,1); //LED light up when temp high
    digitalWrite(LED_2, HIGH);
    
  }
  if(data.temperature<26){ //set what happen temperature lower than it minimum value
    display.clearDisplay();
    print_line("TEMP LOW", 0, 40,1); //LED light up when tempurature is low
    digitalWrite(LED_2, HIGH);
    
  }
  if(data.temperature>26 && data.temperature<32 ){ 
    digitalWrite(LED_2, LOW); // keeping turn off LED lightinting when temperature is in better range 
    
  }
  if(data.humidity>80){  // set what happen humidity greater than it maximum value
    display.clearDisplay();
    print_line("HUMIDITY HIGH", 0, 50,1); //LED light up when humidity high
    digitalWrite(LED_3, HIGH);
    
  }
  if(data.humidity<60){ //set what happen humidity lower than it minimum value
    display.clearDisplay();
    print_line("HUMIDITY LOW", 0, 40,1); //LED light up when humidity is low
    digitalWrite(LED_3, HIGH);
    
  }
  if(data.humidity>60 && data.humidity < 80 ){
    digitalWrite(LED_3, LOW); // keeping turn off LED lightinting when humidity is in better range 
    
  }
}
