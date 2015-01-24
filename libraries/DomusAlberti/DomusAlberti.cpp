#include <SPI.h>
#include <Ethernet.h>

#include <DomusAlberti.h>
#include <../DHT/DHT.h>

#include <string.h>

#define bufferMax 128

dht DHT;

void domusalberti::SetupSamplePorts() {
  RELAY_ON = 0;
  RELAY_OFF = 1;

  RELAY_1_PIN = 3;
  RELAY_2_PIN = 4;
  RELAY_3_PIN = 5;
  RELAY_4_PIN = 6;

  DHT11_PIN = 2;
	
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);
}

void domusalberti::WaitForRequest(EthernetClient client, int bufferSize, char* buffer) {
  bufferSize = 0;
 
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n')
        break;
      else
        if (bufferSize < bufferMax)
          buffer[bufferSize++] = c;
        else
          break;
    }
  }
  
  Serial.println("");
  PrintNumber("bufferSize", bufferSize);
}

void domusalberti::ParseReceivedRequest(char* cmd, char* buffer, char* param1, char* param2) {
  Serial.println("in ParseReceivedRequest");
  Serial.println(buffer);
  
  //Received buffer contains "GET /cmd/param1/param2 HTTP/1.1".  Break it up.
  char* slash1;
  char* slash2;
  char* slash3;
  char* space2;
  
  slash1 = strstr(buffer, "/") + 1; // Look for first slash
  slash2 = strstr(slash1, "/") + 1; // second slash
  slash3 = strstr(slash2, "/") + 1; // third slash
  space2 = strstr(slash2, " ") + 1; // space after second slash (in case there is no third slash)
  if (slash3 > space2) slash3=slash2;

  PrintString("slash1",slash1);
  PrintString("slash2",slash2);
  PrintString("slash3",slash3);
  PrintString("space2",space2);
  
  // strncpy does not automatically add terminating zero, but strncat does! So start with blank string and concatenate.
  cmd[0] = 0;
  param1[0] = 0;
  param2[0] = 0;
  strncat(cmd, slash1, slash2-slash1-1);
  strncat(param1, slash2, slash3-slash2-1);
  strncat(param2, slash3, space2-slash3-1);
  
  PrintString("cmd",cmd);
  PrintString("param1",param1);
  PrintString("param2",param2);
}

void domusalberti::PerformRequestedCommands(char* cmd, int controlloManuale) {
  if ( strcmp(cmd,"digitalWrite") == 0 ) RemoteDigitalWrite();
  if ( strcmp(cmd,"analogRead") == 0 ) RemoteAnalogRead();
  
  if (controlloManuale == 0) {
    if ( strcmp(cmd,"tempRead") == 0 ) RemoteTempRead();
  }
  
  if ( strcmp(cmd,"manualControl") == 0 ) RemoteControlloManuale();
}

void domusalberti::RemoteControlloManuale() {
  controlloManuale = param1[0] - '0';
  
  Serial.print("CONTROLLO MANUALE: ");
  Serial.println(controlloManuale);
}

void domusalberti::RemoteDigitalWrite() {
  int ledPin = param1[0] - '0';
  int ledState = param2[0] - '0';
  
  digitalWrite(ledPin, !ledState);

//  eserver.print("D");
//  eserver.print(ledPin, DEC);
//  eserver.print(" is ");
//  eserver.print( (ledState==1) ? "ON" : "off" );

  Serial.println("RemoteDigitalWrite");
  PrintNumber("ledPin", ledPin);
  PrintNumber("ledState", ledState);
  
  eserver.println((ledState==1) ? "ACCESO!" : "SPENTO!");
}

void domusalberti::RemoteAnalogRead() {
  int analogPin = param1[0] - '0';
  int analogValue = analogRead(analogPin);
  
//  eserver.print("A");
//  eserver.print(analogPin, DEC);
//  eserver.print(" is ");
//  eserver.print(analogValue,DEC);
  
  Serial.println("RemoteAnalogRead");
  PrintNumber("analogPin", analogPin);
  PrintNumber("analogValue", analogValue);
}

void domusalberti::RemoteTempRead() {
  int chk = DHT.read11(DHT11_PIN);
  
  switch (chk) {
    case DHTLIB_OK:  
      eserver.print("OK,\t"); 
    break;
    
    case DHTLIB_ERROR_CHECKSUM: 
      eserver.print("Checksum error,\t"); 
    break;
    
    case DHTLIB_ERROR_TIMEOUT: 
      eserver.print("Time out error,\t"); 
    break;
    
    default: 
      eserver.print("Unknown error,\t"); 
    break;
  }

  float umidita = DHT.humidity;
  float temperatura = DHT.temperature;
  
  eserver.print(umidita, 1);
  eserver.print(",\t");
  eserver.print(temperatura, 1);
  
  EthernetClient eclient;
  byte server[] = {192, 168, 1, 130};
  if (eclient.connect(server, 80)) {
    if (DHT.temperature > 25) {
      digitalWrite(RELAY_1_PIN, RELAY_ON);
      
      scriviDati(eclient, temperatura, umidita);
      
      eserver.println(",\tRelay Acceso");
    } else {
      digitalWrite(RELAY_1_PIN, RELAY_OFF);
      
      scriviDati(eclient, temperatura, umidita);
      
      eserver.println(",\tRelay Spento");
    }
   
    eclient.stop();
  }
}

void domusalberti::scriviDati(EthernetClient client, float temperatura, float umidita) {
  eserver.print(" Scrivo temperatura ");
  eserver.print(temperatura);
  client.print("GET http://192.168.1.130/domusalberti/dati/InserisciDati.php?valore=temperatura&temperatura=");
  client.print(temperatura);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println("http://192.168.1.130");
  client.println();
  
  eserver.print(" Scrivo umidita' ");
  eserver.print(umidita);
  client.print("GET http://192.168.1.130/domusalberti/dati/InserisciDati.php?valore=umidita&umidita=");
  client.print(umidita);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println("http://192.168.1.130");
  client.println();
}

void domusalberti::PrintString(char* label, char* str) {
  Serial.print(label);
  Serial.print("=");
  Serial.println(str);
}

void domusalberti::PrintNumber(char* label, int number) {
  Serial.print(label);
  Serial.print("=");
  Serial.println(number, DEC);
}
