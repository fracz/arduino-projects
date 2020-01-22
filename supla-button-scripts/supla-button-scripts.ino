#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>

ESP8266WebServer server(80);


/*
 * Liczba przycisków w systemie = max 3, deklaracja z poziomu konfiguratora.
Przycisk S1 = GPIO 0
Przycisk S2 = GPIO 4
Przycisk S3 = GPIO 5
Wybranie 1: Sterowanie przy pomocy jednego przycisku wszystkich scen w konfiguracji S1=1,2,3,4,5,6,7,8
Wybranie 2: Sterowanie przy pomocy dwóch przycisków wszystkich scen w konfiguracji S1=1,2,3,4 S2=5,6,7,8
Wybranie 3: Sterowanie przy pomocy trzech przycisków wszystkich scen w konfiguracji S1=1,2,3 S2=4,5,6 S2=7,8
Przycisk S1 jest nadrzędny i wchodzi w tryb konfiguracji po 10 kliknięciach.
Rozbudowano podgląd pamięci EEPROM z poziomu terminala do przeglądu bierzącej sytuacji
Zmodyfikowano zapis i kasowanie EEPROM. Jeśli komórka uległa zmianie to jest kasowana, jeśli nie to pozostaje bez zmian - ma to na celu ochronę przed uszkodzeniem pamięci EEPROM przed zbyt częstym zapisywaniem.
Dodano konfigurację IP, Maska, Brama - pozwala to zachować porządek w domowej sieci komputerowej (wymagana wszystkie pola, wybranie 0.0.0.0 lub pozostawienie któregokolwiek pola pustego uruchamia DHCP).
https://forum.supla.org/viewtopic.php?p=22574#p22574
 * 
 */

String st;
String content;
int statusCode;
const unsigned Czas_max_timeout = 10000;

const char* Wifi_name = "Supla WALL BUTTON";
const char* Wifi_pass = "12345678";
const char* version_ = " V2.2.3 BETA";
const char* copyright = " by Duch";

byte button_S1 = 0;
word button_S1_time = 0;
word button_S1_timeout = 0;
byte button_S1_counter = 0;

byte button_S2 = 0;
word button_S2_time = 0;
word button_S2_timeout = 0;
byte button_S2_counter = 0;

byte button_S3 = 0;
word button_S3_time = 0;
word button_S3_timeout = 0;
byte button_S3_counter = 0;


byte tryb_konfiguracji = 0;
byte status_polaczenia = 0;

byte mac[6];

word url_eep_adres = 0;
word tok_eep_adres = 0;
String esid = "";  
String epass = "";
const unsigned led_pin = 2;
word led_blink = 0;
String zmienna1 = ""; 
String zmienna2 = "";
byte error = 1;
int int_tryb_led = 0; 
byte IP[4] = {0,0,0,0};
String zmienna = "";
int Liczba_przyciskow_w_systemie = 0;


//****************************************************************************************************************************************
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);
  pinMode(0, INPUT);
  Serial.begin(115200);
  EEPROM.begin(4096);
  delay(10);
  Serial.println("");
  Serial.println("");
  Serial.print("Startup ");
  Serial.print(Wifi_name);
  Serial.print(" ");
  Serial.print(version_);
  Serial.print(" ");
  Serial.println(copyright);
   
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);
  Pokaz_zawartosc_eeprom();
 

  if (char(EEPROM.read(0) == 0xFF) && char(EEPROM.read(32) == 0xFF))             //Sprawdzanie czy procesor ma odpowiednio przygotowany EEPROM
  {
    inicjalizacja_EEPROM();
    tryb_konfiguracji = 1;                                                       //Jeli nie to wejcie w tryb konfiguracji
  }

  if (char(EEPROM.read(0) == 0x00) && char(EEPROM.read(32) == 0x00))             //Sprawdzanie czy procesor ma odpowiednio przygotowany EEPROM
  {
    tryb_konfiguracji = 1;                                                       //Jeli nie to wejcie w tryb konfiguracji
  }

  String eswitch_num = "";
  eswitch_num  += char(EEPROM.read(4094));                       
  Liczba_przyciskow_w_systemie = eswitch_num.toInt();
  if(Liczba_przyciskow_w_systemie < 1){Liczba_przyciskow_w_systemie = 1;}
  if(Liczba_przyciskow_w_systemie > 3){Liczba_przyciskow_w_systemie = 3;}
  Serial.print("Liczba przyciskow w systemie: ");
  Serial.println(Liczba_przyciskow_w_systemie);


  String etryb_led = "";
  etryb_led += char(EEPROM.read(4095));                       
  int_tryb_led = etryb_led.toInt();


  
}
//****************************************************************************************************************************************

