#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
  #include "user_interface.h"
}

uint8_t mac[] = {0x2C, 0x3A, 0xE8, 0x4E, 0x3C, 0xAE};
void initVariant() {
  wifi_set_macaddr(STATION_IF, &mac[0]);
}


#define MENU_ITEMS  5

byte cnt=0;

#define PATTERN       0
#define FRAMERATE     1
#define MOTORSPEED    2
#define FINEFRAMERATE 3
#define ONTIME        4


uint8_t txrxData[MENU_ITEMS];

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("\r\nESP_Now MASTER CONTROLLER\r\n");

 esp_now_init();
 esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  // Serial.begin(115200);
 // Serial.println("\r\nESP_Now MASTER CONTROLLER\r\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("\r\n\r\nDevice MAC: ");
  Serial.println(WiFi.macAddress());
 // initVariant();  
 esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
  
    memcpy(txrxData, data, len);
  
  Serial.printf("Got hue menu =\t%i\n\r", txrxData[PATTERN]);
  Serial.printf("Got frame menu =\t%i\n\r", txrxData[FRAMERATE]);
  Serial.printf("Got motorspeed menu =\t%i\n\r", txrxData[MOTORSPEED]);
  Serial.printf("Got fineframe menu =\t%i\n\r", txrxData[FINEFRAMERATE]);
  Serial.printf("Got Ontime menu =\t%i\n\r", txrxData[ONTIME]);

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
     //changeFrameInterval();
  
     // need a map here
  });
}


void loop()
{
  
}
