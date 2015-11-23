#include <XBee.h>
#include <math.h>
#include <SoftwareSerial.h>

// Define baud rate here
#define BAUD 9600
// Create an xBee object
SoftwareSerial xbee(2,3); // Rx, Tx

//------------------------------------------------
//current node
int identity;
int status = 0;
int leaderID;
boolean isLeader;

// Looping Variables
int temp_id;


//Counting timer
int checkLeaderTimer = 0;
int electionTimer = 0;
int leaderTimer = 0;

// Timeout variables
// TODO election_timeout is a little bit too long , maybe try shorter like 2
int election_timeout = 2;
//TODO checkLeader_timeout is a little bit too long , maybe try shorter like 6
int checkLeader_timeout = 5;

int leader_timeout = 1;
boolean electionFinished = false;
int electionFinished_count = 0;

// Declare LED pin number
int red = 4;
int green = 5;
int blue = 6;

// input pin status
int switchState = 0;

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

//ReadMessage Function
String readTheMsg() {
  String msg  = "";
  while(xbee.available() > 0) {
    char c = char(xbee.read());
    // cut the msg by ‘\n’
    if (c == '\n') {
      break;
    }
    msg += c;
  }
  Serial.println(msg);
  return msg;
}

/*
  Response function :
  1. Get msg from other nodes
    (1). Infection msg : None-leaders should response
    (2). Curing msg : None-leaders should response
    (3). Leader msg:
      - (1). Sent from leader : checkLeaderTimer reset
      - (2). Sent from None-leaders: Having an election
  2. Don't get anything
    Check if leader is alive.
*/
void processResponse(){
  if (xbee.available()) {
    // Received MSG
    String msg = readTheMsg();
    String info = msg.substring(msg.indexOf(':') + 1);
    int id = msg.substring(0, msg.indexOf(':')).toInt();
    if (info == "Infection") {
      // Infection msg
      infection();
    } else if (info == "Curing") {
      // Cure msg
      cure();
      checkLeaderTimer = 0;
    } else {
      // Leader msg
      Serial.println(info);
      String othersStatus = info.substring(info.indexOf(':') + 1);
      info = info.substring(0, info.indexOf(':'));
      if (othersStatus == "1") {
        // Only update the status when someone is infected
        status = 1;
      }
      isLeader = false;
      checkLeaderTimer = 0;
      if (id == leaderID) {
        if (leaderID == identity) {
          leaderBroadcast();
        } else {
          // leader msg, sent from leader : leader is alive
          Serial.println("leader is alive");
        }
      } else {
        // leader msg, sent from None-leaders : election
        election(id);
      }
    }
  } else {
    // No msg received, check if leader is still alive
    checkLeader();
  }
}

//Infection function
void infection(){
  // only affect None-leaders
  if(!isLeader){
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
    // set the status to be 1 (infected)
    status = 1;
  }
}

//Cure function
void cure(){
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  status = 0;
}



/*
  Check if the election is finished
  If finished, the following two statments should be discarded
*/
boolean electionBuffer() {
  if (electionFinished) {
    if (electionFinished_count < 2) {
      electionFinished_count++;
    } else {
      electionFinished = false;
      electionFinished_count = 0;
    }
  }
  return electionFinished;
}

/*
  Election process
  If new candidate came in with a higher ID, it will refresh the timer
  When the election process is done, it will assain the new leader
*/
void election(int id) {
  Serial.println("Electing...");
  digitalWrite(blue, LOW);
  if (electionBuffer()) {
    Serial.println("election over");
    return;
  }
  leaderID = -1;
  if (id > temp_id) {
    // New candidate with higher ID came in
    temp_id = id;
    electionTimer = 0;
    broadcastMsg(temp_id);
    Serial.println("new candidate");
  } else {
    if (electionTimer >= election_timeout){
      // Election is over
      electionTimer = 0;
      electionFinished_count = 0;
      electionFinished = true;
      assignLeader();
      Serial.println("election timeout");
    } else {
      electionTimer++;
      broadcastMsg(temp_id);
      Serial.println("election continue" + String(electionTimer));
    }
  }
}

/*
  Assign the leader and also check out the status of current node
*/
void assignLeader() {
  leaderID = temp_id;
  temp_id = identity;
  Serial.println("Leader ID : "  + String(leaderID));
  if (leaderID == identity) {
    status = 0;
    digitalWrite(green, HIGH);
  } else {
    if (status == 1) {
      digitalWrite(red, HIGH);
      digitalWrite(green, LOW);
    } else {
      digitalWrite(green, HIGH);
      digitalWrite(red, LOW);
    }
  }
}

/*
  Check leader is alive or not.
  If leader is myself, broadcast "I am alive" msg to the other none-leaders
*/
void checkLeader() {
  if (leaderID == identity) {
    if (leaderTimer >= leader_timeout) {
      leaderTimer = 0;
      leaderBroadcast();
    } else {
      leaderTimer++;
    }
  } else if(checkLeaderTimer >= checkLeader_timeout){
        //fix the bug when remove the rest Arduino but leave one
        checkLeaderTimer = 0;
        leaderID = -1;
        broadcastMsg(identity);
        Serial.println("Leader "+String(leaderID) + " is dead.");
    }else {
      Serial.println("checkLeaderTimer : " + String(checkLeaderTimer) + "electionTimer : " +  String(electionTimer));
      if (leaderID == -1) {
        // leader is not assigned yet, election is not finished
        checkLeaderTimer = 0;
        if (electionTimer < election_timeout) {
          broadcastMsg(temp_id);
          electionTimer++;
        } else {
          electionTimer = 0;
          assignLeader();
        }
      } else {
         checkLeaderTimer++;
      }
    }
}

/*
  Broadcast Leader Infomation Function
  ID will be the temp_id
  Status will be the status of the current node
  The syntax is "ID:Leader:status"
*/
void broadcastMsg(int id) {
  xbee.print(String(id) + ":Leader:" + String(status) + "\n");
  Serial.println("Temp Leader :" + String(id) + " Status : " + String(status));
}

/*
  Leader Broadcast "I am alive." to all the other None-leaders
  The syntax is "ID:Leader:2" (2 means nothing to the none-leaders)
*/
void leaderBroadcast() {
  xbee.print(String(identity)+ ":Leader:2\n");
  Serial.println("New Leader :" + String(leaderID));
  digitalWrite(blue, HIGH);
  digitalWrite(green, HIGH);
  digitalWrite(red, LOW);
  isLeader = true;
}

// infecting other none-leaders
void infectOthers() {
  xbee.print(String(identity) + ":Infection\n");
  Serial.println("Infecting others!!!");
  digitalWrite(red, HIGH);
  digitalWrite(green, LOW);
  status = 1;
}

// curing other none-leaders
void cureOthers() {
  xbee.print(String(identity) + ":Curing\n");
  status = 0;
  Serial.println("Leader is curing others !!!");
}

// check the input :
//    if 0 : do nothing
//    if 1 : infection or cure
void checkStatus () {
  switchState = digitalRead(8);
  if(switchState == 1){
    if(isLeader){
      cureOthers();
    }else{
      infectOthers();
    }
  }
}

void setup() {
  xbee.begin(BAUD);
  Serial.begin(BAUD);

// Pin setup
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(8, INPUT);
  digitalWrite(green,HIGH);

  isLeader = false;
  identity = getIdentity();
  temp_id = identity;
  leaderID = -1;
  Serial.println("My Identity is : "+ String(identity));
  xbee.flush();
  Serial.println("Setup Complete");
}

void loop(){
  checkStatus();
  processResponse();
  delay(1000);
}
