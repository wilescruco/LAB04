#include <ESP32Servo.h>
#include <ESP32_MailClient.h>
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"    // Libreria del sensor DHT11
#include "ThingSpeak.h"
#include "FirebaseESP32.h"
#include "time.h"

//**************************************
//*********** NTP ***************
//**************************************
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec =  -18000;
const int daylightOffset_sec = 3600;


SMTPData datosSMTP;
#define DHTPIN 13                // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11           // Definimos el tipo de sensor (DHT 11)
DHT Sensor11(DHTPIN, DHTTYPE);  // Se crea el objeto sensor11 (Sensor DHT11)
Servo motor;
#define servopin 27
int Led_puerta =12;
int Led_altaTemp =14;
int ventilador = 26;
int sensorgas = 32;
int bombillo = 33;



//**************************************
//*********** ThingSpeak ***************
//**************************************
unsigned long channelID = 1579096;                //ID de vuestro canal.
const char* WriteAPIKey = "DFO9RSC9K9U2CWVE";     //Write API Key de vuestro canal.

//**************************************
//*********** Firebase ***************
//**************************************
#define FIREBASE_HOST "https://lab04-8ffc9-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "rKpUzzrapYA3GKtTGgUVLYn7BF3eNBYjrnY1FdiK"



//**************************************
//*********** MQTT CONFIG **************
//**************************************
const char *mqtt_server = "node02.myqtthub.com";
const int mqtt_port = 1883;
const char *mqtt_user = "esp32";
const char *mqtt_pass = "esp32";
const char *root_topic_subscribe1 = "Temperatura/esp32";
const char *root_topic_subscribe2 = "Puerta/esp32";
const char *root_topic_publish = "Topic_raiz/public_esp32";
const char *root_topic_publish1 = "Temperatura/public_esp32";
const char *root_topic_publish2 = "Humedad/public_esp32";
const char *root_topic_publish3 = "Puerta/public_esp32";
const char *root_topic_publish4 = "Concentración_Gas/public_esp32";


//**************************************
//*********** WIFICONFIG ***************
//**************************************
const char* ssid = "CLARO_WIFI79";
const char* password =  "CLAROI18069";



//**************************************
//*********** GLOBALES   ***************
//**************************************
WiFiClient espClient;
WiFiClient espClientThinSpeak;
PubSubClient client(espClient);
char msg[90];
char msg1[40];
char msg2[40];
char msg3[40];
char msg4[40];
long count1=0;
long count2=0;
long count3=0;
long set=35;
String puerta ="Cerrar";
String estado_puerta ="cerrada";
String tema="";
float temp;
FirebaseData firebaseData;
int estado_gas;

String nodo1 = "/Datos";
String nodo2 = "/Alta_Temp";
String nodo3 = "/Alta_Conc";


//************************
//** F U N C I O N E S ***
//************************
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();
void CerrarPuerta();
void AbrirPuerta();
void printLocalTime();



void setup() {
  Serial.begin(115200);
  setup_wifi();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  ThingSpeak.begin(espClientThinSpeak);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Sensor11.begin();
  pinMode(Led_puerta, OUTPUT);
  pinMode(Led_altaTemp, OUTPUT);
  pinMode(ventilador, OUTPUT);
  pinMode(sensorgas, INPUT);
  pinMode(bombillo, OUTPUT);
  motor.attach(servopin);
  digitalWrite(Led_puerta, LOW);
  digitalWrite(Led_altaTemp, LOW);  
  digitalWrite(bombillo, LOW); 
  motor.write(0);
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);  
}

