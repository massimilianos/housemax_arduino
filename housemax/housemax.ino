// INCLUDE PER LE FUNZIONI RELATIVE AL TEMPO
#include <Time.h>

// INCLUDE PER LO SHIELD ETHERNET
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// INCLUDE PER LA LIBRERIA WEBDUINO
#include <WebServer.h>

// INCLUDE PER IL SENSORE DHT11 DI TEMPERATURA ED UMIDITA'
#include <DHT.h>

// INCLUDE PER LE FUNZIONI RELATIVE ALLE STRINGHE
#include<string.h>

// INDIRIZZI SERVER NTP
IPAddress timeServer(193, 204, 114, 232);

// DICHIARAZIONE DEL SENSORE ED ASSEGNAZIONE DELLA PORTA ANALOGICA
dht DHT;
#define DHT11_PIN 2

// ATTENZIONE!!! I RELAY HANNO ACCENSIONE E SPEGNIMENTO AL CONTRARIO
#define RELAY_ON  0
#define RELAY_OFF 1

// MAC ADDRESS, INDIRIZZO E PORTA DELLO SHIELD ETHERNET
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,134);
IPAddress dns_google(8,8,8,8);
IPAddress gateway(192,168,1,254);
IPAddress subnet(255,255,255,0);

// PORTA DI ASCOLTO DEL SERVER WEBDUINO
WebServer webserver("", 82);

// VARIABILI E COSTANTI RELATIVE AL TEMPO
EthernetUDP Udp;
time_t prevDisplay = 0;
const int timeZone = 1; // EUROPA CENTRALE
unsigned int localPort = 8888; // PORTA DI ASCOLTO UDP

// FLAG CHE INDICA AL SISTEMA SE E' ATTIVO IL CONTROLLO MANUALE DEI RELAY
int controlloManuale = 0;

// FLAG CHE INDICA AL SISTEMA LA MODALITA' IMPOSTATA (ESTATE O INVERNO)
// PER DECIDERE SE UTILIZZARE IL CLIMATIZZATORE O IL RISCALDAMENTO
#define MODALITA_INVERNO 0
#define MODALITA_ESTATE  1
int modalita = MODALITA_INVERNO;

// VALORE CHE INDICA LA TEMPERATURA CHE DEVE ESSERE PRESENTE NELL'AMBIENTE
int temperaturaControllo = 0;

// VARIABILI CHE CONTENGONO I VALORI DI TEMPERATURA ED UMIDITA'
float temperature = 0;
float temperaturePrec = 0;
float humidity = 0;
float humidityPrec = 0;

// CODICE PER LA GESTIONE NTP
String scriviOra() {
  String orario = "";
  
  int ora = hour();
  int minuto = minute();
  int secondo = second();

  if(ora < 10)
    orario += "0";
  orario += ora;
  
  orario += ":";
  if(minuto < 10)
    orario += "0";
  orario += minuto;
  
  orario += ":";
  if(secondo < 10)
    orario += "0";
  orario += secondo;
  
  Serial.print("Ora letta: ");
  Serial.print("'");
  Serial.print(orario);
  Serial.println("'");
  
  return(orario);
}

