//MEDIBOX PROJECT......................................................
//setup function
//210221P  HETTIARACHCHI.S.L

#include <WiFi.h> //(1)to connecting the wifi connection
#include <PubSubClient.h> //(3) to sen the temp via mqtt
#include "DHTesp.h"//(2) to take the temperature value and send it to the dashboard
#include <NTPClient.h> //(7) to take time to our application 
#include <WiFiUdp.h> //(7) to initiate the ntp client library, we need that library
#include <ESP32Servo.h>//(8) import servo motor

//defining the PIN
#define DHT_PIN 15 //(2) defining DHT22
#define BUZZER 12 // (5)defining the buzzer pin
#define LDR1 34 // defining ldr
#define LDR2 35 //defining LDR
#define MOTOR 18 // defining motor

//defining minimum angle and the control factor for the cumstom medicine
float minAngle=30.0;// Minimum angle of the shaded sliding window
float controlFac=0.75;// Controlling factor used to calculate motor angle

float select_medicine = 0;

//defining minimum angle and the control factor for the pre stored medicine

//minimum angle and the controling factor for the tablet A
float minAngle_A =28;// Minimum angle 
float controlFac_A =0.55;// Controlling factor 


//minimum angle and the controling factor for the tablet B
float minAngle_B =26;// Minimum angle 
float controlFac_B =0.65;// Controlling factor 

//minimum angle and the controling factor for the tablet B
float minAngle_C =25;// Minimum angle 
float controlFac_C =0.6;// Controlling factor 




Servo motor; //(8)  initiating servometer

WiFiClient espClient; //(3)
PubSubClient mqttClient(espClient); //(3) pubsubclient library initiating

WiFiUDP ntpUDP; //(7) instance of wifi udp client
NTPClient timeClient(ntpUDP); //(7)start the ntp client 


char tempAr[6]; //to take temperature values as characters(2)
char lightAr[6];// Array to store light intensity as a string

//initiate the sensor/create instance
DHTesp dhtSensor; //(2)

bool isScheduledON = false; // to keep schedule time
unsigned long scheduledonTime;

void setup() {
  Serial.begin(115200);
  setupWifi(); //(1)method to connect WiFi network
  setupMqtt();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22); //(2) BEGIN THE SENSOR

  timeClient.begin(); //(7)
  timeClient.setTimeOffset(5.5*3600);

  pinMode(BUZZER, OUTPUT);//(5)
  digitalWrite(BUZZER, LOW); //(5) switch of the buzzer

  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);

  motor.attach(MOTOR, 500, 2400);// Attach servo motor to pin
}



// main loop function
void loop() {
  // (4)check the mqtt connection
  if(!mqttClient.connected()){ // checking is connected to the mqttpClient
    connectToBroker(); //(4)if it is not  connectted mentioned function will be calling

  }
  mqttClient.loop(); //(4) To handel the mqtt client options


  updateTemperature();
  Serial.println(tempAr);
  mqttClient.publish("TEMPERATURE-MY",tempAr); // (4) publish the temperature

  checkSchedule();
  updateLightIntensity();// Read light intensity from LDR  
  mqttClient.publish("LIGHTINTENSITY-MY",lightAr);// Publish light intensity to MQTT topic


  delay(1000);
  

}


//(7)get time  , in this time will be in seconds
unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}


//(5)Buzzer switch on function
void buzzerOn(bool on){
  if (on){
    tone(BUZZER, 256);

  }else{
    noTone(BUZZER);
  }


}

//(3)setup/initialize the Mqtt
void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org",1883); //set the server
  mqttClient.setCallback(receiveCallback);//(6) this method is use to catch the incomming messages
}

