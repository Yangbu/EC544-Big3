/*  Group 5
 *  EC544 Challenge 6
 *  InfectiousSwarm.ino
 */


#include <math.h>
#include <SoftwareSerial.h>

// Define baud rate here
#define BAUD 9600
// Create an xBee object
SoftwareSerial xbee(2,3); // Rx, Tx
String identity = "";

// Looping Variables
boolean isLeader;
int leaderID = -1;
int final_id;

// Function to read in the Thermometer NI value

int checkLeader_timer = 0;
int election_timer = 0;
int leader_timer = 0;

int election_timeout = 5;
int checkLeader_timeout = 10;
int leader_timeout = 3;

String getIdentity() {
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

  return s;
}

void setup() {
  xbee.begin(BAUD);
  Serial.begin(BAUD);
  isLeader = false;
  identity = getIdentity();
  final_id = identity.toInt();
  Serial.println("Setup Complete");
}

// Read in the message
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
void rebroadcastMsg(int id) {
  xbee.println(String(id) + ":Leader");
}

void leaderBroadcast() {
  xbee.println(identity+ ":Alive");
}

boolean checkLeaderExpire() {
  if (checkLeader_timer > checkLeader_timeout || leaderID == -1) {
    leaderID = -1;
    return true;
  } else {
    return false;
  }
}



void loop() {
  if (leaderID == identity.toInt()) {
    if (leader_timer == leader_timeout) {
      leader_timer = 0;
      leaderBroadcast();
    } else {
      leader_timer++;
    }
  } else {
    if (checkLeaderExpire()) {
      rebroadcastMsg(final_id);
    }
  }
  if (xbee.available() > 0) {
    String msg = readTheMsg();
    String info = msg.substring(msg.indexOf(":") + 1);
    int id = msg.substring(0,msg.indexOf(":")).toInt();
    if (info.equals("Leader")) {
      if (id > final_id) {
        final_id = id;
        election_timer = 0;
        rebroadcastMsg(final_id);
      } else if (election_timer < election_timeout){
        election_timer++;
        rebroadcastMsg(final_id);
      } else {
        election_timer = 0;
        leaderID = final_id;
      }
    } else if (info.equals("Alive") && leaderID == id){
      checkLeader_timer = 0;
    }
  }
  delay(500);
}