void loop() {  
     
  if (!client.connected()) {
    reconnect();
  }
  if (client.connected()){
    
    float hum = Sensor11.readHumidity();      //Definimos la variable "hum" de tipo flotante que almacena la lectura del dato humedad del sensor11
    float temp = Sensor11.readTemperature();  //Definimos la variable "temp" de tipo flotante que almacena la lectura del dato temperatura (en grados celsius) del sensor11
    estado_gas = analogRead(sensorgas);
    
    if(estado_gas > 800){
      count2++;
      digitalWrite(bombillo, HIGH);
      Serial.println("");
      Serial.println("*********A L E R T A !!*********");
      Serial.println("***Concentración de gas alta ***");
      Firebase.setFloat(firebaseData, nodo3 + "/Alarma" + count2, estado_gas);           
    }else{
      digitalWrite(bombillo, LOW);
    }
    

    if (isnan(hum) || isnan(temp)) {
    Serial.println(F("Sistema Control de temperatura apagado")); // Imprime ese comentario cuando no lee ningun dato del sensor11
    //return;
    }

    ThingSpeak.setField(1,temp);
    ThingSpeak.setField(2,hum);
    ThingSpeak.setField(3,estado_gas);

    Firebase.setFloat(firebaseData, nodo1 + "/Temperatura", temp);
    Firebase.setFloat(firebaseData, nodo1 + "/Humedad", hum);
    Firebase.setFloat(firebaseData, nodo1 + "/Concentracion_Gas", estado_gas);


    if (temp > set) {
      digitalWrite(Led_altaTemp, HIGH); // Cambiar estado del pin
      digitalWrite(ventilador, HIGH);
      Serial.println(""); 
      Serial.println("ALTA TEMPERATURA!!!");            
    }else {
      digitalWrite(Led_altaTemp, LOW); // Cambiar estado del pin   
      digitalWrite(ventilador, LOW);
    }

    if (puerta=="Cerrar"){
      estado_puerta="cerrada";
      digitalWrite(Led_puerta, LOW); // Cambiar estado del pin
    }else if (puerta=="Abrir"){
      estado_puerta="abierta";
      digitalWrite(Led_puerta, HIGH); // Cambiar estado del pin          
    }
       
    String str  ="Temp (°C): " + String(temp) + " / Hum (%): " + String(hum) + " / Puerta: " + (estado_puerta) + " / Concentración gas: " + (estado_gas);
    String str1 ="Temp (°C): " + String(temp);
    String str2 ="Hum (%): " + String(hum);
    String str3 ="Puerta: " + (estado_puerta);
    String str4 ="Concentración de gas: " + String(estado_gas);
    str.toCharArray(msg,90);
    str1.toCharArray(msg1,40);
    str2.toCharArray(msg2,40);
    str3.toCharArray(msg3,40);
    str4.toCharArray(msg4,40);
    client.publish(root_topic_publish,msg);
    client.publish(root_topic_publish1,msg1);
    client.publish(root_topic_publish2,msg2);
    client.publish(root_topic_publish3,msg3);
    client.publish(root_topic_publish4,msg4);
    count1++;

    Serial.println("");
    printLocalTime();
    Serial.println("(" + String (count1) + ") PUBLICANDO A: ");
    Serial.println("       Topic_raiz/public_esp32:  " + String (msg));
    Serial.println("      Temperatura/public_esp32:  " + String (msg1));
    Serial.println("          Humedad/public_esp32:  " + String(msg2));
    Serial.println("           Puerta/public_esp32:  " + String(msg3));
    Serial.println("Concentración_Gas/public_esp32:  " + String(msg4));
    Serial.println("");
    
    ThingSpeak.writeFields(channelID,WriteAPIKey);
    Serial.println("Datos enviados a ThingSeak");   
    Serial.println("");       
    delay(12000);
  }
  client.loop();
}



//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi(){
  delay(5000);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ssid: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}



//*****************************
//***    CONEXION MQTT      ***
//*****************************

