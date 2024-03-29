#include <ErriezRobotDyn4DigitDisplay.h>
#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

// OTA packages
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

//define router and broker
#define SSID          "NETGEAR68"
#define PWD           "excitedtuba713"

#define MQTT_SERVER   "192.168.1.2" // could change if the setup is moved
#define MQTT_PORT     1883

bool connected;
bool gestart;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// OTA server
AsyncWebServer server(80);

//define callback method
void callback(char *topic, byte *message, unsigned int length);

// function for establishing wifi connection, do not touch
void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Connect display pins to the ESP32 DIGITAL pins
#define TM1637_CLK_PIN      23
#define TM1637_DIO_PIN      22

// Create display object
RobotDyn4DigitDisplay display(TM1637_CLK_PIN, TM1637_DIO_PIN);

uint8_t m;
uint8_t s;
int temp;

void setup()
{
    connected = false;
    gestart = true;
    //setup wifi
    Serial.begin(115200);

    setup_wifi();
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);

    // Initialize OTA handler

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32 timer.");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");
    
    // Initialize TM1637
    display.begin();
    display.time(60,0,true,false);
    delay(10000);

    m = 60;
    s = 0;
}

// callback function, only used when receiving messages
void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // When receiving a message on "esp32/+/control" a check should be excecuted

  // If a message is received on the topic esp32/control, you check if the message is either "start" or "stop" (or "reset").
  // Changes the state according to the message
  if(String(topic) == "esp32/timer/min"){
      m=messageTemp.toInt();
      display.time(m,s,true,false);
      }
  if(String(topic) == "esp32/timer/sec"){
      s=messageTemp.toInt();
      display.time(m,s,true,false);
      }
  if(String(topic) == "esp32/timer/control")
  {
    
    if(messageTemp.equals("start")){
      gestart=true;
    }
    if(messageTemp.equals("stop")){
      gestart=false;
    }
    if(messageTemp.equals("reset")){
      gestart=false;
      m=60;
      s=0;
      display.time(m,s,true,false);
    }
    if(messageTemp.equals("hardreset")){
      ESP.restart();
    }
  }
}

// function to establish MQTT connection
void reconnect()
{
  delay(10);
  // Loop until we're reconnected
  while (!connected)
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("timer"))
    {
      Serial.println("connected");
      connected = true;
      // Publish
      client.publish("esp32/timer/control", "start");
      // ... and resubscribe
      client.subscribe("esp32/timer/#");
      Serial.print("gelukt");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!connected)
    {
    reconnect();
    }
  client.loop();
  
  if(gestart){
    display.time(m,s,true,false);
    if(s!=0){
      s--;  
    } else if(m!=0) {
      m--;
      s=59;
    } else {
      display.rawDigit(0, 0b00111101);  // G
      display.rawDigit(1, 0b01110111);  // A
      display.rawDigit(2, 0b00110111);  // M
      display.rawDigit(3, 0b01111001);  // E
      delay(1000);
      display.rawDigit(0, 0b00111111);  // O
      display.rawDigit(1, 0b00111110);  // V
      display.rawDigit(2, 0b01111001);  // E
      display.rawDigit(3, 0b01010000);  // R
      delay(1000);
      client.publish("esp32/alohomora/control", "open");
      delay(5000);
      client.publish("esp32/alohomora/control", "open");
      client.subscribe("esp32/timer/#");
      }
    
    
  
    long now = millis();
    if (now - lastMsg > 5000)
    {
      lastMsg = now;
    }
  }
  delay(1000);
  //Serial.println("I'm working");
}
