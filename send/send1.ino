#include <XBee.h>
#include <SoftwareSerial.h>


XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

uint8_t dbCommand[] = {'D','B'};
AtCommandRequest atRequest = AtCommandRequest(dbCommand);

ZBTxStatusResponse txStatus = ZBTxStatusResponse();
AtCommandResponse atResponse = AtCommandResponse();


// Define NewSoftSerial TX/RX pins
// Connect Arduino pin 2 to Tx and 3 to Rx of the XBee
// I know this sounds backwards, but remember that output
// from the Arduino is input to the Xbee
#define ssRX 2
#define ssTX 3
SoftwareSerial nss(ssRX, ssTX);
SoftwareSerial xbeeSerial(2,3);

int red=4;
int blue=6;
int green=5;
int switchState = 0;
uint8_t id = '3';
int id_list[5];
unsigned long id_time[5];
int leaderID = 0;
bool leader = true;
int max=0;

void setup() {  
  // start serial
  Serial.begin(9600);
  xbeeSerial.begin(9600);
  xbee.setSerial(xbeeSerial);
  Serial.println("Initializing...");
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(red, OUTPUT);

  pinMode(8, INPUT);
  digitalWrite(green,HIGH);
}

void loop() 
{


//       Serial.println("!!!!!!!!!!!!");
      /*Send Message*/
      uint8_t payload[] = {'Y', id};
      // Specify the address of the remote XBee (this is the SH + SL)
      XBeeAddress64 addr64 = XBeeAddress64(0x0, 0x0000FFFF);
      // Create a TX Request
      ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
      // Send your request
      xbee.send(zbTx);
//      }

      
      delay(300);

      /*Receive Data*/
      xbee.getResponse().getZBRxResponse(rx);

      
     for (int i= 0; i < rx.getDataLength(); i++){
        if (rx.getData()[i] == 'Y' )
        {   
           for (int j= i; j < i+2; j++)   
           {
              uint8_t data = rx.getData()[j];
              
              if (data == '1')
              {
                delay(100);
//                id_list[1] = 1;
//                id_time[1] = millis();
//                Serial.write(data);
                Serial.println("Received id 1");
                digitalWrite(green, LOW);
                digitalWrite(red, LOW);
                digitalWrite(blue, HIGH);
//                Serial.print(id_list[1]);
//                Serial.print(id_time[1]);
                //Serial.write(millis());
              }
              if (data == '2')
              {
//                id_list[2] = 2;
//                id_time[2] = millis();
//                Serial.write(data);
                Serial.println("Received id 2");
                digitalWrite(green, LOW);
                digitalWrite(red, LOW);
                digitalWrite(blue, HIGH);
//                Serial.write(id_list[2]);
//                Serial.write(id_time[2]);
              }
              if (data == '3')
              {
//                id_list[3] = 3;
//                id_time[3] = millis();
//                Serial.write(data);
                Serial.println("Received id 3");
                digitalWrite(green, LOW);
                digitalWrite(red, LOW);
                digitalWrite(blue, HIGH);
//                Serial.print(id_list[3]);
//                Serial.print(id_time[3]);
              }
              if (data == '4')
              {
//                id_list[4] = 4;
//                id_time[4] = millis();
//                Serial.write(data);
                Serial.println("Received id 4");
//                Serial.print(id_list[4]);
//                Serial.print(id_time[4]);
              }
           }
           break;
        }
      }
      Serial.println();

}
