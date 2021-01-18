

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h> //WiFiManager by tzapu,tablatronix v2.0.3-alpha
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson v5.13.4

#define  TOTAL_PINS 2 
int PINS_INFO[TOTAL_PINS] = {5, 4}; //pino 5 ou D1;


MDNSResponder mdns;
ESP8266WebServer  server(80);

bool shouldSaveConfig = false;


char hostName[20] = "esp8266";
 
int pino = 5; //pino 5 ou D1;
int status = 0;

 WiFiManager wifiManager; 
 

void setup() {
   Serial.begin(115200);
   
   pinMode(LED_BUILTIN, OUTPUT);
   digitalWrite(LED_BUILTIN, LOW);
   delay(1000);

   
    initializePins();

    readHostnameValue();

    initializeWifiManager();
    
   
    if (mdns.begin(hostName, WiFi.localIP())) 
    {
      Serial.print(hostName);  Serial.println(" ready!");
      Serial.print("\nDNS hostName "); Serial.print(hostName); Serial.println(" ready!");
    }
  
    
    // Define processamento das requisicoes HTTP
    server.on("/", []()
    {
         server.send(200, "text/html", renderHtml());
    });
    
    server.on("/pin",handlepin);
   
    
     server.on("/reset", []()
     {
        Serial.println("Resetando configuracoes atuais...");
       
        server.send(200, "text/html", renderHtml());
       
       //  WiFiManager wifiManager;
        wifiManager.resetSettings();
    });
    
    server.onNotFound(handleNotFound);
  
    // Inicializa servidor Web
    server.begin();
  
    Serial.print("\nServior HTTP iniciado no IP ");Serial.println(WiFi.localIP());
    MDNS.addService("http", "tcp", 80);
}

// Loop -------------------------------------------------
void loop() 
{
  mdns.update();
  server.handleClient();
}



void initializeWifiManager()
{
    WiFiManagerParameter custom_hostName("hostName", "host name", hostName, 20);
      
    //WiFiManager wifiManager; 
   
    wifiManager.setSaveConfigCallback(wifiManager_OnSaveChanges);
      
    wifiManager.addParameter(&custom_hostName);
        
    wifiManager.autoConnect("TheNextLoop_AP", "123456789");
      
    Serial.println("conectado! :) ");
    
    strcpy(hostName, custom_hostName.getValue());

    trySaveChanges();

    Serial.print("IP address: "); Serial.println(WiFi.localIP());
    
    Serial.print("\nPreparing DNS hostName: "); Serial.println(hostName);
      
}


//callback notifying us of the need to save config
void wifiManager_OnSaveChanges () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void trySaveChanges()
{
  
    //save the custom parameters to FS
    if (shouldSaveConfig) {
        Serial.print("\nsaving config: ");
        Serial.print(hostName);
        
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["hostName"] = hostName;
    
        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
          Serial.println("failed to open config file for writing");
        }
    
        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
        //end save
    }

}


void initializePins()
{
  for (int i = 0; i <= (TOTAL_PINS-1); i++) 
  {
    pinMode(PINS_INFO[i], OUTPUT);
    digitalWrite(PINS_INFO[i], LOW);
  }
}

void readHostnameValue()
{
      if (SPIFFS.begin()) 
      {
        Serial.println("flash memory ready");
        if (SPIFFS.exists("/config.json")) 
        {
        
          Serial.println("try to read WifiManager file...");
          
          File configFile = SPIFFS.open("/config.json", "r");
          if (configFile) 
          {
            size_t size = configFile.size();
            std::unique_ptr<char[]> buf(new char[size]);
            configFile.readBytes(buf.get(), size);
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject(buf.get());
           
            if (json.success()) 
            {
             
              if (json.containsKey("hostName"))
              {
                   strcpy(hostName, json["hostName"]);
              }
           
            } 
            else 
            {
              Serial.println("Problem reading config.json");
            }
          }
        }
      } 
      else 
      {
        Serial.println("Problem reading flash memory");
      }  
}



//-----------------------------------------------------------
// Retorna qual é o relë que se deseja acionar e qual é o comando que o rele deve executar (ligar/desligar).
//---------------------------------------------------------
void handlepin()
{
    if (server.args() == 0)
    {
       writeResponse(202, "Nenhum parâmetro foi informado.");
       return;
    }

  
    String message = "";


    int index;
    bool indexSpecified = false;
    int action;
    bool actionSpecified = false;
    
    for (int i = 0; i < server.args(); i++) 
    {   
        String queryName = server.argName(i);
        if (queryName.equalsIgnoreCase("index"))
        {
           // String relay = server.argName(i);
            index =  server.arg(i).toInt();
            indexSpecified = true;
        }
        else if (queryName.equalsIgnoreCase("action"))
        {
            action =  server.arg(i).toInt();
            actionSpecified = true;
        }
    } 

    if (!indexSpecified)
    {
       notifyError("index not specified");
        return;
    }

   if (index < 0 || index >  (TOTAL_PINS-1) )
   {
        notifyError("The provided index does not exists.");
        return;
    }


    
    if (!actionSpecified)
    {   
        notifyError("action not specified");
        return;
    }

   if (!(action == 0 || action == 1))
   {
        notifyError("The provided action is invalid");
        return;
   }
   
    String response = "";
   if (action == 0) //desligar o relê
   {

      if (digitalRead(PINS_INFO[index]) == HIGH)
      {
         digitalWrite(PINS_INFO[index], LOW);
         response = "PIN was set to off";
      }
      else
      {
         response = "PIN was already OFF";
      }
    
       writeResponse(200, response);
       return;
   }
   else  if (action == 1) //ligar o relê
   {
  
      if (digitalRead(PINS_INFO[index]) == HIGH)
      {
        response = "PIN was already ON!";
      }
      else
      {
         digitalWrite(PINS_INFO[index], HIGH);
         response = "PIN set to ON!";
      }
    
       writeResponse(200, response);
       return;
   }

    writeResponse(200, "invalid option" );
}



