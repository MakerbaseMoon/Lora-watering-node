#include <Arduino.h>

#include <header.h>

#include <DHT.h>
#include <SPI.h>       
#include <LoRa.h>

#define ss  10
#define rst  9
#define dio0 2
#define DHTTYPE DHT22

int RelayPin    =  4;  // For relay
int DhtPin      = A0;  // DHT
int SensorPin   = A2;  // Soil moisture

byte msgCount   = 0;
byte MasterNode = MASTER_NODE_ADDRESS;

byte Node1      = NODE1_LORA_ADDRESS;

String Mymessage = "";

int soilMoistureValue = 0;
float humidity;
float temperature; 

bool wateringSW     = false;
bool wateringState  = false;

String outgoing;

DHT dht(DhtPin, DHTTYPE);

// Tracks the time since last event fired
unsigned long     previousMillis = 0;
unsigned long int previoussecs   = 6;
unsigned long     currentMillis  = 0;
unsigned long     interval       = WATERING_TIME * 1000;

void onReceive(int packetSize);
void sendMessage(String outgoing, byte MasterNode, byte Node1);

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("LoRa Node1");

    pinMode(RelayPin, OUTPUT);
    digitalWrite(RelayPin, HIGH);
    dht.begin();

    LoRa.setPins(ss, rst, dio0);

    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
}

void loop() {
    humidity    = dht.readHumidity();
    temperature = dht.readTemperature();

    soilMoistureValue = analogRead(SensorPin);

    currentMillis = millis();
    if( wateringState && ( (currentMillis - previoussecs) >= interval ) ) {
        digitalWrite(RelayPin, HIGH);
        wateringState = false;

    } else {
        if(soilMoistureValue < SOIL_MOISTURE_WATERING_VALUE || wateringSW == true) {
            digitalWrite(RelayPin, LOW);
            previoussecs = millis();

            wateringState = true;
            wateringSW    = false;
        }
    }

    onReceive(LoRa.parsePacket());
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;          // if there's no packet, return
    if (Mymessage != "") return;

    // read packet header bytes:
    int recipient = LoRa.read();          // recipient address
    byte sender = LoRa.read();            // sender address
    byte incomingMsgId = LoRa.read();     // incoming msg ID
    byte incomingLength = LoRa.read();    // incoming msg length

    String incoming = "";

    while (LoRa.available()) {
        incoming += (char)LoRa.read();
    }

    if (incomingLength != incoming.length()) {   // check length for error
        // Serial.println("error: message length does not match length");
        return;
    }

    // if the recipient isn't this device or broadcast,
    if (recipient != Node1 && recipient != MasterNode) {
        //Serial.println("This message is not for me.");
        return;
    }
    Serial.println("Node Received: " + incoming);
    int index = incoming.indexOf(",");
    int Val1 = incoming.substring(0, index).toInt();
    int Val2 = incoming.substring(index + 1).toInt();

    if (Val1 == 10) {
        Mymessage = Mymessage + temperature + "," 
                            + humidity + "," 
                            + soilMoistureValue + ","
                            + String(LoRa.packetRssi());
        sendMessage(Mymessage, MasterNode, Node1);
        delay(100);
    }

    if(Val2 == 1) {
        wateringSW = true;
    }

    Mymessage = "";
}

void sendMessage(String outgoing, byte MasterNode, byte Node1) {
    LoRa.beginPacket();
    LoRa.write(MasterNode);
    LoRa.write(Node1);
    LoRa.write(msgCount);
    LoRa.write(outgoing.length());
    LoRa.print(outgoing);
    LoRa.endPacket();
    msgCount++;
}
