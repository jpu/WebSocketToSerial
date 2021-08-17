
#ifndef _WEBSOCKETTOSERIAL_H
#define _WEBSOCKETTOSERIAL_H

#include <arduino.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>

/* 
 *  FOR NETWORK AND HARDWARE SETTINGS COPY OR RENAME 'myconfig.sample.h' TO 'myconfig.h' AND EDIT THAT.
 *
 * By default this sketch will assume an AI-THINKER ESP-CAM and create
 * an accesspoint called "ESP32-CAM-CONNECT" (password: "InsecurePassword")
 *
 */

struct station { const char ssid[65]; const char password[65]; const bool dhcp;};  // do no edit
#include "myconfig.h"

// Define here the target serial you connected to this app
// -------------------------------------------------------
// Classic terminal connected to RX/TX of ESP8266 
// using Arduino IDE or other terminal software
#define MOD_TERMINAL

#ifdef MOD_TERMINAL
#define SERIAL_DEVICE Serial
#define SERIAL_DEBUG  Serial1
#endif

// Maximum number of simultaned clients connected (WebSocket)
#define MAX_WS_CLIENT 5

// Client state machine
#define CLIENT_NONE     0
#define CLIENT_ACTIVE   1

#define HELP_TEXT "[[b;green;]WebSocket2Serial HELP]\n" \
                  "---------------------\n" \
                  "[[b;cyan;]/?] or [[b;cyan;]/help] show this help\n" \
                  "[[b;cyan;]/swap]      swap serial UART pin to GPIO15/GPIO13\n" \
                  "[[b;cyan;]/ping]      send ping command\n" \
                  "[[b;cyan;]/heap]      show free RAM\n" \
                  "[[b;cyan;]/whoami]    show client # we are\n" \
                  "[[b;cyan;]/who]       show all clients connected\n" \
                  "[[b;cyan;]/fw]        show firmware date/time\n"  \
                  "[[b;cyan;]/baud]      show current serial baud rate\n" \
                  "[[b;cyan;]/baud n]    set serial baud rate to n\n" \
                  "[[b;cyan;]/reset p]   do a reset pulse on gpio pin number p\n" \
                  "[[b;cyan;]/hostname]  show network hostname of device\n" \
                  "[[b;cyan;]/debug]     show debug information\n" \

// Web Socket client state
typedef struct {
  uint32_t  id;
  uint8_t   state;
} _ws_client; 


// Exported vars
extern AsyncWebSocket ws;

void execCommand(AsyncWebSocketClient * client, char * msg) ;

#endif