//(6) this function basically used to recieved message
void receiveCallback(char* topic, byte* payload, unsigned int length){ 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char payloadCharAr[length];
  for (int i =0;i<length;i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }

  Serial.println();

  //checking incoming topic
  if(strcmp(topic, "ON-OFF-MY")==0){
    buzzerOn(payloadCharAr[0] =='1');
  }else if(strcmp(topic, "SCH-ON-MY")==0){ //(7)
    if(payloadCharAr[0] =='N'){
      isScheduledON = false;
    }else{
      isScheduledON = true ;
    scheduledonTime = atol(payloadCharAr);
    }  
  }
  if (strcmp(topic,"MINIMUM-ANG-MY")==0){
      minAngle = atof(payloadCharAr);
      Serial.println(minAngle);
  }
  if (strcmp(topic,"CONTROL-FAC-MY")==0){
      controlFac = atof(payloadCharAr);
      Serial.println(controlFac);
  }
  if (strcmp(topic,"DROP-DOWN-MY")==0){
      select_medicine = atof(payloadCharAr);
      Serial.println(select_medicine);
  }
}



//(4) connecting broker
void connectToBroker(){ 
  while(!mqttClient.connected()){
    Serial.print("Attempting MQTT connection...");
    // trying to connect he function
    if(mqttClient.connect("ESP32-12345678")){
      Serial.println("connected");
      mqttClient.subscribe("ON-OFF-MY"); //suscribing part
      mqttClient.subscribe("SCH-ON-MY");
      mqttClient.subscribe("MINIMUM-ANG-MY");
      mqttClient.subscribe("CONTROL-FAC-MY");
      mqttClient.subscribe("DROP-DOWN-MY");  

    } else{
      Serial.println("failed");
      Serial.print(mqttClient.state());
      delay(5000);
    }

  }
}

//(2) update temperature to that character array
void updateTemperature(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity(); //get the DHT22 data to variable
  String(data.temperature, 2).toCharArray(tempAr, 6); //in here get only temperature data and convert that double value to string.Then store character array
}


//(9) update light intensity
void updateLightIntensity() {
  
  float sensorRight = analogRead(LDR1); // get the analog value to variable
  float sensorLeft = analogRead(LDR2);
  
  const float analogMinValue = 0.0;   
  const float analogMaxValue = 1023.0; 
  const float intensityMin = 0.0;  
  const float intensityMax = 1.0;  

  if((sensorRight > sensorLeft) && (sensorRight <= 1023)) { //check which sensor shows the highest analog output
    mqttClient.publish("MAXIMUM-INT-LDR","RIGHT LDR SHOWS MAXIMUM"); // show the dash board which sensor shows highest intensity
    double D = 0.5;
    float intensity = (sensorRight)/(analogMaxValue); // calculaye intensity based on the analog out put of the sensor
    Serial.println("LDR_1:"+String(sensorRight)+"  "+String(intensity));
    String(intensity, 2).toCharArray(lightAr, 6);
    calculateAngle(intensity,D); // send light intensity and D values as parameters to  calculate the motor angle
   
  }else if((sensorRight > sensorLeft) && (sensorRight > 1023)){ //if sensor analog output greater than 1023 we have to get 1 as intensity value
    mqttClient.publish("MAXIMUM-INT-LDR","RIGHT LDR SHOWS MAXIMUM");  // publishing highest intensity given sensor
    double D = 0.5;
    Serial.println("LDR_1:"+String(sensorRight)+"  "+String(1));
    String(1, 2).toCharArray(lightAr, 6);
    calculateAngle(1,D);

  }else if((sensorRight < sensorLeft) && (sensorLeft <= 1023)){
    mqttClient.publish("MAXIMUM-INT-LDR","LEFT LDR SHOWS MAXIMUM"); // publishing highest intensity given sensor
    double D = 1.5;
    float intensity = (sensorLeft)/(analogMaxValue);
    Serial.println("LDR_2:"+String(sensorLeft)+"  "+String(intensity));
    String(intensity, 2).toCharArray(lightAr, 6);
    calculateAngle(intensity,D);

  }else if((sensorRight < sensorLeft) && (sensorLeft > 1023)){
    mqttClient.publish("MAXIMUM-INT-LDR","LEFT LDR SHOWS MAXIMUM"); // publishing highest intensity given sensor
    double D = 1.5;
    Serial.println("LDR_2:"+String(sensorLeft)+"  "+String(1));
    String(1, 2).toCharArray(lightAr, 6);
    calculateAngle(1,D);

  }else if((sensorRight == sensorLeft) && (sensorLeft <= 1023)){
    mqttClient.publish("MAXIMUM-INT-LDR","BOTH_SHOWS_SAME_INTENSITY"); // publishing highest intensity given sensor
    double D = 0; //in assignmnet not given a value for D when LDR input are same, i get D =0 
    float intensity = (sensorLeft)/(analogMaxValue);
    Serial.println("LDR_2=LDR_1:"+String(sensorLeft)+"  "+String(intensity));
    String(intensity, 2).toCharArray(lightAr, 6);
    calculateAngle(intensity,D);

  }else{
    mqttClient.publish("MAXIMUM-INT-LDR","BOTH_SHOWS_SAME_INTENSITY"); // publishing highest intensity given sensor
    Serial.println("LDR_2=LDR_1:"+String(sensorLeft)+"  "+String(1));
    double D = 0;  //in assignmnet not given a value for D when LDR input are same, i get D =0 
    String(1, 2).toCharArray(lightAr, 6);
    calculateAngle(1,D);
  }
  
}


