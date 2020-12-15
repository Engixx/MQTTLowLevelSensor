
/*********************************************************************
 *   autor: Aleksandr Sidorov
 *   ver. 1.0.1 - December - 2020 (test version)
 *   device: LoLin NodeMCU v.3 and ifm wireless KQ6005 Low-Level Sensor
 *   GND - D5 - Reset
 *   GND - A0 -> Voltage devided to 2 ->  LevelContact
 *   
 *   WiFiNetwork: ______________
 *   Password:    ______________
 *   - WiFiManager allows you to connect your ESP8266 to different Access Points (AP) without having to hard-code and upload new code to your board 
 *   - MQTT manager is sended the message to the MQTT brocker (engix.ie:1883 topic: IFM_LowLevelSensor_001
 *   - Remoute update from the engix.ie:5000 web-server
 *********************************************************************/


#define USING_AXTLS
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
 //#include "Gsender.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ArduinoMqttClient.h>
//#include <ArduinoOTA.h>
#define True 1
#define False 0

#define VERSION "v1.0.3"
#define HOST "IFM_LowLevelSensor_002"

int InteragationOTAUpdate = 60000; // The iteragation interval for check OTA update (ms)
//int InteragationOTAUpdate = 6000; // The iteragation interval for check OTA update (ms)

const char* urlUpdateServer = "http://engix.ie:5000/update";

#pragma region Globals

uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
bool FlagUpdate;
#pragma endregion Globals
int SWarray[]={1, 1, 1, 1, 1};
WiFiManager wifiManager;
// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate 
//    flashed in the WiFi module.
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
const char broker[] = "engix.ie";
int        port     = 1883;
const char topic[]  = "IFM_LowLevelSensor_002";

// ---- SW ----------------------------------------------------------------------------------------------

int SWpin =  D2;    //SW to LowLevel sensor
int IFMpin = A0;

int SWValue = 1;   // SW_Value = 0 if tunk full (contact close)
                   // SW_Value = 1 if tunk empty (contact open)

// ---- Reset Confi--------------------------------------------------------------------------------------

int EraseMemorypin =   D5;         // Reset config pin
int ResetValue = 0;
int count = 0;

// ---- FlagMsg -----------------------------------------------------------------------------------------

bool FlagMsgSended = true;                //In futer version will be store in memory
bool FlagMsgTestSended = false;           //In futer version will be store in memory

//long MinMsgSenderPeriod = 3600;           // The minimum time period between message [number of interagation intervals]. (production settings equvalent 30 hours) 
long MinMsgSenderPeriod = 10;           // The minimum time period between message. (test settings)

//long InteragationInterval = 30000;        // interragaition interval [msec]. (production settings) by default 30 sec.
long InteragationInterval = 10000;       // interragaition interval [msec]. (test settings) 

long previousMillis = 0;
long LastUpdateMillis = 0;
long CounterMsgSenderPeriod = MinMsgSenderPeriod - 5;       // The minimum time period between message [sec].

// ---- WiFiConnect -------------------------------------------------------------------------------------

uint8_t WiFiConnect()
{

    Serial.println("Connection: ESTABLISHED");
    wifiManager.autoConnect();
    Serial.print("Got IP address: ");
    Serial.println(WiFi.localIP());
    return true;
}

void(* resetFunc) (void) = 0; // объявляем функцию reset

void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}

