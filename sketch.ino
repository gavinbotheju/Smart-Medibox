#include <PubSubClient.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

//Initiating parameters for the DHT22 sensor
DHTesp dhtsensor;
const int DHT_PIN = 15;

char tempAr[6];
char humidAr[6];
char ldrAr[6];

//Initializing parameters for pins
#define BUZZER 25
#define LDRL 34
#define LDRR 35
#define SERVO 19

#define LED_BUILTIN 16

Servo servo;

//Getting the wificlient to initiate (MQTT)pubsubclient library
WiFiClient espClient;
//Creating an instance of pubsubclient library
PubSubClient mqttClient(espClient);

//initializing time parameters
#define NTP_SERVER     "pool.ntp.org"
int UTC_OFFSET = 5*60*60 + 30*60;;
#define UTC_OFFSET_DST 0

//Taking at instance of wifiudp to initiate ntp client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Initiating parameters related to scheduling time
bool isScheduleON = false;
unsigned long scheduledOnTime;

//Initialization parameters for ldr and servo
int minAngle = 30;
float cF = 0.75;
float D;
float intensity;
int orientation = 0;


void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();

  dhtsensor.setup(DHT_PIN, DHTesp::DHT22);
  servo.attach(SERVO, 500, 2400);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(LDRL, INPUT);
  pinMode(LDRR, INPUT);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  
  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {

  //Check if connection to mqtt client is established and connecting
  if (!mqttClient.connected()) {
    connectToBroker();
  }

  //Looping through MQTT client(to handle mqtt functions)
  mqttClient.loop();

  updateTemperature();
  //Serial.println(tempAr);
  delay(1000);

  
  changeLightIntensity();
  changeShadedSlidingWindow();
  
  mqttClient.publish("ENTC-TEMP_", tempAr);
  mqttClient.publish("ENTC-HUMID", humidAr);
  mqttClient.publish("LDR_MAX", ldrAr);
  delay(1000);

  checkSchedule();
  delay(500);

  

 
}


//Function to connect to wifi
void setupWifi() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "");

  //Checking the wifi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //Printing for convenience
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

//Function to setup MQTT
void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);

  //To get messages from NodeRed
  mqttClient.setCallback(receiveCallback);

}

//Connecting to mqtt  
void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print ("Attempting MQTT connection...");
      //Checking for authentication(only if required), random ID used here  
      //Checking if connection is successful  
    if (mqttClient.connect ("ESP-8266-000789458")) { 
      Serial.println("connected");

      //Get messages from NodeRed
      mqttClient.subscribe ("MEDIBOX-ON-OFF");
      mqttClient.subscribe ("MEDIBOX-SCH-ON");
      mqttClient.subscribe ("LDR_Ranged");
      mqttClient.subscribe ("minAngle");
      mqttClient.subscribe ("controlFactor");


    }else {
      Serial.print ("failed");
      Serial.print (mqttClient.state()); 
      delay (5000);
    }
  }  
}

//Function to get temperature from DHT22
void updateTemperature() {
  TempAndHumidity data = dhtsensor.getTempAndHumidity();
  String(data.temperature,2).toCharArray(tempAr, 6);
  String(data.humidity,2).toCharArray(humidAr, 6);

}

// To get a message from the dashboard
void receiveCallback(char* topic, byte* payload, unsigned int length) { 
  Serial.print ("Message arrived [");
  Serial.print (topic);
  Serial.print("] ");

  char payloadCharAr[length];

  for (int i = 0; i < length; i++) { 
    Serial.print((char) payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }

  Serial.println();

  //Getting confirmed what the topic is
  if (strcmp(topic, "MEDIBOX-ON-OFF") == 0) {
    if(payloadCharAr[0] == 't' ) {
      digitalWrite(LED_BUILTIN, HIGH);
      //tone (BUZZER, 256);
      delay(500);
      //noTone(BUZZER);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      noTone(BUZZER);
    }
    //buzzerOn(payloadCharAr[0] == 1);
  }else if (strcmp(topic, "MEDIBOX-SCH-ON") == 0) {
    if (payloadCharAr[0] == 'N'){
      isScheduleON =false;  
    }else{
      isScheduleON = true ;
      scheduledOnTime =atol(payloadCharAr);
    }
  }

  if(strcmp(topic,"minAngle") == 0){
    minAngle = atol(payloadCharAr);
  }

    if(strcmp(topic,"controlFactor") == 0){
    cF = atol(payloadCharAr);
  }

    if(strcmp(topic,"LDR_Ranged") == 0){
    intensity = atol(payloadCharAr);
  }


  
} 

//Function for buzzer
void buzzerOn(bool on){
  if (on) {
    tone(BUZZER, 256);
  }else {
    noTone(BUZZER);
  }

}

unsigned long getTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}

//Function to check schedule from Node Red
void checkSchedule() {
  if (isScheduleON){
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime) {
      //buzzerOn(true)
      tone(BUZZER, 256);
      delay(5000);
      tone(BUZZER, 256);
      isScheduleON =false;
      Serial.println("Scheduled ON") ;
      mqttClient.publish("MAIN-SWITCH-ESP", "1");
      mqttClient.publish("SCHEDULE-SWITCH-ESP", "0");
    }
  }
}

void changeLightIntensity(){
   // Sensor Reading initialization
   long sensorValueL=4095-analogRead(LDRL);
   long sensorValueR=4095-analogRead(LDRR);
   

   // Max sensor reading
   long ldrVal = max(sensorValueL,sensorValueR);
   String ldrValString = String(map(ldrVal, 4095, 0, 1024, 0));
   
   if (sensorValueL<sensorValueR){
     ldrValString = 'R' + ldrValString;
     D=0.5;
   }else{
     ldrValString = 'L' + ldrValString;
     D=1.5;
   }
   ldrValString.toCharArray(ldrAr, 6);
}

void changeShadedSlidingWindow(){

   int newOrientation = round(minAngle*D+(180-minAngle)*intensity*cF);
    
   orientation = (min(newOrientation,180));
   
   servo.write(orientation);
   delay(15);
 }