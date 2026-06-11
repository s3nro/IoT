
// change pin number based on esp32
int trigpin=1;
int echopin=2;
int yellowpin=4;
int redpin=5;
int greenpin=9;
int trigpin2=7;
int echopin2=8;
float t; //time of first sensor
int d;//distance of first sensor
int duration;
int relaypin=6;// for pump,relay should output from NO
float t2;// time of 2nd sensor
int d2;// distance of 2nd sensor
bool RFID = false;
unsigned long present_time=0;
unsigned long interval = 3000;

void setup(){

  pinMode(trigpin,OUTPUT);
  pinMode(echopin,INPUT);

  pinMode(yellowpin,OUTPUT);
  pinMode(redpin,OUTPUT);
  pinMode(greenpin,OUTPUT);

  pinMode(relaypin,OUTPUT);

  pinMode(trigpin2,OUTPUT);
  pinMode(echopin2,INPUT);
  
  digitalWrite(redpin,1);
  delay(50);
  digitalWrite(yellowpin,1);
  delay(50);
  digitalWrite(greenpin,1);
  delay(50);
  digitalWrite(yellowpin,0);
  digitalWrite(greenpin,0);
  

  
 
}


bool sensor_read1(){
  // if it detects hand between 0 and 5cm then return true for this bool function else false
  digitalWrite(redpin,1);

  digitalWrite(trigpin,0);
  delayMicroseconds(2);
  digitalWrite(trigpin,1);
  delayMicroseconds(10);
  digitalWrite(trigpin,0);

  t=pulseIn(echopin,1);
  d=t*0.034/2;

  if (d >=0 && d<= 5){

    digitalWrite(redpin,0);
    digitalWrite(yellowpin,1);
    return true;
    }
 
  else{
    return false;
    }
}
void sensor_read2(){
  // if RFID is detected or true  then activate sensor 2 then gives alcohol if hand is within 5cm
  if (RFID ){
    digitalWrite(yellowpin,0);
    digitalWrite(greenpin,1);

    digitalWrite(trigpin2,0);
    delayMicroseconds(2);
    digitalWrite(trigpin2,1);
    delayMicroseconds(10);
    digitalWrite(trigpin2,0);

    t2=pulseIn(echopin2,1);
    d2=t2*0.034/2;
    if (d2>=0 && d2<= 5){
      
      if (present_time == 0){
        present_time=millis();
      }
      if (millis()-present_time>=interval){
        digitalWrite(relaypin,1);
        delay(1000);
        digitalWrite(relaypin,0);
        digitalWrite(greenpin,0);
        RFID=false;
        present_time=0;
      }
    }
    
    else{
      // no hand detected
    }
  }
  else{
      RFID = false;
      present_time = 0;
  }
}

void loop(){

  if (sensor_read1()==true){
    //If hand detected then checks rfid if true then proceed to second sensor to read if there is hand nearby 
      sensor_read2();
  }
  
  else{
    present_time=0;
  }

}
  
  

  
  







