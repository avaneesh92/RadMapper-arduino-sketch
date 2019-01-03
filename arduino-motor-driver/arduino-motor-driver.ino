/*
 * Serial motor driver
 *  Drives differential motors by accepting serial commands
 *   ******Hardware config *******************
 *   Arduino Pin 2 connected to RX via 330 ohm resister to enable wakeup on receive 
 *   
 *   Left motor on Arduino pin 4 and 5
 *   Right motor on Arduino pin 6 and 7
 *   
 *  Command format -> "\>CMD Speed Timeout<"
 *    "\" has to be added before start of each command to wake up arduino from sleep
 *    ">" marks start of command and "<" marks end of command
 *    Range for speed is 0 to 255
 *    Range for timeout is 1 to 10000 milli seconds (0 is treated as no timeout, keep motors running)
 *    
 *    Return value "OK" indicates that command is received and will be executed immediately
 *    Return value "ERROR number" indicates error number 
 *    
 *    ***** ERROR numbers*********
 *    1 => "Invalid Command"
 *    2 => "End of command not received within 100 ms"
 *    3 => "Invalid Speed value"
 *    4 => "Invalid Timeout value"
 *    
 *    ***** COMMANDS **************
 *    => ">FW speed timeout<" # Move forward
 *    => ">BW speed timeout<" # Move backward
 *    => ">RT speed timeout<" # Turn right with given speed
 *    => ">LT speed timeout<" # Turn left with given speed
 *    => ">STOP 0 0<"             # Stop moving
 */

//Headers
#include <avr/sleep.h>

int sendBackDelay = 100;
int speedupRampDelay = 3;
// Global variables
int MOTOR_R_P = 5;
int MOTOR_R_N = 4;
int MOTOR_L_P = 6;
int MOTOR_L_N = 7;

uint16_t timeout = 1;
uint8_t speedC = 0;

int inChar = 0;
String inSpeed = "";
String inCmd = "";
String inTimeout = "";
bool cmdComplete = false;
bool speedComplete = false;
bool timeoutComplete = false;
bool inCMDFlag = false;
bool invalidCmd = false;
int errNumber = 0;
bool err = false;

//Motor functions
void moveForward(int sp){
  digitalWrite(MOTOR_R_N,LOW);
  digitalWrite(MOTOR_L_N,LOW);
  //Apply Ramp for speedup
  for(int i=0;i <= sp; i++){
    analogWrite(MOTOR_R_P,i);
    analogWrite(MOTOR_L_P,i);
    delay(speedupRampDelay);
  }
}
void moveBackward(int sp){
  int spInt = 255 - sp;
  digitalWrite(MOTOR_R_N,HIGH);
  digitalWrite(MOTOR_L_N,HIGH);
  for(int i=255; i >= spInt; i--){
    analogWrite(MOTOR_R_P,i);
    analogWrite(MOTOR_L_P,i);
    delay(speedupRampDelay);
  }   
}
void turnRight(int sp){
  int spInt = 255 - sp;
  digitalWrite(MOTOR_R_N,LOW);
  analogWrite(MOTOR_R_P,sp);
  digitalWrite(MOTOR_L_N,HIGH);
  analogWrite(MOTOR_L_P,spInt); 

}
void turnLeft(int sp){
  int spInt = 255 - sp;
  digitalWrite(MOTOR_R_N,HIGH);
  analogWrite(MOTOR_R_P,spInt);
  digitalWrite(MOTOR_L_N,LOW);
  analogWrite(MOTOR_L_P,sp);
}
void stopMotors(void){
  digitalWrite(MOTOR_R_N,LOW);
  analogWrite(MOTOR_R_P,0);
  digitalWrite(MOTOR_L_N,LOW);
  analogWrite(MOTOR_L_P,0); 
}

//Sleep related functions

//INT0 ISR
void wakeUpOnPin2(void){
  //Wakeup due to activity on pin2 --> INT0
  //No action needed
}