//(1) 
void setupWifi(){
  Serial.println();
  Serial.print("Connecting to");//(1)connecting wokwi platform
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "");

  //WHILE LOOP is use to wait until wifi is connected
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  //After connection
  Serial.println("WiFi connected");
  Serial.println("IP adress:");
  Serial.println(WiFi.localIP());

}


//(7)schedule checking
void checkSchedule(){
  if(isScheduledON){
    unsigned long currentTime = getTime();
    if(currentTime > scheduledonTime){
      buzzerOn(true);
      isScheduledON = false;
      mqttClient.publish("ON-OFF-ESP-MY", "1");
      mqttClient.publish("SCH-ESP-ON-MY", "0");
      Serial.println("Scheduled ON");
    }
  }

}

//(8)
void calculateAngle(double lightintensity,double D){
  if(select_medicine == 1){ //check is the select medicine == A 
    double angle = minAngle_A *D +(180.0-minAngle_A)*lightintensity*controlFac_A ;// Calculate the angle based on light intensity,minimum an gle an the controll factor
    Serial.println(angle);
    motor.write(angle);// Set the angle of the servo motor to adjust the shaded sliding window  
  }
  else if(select_medicine == 2){//check is the select medicine == B 
    double angle = minAngle_B *D +(180.0-minAngle_B)*lightintensity*controlFac_B;// Calculate the angle based on light intensity,minimum an gle an the controll factor
    Serial.println(angle);
    motor.write(angle);// Set the angle of the servo motor to adjust the shaded sliding window  

  }
  else if(select_medicine == 3){ //check is the select medicine == B 
    double angle = minAngle_C *D +(180.0-minAngle_C)*lightintensity*controlFac_C;// Calculate the angle based on light intensity,minimum an gle an the controll factor
    Serial.println(angle);
    motor.write(angle);// Set the angle of the servo motor to adjust the shaded sliding window  
  }
  else{
    double angle = minAngle *D +(180.0-minAngle)*lightintensity*controlFac; // Calculate the angle based on light intensity,minimum an gle an the controll factor
    Serial.println(angle);
    motor.write(angle);// Set the angle of the servo motor to adjust the shaded sliding window  
  }

  
  
}


//(1) - setup the wifi connection to our application
//(2) - get DHT22 sensor and get the temperature
//(3) - setup the mqtt
//(4) - publish the temperature 
//(5) -  set up the buzzer to our application
//(6) - get message from user
//(7) - get time to our application
//(8) - get the motor angle based on medicine type
//(9) - check light intensity
