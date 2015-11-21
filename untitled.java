#include <XBee.h>
#include <math.h>
#include <SoftwareSerial.h>

// Define baud rate here
#define BAUD 9600
// Create an xBee object
SoftwareSerial xbeeSerial(2,3); // Rx, Tx
// Looping Variables
int leaderID = -1;;
int final_id = -1;;

//identity of this node
uint8_t identity = 3;

boolean timeout_flag = false;
int timeout_count = 0;

//Timer
int checkLeader_timer = 0;
int election_timer = 0;
int leader_timer = 0;

//TimeOut Const
int election_timeout = 8;
int checkLeader_timeout = 8;
int leader_timeout = 5;

bool expireFlag = true; //new

XBee xbee = XBee();
XBeeResponse response  = XBeeResponse();

//create reusable objects for responses we expect to handle
ZBRxResponse rx = ZBRxResponse();

XBeeAddress64 broadcastAddr = XBeeAddress64(0x00000000, 0x0000FFFF);

void processResponse(){
  if (xbee.getResponse().isAvailable()) {
      // got something
      //xbee conntected
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
           // the sender got an ACK
            Serial.println("packet acknowledged");
        } else {
          Serial.println("packet not acknowledged");
        }
        int id = rx.getData()[0];
        Serial.println("GET ID : " + String(id));
        if (id == leaderID) {
          checkLeader_timer = 0;
        } else {
          election(id);
        }
      }
    } else if (xbee.getResponse().isError()) {
      Serial.print("error code:");
      Serial.println(xbee.getResponse().getErrorCode());
    } else {
      checkLeader();
    }
}

void checkLeader() {
  if (leaderID == identity.toInt()) {
    if (leader_timer == leader_timeout) {
      leader_timer = 0;
      leaderBroadcast();
    } else {
      leader_timer++;
    }
  } else if(checkLeaderExpire){
    //fix the bug when remove the rest Arduino but leave one
    checkLeader_timer = 0;
    broadcastMsg(identity);
  } else {
   checkLeader_timer++;
  }
}

void setup (){
  Serial.begin(BAUD);
  xbeeSerial.begin(BAUD);
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing transmitter...");
}

//rebroadcast leader id
void broadcastMsg(int w) {
  uint8_t value = (uint8_t)id;
  uint8_t payload[] = {value};
  ZBTxRequest zbTx = ZBTxRequest(broadcastAddr, payload, sizeof(payload));
  xbee.send(zbTx);
}

void leaderBroadcast() {
  uint8_t payload[] = {identity};
  ZBTxRequest zbTx = ZBTxRequest(broadcastAddr, payload, sizeof(payload));
  xbee.send(zbTx);
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
    return;
  }
  leaderID = -1;
  if (id > final_id) {
    final_id = id;
    election_timer = 0;
    broadcastMsg(final_id);
  } else {
    if (election_timer >= election_timeout){
      election_timer = 0;
      timeout_count = 0;
      timeout_flag = true;
      leaderID = final_id;
      final_id = -1;
    } else {
      election_timer++;
      broadcastMsg(final_id);
    }
  }
}

void loop(){
//  sendTx(zbTx);
  delay(1000);
  xbee.readPacket();
  processResponse();
}
