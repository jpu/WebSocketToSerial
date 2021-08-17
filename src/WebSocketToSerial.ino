#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>

// Main project include file
#include "WebSocketToSerial.h"

char thishost[17];

String inputString = "";
bool serialSwapped = false;

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// State Machine for WebSocket Client;
_ws_client ws_client[MAX_WS_CLIENT]; 

/* ======================================================================
Function: execCommand
Purpose : translate and execute command received from serial/websocket
Input   : client if coming from Websocket
          command received
Output  : - 
Comments: -
====================================================================== */
void execCommand(AsyncWebSocketClient * client, char * msg) {
  uint16_t l = strlen(msg);
  uint8_t index=MAX_WS_CLIENT;

  // Search if w're known client
  if (client) {
    for (index=0; index<MAX_WS_CLIENT ; index++) {
      // Exit for loop if we are there
      if (ws_client[index].id == client->id() ) 
        break;
    } // for all clients
  }

  //if (client)
  //  client->printf_P(PSTR("command[%d]='%s'"), l, msg);
  // Display on debug
  SERIAL_DEBUG.printf("  -> \"%s\"\r\n", msg);

  // Custom command to talk to device
  if (!strcmp_P(msg,PSTR("/ping"))) {
    if (client)
      client->printf_P(PSTR("received your [[b;cyan;]ping], here is my [[b;cyan;]pong]"));

  } else if (!strcmp_P(msg,PSTR("/swap"))) {
    Serial.swap();
    serialSwapped =! serialSwapped;
    if (client)
      client->printf_P(PSTR("Swapped UART pins, now using [[b;green;]RX-GPIO%d TX-GPIO%d]"),
                              serialSwapped?13:3,serialSwapped?15:1);

  // Debug information
  } else if ( !strncmp_P(msg,PSTR("/debug"), 6) ) {
    int br = SERIAL_DEVICE.baudRate();
    if (client) {
      client->printf_P(PSTR("Baud Rate : [[b;green;]%d] bps"), br);
    }

  // baud only display current Serial Speed
  } else if ( client && l==5 && !strncmp_P(msg,PSTR("/baud"), 5) ) {
    client->printf_P(PSTR("Current Baud Rate is [[b;green;]%d] bps"), SERIAL_DEVICE.baudRate());

  // baud speed only display current Serial Speed
  } else if (l>=7 && !strncmp_P(msg,PSTR("/baud "), 6) ) {
    uint32_t br = atoi(&msg[5]);
    if ( br==115200 || br==57600 || br==19200 || br==9600 ) {
      #ifdef MOD_TERMINAL
        SERIAL_DEVICE.begin(br);      
      #endif
      if (client)
        client->printf_P(PSTR("Serial Baud Rate is now [[b;green;]%d] bps"), br);
    } else {
      if (client) {
        client->printf_P(PSTR("[[b;red;]Error: Invalid Baud Rate %d]"), br);
        client->printf_P(PSTR("Valid baud rate are 115200, 57600, 19200 or 9600"));
      }
    } 
  } else if (client && !strcmp_P(msg,PSTR("/hostname")) ) {
    client->printf_P(PSTR("[[b;green;]%s]"), thishost);

  // no delay in client (websocket)
  // ----------------------------
  } else if (client==NULL && l>=8 && !strncmp_P(msg,PSTR("/delay "), 7) ) {
    uint16_t v= atoi(&msg[6]);
    if (v>=0 && v<=60000 ) {
      while(v--) {
        delay(1);
      }
    }

  // ----------------------------
  } else if (l>=8 && !strncmp_P(msg,PSTR("/reset "), 7) ) {
    int v= atoi(&msg[6]);
    if (v>=0 && v<=16) {
      pinMode(v, OUTPUT);
      digitalWrite(v, HIGH);
      if (client)
        client->printf_P(PSTR("[[b;orange;]Reseting pin %d]"), v);
      digitalWrite(v, LOW);
      delay(50);
      digitalWrite(v, HIGH);
    } else {
      if (client) {
        client->printf_P(PSTR("[[b;red;]Error: Invalid pin number %d]"), v);
        client->printf_P(PSTR("Valid pin number are 0 to 16"));
      }
    }

  } else if (client && !strcmp_P(msg,PSTR("/fw"))) {
    client->printf_P(PSTR("Firmware version [[b;green;]%s %s]"), __DATE__, __TIME__);

  } else if (client && !strcmp_P(msg,PSTR("/whoami"))) {
    client->printf_P(PSTR("[[b;green;]You are client #%u at index[%d&#93;]"),client->id(), index );

  } else if (client && !strcmp_P(msg,PSTR("/who"))) {
    uint8_t cnt = 0;
    // Count client
    for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
      if (ws_client[i].id ) {
        cnt++;
      }
    }
    
    client->printf_P(PSTR("[[b;green;]Current client total %d/%d possible]"), cnt, MAX_WS_CLIENT);
    for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
      if (ws_client[i].id ) {
        client->printf_P(PSTR("index[[[b;green;]%d]]; client [[b;green;]#%d]"), i, ws_client[i].id );
      }
    }

  } else if (client && !strcmp_P(msg,PSTR("/heap"))) {
    client->printf_P(PSTR("Free Heap [[b;green;]%ld] bytes"), ESP.getFreeHeap());

  } else if (client && (!strcmp_P(msg,PSTR("/?")) || !strcmp_P(msg,PSTR("/help")))) {
    client->printf_P(PSTR(HELP_TEXT));

  // all other to serial Proxy
  }  else if (*msg) {
    //SERIAL_DEBUG.printf("Text '%s'", msg);
    // Send text to serial
    SERIAL_DEVICE.print(msg);
    SERIAL_DEVICE.print("\r\n");
  }
}