void Pokaz_zawartosc_eeprom()
{
Serial.println("|--------------------------------------------------------------------------------------------------------------------------------------------|");
Serial.println("|                                             HEX                                                     |                STRING                |");
                

byte  eeprom = 0;
  String  eeprom_string = "";
  String znak = "";
  
  byte licz_wiersz = 0;  
  for (int i = 0; i < 4096; ++i)   {
    ESP.wdtFeed(); // Reset watchdoga
    eeprom = (EEPROM.read(i));
    znak = char(EEPROM.read(i));
    if (znak == ""){
      eeprom_string += " ";  
    }
    else
    {
      eeprom_string += znak;
    }
    znak = "";
    
    if (licz_wiersz == 0){
      Serial.print("|   ");
    }
    licz_wiersz++;
    
    if (licz_wiersz >=0 && licz_wiersz < 32){;

      printf("%02X", eeprom);
      Serial.print(" ");
    }
    if (licz_wiersz == 32){
      printf("%02X", eeprom);
      Serial.print("   |   ");
      Serial.print(eeprom_string);
      Serial.println("   |");
      eeprom_string = "";
      licz_wiersz = 0;
    }
  }
  Serial.println("|--------------------------------------------------------------------------------------------------------------------------------------------|");   
  Serial.println("");  
  Serial.println("");

}
//****************************************************************************************************************************************

void inicjalizacja_EEPROM()
{
  Clear_eeprom(0, 4096); //Wykasuj cala zawartosc pamieci EEPROM
}

//****************************************************************************************************************************************
void loop() {
  
  ESP.wdtFeed();
  Inicjalizacja();
  Obsluga_przycisku();
  Tryb_konfiguracji();
  if (WiFi.status() != WL_CONNECTED){
    status_polaczenia = 0;
    WiFi.disconnect();     
  }

  
  if (error == 1){
    digitalWrite(led_pin, HIGH);
    delay(200);  
    digitalWrite(led_pin, LOW);
    error = 0;   
  }

  if (error == 2){
    digitalWrite(led_pin, HIGH);
    delay(200);  
    digitalWrite(led_pin, LOW);
    delay(200);
    digitalWrite(led_pin, HIGH);
    delay(200);  
    digitalWrite(led_pin, LOW);
    error = 0; 
  }

  if (tryb_konfiguracji == 4){
    ESP.restart();  
  }
  delay(1);




  
}

//****************************************************************************************************************************************

void Odczytaj_parametry_IP()
{

  bool DHCP = 0;

  String eip_address = "";  for (int i = 4000; i < 4015; ++i)   {eip_address += char(EEPROM.read(i));}
  zmienna = eip_address.c_str();
  int IP[4] = {0,0,0,0};
  int Part_IP = 0;
  for ( int i=0; i<zmienna.length(); i++ )
    {
    char c = zmienna[i];
    if ( c == '.' )
      {
         Part_IP++;
         continue;
      }
    IP[Part_IP] *= 10;
    IP[Part_IP] += c - '0';
  }
 



  

  String eip_mask = "";  for (int i = 4015; i < 4030; ++i)   {eip_mask += char(EEPROM.read(i));}
  zmienna = eip_mask.c_str();
  int MASK[4] = {0,0,0,0};
      int Part_MASK = 0;
      for ( int i=0; i<zmienna.length(); i++ )
      {
      char c = zmienna[i];
      if ( c == '.' )
      {
         Part_MASK++;
         continue;
      }
      MASK[Part_MASK] *= 10;
      MASK[Part_MASK] += c - '0';
      }
  



  String eip_gateway = "";  for (int i = 4030; i < 4045; ++i)   {eip_gateway += char(EEPROM.read(i));}
  zmienna = eip_gateway .c_str();
  int GW[4] = {0,0,0,0};
      int Part_GATEWAY = 0;
      for ( int i=0; i<zmienna.length(); i++ )
      {
      char c = zmienna[i];
      if ( c == '.' )
      {
         Part_GATEWAY++;
         continue;
      }
      GW[Part_GATEWAY] *= 10;
      GW[Part_GATEWAY] += c - '0';
      }
  
  if (IP[0] == 0 && IP[1] < 1 && IP[2] == 0 && IP[3] == 0){
    Serial.println("Niewlasciwy adres IP");
    DHCP = 1;    
  }

  if (IP[0] > 248 && IP[1] >= 0 && IP[2] >= 0 && IP[3] >= 0){
    Serial.println("Niewlasciwy adres IP");
    DHCP = 1;    
  }

  if (MASK[0] == 0 && MASK[1] == 0 && MASK[2] == 0 && MASK[3] == 0){
    Serial.println("Niewlasciwy adres MASKI");
    DHCP = 1;    
  }

  if (GW[0] == 0 && GW[1] < 1 && GW[2] == 0 && GW[3] == 0){
    Serial.println("Niewlasciwy adres BRAMY");
    DHCP = 1;    
  }

  if (GW[0] >= 248 && GW[1] >= 0 && GW[2] >= 0 && GW[3] >= 0){
    Serial.println("Niewlasciwy adres BRAMY");
    DHCP = 1;    
  }
  
  
  
  if (DHCP == 1){
    Serial.println("Pobieram adres z DHCP");
  }
 
  
  
  
  if (DHCP == 0){
    Serial.println("Uruchamiam statyczne IP");
  
  
   IPAddress staticIP(IP[0],IP[1],IP[2],IP[3]);
   IPAddress subnet(MASK[0],MASK[1],MASK[2],MASK[3]);
   IPAddress gateway(GW[0],GW[1],GW[2],GW[3]);
  
   WiFi.config(staticIP, gateway, subnet);
  
  }    
}

