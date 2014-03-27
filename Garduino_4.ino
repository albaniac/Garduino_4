// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include <SoftwareSerial.h>
#include "RTClib.h"
#include "DHT.h"
#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

RTC_DS1307 rtc;
SoftwareSerial lcdSerial(10, 11);//rx,tx


//declare pins for sensors, actuators
const int T[2]={A0,A1};
const int lights=2;
const int heater=3;
const int fan=4;
const int pins[3]={2,3,4};
const int startHour=7;
const int endHour=21;
float t1Raw=0.0;
float t2Raw=0.0;
volatile int States[3]={LOW,LOW,LOW};//lights, heater, fan
int lStates[3];
float lowSetp=23.0;
float highSetp=25.0;
const int faultSetp=30;
volatile float tState; //previous temperature state. Us
volatile float hState; //previous humidity state.
float hum;
float tem;
int dhtFaultCnt;
const int faultLim=10; // Will allow 10 bad readings from the DHT22 before it goes into fault mode.



void setup () {
  Serial.begin(9600);
  lcdSerial.begin(9600);
  lcdSerial.write(0xFE);
  lcdSerial.write(0x7c);
  lcdSerial.write(140);
  dht.begin();
  dhtData();
  
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  
  pinMode(T[0], INPUT);
  pinMode(T[1], INPUT);
  pinMode(lights, OUTPUT);
  digitalWrite(lights, LOW);
  pinMode(heater, OUTPUT);
  digitalWrite(heater,LOW);
  pinMode(fan, OUTPUT);
  digitalWrite(fan, LOW);
  delay(1500);
}

void loop () {
  //dhtData();
  //pdelay(300);
  lcdSerial.write(0xFE);
  lcdSerial.write(0x01);
  currentStates();
  timeCheck();
  dateTime();
  checkGarden();
  compareStates(); 
  delay(5000);
  Serial.println();
    }
    
    
 void dateTime(){
   DateTime now = rtc.now();
    //Serial.print(now.year(), DEC);
    //Serial.print('/');
    //Serial.print(now.month(), DEC);
    //Serial.print('/');
    //Serial.print(now.day(), DEC);
    //Serial.print(' ');

    Serial.print(now.hour(), DEC);
    lcdSerial.print(now.hour(), DEC);
    Serial.print(':');
    lcdSerial.print(':');
    Serial.print(now.minute(), DEC);
    lcdSerial.print(now.minute(), DEC);
    Serial.print(':');
    lcdSerial.print(':');
    Serial.print(now.second(), DEC);
    lcdSerial.print(now.second(), DEC);
    Serial.print(",");
 
 }
 
 void checkGarden(){
   //dhtData();
  float currentTemp= dhtData();
  lcdSerial.write(0xFE);
  lcdSerial.write(137);
  lcdSerial.print("T=");
  Serial.print("Temp=");
  lcdSerial.print(currentTemp);
  Serial.print(currentTemp);
  Serial.print(", ");
  lcdSerial.write(0xFE);
  lcdSerial.write(192);
  lcdSerial.print("S:");
  Serial.print("States=");
  Serial.print(States[0]);
  Serial.print(States[1]);
  Serial.print(States[2]); 
  lcdSerial.print(States[0]);
  lcdSerial.print(States[1]);
  lcdSerial.print(States[2]); 
  //Serial.write(0xFE);
  //Serial.write(202);
  tempCheck(currentTemp);
  
  
}


//read temperature
int rawTemp(int pin){
  int result;
  result= analogRead(pin);
  return result;
}


//this is a function to scale the analog input per the ADC
float ADConv(int raw){
  float conv;
  conv= raw *(5/1024.0);
  return conv;
}


//this is the bit of code that converts the mV to degrees C
int convert(float raw){
  float temp = raw -.5;
  temp = temp/.01;
  return temp;
}