void notifyError(String errorMessage)
{
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);

        writeResponse(400, errorMessage);
 }




//callback que indica que o ESP entrou no modo AP
void configModeCallback (WiFiManager *myWiFiManager) 
{  
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); //imprime o SSID criado da rede

}

 

void handleNotFound() {
  // Piscar ou comando nao identificado
  writeResponse(404, "Command not recognized or not found");

  for (byte b = 0; b < 3; b++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
  }
}


bool useTextPlain()
{
    String jarvisString = server.arg("textPlain");
    bool jarvis = false;
    if (jarvisString != "")
    {
       if (jarvisString.equalsIgnoreCase("true"))
       {
         jarvis = true;
       }
     }
  return jarvis;
}


void writeResponse(int statusCode, String text)
{
    bool useTextPlainResponse = useTextPlain();
    
      if (useTextPlainResponse)
      {
        server.send(statusCode, "text/plain", text);
        return;
      }
      else
      {
          server.send(statusCode, "text/html", renderHtml());
      }
}

String renderHtml()
{
      String host = ((String)hostName);
      
      String html = "";
      html = "<!DOCTYPE html>\n<html lang=\"pt-br\">\n";
      html += "<head>";
      html += "<meta charset=\"utf-8\" />";
      html += "<meta http-equiv=\"Content-Language\" content=\"pt-br\">";
      html += "<title>ESP8266</title>";
      html += "<style type=\"text/css\">@media screen and (max-width: 767px) {.tg {width: auto !important;}.tg col {width: auto !important;}.tg-wrap {overflow-x: auto;-webkit-overflow-scrolling: touch;margin: auto 0px;}}</style>";
      html += "<style type=\"text/css\"> .pinOn{background-color: #4CAF50;} .pinOff{background-color: #f44336;}</style>";
      html += "</head>\n"; 
      html += "<body>\n";                    
      html += "<h1>HTTP Web Interface (" + host +")</h1>\n";
      html += "<h3>Web Interface for controlling digital output PINS</h3>\n";
      html += "<p>Change to <a href=\"reset\" onClick=\"return confirm('Ao clicar em OK sua placa ESP8266 entrará no modo Access Point. Neste caso o RESET fisico desta placa ESP8266  devera ser feito POR VOCE e apos isso, voce deverá conectar-se à rede TheNextLoop_AP que sera criada para atualizar as configuracoes. Tem certeza que deseja continuar?')\">Access Point </a> mode&nbsp;</p>\n";
      html += "<div class=\"tg-wrap\"><table class=\"tg\">";
      html += "    <thead>";      
      html += "      <tr>";
      html += "        <th class=\"tg-wp8o\">PINS_INFO Index</th>";
      html += "        <th class=\"tg-0lax\">Commands</th>";
      html += "      </tr>";
      html += "    </thead>";
      html += "    <tbody>";
         
      for (int i = 0; i <= (TOTAL_PINS-1); i++) 
      {

       String pinStatus = "";
       String pinOn = "<input type=\"button\" onclick=\"window.location.href='pin?index="   + ((String)i)  + "&action=1';\" value=\"Turn On\" />"; 
       String pinOff = "<input type=\"button\" onclick=\"window.location.href='pin?index="   + ((String)i)  + "&action=0';\" value=\"Turn Off\" />"; 
    
        if (digitalRead(PINS_INFO[i]) == HIGH)
        {
           pinStatus = "<input type=\"button\" class=\"pinOn\"   value=\"Pin is On\" />"; 
        }
        else
        {
          pinStatus = "<input type=\"button\" class=\"pinOff\"   value=\"Pin is Off\" />"; 
        }

         html += "          <tr>";
         html += "            <td class=\"tg-i817\">"   + ((String)i) + "</td>";
         html += "            <td class=\"tg-buh4\">" + pinStatus + " " + pinOn + " " + pinOff + "</td>";
         html += "          </tr>";

        
      }
      html += "    <tbody>";
      html += "</table></div>";
      html += "</body>\n</html>\n";
    return html;
}
