# MQTTLowLevelSensor
IOT project. Low Level Sensor via LoLin NodeMCU V3(ESP12E) send the data to MQTT brocker. OverTheAir update is supported.

Project contains: 
- firmware for LoLin NodeMCU V3(ESP12E) (wemos.cc)
    - collect data from IFM electronica KQ6005 wireless sensor (pin D2);
    - ESP8266 WiFi Connection manager with fallback web configuration portal  (https://github.com/tzapu/WiFiManager)
    - Over the
- docker-compose file to deploy MQTT brocker
- docker-compose file for deploy OTA server

=================================================================================== Amazon Web Server ========
||    _______________________ Docker containers ____________________________________________                 ||
||    ||  ----------------------------------            --------------------------------- ||                 ||
||    ||  |  OTA - server                  |            | MQTT - brocker                | ||                 ||
||    ||  |  server: engix.ie              |            | server: engix.ie              | ||                 ||
||    ||  |  port:  5000                   |            | port: 1883                    | ||                 ||
||    ||  |  HOST:  IFM_LowLevelSensor_001 |            | topic: IFM_LowLevelSensor_001 | ||                 ||
||    ||  ----------------------------------            --------------------------------- ||                 ||
||    ||____|____________________________________________________________________________ ||                 ||
|| =========|================================================================================================|| 
            |                                                    |
            |                                                    |
            |       +++++++++++++++++++++++++++++++++++++        |
            |       +  LoLin NodeMCU V3(ESP12E)         +        |  
            |       +    - pin D2 (Open/Close)          +        |
            |       +    - WiFi Connection manager      +        |
            |------>+    - Update Over the Air          +        |    
                    +    - MQTT-brocker                 <--------|
                    +++++++++++++++++++++++++++++++++++++