/***************************************************/
void checkForUpdates(void)
{
  String checkUrl = String( urlUpdateServer);
  checkUrl.concat( "?ver=" + String(VERSION) );
  checkUrl.concat( "&dev=" + String(HOST) );

  Serial.println("INFO: Checking for updates at URL: " + String( checkUrl ) );
  mqttClient.beginMessage(topic);
        mqttClient.println("{"); 
        mqttClient.print("   {'Message':        '");    mqttClient.print("INFO: Checking for updates at URL: " + String( checkUrl ));       mqttClient.println("'}");
        mqttClient.println("}");
  mqttClient.endMessage();
  
  t_httpUpdate_return ret = ESPhttpUpdate.update( checkUrl );

  switch (ret) {
    default:
    case HTTP_UPDATE_FAILED:
      Serial.println("ERROR: HTTP_UPDATE_FAILD Error (" + String(ESPhttpUpdate.getLastError()) + "): " + ESPhttpUpdate.getLastErrorString().c_str());
      mqttClient.beginMessage(topic);
        mqttClient.println("{"); 
        mqttClient.print("   {'Message':        '");    mqttClient.print("ERROR: HTTP_UPDATE_FAILD Error (" + String(ESPhttpUpdate.getLastError()) + "): " + ESPhttpUpdate.getLastErrorString().c_str());       mqttClient.println("'}");
        mqttClient.println("}");
      mqttClient.endMessage();      
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("INFO: HTTP_UPDATE_NO_UPDATES");
      mqttClient.beginMessage(topic);
        mqttClient.println("{"); 
        mqttClient.print("   {'Message':        '");    
          mqttClient.print("INFO: HTTP_UPDATE_NO_UPDATES");       
        mqttClient.println("'}");
        mqttClient.println("}");
      mqttClient.endMessage();       
      break;
    case HTTP_UPDATE_OK:
      Serial.println("INFO status: HTTP_UPDATE_OK");
      mqttClient.beginMessage(topic);
        mqttClient.println("{"); 
        mqttClient.print("   {'Message':        '");    
          mqttClient.print("INFO status: HTTP_UPDATE_OK");       
        mqttClient.println("'}");
        mqttClient.println("}");
      mqttClient.endMessage();            
      break;
    }
}
/***************************************************/
void setup()
{
    SWValue = 0;
    Serial.begin(115200);

    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

            
    pinMode(SWpin, INPUT_PULLUP);
    pinMode(EraseMemorypin, INPUT_PULLUP);
    

// ---- WiFiConnect -------------------------------------------------------------------------------------
    connection_state = WiFiConnect();
    if(!connection_state)  // if not connected to WIFI
        Awaits();          // constantly trying to connect
     else
        wifiManager.setConfigPortalTimeout(60);

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser to see it working");
  
 // The message about sucessful connection to the local network
  
 /* Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
 String subject = "From Test: The device williamagrainger@gmail.com connected";
 if(gsender->Subject(subject)->Send("woodco.data@gmail.com", "The device williamagrainger@gmail.com (ver.3) connected.")) {
      Serial.println("The message about sucessful connection to the local network sended.");
 }
*/

      
// Attempting to connect to the MQTT broker

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  mqttClient.beginMessage(topic);
        mqttClient.println("{"); 
          mqttClient.print("   {'Message':     '");    mqttClient.print("Power ON.");        mqttClient.println("'}");
          mqttClient.print("   {'HOST':        '");    mqttClient.print(HOST);               mqttClient.println("'}");
          mqttClient.print("   {'VERSION':     '");    mqttClient.print(VERSION);            mqttClient.println("'}");
          mqttClient.print("   {'MAC':         '");    mqttClient.print(WiFi.macAddress());  mqttClient.println("'}");        
        mqttClient.println("}");
  mqttClient.endMessage();
}

