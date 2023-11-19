//LIBRARIES
#include <ESP8266WiFi.h>
#include <Espalexa.h>
#include <DallasTemperature.h>
#include <OneWire.h>


//PINS
#define ONE_WIRE_BUS 4      //D2 for temp sensor
#define RELAY_PIN    12     //D6 for relay pin


//CONSTANTS
#define MAX_HEAT_TIME 330000  //maximum heat time
#define MAX_TEMP_DELTA 1.8    //maximum delta for heating

#define LOW_TEMP_MARGIN 15 //margin on lowest temperature (41C)
#define HIGH_TEMP_MARGIN 3 //margin on highest temperature (100C)

#define TEMP_UPDATE_DELAY 500           //update temp every 1000/n times per sec
#define DELTA_UPDATE_DELAY 1000         //update delta every 1000/n times per sec
#define SAFETY_CHECK_UPDATE_DELAY 100  //safety check every 1000/n times per sec


//Global Objects
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Espalexa espalexa;


//Global Vars
const char* ssid = "DHome";                                                 //wifi name
const char* password = "23332330";                                          //wifi pass
String Device_1_Name = "YOBA_CHAINIK_666";                                  //device name
uint8_t sensAd[8] = { 0x28, 0x0A, 0x54, 0xC7, 0x58, 0x20, 0x01, 0x25  };    //temp sens address


//Function declaration
void alexaAction(uint8_t brightness);   //function called when alexa calls YOBA_CHAINIK_666
boolean connectWifi();                  //connects wifi


//Short functions//
void(* resetFunc) (void) = 0;                   //reset function
void p(String text){ Serial.println(text); }    //quick print function


void setup(){

  Serial.begin(9600);    //open serial
  delay(5000);
  
  Serial.println("Hello imma boil your ass :)");
  
  sensors.begin();      //begin sensors

  if (connectWifi()){  //try to connect wifi
    //wifi connected
    
    espalexa.addDevice(Device_1_Name, alexaAction);     //make device visible for alexa
    espalexa.begin();                                   //begin alexa flex
  }
  else{
    //wifi not connected
    
    Serial.println("Cannot connect to WiFi.");
    delay(1000);

    resetFunc(); //reboot
  }

  pinMode(RELAY_PIN, OUTPUT);   //set pinmode for relay
  turnOffPower();  

  Serial.println("READY");
}


bool heating = false;
unsigned long heatStartTime = 0;

float currentTemp = 0;
byte targetTemp = 0;
byte tempMargin = 10;

int cnt = 0;
void loop(){

  espalexa.loop(); // alexa communication

  updateTemp();
  safetyCheck();
  updateDelta();
  millisTimeoutCheck();
  

  if(heating){

    if((targetTemp - currentTemp <= tempMargin || currentTemp - targetTemp >= tempMargin) || (millis() - heatStartTime >= MAX_HEAT_TIME)){ //if temperature is alright to go OR heat timeout

      p("Heated");

      turnOffPower();
    }
  }  

  delay(10);
}


//updates temperature
unsigned long tempUpdateTimer = 0;
void updateTemp(){

  if(millis() - tempUpdateTimer >= TEMP_UPDATE_DELAY){

    tempUpdateTimer = millis();
    
    currentTemp = getTemperature();

    Serial.print("Current temp is "); Serial.print(currentTemp); Serial.println('C');
  }
}


//updates delta
unsigned long deltaTimer = 0;
float tempDelta = 0, prevTemp = 0;
void updateDelta(){

  if(millis() - deltaTimer >= DELTA_UPDATE_DELAY){

    deltaTimer = millis();

    tempDelta = currentTemp - prevTemp;
    prevTemp = currentTemp;

    Serial.print(tempDelta); Serial.println("C / sec");
  }
}


//perform safety check
unsigned long safetyTimer = 0;
void safetyCheck(){

  if(millis() - safetyTimer >= SAFETY_CHECK_UPDATE_DELAY){
    
    safetyTimer = millis();
    
    if(currentTemp < 10 || tempDelta >= MAX_TEMP_DELTA){

      p("Safety check failed");
      turnOffPower();
    }
    
    else{
      p("Safety check passed");

    }
  }
}

//millis runover handle
void millisTimeoutCheck(){

  if(!heating && millis() >= 4294967000) //reset
    resetFunc();
}


//read temp sens
float getTemperature(){

  sensors.requestTemperatures();  

  return sensors.getTempC(sensAd);
}


//turn on relay
void turnOnPower(){

    heating = true;

    digitalWrite(RELAY_PIN, HIGH);
}


//turn off relay
void turnOffPower(){

    heating = false;

    digitalWrite(RELAY_PIN, LOW);
}


//when allexa calls YOBA
void alexaAction(uint8_t brightness){
  
  targetTemp = map(brightness, 0, 255, 0, 100);

  if(targetTemp < 40){ //target Temp to low

    targetTemp = -1;

    turnOffPower();

    p("Target temp too low");
  }

  else if(targetTemp >= 40 && targetTemp <= 100){

    Serial.print("Target temp set to "); Serial.println(targetTemp);

    tempMargin = map(targetTemp, 40, 100, LOW_TEMP_MARGIN, HIGH_TEMP_MARGIN);
    Serial.print("Temp margin set to "); Serial.println(tempMargin);
    
    if(getTemperature() + tempMargin > targetTemp){

      p("Its already hot as fuck you boy");
      return;
    }
 
    turnOnPower();

    if(heatStartTime == 0 || millis() - heatStartTime >= MAX_HEAT_TIME) //if cd passed reset heat timer
      heatStartTime = millis();
  }
}

//connecta wifi yopta
boolean connectWifi(){
  
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }

  return state;
}