/* ======================================================================
Function: execCommand
Purpose : translate and execute command received from serial/websocket
Input   : client if coming from Websocket
          command from Flash
Output  : - 
Comments: -
====================================================================== */
void execCommand(AsyncWebSocketClient * client, PGM_P msg) {
  size_t n = strlen_P(msg);
  char * cmd = (char*) malloc(n+1);
  if( cmd) {
    for(size_t b=0; b<n; b++) {
      cmd[b] = pgm_read_byte(msg++);
    }
    cmd[n] = 0;
    execCommand(client, cmd);
  } // if cmd
}

/* ======================================================================
Function: onEvent
Purpose : Manage routing of websocket events
Input   : -
Output  : - 
Comments: -
====================================================================== */
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    uint8_t index;
    SERIAL_DEBUG.printf("ws[%s][%u] connect\n", server->url(), client->id());

    for (index=0; index<MAX_WS_CLIENT ; index++) {
      if (ws_client[index].id == 0 ) {
        ws_client[index].id = client->id();
        ws_client[index].state = CLIENT_ACTIVE;
        SERIAL_DEBUG.printf("added #%u at index[%d]\n", client->id(), index);
        client->printf("[[b;green;]Hello Client #%u, added you at index %d]", client->id(), index);
        client->ping();
        break; // Exit for loop
      }
    }
    if (index>=MAX_WS_CLIENT) {
      SERIAL_DEBUG.printf("not added, table is full");
      client->printf("[[b;red;]Sorry client #%u, Max client limit %d reached]", client->id(), MAX_WS_CLIENT);
      client->ping();
    }

  } else if(type == WS_EVT_DISCONNECT){
    SERIAL_DEBUG.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
      if (ws_client[i].id == client->id() ) {
        ws_client[i].id = 0;
        ws_client[i].state = CLIENT_NONE;
        SERIAL_DEBUG.printf("freed[%d]\n", i);
        break; // Exit for loop
      }
    }
  } else if(type == WS_EVT_ERROR){
    SERIAL_DEBUG.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    SERIAL_DEBUG.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*) arg;
    char * msg = NULL;
    size_t n = info->len;
    uint8_t index;

    // Size of buffer needed
    // String same size +1 for \0
    // Hex size*3+1 for \0 (hex displayed as "FF AA BB ...")
    n = info->opcode == WS_TEXT ? n+1 : n*3+1;

    msg = (char*) calloc(n, sizeof(char));
    if (msg) {
      // Grab all data
      for(size_t i=0; i < info->len; i++) {
        if (info->opcode == WS_TEXT ) {
          msg[i] = (char) data[i];
        } else {
          sprintf_P(msg+i*3, PSTR("%02x "), (uint8_t) data[i]);
        }
      }
    }

    SERIAL_DEBUG.printf("ws[%s][%u] message %s\n", server->url(), client->id(), msg);

    // Search if it's a known client
    for (index=0; index<MAX_WS_CLIENT ; index++) {
      if (ws_client[index].id == client->id() ) {
        SERIAL_DEBUG.printf("known[%d] '%s'\n", index, msg);
        SERIAL_DEBUG.printf("client #%d info state=%d\n", client->id(), ws_client[index].state);

        // Received text message
        if (info->opcode == WS_TEXT) {
          execCommand(client, msg);
        } else {
          SERIAL_DEBUG.printf("Binary 0x:%s", msg);
        }
        // Exit for loop
        break;
      } // if known client
    } // for all clients

    // Free up allocated buffer
    if (msg) 
      free(msg);

  } // EVT_DATA
}