String scriviGiorno() {
  String giornoMese = "";
  
  int giorno = day();
  int mese = month();
  int anno = year();
  
  if(giorno < 10)
    giornoMese += "0";
  giornoMese += giorno;
  
  giornoMese += "-";
  if(mese < 10)
    giornoMese += "0";
  giornoMese += mese;
  
  giornoMese += "-";
  giornoMese += anno;
  
  Serial.print("Giorno letto: ");
  Serial.print("'");
  Serial.print(giornoMese);
  Serial.println("'");
  
  return(giornoMese);
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print("-");
  Serial.print(month());
  Serial.print("-");
  Serial.print(year()); 
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// FUNZIONE PER LA GESTIONE DEL SENSORE DHT11
float dht11Read(char *tipoSensore) {  
  int chk = DHT.read11(DHT11_PIN);
  
  switch (chk) {
    case DHTLIB_OK:  
    break;
    
    case DHTLIB_ERROR_CHECKSUM: 
    break;
    
    case DHTLIB_ERROR_TIMEOUT: 
    break;
    
    default:
    break;
  }
  
  if (tipoSensore == "temperature") {
    temperature = DHT.temperature;
  
    Serial.print("TEMP: '");
    Serial.print(temperature);
    Serial.println("'");
    
    if (temperature < 0 || temperature > 50) {
      temperature = temperaturePrec;
    } else {
      temperaturePrec = temperature;
    }

    if (controlloManuale == 0) {
      if (modalita == MODALITA_INVERNO) {
        if (temperature < temperaturaControllo) {
          digitalWrite(3, RELAY_ON);
        } else {
          digitalWrite(3, RELAY_OFF);
        }
      } else if (modalita == MODALITA_ESTATE) {
        if (temperature > temperaturaControllo) {
          digitalWrite(3, RELAY_ON);
        } else {
          digitalWrite(3, RELAY_OFF);
        }
      }
    }

    return(temperature);
  } else if (tipoSensore == "humidity") {
    humidity = DHT.humidity;
    
    Serial.print("UMID: '");
    Serial.print(humidity);
    Serial.println("'");
  
    if (humidity < 20 || humidity > 80) {
      humidity = humidityPrec;
    } else {  
      humidityPrec = humidity;
    }
  
    return(humidity);
  }
}

void Start(WebServer &server, WebServer::ConnectionType type, char *url_param, bool param_complete) {
  server.httpSuccess();
 
  if (type != WebServer::HEAD) {
    String comando = "";
 
    if (param_complete == true) {
      comando = url_param;
      
      Serial.print("comando: '");
      Serial.print(comando);
      Serial.println("'");
      
      if (comando == "ReadModalita") {
        server.print(modalita);
      } else if (comando.startsWith("SetModalita")) {
        int lengthComando = comando.length();
        String modalitaString = comando.substring(comando.indexOf("=") + 1, lengthComando);
        modalita = modalitaString.toInt();
        
        server.print("OK");
      } else if (comando == "ReadManualControl") {
        server.print(controlloManuale);
      } else if (comando == "ManualControl=ON") {
        controlloManuale = 1;
        
        server.print("OK");
      } else if (comando == "ManualControl=OFF") {
        controlloManuale = 0;
        
        server.print("OK");
      } else if (comando == "Relay1=ON") {
        digitalWrite(3, RELAY_ON);
        
        server.print("OK");
      } else if (comando == "Relay1=OFF") {
        digitalWrite(3, RELAY_OFF);
        
        server.print("OK");
      } else if (comando == "Relay2=ON") {
        digitalWrite(4, RELAY_ON);
        
        server.print("OK");
      } else if (comando == "Relay2=OFF") {
        digitalWrite(4, RELAY_OFF);
        
        server.print("OK");
      } else if (comando == "Relay3=ON") {
        digitalWrite(5, RELAY_ON);
        
        server.print("OK");
      } else if (comando == "Relay3=OFF") {
        digitalWrite(5, RELAY_OFF);
        
        server.print("OK");
      } else if (comando == "Relay4=ON") {
        digitalWrite(6, RELAY_ON);
        
        server.print("OK");
      } else if (comando == "Relay4=OFF") {
        digitalWrite(6, RELAY_OFF);
        
        server.print("OK");
      } else if (comando == "ReadHour") {
        String ora = scriviOra();
        
        server.print(ora);
      } else if (comando == "ReadDay") {
        String giorno = scriviGiorno();
        
        server.print(giorno);
      } else if (comando == "TemperatureRead") {
        temperature = dht11Read("temperature");
        
        server.print((int)temperature);
      } else if (comando == "HumidityRead") {
        humidity = dht11Read("humidity");
        
        server.print((int)humidity);
      } else if (comando.startsWith("RelayStatus")) {
        int lengthComando = comando.length();
        String relayNumberString = comando.substring(comando.indexOf("=") + 1, lengthComando);
        int relayNumber = relayNumberString.toInt() + 2;
        int statoRelay = digitalRead(relayNumber);
        
        server.print(!statoRelay);
      } else if (comando == "ReadTempControl") {
        server.print(temperaturaControllo);
      } else if (comando.startsWith("SetTempControl")) {
        int lengthComando = comando.length();
        String temperaturaControlloString = comando.substring(comando.indexOf("=") + 1, lengthComando);
        temperaturaControllo = temperaturaControlloString.toInt();
        
        server.print("OK");
      }
    }
  }
}

void setup() {
  Ethernet.begin(mac, ip, dns_google, gateway, subnet);
//  Ethernet.begin(mac);
  
  Serial.begin(9600);
  
  Serial.print("IP = ");
  Serial.println(Ethernet.localIP());
  
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);

  webserver.setDefaultCommand(&Start);
  webserver.addCommand("index.htm", &Start);
  webserver.begin();
 
  delay(100);
 
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  digitalWrite(3, 1);
  digitalWrite(4, 1);
  digitalWrite(5, 1);
  digitalWrite(6, 1);
    
  modalita = 0;
  controlloManuale = 0;
  temperaturaControllo = 22;
}
 
void loop() {
  webserver.processConnection();
  
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
}