//****************************************************************************************************************************************
void Inicjalizacja()
{
  if (tryb_konfiguracji == 0 && status_polaczenia == 0) {
    esid = "";   for (int i = 0; i < 32; ++i)     { esid += char(EEPROM.read(i));}
    epass = "";  for (int i = 32; i < 96; ++i)    {epass += char(EEPROM.read(i));}
    
    WiFi.mode(WIFI_STA);
    WiFi.hostname(Wifi_name);
    WiFi.begin(esid.c_str(), epass.c_str());
    Odczytaj_parametry_IP();
    Serial.println("Laczenie z WIFI");
    for (unsigned timeout=0; timeout <= Czas_max_timeout; timeout++){
      if (WiFi.status() != WL_CONNECTED){
        delay(1);
        ++led_blink;
        if (led_blink == 2000){
          digitalWrite(led_pin, LOW);  
        }
        if (led_blink > 4000){
          digitalWrite(led_pin, HIGH);
          led_blink = 0;  
        }
        Obsluga_przycisku();

      }    
      else
      {
        Serial.println("Polaczono z WIFI");
        status_polaczenia = 1;
          digitalWrite(led_pin, LOW);
          Serial.print("MAC:");Serial.print(WiFi.macAddress()); 
          Serial.print("     IP:");Serial.print(WiFi.localIP());
          Serial.print("     M:");Serial.print(WiFi.subnetMask());
          Serial.print("     GW:");Serial.println(WiFi.gatewayIP());
        timeout = Czas_max_timeout;      
      }
    }
  }  
}