void reconnect() {

  while (!client.connected()) {
    Serial.println("");
    Serial.print("Intentando conexión Broker...");
    // Creamos un cliente ID
    String clientId = "Micro_Esp32";
    
    // Intentamos conectar
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      Serial.println("Conectado al broker!");
      // Nos suscribimos
      if(client.subscribe(root_topic_subscribe1)){
        Serial.println("Suscripcion a topic "+ String(root_topic_subscribe1));
      }else{
        Serial.println("fallo Suscripciión a topic "+ String(root_topic_subscribe1));
      }
      if(client.subscribe(root_topic_subscribe2)){
        Serial.println("Suscripcion a topic "+ String(root_topic_subscribe2));
        Serial.println("");
      }else{
        Serial.println("fallo Suscripciión a topic "+ String(root_topic_subscribe2));
      }
    } else {
      Serial.print("falló conexión broker:( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");
      delay(5000);
    }
  }
}



//*****************************
//***       CALLBACK        ***
//*****************************

void callback(char* topic, byte* payload, unsigned int length){
  String incoming = "";
  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  tema=topic;
    
  if (tema=="Puerta/esp32"){
    if (incoming=="Cerrar"){
    puerta=incoming;
    Serial.println("");
    Serial.println("=====");
    Serial.println("Mensaje recibido de: Puerta/esp32: " + incoming);
    Serial.println("=====");
    CerrarPuerta();
    }else if (incoming=="Abrir"){
      puerta=incoming;
      Serial.println("");
      Serial.println("=====");
      Serial.println("Mensaje recibido de: Puerta/esp32: " + incoming);
      Serial.println("=====");
      AbrirPuerta();
      datosSMTP.setLogin("smtp.gmail.com",465,"electivaiotunisangil@gmail.com","ElectivaIOT20");
      datosSMTP.setSender("Micro_Esp32","electivaiotunisangil@gmail.com");
      datosSMTP.setPriority("High");
      datosSMTP.setSubject("Información del estado de la puerta");
      datosSMTP.setMessage("LA PUERTA SE ABRIO!!!",false);
      datosSMTP.addRecipient("electivaiotunisangil@gmail.com");
      if (!MailClient.sendMail(datosSMTP)){
        Serial.println("Error enciando correo, " +  MailClient.smtpErrorReason());
        }
      datosSMTP.empty();
    }
  } 
  
  if (tema=="Temperatura/esp32"){
    temp= Sensor11.readTemperature();
    set=incoming.toInt();
    Serial.println("");
    Serial.println("=======");
    Serial.println("Mensaje recibido de:");
    Serial.println("Temperatura/esp32 / " + incoming);
    Serial.println("Setting de temperatura: " + String (set) + " y  temp actual: " + String (temp) );
    Serial.println("======"); 
     
    if (temp>set){
      count3++;
      Firebase.setFloat(firebaseData, nodo2 + "/Alarma" + count3, temp);
      datosSMTP.setLogin("smtp.gmail.com",465,"electivaiotunisangil@gmail.com","ElectivaIOT20");
      datosSMTP.setSender("Micro_Esp32","electivaiotunisangil@gmail.com");
      datosSMTP.setPriority("High");
      datosSMTP.setSubject("Información del control de temperatura");
      datosSMTP.setMessage("ALTA TEMPERATURA!!!",false);
      datosSMTP.addRecipient("electivaiotunisangil@gmail.com");
      Serial.println("S E   E N V I Ó  C O R R E O");
      Serial.println("======");
      if (!MailClient.sendMail(datosSMTP)){
        Serial.println("Error enciando correo, " +  MailClient.smtpErrorReason());
      }
    datosSMTP.empty();
    }
  }
}



//*****************************
//***    ABRIR PUERTA      ***
//*****************************

void AbrirPuerta(){
  motor.write(20);
  delay(500);
   motor.write(40);
  delay(500);
  motor.write(60);
  delay(500);
  motor.write(80);
  delay(500);
  motor.write(100);
  delay(500);

}

//*****************************
//***    CERRAR PUERTA      ***
//*****************************

void CerrarPuerta(){
  motor.write(100);
  delay(500);
  motor.write(80);
  delay(500);
  motor.write(60);
  delay(500);
  motor.write(40);
  delay(500);
  motor.write(20);
  delay(500);
  motor.write(0);
  delay(500);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No se pudo obtener el tiempo");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}