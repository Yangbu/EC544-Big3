#include <XBee.h>
#include <math.h>
#include <SoftwareSerial.h>

// Define baud rate here
#define BAUD 9600
// Create an xBee object
SoftwareSerial xbeeSerial(2,3); // Rx, Tx
String identity = "";

// Looping Variables
boolean isLeader;
int leaderID;
int final_id;

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
ModemStatusResponse msr = ModemStatusResponse();

uint8_t command1[] = {'D','B'};  //command one
AtCommandRequest atRequest = AtCommandRequest(command1);

ZBTxStatusResponse txStatus = ZBTxStatusResponse();
AtCommandResponse atResponse = AtCommandResponse();

uint8_t payload[] = {'w','y'};
XBeeAddress64 broadcastAddr = XBeeAddress64(0x00000000, 0x0000FFFF); 
ZBTxRequest zbTx = ZBTxRequest(broadcastAddr, payload, sizeof(payload));

void processResponse(){
  if (xbee.getResponse().isAvailable()) {
      // got something

      //xbee conntected
      Serial.println("xbee.getResponse is available");
      Serial.println(xbee.getResponse().getApiId());
      Serial.println(ZB_RX_RESPONSE);     
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
      
        Serial.println("Got an rx packet!");
            
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
            // the sender got an ACK
            Serial.println("packet acknowledged");
        } else {
          Serial.println("packet not acknowledged");
        }
        
        Serial.print("checksum is ");
        //16 
        Serial.println(rx.getChecksum(), HEX);
        Serial.print("packet length is ");
        //10
        Serial.println(rx.getPacketLength(), DEC);
        
         for (int i = 0; i < rx.getDataLength(); i++) {
          Serial.print("payload [");
          Serial.print(i, DEC);
          Serial.print("] is ");
          Serial.println(rx.getData()[i], HEX);
        }
        Serial.println(xbee.getResponse().getFrameDataLength());
        
//       for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
//        Serial.print("frame data [");
//        Serial.print(i, DEC);
//        Serial.print("] is ");
//        Serial.println(xbee.getResponse().getFrameData()[i], HEX);
//      }
//        
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

// Read in the message
String readTheMsg() {
  String msg  = "";
  while(xbee.getResponse().isAvailable() > 0) {
//    char c = char(xbee.read());
//    if (c == '\n') {
      break;
    }
//    msg += c;
  }
//  Serial.println(msg);
//  return msg;
//}

//rebroadcast leader id
void rebroadcastMsg(int id) {
//  xbee.print(String(id) + ":Leader\n");
  Serial.println("Temp Leader :" + String(id));
}

void leaderBroadcast() {
//  xbee.print(identity+ ":Alive\n");
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

void election(String info, int id) {
  Serial.println("Electing...");
  if (checkElectionTimeOut()) {
    return;
  }
  if (id > final_id) {
    final_id = id;
    election_timer = 0;
    rebroadcastMsg(final_id);
    Serial.println("here2");
  } else {
    if (election_timer >= election_timeout){
      election_timer = 0;
      timeout_count = 0;
      timeout_flag = true;
      leaderID = final_id;
      Serial.println("here4");
    } else {
      election_timer++;
      rebroadcastMsg(final_id);
      Serial.println("here3");
    }
  }
}
void sendTx(ZBTxRequest zbTx){
  xbee.send(zbTx);

   if (xbee.readPacket(5000)) {
    Serial.println("Got a Tx status response!");
    // got a response!

    // should be a znet tx status              
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      xbee.getResponse().getZBTxStatusResponse(txStatus);

      // get the delivery status, the fifth byte
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        // success.  time to celebrate
        Serial.println("SUCCESS!");
      } else {
        Serial.println("FAILURE!");
        // the remote XBee did not receive our packet. is it powered on?
      }
    }
    else{
      Serial.println("ZB_TX_STATUS_RESPONSE false");
      Serial.println(txStatus.getDeliveryStatus());
    }
  } else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());
  } else {
    // local XBee did not provide a timely TX Status Response -- should not happen
    Serial.println("This should never happen...");
  }
}

void loop(){
  xbee.send(zbTx);
//  sendTx(zbTx);
  delay(1000);
  xbee.readPacket();
  processResponse();


  
}
