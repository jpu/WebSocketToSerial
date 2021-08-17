Transparent TCP Network to Serial Proxy using WebSocket for ESP8266
===================================================================

This is a pure transparent bridge between Wifi and serial using any ESP8266 device. It's very useful for debugging or talking to remote serial device that have no network connection.

I'm using it on WeMos target, you can find more information on WeMos on their [site][1], it's really well documented.
I now use WeMos boards instead of NodeMCU's one because they're just smaller, features remains the same, but I also suspect WeMos regulator far better quality than the one used on NodeMCU that are just fake of originals AMS117 3V3.

This project is mainly based on excellent @me-no-dev [ESPAsyncWebServer][4] library and great [JQuery Terminal][3] done by Jakub Jankiewicz.

Documentation
=============

Once uploaded SPIFFS data (web page) you can connect with a browser to http://ip_of_esp8266 and start playing with it.
The main `index.htm` web page include a full javascript terminal so you can type command and receive response.

The main web page can also be hosted anywhere and it's not mandatory to have it on the device (except if device and your computer have no access on Internet). I've published the fully fonctionnal WEB page from github so you can access it from [here][8] and then connect to your device on wich you flashed the firmware.

Some commands will be interpreted by the target (ESP8266) and not passed to serial, so you can interact with ESP8266 doing some variable stuff.

Test web page without ESP8266
-----------------------------

You need to have [nodejs][7] and some dependencies installed.

[webdev][9] folder is the development folder to test and validate web pages. It's used to avoid flashing the device on each modification.
All source files are located in this folder the ESP8266 `data` folder (containing web pages) is filled by a nodejs script launched from [webdev][9] folder. This repo contain in [data][12] lasted files so if you don't change any file, you can upload to SPIFFS as is.

To test the UI locally:
 * go to a command line
 * cd into the [webdev][9] folder
 * install dependencies with `npm install`
 * run with `npm run serve`
 * connect your browser to http://localhost:8080
 
If you want to modify the source, run with `npm run dev` to get automatic reloading of the server code.
    
Once all is okay issue a:    
`npm run build`     
this will gzip file and put them into [data][12] folder, after that you can upload from Arduino IDE to device SPIFFS

See comments in both [create_spiffs.js][10] and [web_server.js][11] files.

Terminal Commands:
------------------
- connect : connect do target device
- help : show help

Commands once connected to remote device:
-----------------------------------------
- `/close` or `/quit` or CTRL-D: close connection
- `/exit` exit the session, leave connection open
- `/cls` or `/clear` clear the screen
- `/swap` swap ESP8266 UART pin between GPIO1/GPIO3 with GPIO15/GPIO13
- `/ping` typing ping on terminal and ESP8266 will send back pong
- `/?` or `/help` show help
- `/heap` show ESP8266 free RAM
- `/whoami` show WebSocket client # we are
- `/who` show all WebSocket clients connected
- `/fw` show firmware date/time
- `/baud n` set ESP8266 serial baud rate to n (to be compatble with device driven)
- `/reset p` reset gpio pin number p


Every command in file `startup.ini` are executed in the setup() method. 

See all in action    
http://cdn.rawgit.com/hallard/WebSocketToSerial/master/webdev/index.htm

Known Issues/Missing Features:
------------------------------
- More configuration features

Dependencies
------------
- Arduino [ESP8266][6]
- @me-no-dev [ESPAsyncWebServer][4] library
- @me-no-dev [ESPAsyncTCP][5] library 
- [nodejs][7] for web pages development test 

Misc
----
See news and other projects on my [blog][2] 
 
[1]: http://www.wemos.cc/
[2]: https://hallard.me
[3]: http://terminal.jcubic.pl/
[4]: https://github.com/me-no-dev/ESPAsyncWebServer
[5]: https://github.com/me-no-dev/ESPAsyncTCP
[6]: https://github.com/esp8266/Arduino/blob/master/README.md
[7]: https://nodejs.org/
[8]: http://cdn.rawgit.com/hallard/WebSocketToSerial/master/webdev/index.htm
[9]: https://github.com/hallard/WebSocketToSerial/tree/master/webdev
[10]: https://github.com/hallard/WebSocketToSerial/blob/master/webdev/create_spiffs.js
[11]: https://github.com/hallard/WebSocketToSerial/blob/master/webdev/web_server.js
[12]: https://github.com/hallard/WebSocketToSerial/tree/master/data
