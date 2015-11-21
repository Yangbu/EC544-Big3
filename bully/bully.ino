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

int checkLeader_timer = 0;
int election_timer = 0;
int leader_timer = 0;

int election_timeout = 3;
int checkLeader_timeout = 8;
int leader_timeout = 4;


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

void processResponse(){
  if (xbee.available()) {
    String msg = readTheMsg();
    String info = msg.substring(msg.indexOf(':') + 1);
    int id = msg.substring(0,msg.indexOf(':')).toInt();
    if (info == "Leader") {
      if (id == leaderID) {
        Serial.println("leader is alive");
        checkLeader_timer = 0;
      } else {
        election(id);
      }
    } 
  } else {
    checkLeader();
  }
}

void setup() {
  xbee.begin(BAUD);
  Serial.begin(BAUD);
  isLeader = false;
  identity = getIdentity();
  final_id = identity;
  leaderID = -1;
  Serial.println("My Identity is : "+ identity);
  xbee.flush();
  Serial.println("Setup Complete");
}

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

//rebroadcast leader id
void broadcastMsg(int id) {
  xbee.print(String(id) + ":Leader\n");
  Serial.println("Temp Leader :" + String(id));
}

void leaderBroadcast() {
  xbee.print(String(identity)+ ":Leader\n");
  Serial.println("New Leader :" + String(leaderID));
}

boolean checkLeaderExpire() {
  if (checkLeader_timer >= checkLeader_timeout || leaderID == -1) {
    leaderID = -1;
    return true;
  } else {
    return false;
  }
}

boolean checkElectionTimeOut() {
  if (timeout_flag) {
    if (timeout_count < 3) {
      timeout_count++;
    } else {
      timeout_flag = false;
      timeout_count = 0;
    }
  }
  return timeout_flag;
}

void election(int id) {
  Serial.println("Electing...");
  if (checkElectionTimeOut()) {
    Serial.println("election over");
    return;
  }
  leaderID = -1;
  if (id > final_id) {
    final_id = id;
    election_timer = 0;
    broadcastMsg(final_id);
    Serial.println("new candidate");
  } else {
    if (election_timer >= election_timeout){
      election_timer = 0;
      timeout_count = 0;
      timeout_flag = true;
      leaderID = final_id;
      final_id = identity;
      Serial.println("election timeout");
    } else {
      election_timer++;
      broadcastMsg(final_id);
       Serial.println("election continue" + String(election_timer));
    }
  }
}

void checkLeader() {
  if (leaderID == identity) {
    if (leader_timer == leader_timeout) {
      leader_timer = 0;
      leaderBroadcast();
    } else {
      leader_timer++;
    }
  } else if(checkLeader_timer==checkLeader_timeout){
        //fix the bug when remove the rest Arduino but leave one
        checkLeader_timer = 0;
        Serial.println("Leader ID : "+String(leaderID));
    }else {
      checkLeader_timer++;
      Serial.println("checkLeader_timer : " + String(checkLeader_timer) + "election_timer : " +  String(election_timer));
      if (checkLeaderExpire()) {
        if (election_timer < election_timeout) {
//          Serial.println("here6");
            broadcastMsg(final_id);
          election_timer++;
        } else {
          // election_timer = 0
          leaderID = final_id;
        }
      }
    }
}

void loop(){
  processResponse();
  delay(1500);
}