void Obsluga_przycisku()
{
      
      if (Liczba_przyciskow_w_systemie <= 1){
        button_S1 = digitalRead(0); if (button_S1 == 0 ) {++button_S1_time;}
        if (button_S1_time == 1 ) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, HIGH);}}
        if (button_S1_time > 0 && button_S1 == 1) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, LOW);}
        if (button_S1_time >= 100 && button_S1_time < 500) {++button_S1_counter;button_S1_timeout = 1000;}button_S1_time = 0;}
        if (button_S1_timeout > 0) {--button_S1_timeout;}
        if (button_S1_timeout == 0 && button_S1_counter > 0) {
          Serial.print("Ilosc nacisniec S1: ");Serial.println(button_S1_counter);
          if (button_S1_counter == 1 && tryb_konfiguracji == 2){tryb_konfiguracji = 3;}
          if (button_S1_counter == 5 && tryb_konfiguracji == 2){inicjalizacja_EEPROM();tryb_konfiguracji = 3;}
          if (button_S1_counter == 10 && tryb_konfiguracji == 0){tryb_konfiguracji = 1;}
          if (int_tryb_led == 2 || int_tryb_led == 3){for (int i=1; i <= button_S1_counter; i++){digitalWrite(led_pin, HIGH);delay(200);digitalWrite(led_pin, LOW);delay(300);}delay(500);}
          if (button_S1_counter == 1 && tryb_konfiguracji == 0){url_eep_adres = 201; tok_eep_adres = 301; Wykonaj_scene();}
          if (button_S1_counter == 2 && tryb_konfiguracji == 0){url_eep_adres = 601; tok_eep_adres = 701; Wykonaj_scene();}
          if (button_S1_counter == 3 && tryb_konfiguracji == 0){url_eep_adres = 1001; tok_eep_adres = 1101; Wykonaj_scene();}
          if (button_S1_counter == 4 && tryb_konfiguracji == 0){url_eep_adres = 1401; tok_eep_adres = 1501; Wykonaj_scene();}
          if (button_S1_counter == 5 && tryb_konfiguracji == 0){url_eep_adres = 1801; tok_eep_adres = 1901; Wykonaj_scene();}
          if (button_S1_counter == 6 && tryb_konfiguracji == 0){url_eep_adres = 2201; tok_eep_adres = 2301; Wykonaj_scene();}
          if (button_S1_counter == 7 && tryb_konfiguracji == 0){url_eep_adres = 2601; tok_eep_adres = 2701; Wykonaj_scene();}
          if (button_S1_counter == 8 && tryb_konfiguracji == 0){url_eep_adres = 3001; tok_eep_adres = 3101; Wykonaj_scene();}
          button_S1_counter = 0;
        } 
      }


      if (Liczba_przyciskow_w_systemie == 2){
        button_S1 = digitalRead(0); if (button_S1 == 0 ) {++button_S1_time;}
        if (button_S1_time == 1 ) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, HIGH);}}
        if (button_S1_time > 0 && button_S1 == 1) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, LOW);}
        if (button_S1_time >= 100 && button_S1_time < 500) {++button_S1_counter;button_S1_timeout = 1000;}button_S1_time = 0;}
        if (button_S1_timeout > 0) {--button_S1_timeout;}
        button_S2 = digitalRead(4); if (button_S2 == 0 ) {++button_S2_time;}
        if (button_S2_time == 1 ) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, HIGH);}}
        if (button_S2_time > 0 && button_S2 == 1) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, LOW);}
        if (button_S2_time >= 100 && button_S2_time < 500) {++button_S2_counter;button_S2_timeout = 1000;}button_S2_time = 0;}
        if (button_S2_timeout > 0) {--button_S2_timeout;}
        if (button_S1_timeout == 0 && button_S1_counter > 0 || button_S2_timeout == 0 && button_S2_counter > 0) {
          Serial.print("Ilosc nacisniec S1: ");Serial.println(button_S1_counter);
          Serial.print("Ilosc nacisniec S2: ");Serial.println(button_S2_counter);
        if (button_S1_counter == 1 && tryb_konfiguracji == 2){tryb_konfiguracji = 3;}
        if (button_S1_counter == 5 && tryb_konfiguracji == 2){inicjalizacja_EEPROM();tryb_konfiguracji = 3;}
        if (button_S1_counter == 10 && tryb_konfiguracji == 0){tryb_konfiguracji = 1;}
        if (int_tryb_led == 2 || int_tryb_led == 3){for (int i=1; i <= button_S1_counter; i++){digitalWrite(led_pin, HIGH);delay(200);digitalWrite(led_pin, LOW);delay(300);}delay(500);}
        if (int_tryb_led == 2 || int_tryb_led == 3){for (int i=1; i <= button_S2_counter; i++){digitalWrite(led_pin, HIGH);delay(200);digitalWrite(led_pin, LOW);delay(300);}delay(500);}
        if (button_S1_counter == 1 && tryb_konfiguracji == 0){url_eep_adres = 201; tok_eep_adres = 301; Wykonaj_scene();}
        if (button_S1_counter == 2 && tryb_konfiguracji == 0){url_eep_adres = 601; tok_eep_adres = 701; Wykonaj_scene();}
        if (button_S1_counter == 3 && tryb_konfiguracji == 0){url_eep_adres = 1001; tok_eep_adres = 1101; Wykonaj_scene();}
        if (button_S1_counter == 4 && tryb_konfiguracji == 0){url_eep_adres = 1401; tok_eep_adres = 1501; Wykonaj_scene();}
        if (button_S2_counter == 1 && tryb_konfiguracji == 0){url_eep_adres = 1801; tok_eep_adres = 1901; Wykonaj_scene();}
        if (button_S2_counter == 2 && tryb_konfiguracji == 0){url_eep_adres = 2201; tok_eep_adres = 2301; Wykonaj_scene();}
        if (button_S2_counter == 3 && tryb_konfiguracji == 0){url_eep_adres = 2601; tok_eep_adres = 2701; Wykonaj_scene();}
        if (button_S2_counter == 4 && tryb_konfiguracji == 0){url_eep_adres = 3001; tok_eep_adres = 3101; Wykonaj_scene();}
        button_S1_counter = 0;
        button_S2_counter = 0;
       } 
     }


      if (Liczba_przyciskow_w_systemie == 3){
        button_S1 = digitalRead(0); if (button_S1 == 0 ) {++button_S1_time;}
        if (button_S1_time == 1 ) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, HIGH);}}
        if (button_S1_time > 0 && button_S1 == 1) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, LOW);}
        if (button_S1_time >= 100 && button_S1_time < 500) {++button_S1_counter;button_S1_timeout = 1000;}button_S1_time = 0;}
        if (button_S1_timeout > 0) {--button_S1_timeout;}
        button_S2 = digitalRead(4); if (button_S2 == 0 ) {++button_S2_time;}
        if (button_S2_time == 1 ) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, HIGH);}}
        if (button_S2_time > 0 && button_S2 == 1) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, LOW);}
        if (button_S2_time >= 100 && button_S2_time < 500) {++button_S2_counter;button_S2_timeout = 1000;}button_S2_time = 0;}
        if (button_S2_timeout > 0) {--button_S2_timeout;}
        button_S3 = digitalRead(5); if (button_S3 == 0 ) {++button_S3_time;}
        if (button_S3_time == 1 ) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, HIGH);}}
        if (button_S3_time > 0 && button_S3 == 1) {if (int_tryb_led == 1 || int_tryb_led == 3){digitalWrite(led_pin, LOW);}
        if (button_S3_time >= 100 && button_S3_time < 500) {++button_S3_counter;button_S3_timeout = 1000;}button_S3_time = 0;}
        if (button_S3_timeout > 0) {--button_S3_timeout;}
        if (button_S1_timeout == 0 && button_S1_counter > 0 || button_S2_timeout == 0 && button_S2_counter > 0 || button_S3_timeout == 0 && button_S3_counter > 0) {
          Serial.print("Ilosc nacisniec S1: ");Serial.println(button_S1_counter);
          Serial.print("Ilosc nacisniec S2: ");Serial.println(button_S2_counter);
          Serial.print("Ilosc nacisniec S3: ");Serial.println(button_S3_counter);
        if (button_S1_counter == 1 && tryb_konfiguracji == 2){tryb_konfiguracji = 3;}
        if (button_S1_counter == 5 && tryb_konfiguracji == 2){inicjalizacja_EEPROM();tryb_konfiguracji = 3;}
        if (button_S1_counter == 10 && tryb_konfiguracji == 0){tryb_konfiguracji = 1;}
        if (int_tryb_led == 2 || int_tryb_led == 3){for (int i=1; i <= button_S1_counter; i++){digitalWrite(led_pin, HIGH);delay(200);digitalWrite(led_pin, LOW);delay(300);}delay(500);}
        if (int_tryb_led == 2 || int_tryb_led == 3){for (int i=1; i <= button_S2_counter; i++){digitalWrite(led_pin, HIGH);delay(200);digitalWrite(led_pin, LOW);delay(300);}delay(500);}
        if (int_tryb_led == 2 || int_tryb_led == 3){for (int i=1; i <= button_S3_counter; i++){digitalWrite(led_pin, HIGH);delay(200);digitalWrite(led_pin, LOW);delay(300);}delay(500);}
        if (button_S1_counter == 1 && tryb_konfiguracji == 0){url_eep_adres = 201; tok_eep_adres = 301; Wykonaj_scene();}
        if (button_S1_counter == 2 && tryb_konfiguracji == 0){url_eep_adres = 601; tok_eep_adres = 701; Wykonaj_scene();}
        if (button_S1_counter == 3 && tryb_konfiguracji == 0){url_eep_adres = 1001; tok_eep_adres = 1101; Wykonaj_scene();}
        if (button_S2_counter == 1 && tryb_konfiguracji == 0){url_eep_adres = 1401; tok_eep_adres = 1501; Wykonaj_scene();}
        if (button_S2_counter == 2 && tryb_konfiguracji == 0){url_eep_adres = 1801; tok_eep_adres = 1901; Wykonaj_scene();}
        if (button_S2_counter == 3 && tryb_konfiguracji == 0){url_eep_adres = 2201; tok_eep_adres = 2301; Wykonaj_scene();}
        if (button_S3_counter == 1 && tryb_konfiguracji == 0){url_eep_adres = 2601; tok_eep_adres = 2701; Wykonaj_scene();}
        if (button_S3_counter == 2 && tryb_konfiguracji == 0){url_eep_adres = 3001; tok_eep_adres = 3101; Wykonaj_scene();}
        button_S1_counter = 0;
        button_S2_counter = 0;
        button_S3_counter = 0;
       } 
     }
}
//****************************************************************************************************************************************
void Tryb_konfiguracji()
{
  if (tryb_konfiguracji > 0) {
    ++led_blink;
    if (led_blink == 500){
      digitalWrite(led_pin, LOW);  
    }
    if (led_blink >= 1000){
      digitalWrite(led_pin, HIGH);
      led_blink = 0;  
    }
  }
  if (tryb_konfiguracji == 1) {
    Serial.println("Tryb konfiguracji");
    tryb_konfiguracji = 2;
    WiFi.disconnect();
    status_polaczenia = 0;
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(Wifi_name,Wifi_pass);
    delay(100);
    Serial.println("Tryb AP");
    createWebServer();  
    server.begin();
    Serial.println("Start Serwera"); 
  }
  if (tryb_konfiguracji == 2) {
    server.handleClient();
  }
  if (tryb_konfiguracji == 3) {
    digitalWrite(led_pin, HIGH);
    delay(1000);
    tryb_konfiguracji = 4;
    server.stop();
    Serial.println("Stop Serwera");
  }
}
//****************************************************************************************************************************************
void createWebServer()
{    
      server.on("/", []() {
      Stala_WWW1();
      content += "<form method='post' action='set0'>";
      Stala_WWW2();
      content += "<h3>Ustawienia WIFI</h3>";
      esid = "";   for (int i = 0; i < 32; ++i)     { esid += char(EEPROM.read(i));} 
      content += "<i><input name='ssid' value='" + String(esid.c_str()) + "'length=32><label>Nazwa sieci</label></i>";
      epass = "";  for (int i = 32; i < 96; ++i)    {epass += char(EEPROM.read(i));} 
      content += "<i><input name='pass' value='" + String(epass.c_str()) + "'length=64><label>Has³o</label></i>";
      content += "</div>";
      
      
      content += "<div class='w'>";
      content += "<h3>Ustawienia IP</h3>";
      String eip_address = "";  for (int i = 4000; i < 4015; ++i)   {eip_address += char(EEPROM.read(i));} 
      content += "<i><input name='ip_address' value='" + String(eip_address.c_str()) + "'length=15><label>IP:</label></i>";
      String eip_mask = "";  for (int i = 4015; i < 4030; ++i)   {eip_mask += char(EEPROM.read(i));} 
      content += "<i><input name='ip_mask' value='" + String(eip_mask.c_str()) + "'length=15><label>Maska:</label></i>";
      String eip_gateway = "";  for (int i = 4030; i < 4045; ++i)   {eip_gateway += char(EEPROM.read(i));} 
      content += "<i><input name='ip_gateway' value='" + String(eip_gateway.c_str()) + "'length=15><label>Brama:</label></i>";
      content += "</div>";

      
      content += "<div class='w'>";
      content += "<h3>Ustawienia HOST-a</h3>";
      String ehost = "";  for (int i = 96; i < 196; ++i)   {ehost += char(EEPROM.read(i));} 
      content += "<i><input name='host' value='" + String(ehost.c_str()) + "'length=100><label>Adres serwera</label></i>";
      String eport = "";  for (int i = 196; i < 201; ++i)   {eport += char(EEPROM.read(i));} 
      content += "<i><input name='port' value='" + String(eport.c_str()) + "'length=5><label>Port</label></i>";
      content += "</div>";
      content += "<div class='w'>";
      content += "<h3>Tryb LED</h3>";
      String eled_blink = "";  eled_blink += char(EEPROM.read(4095)); 
      content += "<i><input name='led_blink' value='" + String(eled_blink.c_str()) + "'length=1><label>(0,1,2,3)</label></i>";
      content += "</div>";
      content += "<div class='w'>";
      content += "<h3>Liczba przycisków</h3>";
      String eswitch_num = "";  eswitch_num += char(EEPROM.read(4094)); 
      content += "<i><input name='switch_num' value='" + String(eswitch_num.c_str()) + "'length=1><label>(1,2,3)</label></i>";
      content += "</div>";
      content += "<button type='submit'>Zapisz</button></form>";
      content += "<br>";
      content += "<a href='scena1'><button>Dalej</button></a>";
      content += "<br>";
      content += "<br>";
      Stala_WWW4();
      });

      server.on("/set0", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String qhost = server.arg("host");
      String qport = server.arg("port");
      String qled_blink = server.arg("led_blink");
      String qswitch_num = server.arg("switch_num");
      String qip_address = server.arg("ip_address");
      String qip_mask = server.arg("ip_mask");
      String qip_gateway = server.arg("ip_gateway");
      
      Clear_eeprom(0, 201);
      Clear_eeprom(4000, 4095);
              
      for (int i = 0; i < qsid.length(); ++i)  {EEPROM.write(0+i, qsid[i]);}
      for (int i = 0; i < qpass.length(); ++i)  {EEPROM.write(32+i, qpass[i]);}
      for (int i = 0; i < qhost.length(); ++i)  {EEPROM.write(96+i, qhost[i]);}
      for (int i = 0; i < qport.length(); ++i)  {EEPROM.write(196+i, qport[i]);}
      for (int i = 0; i < qip_address.length(); ++i)  {EEPROM.write(4000+i, qip_address[i]);}
      for (int i = 0; i < qip_mask.length(); ++i)  {EEPROM.write(4015+i, qip_mask[i]);}
      for (int i = 0; i < qip_gateway.length(); ++i)  {EEPROM.write(4030+i, qip_gateway[i]);}
      for (int i = 0; i < qswitch_num.length(); ++i)  {EEPROM.write(4094+i, qswitch_num[i]);}
      for (int i = 0; i < qled_blink.length(); ++i)  {EEPROM.write(4095+i, qled_blink[i]);}
     
      
      EEPROM.commit();   
          
      Stala_WWW1();
      Stala_WWW5();       
    });
