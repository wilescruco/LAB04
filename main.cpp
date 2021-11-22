#include <Arduino.h>
#include "DHT.h"
#include "WiFi.h"
#include "ThingSpeak.h"


#define pindht 4

DHT dht1(pindht, DHT11);

const char* ssid = "Red_Celular";                        //SSID de vuestro router.
const char* password = "GERMAN2018BARRETO";                //ContraseÃ±a de vuestro router.

unsigned long channelID = 1576455;                //ID de vuestro canal.
const char* WriteAPIKey = "KD9B7AXALG43J9E0";     //Write API Key de vuestro canal.

WiFiClient cliente;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Inicio de Programa");
  dht1.begin();

  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Wifi conectado!");

  ThingSpeak.begin(cliente);

}

void leersensor();

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
  leersensor();

  ThingSpeak.writeFields(channelID,WriteAPIKey);
  Serial.println("Datos enviados a ThingSeak");
}

void leersensor(){
  float tem= dht1.readTemperature();
  float hum = dht1.readHumidity();

  Serial.println("Temperatura DHT11");
  Serial.println(tem);
  Serial.print("°C");

  Serial.println("Humedad DHT11");
  Serial.println(hum);
  Serial.print("%");
  Serial.println("-------------");


  ThingSpeak.setField(1,tem);
  ThingSpeak.setField(2,hum);

}