// this function returns the average temp.
float avgTemp(){
  int raw1;
  int raw2;
  int sum;
  int t1p;
  int t2p;
  raw1= rawTemp(T[0]);
  t1p= convert(ADConv(raw1));
  ;
  raw2= rawTemp(T[1]);
  t2p= convert(ADConv(raw2));
  Serial.print("T1=");
  Serial.print(t1p);
  Serial.print("T2=");
  Serial.print(t2p);
  sum = raw1+raw2;
  int average= sum/2;
  int temp;
  temp= convert(ADConv(average));
  return temp;
  
}



void currentStates(){
  for(int i=0; i<3; i++){
    lStates[i]=States[i];
    States[i]= digitalRead(pins[i]);
  }
}

//Do this if the temperature gets too low
void lowTemp(){
  //lStates[1]=States[1];
  //lStates[2]=States[2];
  float h = dht.readHumidity();
  //Serial.print("h=");
  //Serial.print(h);
  if(h>60){
  States[1]= HIGH;//turn heater state to high
  States[2]=HIGH;
  }
  else if(h>40&&h<50){
  States[1]=HIGH;
  States[2]=LOW;//turn fan state to low
  }
  else{
  Serial.print("time to water");
  }  
  compareStates();
}

//do this if the temperature gets too high
void highTemp(){
  States[1]=LOW; // set the heater state to low
  States[2]=HIGH;// set the fan state to high.
  compareStates();
}

void changeState(int i){
  //States[i]= !States[i];
  digitalWrite(pins[i], States[i]);
  
}

void compareStates(){
  for(int i=0; i<3; i++){
    int x = States[i];
    int y = lStates[i];
    if(x != y){
      changeState(i);
    }
  }
}

void tempCheck(float i){
  if(i<lowSetp){

    lcdSerial.write(0xFE);
    lcdSerial.write(204);
    lcdSerial.print("LOW");
    Serial.print("LOW");
    lowTemp();
    
  }
  
   else if(i>=faultSetp){
    //noInterrupts();
    fault();
    //interrupts();
  }
    
  else if(i>=highSetp && i<faultSetp){
    lcdSerial.write(0xFE);
    lcdSerial.write(204);
    lcdSerial.print("HIGH");
    Serial.print("HIGH");
    highTemp();
    //digitalWrite(fan, HIGH);
} 
  
 

  else{
  lcdSerial.write(0xFE);
  lcdSerial.write(204);
  lcdSerial.print("GOOD");  
  Serial.print("GOOD");
  States[1]=LOW;
  States[2]=LOW;
  compareStates(); 
  }
}

void fault(){
  
  Serial.println("Fault");
  for (int i=0; i<3; i++){
  digitalWrite(pins[i], LOW);
  }
  while(Serial.available() == 0){
   
    for(int i=0; i>0; i++){
      Serial.print("FAULT ,");
      Serial.print("STATES: ");
      for(int i=0; i<3;i++){
        Serial.print(States[i]);
        Serial.print(", ");
      }
      Serial.println();    
      
    }
  }
}


void timeCheck(){
  DateTime now = rtc.now();
  int nowHour;
  nowHour=now.hour();
  if(nowHour>=3 && nowHour<21){
    States[0]=HIGH;
    compareStates();
  }
  else{
  States[0]=LOW;
  compareStates();
  }
}

float dhtData() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  hum=h;
  tem=t;  

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    //h=hState;
    //t=tState;
    Serial.print("Humidity: "); 
    Serial.print(hState);
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(tState);
    Serial.print(" *C");
    dhtFaultCnt ++;
    Serial.print("f:");
    Serial.println(dhtFaultCnt);
    if (dhtFaultCnt>faultLim){
      fault();
    }
    
  } 
  
    else {
    Serial.print("Humidity: "); 
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(t);
    hum = h;
    tem = t;
    Serial.println(" *C");
    return t;
    tState=tem;
    hState=hum;
    dhtFaultCnt=0;
  }
}


