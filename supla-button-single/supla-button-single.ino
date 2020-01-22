#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//********************************************************PARAMETRY KONFIGURACYJNE****************************************************************

const char* ssid = "ssid";
const char* password = "password";
const char* WiFi_hostname = "Supla_BUTTON_JEDNA_SCENA";

byte Adres_z_DHCP = 0;   //1 = DHCP, 0 = Statyczne_IP
IPAddress staticIP(192,168,1,100);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

const char* host = "supla.fracz.com";
String url = "/api/scenes/execute/XXXXXXX-XXXXXXXXXX-XXXXXXXXX"; //Adres PUBLICZNY SCENY 1
String Klucz_dostepu = "Authorization: Bearer XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const int httpsPort = 443; //Port serwera    
byte button = 0;

#define GPIO         0    //Na którym porcie jest przycisk
#define Moc_nadawcza 20.5 //Mo¿liwoœæ ustawienia w zakresie od 0 do 20.5
    


//************************************************************************************************************************************************

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Brak polaczenia z WIFI");
    WiFi_up();
  } 


  button = digitalRead(GPIO);
  if (button == 0 ){
      button = 1;
      if (WiFi.status() == WL_CONNECTED){
      WiFiClientSecure client;
      if (client.connect(host, httpsPort)) {
        Serial.print("Poloczono z ");
        Serial.println(host);
        client.println(String("GET ") + url + " HTTP/1.1\r\n" +
        "Host: " + host + "\r\n" +
        "User-Agent: SuplaButtonESP8266\r\n" +
        Klucz_dostepu + "\r\n" +
        "Connection: close\r\n\r\n");
      }
    }
    Serial.println("Koniec polaczenia");
  }
}


void WiFi_up(){
  WiFi.setOutputPower(Moc_nadawcza);
  WiFi.disconnect();
  delay(200);

  Serial.print("Moc nadawcza: ");
  Serial.println(Moc_nadawcza);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASSWORD: ");
  Serial.println(password);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (Adres_z_DHCP == 0){
    Serial.print("");
    Serial.println("Uruchamiam statycze IP");
    WiFi.config(staticIP, gateway, subnet);
  }
  if (Adres_z_DHCP == 1){
    Serial.print("");
    Serial.println("Uruchamiam DHCP");
  }
  Serial.print("Laczenie z WIFI");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ");
  Serial.print("localIP: ");
  Serial.println(WiFi.localIP());
  Serial.print("subnetMask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("gatewayIP: ");
  Serial.println(WiFi.gatewayIP());  
}