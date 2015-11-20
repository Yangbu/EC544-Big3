#include <XBee.h>
#include <math.h>
#include <SoftwareSerial.h>

// Define baud rate here
#define BAUD 9600
// Create an xBee object
SoftwareSerial xbeeSerial(2,3); // Rx, Tx

// Looping Variables
boolean isLeader;
int leaderID;
int final_id;

int identity = 1;

boolean timeout_flag = false;
int timeout_count = 0;

int checkLeader_timer = 0;
int election_timer = 0;
int leader_timer = 0;

int election_timeout = 8;
int checkLeader_timeout = 8;
int leader_timeout = 5;

bool expireFlag = true; //new

uint8_t BEACON_ID = 1;

XBee xbee = XBee();

XBeeResponse response  = XBeeResponse();

//create reusable objects for responses we expect to handle

ZBRxResponse rx = ZBRxResponse();

ZBTxStatusResponse txStatus = ZBTxStatusResponse();
AtCommandResponse atResponse = AtCommandResponse();

XBeeAddress64 broadcastAddr = XBeeAddress64(0x00000000, 0x0000FFFF); 

void processResponse(){
  if (xbee.getResponse().isAvailable()) {
      // got something
      //xbee conntected
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
        // if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
        //     // the sender got an ACK
        //     Serial.println("packet acknowledged");
        // } else {
        //   Serial.println("packet not acknowledged");
        // }
        
        int id = rx.getData()[0];
        if (id == leaderID) {
          checkLeader_timer = 0;
        } else {
          election(id);
        }
      }
    } else if (xbee.getResponse().isError()) {
      Serial.print("error code:");
      Serial.println(xbee.getResponse().getErrorCode());
    }
}

void setup (){
  Serial.begin(BAUD);
  xbeeSerial.begin(BAUD);
  isLeader = false;
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing transmitter...");
}

//rebroadcast leader id
void broadcastMsg(int id) {
  uint8_t payload[] = {id};
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
    rebroadcastMsg(final_id);
  } else {
    if (election_timer >= election_timeout){
      election_timer = 0;
      timeout_count = 0;
      timeout_flag = true;
      leaderID = final_id;
    } else {
      election_timer++;
      rebroadcastMsg(final_id);
    }
  }
}

void loop(){
//  sendTx(zbTx);
  delay(1000);
  xbee.readPacket();
  processResponse();
  if (leaderID == identity.toInt()) {
    if (leader_timer == leader_timeout) {
      leader_timer = 0;
      leaderBroadcast();
    } else {
      leader_timer++;
    }
  } else if(checkLeader_timer >= checkLeader_timeout){
    //fix the bug when remove the rest Arduino but leave one
    checkLeader_timer = 0;
    Serial.println("Leader ID : "+String(leaderID));
  }else {
   checkLeader_timer++;
   Serial.println("checkLeader_timer : " + String(checkLeader_timer) + "election_timer : " +  election_timer);
   if (checkLeaderExpire()) {
     if (election_timer < election_timeout) {
//          Serial.println("here6");
       rebroadcastMsg(final_id);
       election_timer++;
     } else {
       // election_timer = 0
       leaderID = final_id;
     }
   }
 }
}
