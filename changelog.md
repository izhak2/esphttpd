# Changelog

#### September '15 ####
* General
  * documentation
  * code conventions and readability
    * local variables initialization
    * naming
    * goto lblCleanup on errors
    * debug strings for each module
    * single include per .c file
  * removed unused code
  * removed unused headers
  * new sample web page
  * rearrange tree - code at /user, headers at /include and general headers at /ext_include
* cgiwifi.c - cgiWifiSetMode - added check for user-entered mode to prevent chip from losing connectivity
* added config.h - quickly change settings (hotspot, debug strings, multiple connections, etc.)
* added shared.h - prevent duplicates of common stuff
* updated makefile
* limited exported urls
* checked under latest SDK (1.3)