//************************************************************    
    server.on("/scena1", []() {
      url_eep_adres = 201; tok_eep_adres = 301;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set1'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 1</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='/'><button>Powrót</button></a><br><br><a href='scena2'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set1", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

    server.on("/scena2", []() {
      url_eep_adres = 601; tok_eep_adres = 701;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set2'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 2</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena1'><button>Powrót</button></a><br><br><a href='scena3'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set2", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************
   
   server.on("/scena3", []() {
      url_eep_adres = 1001; tok_eep_adres = 1101;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set3'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 3</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena2'><button>Powrót</button></a><br><br><a href='scena4'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set3", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

   server.on("/scena4", []() {
      url_eep_adres = 1401; tok_eep_adres = 1501;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set4'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 4</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena3'><button>Powrót</button></a><br><br><a href='scena5'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set4", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

   server.on("/scena5", []() {
      url_eep_adres = 1801; tok_eep_adres = 1901;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set5'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 5</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena4'><button>Powrót</button></a><br><br><a href='scena6'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set5", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

   server.on("/scena6", []() {
      url_eep_adres = 2201; tok_eep_adres = 2301;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set6'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 6</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena5'><button>Powrót</button></a><br><br><a href='scena7'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set6", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

   server.on("/scena7", []() {
      url_eep_adres = 2601; tok_eep_adres = 2701;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set7'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 7</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena6'><button>Powrót</button></a><br><br><a href='scena8'><button>Dalej</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set7", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

   server.on("/scena8", []() {
      url_eep_adres = 3001; tok_eep_adres = 3101;               //Nale¿y ustawiæ
      Stala_WWW1();
      content += "<form method='post' action='set8'>";        //Nale¿y ustawiæ 
      Stala_WWW2();
      content += "<h3>Scena 8</h3>";                          //Nale¿y ustawiæ
      Stala_WWW3();    
      content += "<a href='scena7'><button>Powrót</button></a><br><br>"; //Nale¿y ustawiæ
      Stala_WWW4();   
      });
      server.on("/set8", []() {                               //Nale¿y ustawiæ
      Stala_zapisu(); 
      Stala_WWW1();
      Stala_WWW5();        
    });
//************************************************************

server.on("/exit", []() {
        Stala_WWW1();
        content += "<h3><center>RESTART PROCESORA</center></h3>";          
        content += "<br><br>";
        statusCode = 200;
        server.send(statusCode, "text/html", content);
        tryb_konfiguracji = 3;
    }    
  );
}

void Stala_zapisu(){
      String qurl = server.arg("url");
      String qtoken = server.arg("tok");

      Clear_eeprom(url_eep_adres, url_eep_adres+400);
      
      Serial.println("Zapis do eeprom");
      for (int i = 0; i < qurl.length(); ++i)   {EEPROM.write(url_eep_adres+i, qurl[i]);}
      for (int i = 0; i < qtoken.length(); ++i)  {EEPROM.write(tok_eep_adres+i, qtoken[i]);}
      EEPROM.commit();  
}




void Stala_WWW1(){
      content = "<!DOCTYPE HTML>";
      content += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
      content += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
      content += "<style>body{font-size:14px;font-family:HelveticaNeue,'Helvetica Neue',HelveticaNeueRoman,HelveticaNeue-Roman,'Helvetica Neue Roman',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,'Helvetica Neue Light',HelveticaNeue,'Helvetica Neue',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style>";
      content += "<div class='s'><svg version='1.1' id='l' x='0' y='0' viewBox='0 0 200 200' xml:space='preserve'><path d='M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z'/></svg>";
      content += "<h1><center>" + String(Wifi_name) + String(version_) + String(copyright) + "</center></h1>";  
}

void Stala_WWW2(){
      content += "<div class='w'>"; 
}

void Stala_WWW3(){
      zmienna1 = "";  for (int i = url_eep_adres; i < url_eep_adres + 100; ++i)  {zmienna1 += char(EEPROM.read(i));}   //Odczytaj z pamieci EEPROM adres URL
      content += "<i><input name='url' value='" + String(zmienna1.c_str()) + "'length=100><label>API URL</label></i>";
      zmienna2 = "";  for (int i = tok_eep_adres; i < tok_eep_adres + 300; ++i)  {zmienna2 += char(EEPROM.read(i));}   //Odczytaj z pamieci EEPROM Token
      content += "<i><input name='tok' value='" + String(zmienna2.c_str()) + "'length=300><label>API TOKEN</label></i>";
      content += "</div>";
      content += "<button type='submit'>Zapisz</button></form>";
      content += "<br>";
}

void Stala_WWW4(){
      content += "<form method='post' action='exit'>";
      content += "<button type='submit'>Zakoñcz</button></form></div>";
      content += "<br><br>";
      server.send(200, "text/html", content);
}

void Stala_WWW5(){
      content += "<h3><center>ZAPISANO</center></h3>";
      content += "<br>";
      content += "<a href='/'><button>POWRÓT</button></a></div>";          
      content += "<br><br>";
      statusCode = 200;
      server.send(statusCode, "text/html", content);
}

void Wykonaj_scene(){
  error = 0;
  
  while(button_S1 == 0){
    button_S1 = digitalRead(0);
    ++button_S1_time;
    delay(1);
  }

  if (button_S1_time >= 2000) {
    Serial.println("Przerwano scene");
    error = 2;
  }
  else
  {
  
  String ehost = "";  for (int i = 96; i < 196; ++i)   {ehost += char(EEPROM.read(i));}                           //Odczytaj z pamieci EEPROM adres hosta
  String eport = "";  for (int i = 196; i < 201; ++i)  {eport += char(EEPROM.read(i));}                           //Odczytaj z pamieci EEPROM port
  int intport = eport.toInt();                                                                                    //Zamieñ port z wartosci tekstowej na liczbow¹
  String eurl = "";  for (int i = url_eep_adres; i < url_eep_adres + 100; ++i)  {eurl += char(EEPROM.read(i));}   //Odczytaj z pamieci EEPROM adres URL
  String etok = "";  for (int i = tok_eep_adres; i < tok_eep_adres + 300; ++i)  {etok += char(EEPROM.read(i));}   //Odczytaj z pamieci EEPROM Token
  
  
  if(ehost == ""){ error = 1; Serial.println("Nie zaprogramowano adresu HOST"); }
  if(eport == ""){ error = 1; Serial.println("Nie zaprogramowano PORTU"); }
  if(eurl == ""){  error = 1; Serial.println("Nie zaprogramowano adresu URL dla tej sceny"); }  else {  Serial.println(eurl); }
  if(etok == ""){  error = 1; Serial.println("Nie zaprogramowano TOKENU dla tej sceny"); }  else { Serial.println(etok); }
  if (WiFi.status() != WL_CONNECTED) { error = 1; Serial.println("Brak polaczenia z siecia WIFI"); }

  if(error == 0){
    WiFiClientSecure client;
    client.setInsecure();
    if (client.connect(ehost.c_str(), intport)) {
      client.println(String("GET ") + eurl.c_str() + " HTTP/1.1\r\n" +
      "Host: " + ehost.c_str() + "\r\n" +
      "User-Agent: SuplaButtonESP8266\r\n" +
      etok.c_str() + "\r\n" +
      "Connection: close\r\n\r\n");

      String line = client.readStringUntil('\r');
      if (line == "HTTP/1.1 204 No Content"){
        Serial.println("Wykonano scene");
        error = 0;
      }
      else
      {
        error = 1;
      }
      }
      else
      {
        Serial.println("Blad polaczenia z HOSTEM");
        error = 1;  
      }
    }          
  }
  button_S1_time = 0;      
 }


 //****************************************************************************************************************************************





 int Clear_eeprom(int x, int y){
  byte eeprom = 0;
  byte liczba_skasowanych_komorek = 0;
      Serial.print("Kasowanie komorek EEPROM pamieci od ");
      Serial.print(x);
      Serial.print(" do ");
      Serial.print(y);
      for (int i = x; i < y; ++i) {
        eeprom = (EEPROM.read(i));
        if (eeprom > 0){
          EEPROM.write(i, 0);  //Czyszczenie pamiêci EEPROM tylko wtedy gdy jest wiêksza od 0 - oszczêdza to pamiêæ EEPROM przed szybsz¹ degradacj¹
          liczba_skasowanych_komorek++;
        }
        ESP.wdtFeed(); // Reset watchdoga
      }
      if (liczba_skasowanych_komorek > 0){
        EEPROM.commit();  
      }

      Serial.print(". Skasowano: ");
      Serial.println(liczba_skasowanych_komorek);
      liczba_skasowanych_komorek = 0;
}
