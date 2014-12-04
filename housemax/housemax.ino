// INCLUDE PER LO SHIELD ETHERNET
#include <Ethernet.h>
#include <SPI.h>
#include <Twitter.h>

// INCLUDE PER LA LIBRERIA WEBDUINO
#include <WebServer.h>

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

// MAC ADDRESS, INDIRIZZO E PORTA DELLO SHIELD ETHERNET
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,134);
WebServer webserver("", 82);

// COLLEGAMENTO A TWITTER (arduino-tweet.appspot.com)
Twitter twitter("2896171270-2PPFNbOyzT6Te2n1aKWlic6W0LOh0dQ3G816qd7");

// FLAG CHE INDICA AL SISTEMA SE E' ATTIVO IL CONTROLLO MANUALE DEI RELAY
int controlloManuale = 0;

// FLAG CHE INDICA AL SISTEMA LA MODALITA' IMPOSTATA (ESTATE O INVERNO)
// PER DECIDERE SE UTILIZZARE IL CLIMATIZZATORE O IL RISCALDAMENTO
// 0 = INVERNO
// 1 = ESTATE
int modalita = 0;

// VALORE CHE INDICA LA TEMPERATURA CHE DEVE ESSERE PRESENTE NELL'AMBIENTE
int temperaturaControllo = 0;

// VARIABILI CHE CONTENGONO I VALORI DI TEMPERATURA ED UMIDITA'
float temperature = 0;
float temperaturePrec = 0;
float humidity = 0;
float humidityPrec = 0;

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
      if (temperature < temperaturaControllo) {
        digitalWrite(3, RELAY_ON);

        if (temperature != temperaturePrec) {
          if (twitter.post("ACCESO RELAY 1!")) {
            int status = twitter.wait();
   
            if (status == 200) {
              Serial.println("OK.");
            } else {
              Serial.print("Codice di errore: ");
              Serial.println(status);
            }
          } else {
            Serial.println("Connessione fallita");
          }
        }
      } else {
        digitalWrite(3, RELAY_OFF);
        
        if (temperature != temperaturePrec) {
          if (twitter.post("SPENTO RELAY 1!")) {
            int status = twitter.wait();
   
            if (status == 200) {
              Serial.println("OK.");
            } else {
              Serial.print("Codice di errore: ");
              Serial.println(status);
            }
          } else {
            Serial.println("Connessione fallita");
          }
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
      } else if (comando == "ReadManualControl") {
        server.print(controlloManuale);
      } else if (comando == "ManualControl=ON") {
        controlloManuale = 1;
      } else if (comando == "ManualControl=OFF") {
        controlloManuale = 0;
      } else if (comando == "Relay1=ON") {
        digitalWrite(3, RELAY_ON);
      } else if (comando == "Relay1=OFF") {
        digitalWrite(3, RELAY_OFF);
      } else if (comando == "Relay2=ON") {
        digitalWrite(4, RELAY_ON);
      } else if (comando == "Relay2=OFF") {
        digitalWrite(4, RELAY_OFF);
      } else if (comando == "Relay3=ON") {
        digitalWrite(5, RELAY_ON);
      } else if (comando == "Relay3=OFF") {
        digitalWrite(5, RELAY_OFF);
      } else if (comando == "Relay4=ON") {
        digitalWrite(6, RELAY_ON);
      } else if (comando == "Relay4=OFF") {
        digitalWrite(6, RELAY_OFF);
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
      }
    }
  }
}

void setup() {
  Ethernet.begin(mac, ip);

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
  
  Serial.begin(9600);
  
  Serial.print("IP = ");
  Serial.println(Ethernet.localIP());
}
 
void loop() {
  webserver.processConnection();
}