/* ======================================================================
Function: setup
Purpose : Setup I/O and other one time startup stuff
Input   : -
Output  : - 
Comments: -
====================================================================== */
void setup() {
  // Set Hostname for OTA and network (add only 2 last bytes of last MAC Address)
  // You can't have _ or . in hostname 
  sprintf_P(thishost, PSTR("WS2Serial-%04X"), ESP.getChipId() & 0xFFFF);

  SERIAL_DEVICE.begin(115200);

  SERIAL_DEBUG.begin(115200);
  SERIAL_DEBUG.print(F("\r\nBooting on "));
  SERIAL_DEBUG.println(ARDUINO_BOARD);
  SPIFFS.begin();
  WiFi.mode(WIFI_STA);

  // No empty sketch SSID, try connect 
  if (*ssid!='*' && *password!='*' ) {
    SERIAL_DEBUG.printf("connecting to %s with psk %s\r\n", ssid, password );
    WiFi.begin(ssid, password);
  } else {
    // empty sketch SSID, try autoconnect with SDK saved credentials
    SERIAL_DEBUG.println(F("No SSID/PSK defined in sketch\r\nConnecting with SDK ones if any"));
  }

  // Loop until connected
  while ( WiFi.status() !=WL_CONNECTED ) {
    yield(); 
  }

  SERIAL_DEBUG.print(F("I'm network device named "));
  SERIAL_DEBUG.println(thishost);

  ArduinoOTA.setHostname(thishost);

  // OTA callbacks
  ArduinoOTA.onStart([]() {
    // Clean SPIFFS
    SPIFFS.end();

    ws.textAll("OTA Update Started");
    ws.enable(false);
    ws.closeAll();

  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Do nothing
  });

  ArduinoOTA.onEnd([]() { 
    // Do nothing
  });

  ArduinoOTA.onError([](ota_error_t error) {
    ESP.restart(); 
  });

  ArduinoOTA.begin();
  MDNS.addService("http","tcp",80);

  // Enable and start websockets
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
  
  server.onNotFound([](AsyncWebServerRequest *request){
    SERIAL_DEBUG.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      SERIAL_DEBUG.printf("GET");
    else if(request->method() == HTTP_POST)
      SERIAL_DEBUG.printf("POST");
    else if(request->method() == HTTP_DELETE)
      SERIAL_DEBUG.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      SERIAL_DEBUG.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      SERIAL_DEBUG.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      SERIAL_DEBUG.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      SERIAL_DEBUG.printf("OPTIONS");
    else
      SERIAL_DEBUG.printf("UNKNOWN");

    SERIAL_DEBUG.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      SERIAL_DEBUG.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      SERIAL_DEBUG.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      SERIAL_DEBUG.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        SERIAL_DEBUG.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        SERIAL_DEBUG.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        SERIAL_DEBUG.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      SERIAL_DEBUG.printf("UploadStart: %s\n", filename.c_str());
    SERIAL_DEBUG.printf("%s", (const char*)data);
    if(final)
      SERIAL_DEBUG.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      SERIAL_DEBUG.printf("BodyStart: %u\n", total);
    SERIAL_DEBUG.printf("%s", (const char*)data);
    if(index + len == total)
      SERIAL_DEBUG.printf("BodyEnd: %u\n", total);
  });

  // Set on board led GPIO to outout if not GPIO2 and Debug Serial1
  #if (SERIAL_DEBUG != Serial1) && (BUILTIN_LED != 2)
  pinMode(BUILTIN_LED, OUTPUT);
  #endif

  // Start Server
  WiFiMode_t con_type = WiFi.getMode();
  server.begin();
  SERIAL_DEBUG.print(F("Started "));

  if (con_type == WIFI_STA) {
    SERIAL_DEBUG.print(F("WIFI_STA"));
  } else if (con_type == WIFI_AP_STA) {
    SERIAL_DEBUG.print(F("WIFI_AP_STA"));
  } else if (con_type == WIFI_AP) {
    SERIAL_DEBUG.print(F("WIFI_AP"));
  } else {
    SERIAL_DEBUG.print(F("????"));
  }

  SERIAL_DEBUG.print(F("on HTTP://"));
  SERIAL_DEBUG.print(WiFi.localIP());
  SERIAL_DEBUG.print(F(" and WS://"));
  SERIAL_DEBUG.print(WiFi.localIP());
  SERIAL_DEBUG.println(F("/ws"));
}

/* ======================================================================
Function: loop
Purpose : infinite loop main code
Input   : -
Output  : - 
Comments: -
====================================================================== */
void loop() {
  static bool led_state ; 
  bool new_led_state ; 

  #ifdef SERIAL_DEVICE
  // Got one serial char ?
  if (SERIAL_DEVICE.available()) {
    // Read it and store it in buffer
    char inChar = (char)SERIAL_DEVICE.read();

    // CR line char, discard ?
    if (inChar == '\r') {
      // Do nothing

    // LF ok let's do our job
    } else if (inChar == '\n') {
      // Send to all client without cr/lf
      ws.textAll(inputString);
      // Display on debug
      SERIAL_DEBUG.printf("  <- \"%s\"\r\n", inputString.c_str());

      inputString = "";
    } else {
      // Add char to input string
      if (inChar>=' ' && inChar<='}')
        inputString += inChar;
      else
        inputString += '.';
    }
  }
  #endif

  // Led blink management 
  if (WiFi.status()==WL_CONNECTED) {
    new_led_state = ((millis() % 1000) < 200) ? LOW:HIGH; // Connected long blink 200ms on each second
  } else {
    new_led_state = ((millis() % 333) < 111) ? LOW:HIGH;// AP Mode or client failed quick blink 111ms on each 1/3sec
  }
    // Led management
  if (led_state != new_led_state) {
    led_state = new_led_state;

    #if (SERIAL_DEBUG != Serial1) && (BUILTIN_LED != 2)
      digitalWrite(BUILTIN_LED, led_state);
    #endif
  }

  // Handle remote Wifi Updates
  ArduinoOTA.handle();
}
