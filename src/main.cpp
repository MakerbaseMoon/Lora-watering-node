#include <Arduino.h>
/*
    Lora Node1
    The IoT Projects

*/

#include "DHT.h"
#include <SPI.h>              // include libraries
#include <LoRa.h>

#define ss 10
#define rst 9
#define dio0 2
#define DHTTYPE DHT11

int RelayPin = 4; // For relay
int DhtPin = A1;
int SensorPin = A0;

byte msgCount = 0;            // count of outgoing messages
byte MasterNode = 0xFF;
byte Node1 = 0x01;

int AirValue = 590;   //you need to replace this value with Value_1
int WaterValue = 300;  //you need to replace this value with Value_2
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
float humidity;
float temperature; 

int wateringSW = 0;
String outgoing;              // outgoing message
String Mymessage = "";
DHT dht(DhtPin, DHTTYPE);

void setup() {
    Serial.begin(9600);                   // initialize serial

    while (!Serial);
    pinMode(RelayPin, OUTPUT);
    digitalWrite(RelayPin, LOW);
    dht.begin();
    
    Serial.println("LoRa Node1");

    LoRa.setPins(ss, rst, dio0);

    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
}

void loop() {
    humidity = dht.readHumidity(); //讀取濕度
    temperature = dht.readTemperature(); //讀取攝氏溫度
    if (isnan(humidity) || isnan(temperature)){
        Serial.println("DHT Reading Error !!");
        return;
    }
        
    soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
    //  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    
    if (soilMoistureValue < 400){//if the humidity sensor returns a value<500
        digitalWrite(RelayPin, HIGH);//The water pump waters the plant
    } else { 
        digitalWrite(RelayPin, LOW);//Water pump stops watering
    }

    // parse for a packet, and call onReceive with the result:
    onReceive(LoRa.parsePacket());
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;          // if there's no packet, return

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
        ;
        return;                             // skip rest of function
    }

    // if the recipient isn't this device or broadcast,
    if (recipient != Node1 && recipient != MasterNode) {
        //Serial.println("This message is not for me.");
        ;
        return;                             // skip rest of function
    }
    Serial.println("Node Received: " +incoming);
    int Val = incoming.toInt();
    if (Val == 10)
    {
        Mymessage = Mymessage + temperature + "," 
                            + humidity + "," 
                            + wateringSW + ","
                            + String(LoRa.packetRssi());
        sendMessage(Mymessage, MasterNode, Node1);
        delay(100);
        Mymessage = "";
    }

}
void sendMessage(String outgoing, byte MasterNode, byte Node1) {
    LoRa.beginPacket();                 // start packet
    LoRa.write(MasterNode);             // add destination address
    LoRa.write(Node1);                  // add sender address
    LoRa.write(msgCount);               // add message ID
    LoRa.write(outgoing.length());      // add payload length
    LoRa.print(outgoing);               // add payload
    LoRa.endPacket();                   // finish packet and send it
    msgCount++;                         // increment message ID
}
