// INCLUDE PER LO SHIELD ETHERNET
#include <SPI.h>
#include <Ethernet.h>

// INCLUDE PER IL SENSORE DHT11 DI TEMPERATURA ED UMIDITA'
#include <DHT.h>

// INCLUDE PER LE FUNZIONI SRELATIVE ALLE STRINGHE
#include<string.h>

// DICHIARAZIONE DEL SENSORE ED ASSEGNAZIONE DELLA PORTA ANALOGICA
dht DHT;
#define DHT11_PIN 2

// ATTENZIONE!!! I RELAY HANNO ACCENSIONE E SPEGNIMENTO AL CONTRARIO
#define RELAY_ON 0
#define RELAY_OFF 1

#define lunghezzaMaxComando 255

// MAC ADDRESS, INDIRIZZO E PORTA DELLO SHIELD ETHERNET
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,134);
EthernetServer arduinoServer(82);

char comandoWeb[lunghezzaMaxComando];

char comando[lunghezzaMaxComando];
char parametro1[lunghezzaMaxComando];
char parametro2[lunghezzaMaxComando];

// FLAG CHE INDICA AL SISTEMA SE E' ATTIVO IL CONTROLLO MANUALE DEI RELAY
int controlloManuale = 0;

// VARIABILI CHE CONTENGONO I VALORI DI TEMPERATURA ED UMIDITA'
float temperatura = 0;
float umidita = 0;

char* leggiComando(EthernetClient webClient) {
  int indiceComando = 0;
  while (webClient.connected()) {
    if (webClient.available()) {
      char c = webClient.read();
      if (c == '\n') {
        break;
      } else {
        comandoWeb[indiceComando] = c;
        
        indiceComando++;
      }
    }
  }
  
  return(comandoWeb);
}

void creaComando(EthernetServer arduinoServer) {
  // IL COMANDO E' DEL TIPO "GET /comando/parametro1/parametro2 HTTP/1.1"
  
  char* slash1;
  char* slash2;
  char* slash3;
  char* space2;
  
  slash1 = strstr(comandoWeb, "/") + 1;
  slash2 = strstr(slash1, "/") + 1;
  slash3 = strstr(slash2, "/") + 1;
  space2 = strstr(slash2, " ") + 1;
 
  if (slash3 > space2) slash3 = slash2;

  comando[0] = 0;
  parametro1[0] = 0;
  parametro2[0] = 0;
  
  strncat(comando, slash1, slash2 - slash1 - 1);
  strncat(parametro1, slash2, slash3 - slash2 - 1);
  strncat(parametro2, slash3, space2 - slash3 - 1);
}

char* eseguiComando(EthernetServer arduinoServer) {  
  if (strcmp(comando, "relayOnOff") == 0) {
    return(relayOnOff(arduinoServer));
  }
  
  if (strcmp(comando, "relayStatusRead") == 0) {
    return(relayStatusRead(arduinoServer));
  }
  
  if (strcmp(comando, "temperatureRead") == 0) {
    char tmpString[10];
 
    dtostrf(dht11Read(arduinoServer, "temperature"), 2, 2, tmpString);
 
    return(tmpString);
  }
  
  if (strcmp(comando, "humidityRead") == 0) {
    char tmpString[10];
 
    dtostrf(dht11Read(arduinoServer, "humidity"), 3, 2, tmpString);
    
    return(tmpString);
  }
  
  if (strcmp(comando, "manualControlOnOff") == 0) {
    return(manualControlOnOff(arduinoServer));
  }
}

char* relayOnOff(EthernetServer arduinoServer) {    
  int numeroRelay = parametro1[0] - '0';
  int statoRelay = parametro2[0] - '0';
  
  digitalWrite(numeroRelay, !statoRelay);
  
  arduinoServer.print(statoRelay);
}
  
char* relayStatusRead(EthernetServer arduinoServer) { 
  int numeroRelay = parametro1[0] - '0';
  int statoRelay = digitalRead(numeroRelay);
    
  arduinoServer.print(!statoRelay);
}
  
char* manualControlOnOff(EthernetServer arduinoServer) {
  controlloManuale = parametro1[0] - '0';
  
  arduinoServer.print("OK");
}

double dht11Read(EthernetServer arduinoServer, char* tipoSensore) {  
  int chk = DHT.read11(DHT11_PIN);
  
  switch (chk) {
    case DHTLIB_OK:  
//      arduinoServer.print("OK,\t"); 
    break;
    
    case DHTLIB_ERROR_CHECKSUM: 
//      arduinoServer.print("Checksum error,\t"); 
    break;
    
    case DHTLIB_ERROR_TIMEOUT: 
//      arduinoServer.print("Time out error,\t"); 
    break;
    
    default: 
//      arduinoServer.print("Unknown error,\t"); 
    break;
  }
  
  if (tipoSensore == "temperature") {    
    temperatura = DHT.temperature;
  
    Serial.print("TEMP: '");
    Serial.print(temperatura);
    Serial.println("'");
  
    if (controlloManuale == 0) {
      if (temperatura > 22) {
        digitalWrite(3, 0);
      } else {
        digitalWrite(3, 1);
      }
    }
  
    arduinoServer.print(temperatura);
  } else {
    umidita = DHT.humidity;
    
    Serial.print("UMID: '");
    Serial.print(umidita);
    Serial.println("'");
  
    arduinoServer.print(umidita);
  }
}

void setup() {  
  Ethernet.begin(mac, ip);
  
  arduinoServer.begin();

  Serial.begin(9600);
  
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  digitalWrite(3, 1);
  digitalWrite(4, 1);
  digitalWrite(5, 1);
  digitalWrite(6, 1);
  
  Serial.print("IP = ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  EthernetClient webClient;
  
  webClient = arduinoServer.available();
  if (webClient) {
    leggiComando(webClient);
    
    String tmpStr(comandoWeb); 
    tmpStr.trim();
     if (tmpStr.length() > 0) {
      creaComando(arduinoServer);
      eseguiComando(arduinoServer);
    }
    
    webClient.stop();
  }
}
