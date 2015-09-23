![new esphttpd](https://github.com/izhak2/esphttpd/blob/master/images/screenshot.png)

# esp-httpd #
### Background ###
Web-server for Espressif ESP8266/EX chips.
Spritemods started the project, and it was continued by billprozac which added OTA functionality and few more things.
me, izhak2, re-wrote the whole project, fixed and added few more other things.

### Firmware features ###
* Web server
  * Multiple connections
  * CGI routines
  * HTML tokens replacement
* Filesystem support
  * Compression support (Heatshrink)
* OTA
* **Documentation and code-readability**

### Changelog ###
For changelog from billprozac's version look at changelog.txt

### Sample project ###
* web-server available at 192.168.4.1.
* GPIOs are reset to state 0 on initialization.
* index.tpl displays a sample switch which changes its state on-click, and keeps its state on server-side
  * the switch on index.tpl controls both GPIOs' states
* WiFi settings enable switching between modes as usual

### Setup ###
Follow instructions on https://github.com/pfalcon/esp-open-sdk, then customize makefile parameters to match your own environment:
XTENSA_TOOLS_ROOT, SDK_EXTRA_INCLUDES, SDK_EXTRA_LIBS, SDK_ROOT, SDK_VERSION

### Git ###
1. git clone https://github.com/izhak2/esphttpd
2. cd esphttpd
3. git submodule init
4. git submodule update

Note: I recommend to change manually lib\heatshrink\heatshrink_config.h to use:
"#define HEATSHRINK_MALLOC(SZ) os_malloc(SZ)"
"#define HEATSHRINK_FREE(P, SZ) os_free(P)"
just in case to prevent compiling errors.

### Compiling ###
To compile an OTA version:
make clean
make rawflash OTA=true

### Development ###
As web server is pretty useful for IOT usages, especially on this chip, I hope you find this version easy to adapt for your own purpose.
I plan to expand the feature list to make this distribution more useful, so feel free to contact me either for bugs found or with ideas for new features.
