
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "xPL.h"

#define ONE_WIRE_BUS 49
#define TEMPERATURE_PRECISION 11

OneWire  ds(ONE_WIRE_BUS);
DallasTemperature sensors(&ds);
xPL xpl;

unsigned long timer = 0;

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xCA, 0xD5 };
EthernetClient client;
IPAddress broadcast(255, 255, 255, 255);
EthernetUDP Udp;

byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[15][8];     // enough room for 15 sensors
byte numFound;
byte addrSub;
char temp[6];

void setup() {

  pinMode(43, OUTPUT);
  digitalWrite(43, HIGH);
  Serial.begin(115200);
  // Search for 1-wire devices
  detectDevice();
  sensors.begin();
  sensors.setResolution(TEMPERATURE_PRECISION);
  sensors.requestTemperatures();

  Serial.println("Configuring network...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  // print your local IP address:
  printIPAddress();

  Udp.begin(xpl.udp_port);
  xpl.SendExternal = &SendUdPMessage;  // pointer to the send callback
  xpl.SetSource_P(PSTR("xpl"), PSTR("arduino"), PSTR("90A2DA0DCAD5")); // parameters for hearbeat message
}

void SendUdPMessage(char *buffer)
{
  Udp.beginPacket(broadcast, xpl.udp_port);
  Udp.write(buffer);
  Udp.endPacket();
}

void loop() {
  dumpTemps();
  detectDevice();
}

void dumpTemps() {

  if (!numFound)
    return;
  sensors.requestTemperatures();

  for (addrSub = 0; addrSub < numFound; addrSub++) {

    // the first ROM byte indicates which chip
    Serial.print("Sensor ");
    Serial.println( addrSub );

    switch (addr[addrSub][0]) {
      case 0x10:
        Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;

      case 0x28:
        Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;

      case 0x22:
        Serial.println("  Chip = DS1822");
        type_s = 0;
        break;

      default:
        Serial.println("Device is not a DS18x20 family device.");
        return;
    }

    Serial.print("  Temperature = ");
    ftoa(temp, sensors.getTempC(addr[addrSub]), 2);
    Serial.print(sensors.getTempC(addr[addrSub]));
    Serial.println(" °C");
    Serial.println();

    xPL_Message msg;

    msg.hop = 1;
    msg.type = XPL_TRIG;

    msg.SetTarget_P(PSTR("*"));
    msg.SetSchema_P(PSTR("sensor"), PSTR("basic"));
    String address;
    for ( i = 0; i < 8; i++) {
      address = address + String(addr[addrSub][i], HEX);
    }
    address.toUpperCase();
    msg.AddCommand("device", address.c_str());
    msg.AddCommand_P(PSTR("type"), PSTR("temp"));
    msg.AddCommand("current", temp);

    xpl.SendMessage(&msg);
    delay(5000);
  }
}

char *ftoa(char *a, double f, int precision)
{
  long p[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

void printIPAddress()
{
  Serial.print("  IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  Serial.println();
  Serial.println();
}

void detectDevice() {
  numFound = 0;
  Serial.println("Detecting devices : ");
  for (addrSub = 0; addrSub < 15; addrSub++) {
    if ( ds.search(addr[addrSub])) {
      numFound++;
      Serial.print("  ");
      Serial.print("Device ");
      Serial.print(addrSub);
      Serial.print(" : ");
      for ( i = 0; i < 8; i++) {
        Serial.print(addr[addrSub][i], HEX);
        Serial.write(' ');
      }
      if (OneWire::crc8(addr[addrSub], 7) != addr[addrSub][7]) {
        Serial.println("CRC is not valid!");
        return;
      }
      Serial.println();

    } else {
      Serial.println();
      break;
    }
  }
}