//Arduino enters sleeping mode in this function and wakes up again from here
void sleepNow(void){
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Highest power saving mode
  sleep_enable();
  attachInterrupt(digitalPinToInterrupt(2),wakeUpOnPin2, LOW);
  sleep_mode();// here the device is actually put to sleep!!
  
  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(2));
}

/**********************Command processing functions ***************/

inline void okResp(void){
  Serial.println("OK");
  delay(sendBackDelay);
}
//Reset command receiver
void resetCmd(void){
  cmdComplete = false;
  speedComplete = false;
  timeoutComplete = false;
  inCMDFlag = false;
  inCmd = "";
  inSpeed = "";
  inTimeout = "";
}
//Process received command
void processCmd(void){
  Serial.print(inCmd);
  Serial.print(inSpeed);
  Serial.println(inTimeout);

  if(inCmd == "FW"){
    moveForward(speedC);
    okResp();
  }
  else if(inCmd == "BW"){
    moveBackward(speedC);
    okResp();
  }
  else if(inCmd == "LT"){
    turnLeft(speedC);
    okResp();
  }
  else if(inCmd == "RT"){
    turnRight(speedC);
    okResp();
  }
  else if(inCmd == "STOP"){
    stopMotors();
    okResp();
  }
  else{
    Serial.println(">ERROR 1<");
  }
  resetCmd();
}
//timeout handler. executed every loop
// Returns true if something is running
bool handleTimeout(){
  uint16_t now = millis();
  if(timeout != 0){//If timeout is zero keep the command running
    if(now > (timeout)){
      stopMotors();
      return false;
    } 
  }
  return true;
}
/************************** SETUP ******************/
void setup() {
  pinMode(2, INPUT); //INT0 pin
  pinMode(MOTOR_R_P,OUTPUT);
  pinMode(MOTOR_R_N,OUTPUT);
  pinMode(MOTOR_L_P,OUTPUT);
  pinMode(MOTOR_L_N,OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Rover Initialized and ready to receive commands");
}

/************************** LOOP ******************/
void loop() {
  
  digitalWrite(LED_BUILTIN, HIGH);// LED to indicate that arduino is awake
  delay(100); //Timeout to allow Serial port to receive whole command after wakeup

  bool packetRec = false;
  bool cmdSrtr = false;
  err = false;
  
  while(Serial.available() > 0){ 
   inChar = Serial.read();

   //Check start of cmd
   if((char)inChar == '>'){ 
    cmdSrtr = true;
   }
   else if((char)inChar == '<' && !cmdSrtr){ //Command received completed
    err = true;
   }
   
   else if(cmdSrtr ){
    invalidCmd = false;
    if(cmdComplete == false){//Get command
      if((char)inChar == ' '){ //First part of command received
        cmdComplete = true;
      }
      else{
        inCmd += (char) inChar;
      }
    }
    else if(speedComplete == false){//Get speed
      if((char)inChar == ' '){
        speedComplete = true;
        speedC = inSpeed.toInt();
      }
      else if(isDigit(inChar)){
        inSpeed += (char)inChar;
      }
      else{
        invalidCmd = true;
      }
    }
    else if(timeoutComplete == false){
      if((char)inChar == '<'){
        timeoutComplete = true;
        inCMDFlag = true;
        timeout = inTimeout.toInt();
        timeout += millis(); //Add current time
      }
      else if(isDigit(inChar)){
        inTimeout += (char) inChar;
      }
    }
   }
   
    delay(1); //wait before reading next character
  }
     
  //If new command is received , process it
  if(inCMDFlag){
     processCmd();
  }
  //If invalid condition detected reset cmd
  if(invalidCmd || err){
    invalidCmd = false;
    //Reset everything
    resetCmd();
    Serial.println(">ERROR 2<");
    delay(sendBackDelay);
  }
  //Handle timeout on commands
   if(! handleTimeout()){ //Timeout over and rover is stopped, We can sleep now
    digitalWrite(LED_BUILTIN, LOW);
    sleepNow();
   }
}
