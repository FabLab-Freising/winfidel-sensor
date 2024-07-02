#include <ESPmDNS.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h"
#include "include/PersistSettings.h"
#include "config_winfidel.h"

WiFiManager wifiManager;
AsyncWebServer server(80);
volatile int WiFi_status = WL_IDLE_STATUS;
const char mdnsName[] = MDNS_NAME;
const char wifiName[] = WIFI_HOSTNAME;
uint8_t dbgOTApercent = 100;


//Define HardwareSerial for outputting data
HardwareSerial HWSerial0(0);


void setup()
{
    // Configure Serial communication
    //USB-CDC Serial, Debug prints and averything neede
	Serial.begin(115200);
    //Hardware-Serial on T/R Pins
    //Data for beeing processed elsewhere --> Raw-Data better useful?
    HWSerial0.begin(115200);

    Serial.println("Wireless Inline Filament Estimator, Low-Cost - WInFiDEL");
    WiFi.setHostname(wifiName);

    // wifiManager.resetSettings(); // Wipe WiFi settings. Uncomment for WiFi manager testing
     

    Serial.println("Starting...");

    bool res;
    res = wifiManager.autoConnect("SK-WInFiDEL-Setup");

    if(!res) {
        Serial.println("Failed to connect or hit timeout");
        ESP.restart();
    }
    else {
        //if you get here you have connected to the WiFi
        Serial.println("Connected.");
    }   

	// Initialize LittleFS
    Serial.print("Starting LittleFS...");
    
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    Serial.println("done.");


	Serial.print("WiFi IP: ");
	Serial.println(WiFi.localIP());

    if (!MDNS.begin(mdnsName))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        Serial.println((String) "mDNS http://" + MDNS_NAME + ".local");
    }

    
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);

    EEPROM.begin(sizeof(calibration_t));

    

    calibration_init();

    adc_init();

    

    Serial.print("Starting WebServer...");
	setupWebServer();
	server.begin();
    Serial.println("done.");

    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());

    


    /**
    * Enable OTA update
    */

   
    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
        uint8_t percent = (progress / (total / 100));
        if (percent == dbgOTApercent)
        {
            return;
        }
        if ((percent%5) == 0)
        {
            Serial.printf("Progress: %u%%\r", percent);
            dbgOTApercent = percent;
        }
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("Arduino OTA is on");

    // Debug message to signal we are initialized and entering loop
	Serial.println("Ready to go.");

    i2c_scan_bus();

    

}


void loop()
{
    Measurements_Tick();
    ArduinoOTA.handle();
}
