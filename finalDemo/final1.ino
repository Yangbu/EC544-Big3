#include <XBee.h>
#include <math.h>
#include <SoftwareSerial.h>

// Define baud rate here
#define BAUD 9600
// Create an xBee object
SoftwareSerial xbee(2,3); // Rx, Tx
int identity;

// Looping Variables
boolean isLeader;
int leaderID;
int final_id;


boolean timeout_flag = false;
int timeout_count = 0;

//Counting timer
int checkingTime = 0;
int electionTime = 0;
int leaderTime = 0;

// End time of each process
int election_timeout = 3;
int checkLeader_timeout = 9;
int leader_timeout = 1;

// Declare port
int red = 4;
int green = 5;
int blue = 6;
int switchState = 0;
int infect = 0;

//AT command get id 
int getIdentity() {
  String s;

  // Enter configuration mode - Should return "OK" when successful.
  delay(1000);    // MUST BE 1000
  xbee.write("+++");
  delay(1000);    // MUST BE 1000
  xbee.write("\r");
  delay(100);

  // Get the OK and clear it out.
  while (xbee.available() > 0) {
    Serial.print(char(xbee.read()));
  }
  Serial.println("");

  // Send "ATNI" command to get the NI value of xBee.
  xbee.write("ATNI\r");
  delay(100);
  while (xbee.available() > 0) {
      s += char(xbee.read());
  }
  delay(100);

  // Exit configuration mode
  xbee.write("ATCN\r");
  delay(1000);

  // Flush Serial Buffer Before Start
  while (xbee.available() > 0) {
    Serial.print(char(xbee.read()));
  }
  Serial.println("");
  delay(100);

  return s.toInt();
}

//Response function
void processResponse(){
  if (xbee.available()) {
    String msg = readTheMsg();
    String info = msg.substring(msg.indexOf(':') + 1,msg.indexOf(':') + 2);
    int inf = msg.indexOf('V');
    int cur = msg.indexOf('C');
    String s = msg.substring(msg.indexOf('(') + 1);
    int id = msg.substring(0,msg.indexOf(':')).toInt();
    
    if(s=="N" || s=="A"){
       changeState(s);
    }
    if(inf==0){    
      infection();
    }
    if(cur==0){
      cure();
    }
    
    if (info == "L") {
      isLeader = false;
      digitalWrite(blue, LOW);
      checkingTime = 0;
      if (id == leaderID) {
        if (leaderID == identity) {
          leaderBroadcast();
        } else {
          Serial.println("leader is alive");
        }
      } else {
        election(id);
      }
    }
     
  } else {
    checkLeader();
  }
}

void changeState(String s){
  if(s=="A"){
    digitalWrite(red, LOW);
    digitalWrite(green,HIGH);
  }
  if(s=="N"){
    digitalWrite(red, HIGH);
    digitalWrite(green,LOW);
  }
}
//Infection function
void infection(){
  if(!isLeader){
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
    infect = 1;
  }
}

//Cure function
void cure(){
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  infect = 0;
}

void setup() {
  xbee.begin(BAUD);
  Serial.begin(BAUD);
  isLeader = false;
  
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(8, INPUT);
  digitalWrite(green,HIGH);
  
  identity = getIdentity();
  final_id = identity;
  leaderID = -1;
  Serial.println("My Identity is : "+ identity);
  xbee.flush();
  Serial.println("Setup Complete");
}

//ReadMessage Function
String readTheMsg() {
  String msg  = "";
  while(xbee.available() > 0) {
    char c = char(xbee.read());
    if (c == '\n') {
      break;
    }
    msg += c;
  }
  Serial.println(msg);
  return msg;
}

//Broadcast Template Leader Function
void broadcastMsg(int id) {
  if(infect==1){
      xbee.print(String(id) + ":Leader(Now infected)\n");
  }
  if(infect==0){
      xbee.print(String(id) + ":Leader(Alive)\n");
  }
  Serial.println("Temp Leader :" + String(id));
}

//Broadcast Final Leader Function
void leaderBroadcast() {
  xbee.print(String(identity)+ ":Leader\n");
  Serial.println("New Leader :" + String(leaderID));
  digitalWrite(blue, HIGH);
  digitalWrite(green, HIGH);
  digitalWrite(red, LOW);
  isLeader = true;
}

//Check Election Time
boolean checkElectionTimeOut() {
  if (timeout_flag) {
    if (timeout_count < 2) {
      timeout_count++;
    } else {
      timeout_flag = false;
      timeout_count = 0;
    }
  }
  return timeout_flag;
}

//Election Process
void election(int id) {
  Serial.println("Electing...");
  if (checkElectionTimeOut()) {
    Serial.println("election over");
    return;
  }
  leaderID = -1;
  if (id > final_id) {
    final_id = id;
    electionTime = 0;
    broadcastMsg(final_id);
    Serial.println("new candidate");
  } else {
    if (electionTime >= election_timeout){
      electionTime = 0;
      timeout_count = 0;
      timeout_flag = true;
      leaderID = final_id;
      final_id = identity;
      Serial.println("election timeout");
    } else {
      electionTime++;
      broadcastMsg(final_id);
      Serial.println("election continue" + String(electionTime));
    }
  }
}

//Check Current Leader Removed or not
void checkLeader() {
  if (leaderID == identity) {
    if (leaderTime >= leader_timeout) {
      leaderTime = 0;
      leaderBroadcast();
    } else {
      leaderTime++;
    }
  } else if(checkingTime >= checkLeader_timeout){
        //fix the bug when remove the rest Arduino but leave one
        checkingTime = 0;
        broadcastMsg(identity);
        Serial.println("Leader "+String(leaderID) + " is dead.");
        leaderID = -1;
    }else {
      Serial.println("checkingTime : " + String(checkingTime) + "electionTime : " +  String(electionTime));
      if (leaderID == -1) {
        checkingTime = 0;
        if (electionTime < election_timeout) {
          broadcastMsg(final_id);
          electionTime++;
        } else {
          electionTime = 0;
          leaderID = final_id;
          final_id = identity;
        }
      } else {
         checkingTime++;
      }
    }
}


void loop(){
  switchState=digitalRead(8);
  if(switchState==1){
    if(isLeader){
      xbee.println("Curing Potion is coming ");
      Serial.println("I'm curing others !!!");
    }else{
      xbee.println("Virus is coming");
      Serial.println("Infecting others!!!");
      digitalWrite(red, HIGH);
      digitalWrite(green, LOW);
      infect = 1;
      
    }
  }
  processResponse();
  delay(1000);
}