void loop(){
  unsigned long currentMillis = millis(); 
   
  if(currentMillis < previousMillis) {previousMillis = currentMillis;}  // Reset timer about ones per 50 days to avoid overflow
  
  if((currentMillis - previousMillis) > InteragationInterval){          // The interagation timer (default value 30 sec.)
    FlagUpdate = True;
    previousMillis = currentMillis;   
  }
  else 
    FlagUpdate = False;
    
  
  
    
   if (FlagUpdate) {     
    int SW_sum = 0;
    Serial.print("[");
    for (int j = 0; j < 4; j++) 
    {
      SWarray[j] = SWarray[j+1];
      //Serial.print(" j=");Serial.print(j, DEC); Serial.print(" SWarray[j]=");
      Serial.print(SWarray[j], DEC);
      Serial.print(","); 
      SW_sum = SW_sum + SWarray[j];
    }
    SWarray[4] = digitalRead(SWpin); // Read the current value of SW from SWpin
    SW_sum = SW_sum + SWarray[4];
    Serial.print(SWarray[4], DEC);   Serial.print("]  ");Serial.print("SW_sum =");   Serial.println(SW_sum, DEC);
    Serial.print ("      CounterMsgSenderPeriod =");Serial.println (CounterMsgSenderPeriod);
    
    if (SW_sum < 1) SWValue = 0;
    if (SW_sum > 4) SWValue = 1;  
    if ((!SWValue) & (CounterMsgSenderPeriod >=MinMsgSenderPeriod )){    // If tank Empty long time -> release FlagMsgSended if counter exeded (default value 30 hours) 
      FlagMsgSended = false;     
      FlagMsgTestSended = false;  
    } 
 /*   
 // =============================================================================================
 // SEND MESSAGE "Low level"  if D4 ON
 // =============================================================================================
    
    if ((SWValue) & !FlagMsgSended & (CounterMsgSenderPeriod >=MinMsgSenderPeriod )) {   // If tank Full (SWValue 1) and Message was not sended 
      Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
      String subject = "From Test: Low Level warning message.";
      if(gsender->Subject(subject)->Send("woodco.data@gmail.com", "Bulk storage low-level williamagrainger@gmail.com warning.")) {
          FlagMsgSended = true;
          CounterMsgSenderPeriod = 0;
          Serial.println("Message send.");
//          if (gsender->Subject(subject)->Send("woodco.data@gmail.com", "Bulk storage low-level warning (ver.3)to williamagrainger@gmail.com was sended."))
//            Serial.println("Report to woodco.data@gmail.com was sended.");
      } else {
          Serial.print("Error sending message: ");
          Serial.println(gsender->getError());
          //FlagMsgSended = false;
      }   
    }  
    if (CounterMsgSenderPeriod < MinMsgSenderPeriod) CounterMsgSenderPeriod++;   
 
  }
  */
 // =============================================================================================
 //  HTTP update check procedure once for a one hour
 // =============================================================================================
  if(currentMillis < LastUpdateMillis) 
    LastUpdateMillis = currentMillis;  // Reset timer about ones per 50 days to avoid overflow
    
  if((currentMillis - LastUpdateMillis) > InteragationOTAUpdate) {
    Serial.println();
    Serial.println('starting chekForUpdates()');
    LastUpdateMillis = currentMillis; 
    checkForUpdates();
  }
  
 // =============================================================================================
 // RESET if D5 ON
 // =============================================================================================
       
  if (!digitalRead(EraseMemorypin))  {              // If D5 ON (Command to erase memory)
        if (WiFiConnect()) {
  /*          Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
            String subject = "From Test: The device ngrainger@gpwood.ie memory erased.";
            if(gsender->Subject(subject)->Send("woodco.data@gmail.com", "The device williamagrainger@gmail.com memory erased")) {
              Serial.println("E-mail message 'The device williamagrainger@gmail.com memory erased' sended.");
   */
      mqttClient.beginMessage(topic);
        mqttClient.println("{"); 
        mqttClient.print("   {'Message':        '");    mqttClient.print("Memory had been reseted.");       mqttClient.println("'}");
        mqttClient.println("}");
      mqttClient.endMessage();
   
        }
  }
  //wifiManager.resetSettings(); //  erase all the stored information if D5 on
  Serial.print("digitalRead(EraseMemorypin)");    
  Serial.println(!digitalRead(EraseMemorypin));
  //resetFunc();
  delay(1000);
  }
 // =============================================================================================
 // MQTT Publisher  
 // =============================================================================================
 
    mqttClient.poll(); // MQTT keep alives
    // Measure Signal Strength (RSSI) of Wi-Fi connection
   
    if (FlagUpdate) {
     // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
      mqttClient.println("{"); 
      mqttClient.print("   {'N':        '");    mqttClient.print(count);                  mqttClient.println("'}");
      mqttClient.print("   {'RSSI':     '");    mqttClient.print(WiFi.RSSI());            mqttClient.println("'}");
      mqttClient.print("   {'SWValue':  '");    mqttClient.print(SWValue);                mqttClient.println("'}");
      mqttClient.print("   {'IFMValue': '");    mqttClient.print(analogRead(IFMpin));     mqttClient.println("'}");
      mqttClient.println("}");
    mqttClient.endMessage();
   
    Serial.print("Sended message N"); Serial.println(count); Serial.print("to topic: ");  Serial.println(topic);
    count++;
    }

}
