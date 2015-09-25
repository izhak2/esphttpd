![new esphttpd](https://github.com/izhak2/esphttpd/blob/master/images/screenshot.png)

# esp-httpd #
### Background ###
Web-server for Espressif ESP8266/EX chips.
Spritemods started the project and billprozac continued, adding OTA functionality and few more things.

I, izhak2, re-wrote the whole project, fixed and added few more other things.

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
For changelog from billprozac's version look at changelog.md.

### Sample project ###
* web-server available at 192.168.4.1
* GPIOs are reset to state 0 on initialization
* index.tpl displays a sample switch which changes its state on-click, and keeps its state on server-side
  * the switch on index.tpl controls both GPIOs' states
* WiFi settings enable switching between modes as usual

### Setup ###
Step-by-step guide available [here](iotplayground.blogspot.com/2015/09/introduction-to-esp8266ex-development.html).
Follow the instructions on https://github.com/pfalcon/esp-open-sdk, then customize makefile parameters to match for your own environment:

* XTENSA_TOOLS_ROOT
* SDK_EXTRA_INCLUDES
* SDK_EXTRA_LIBS
* SDK_ROOT
* SDK_VERSION

### Git ###
* git clone https://github.com/izhak2/esphttpd
* cd esphttpd
* git submodule init
* git submodule update

Note: I recommend to change manually lib\heatshrink\heatshrink_config.h to use:

* #define HEATSHRINK_MALLOC(SZ) os_malloc(SZ)
* #define HEATSHRINK_FREE(P, SZ) os_free(P)

just in case to prevent compiling errors.

### Compiling ###
Have a look at config.h and set your own settings, then compile as usual.
For example, to compile an OTA version:

make clean

make rawflash OTA=true

### Development ###
As web server is pretty useful for IOT usages, especially on this chip, I hope you find my project as a convenient starting point.
I plan to expand the supported feature list to make it even more useful, so feel free to contact me either for bugs found or with ideas for new features.

<izhak2@gmail.com>
