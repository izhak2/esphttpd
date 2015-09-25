###############################################
#   dummy libcurl wrapper for ota flashing    #
#   by izhak2@gmail.com                       #
###############################################

import subprocess
import sys

def main(ip_addr):
    print "checking current boot mode for ip %s" % ip_addr
    curl = subprocess.Popen(("curl --connect-timeout 1000 -s %s/getappver.cgi" % ip_addr), 
                            stdout = subprocess.PIPE)
    mode = curl.stdout.read()
    print "current mode is ", mode
    key = raw_input("press 'y' to flash firmware\n")
    if ('y' == key):
        if ('1' == mode):
            print "uploading user1.new.bin"
            ota = subprocess.Popen(("curl -i -X POST %s/flashapp.cgi --data-binary \"@user2.new.bin\"" % ip_addr),
                                stdout = subprocess.PIPE)
            print (curl.stdout.read())
            ota.wait()
        elif ('2' == mode):
            print "uploading user2.new.bin"
            ota = subprocess.Popen(("curl -i -X POST %s/flashapp.cgi --data-binary \"@user1.new.bin\"" % ip_addr),
                                stdout = subprocess.PIPE)
            print(curl.stdout.read())
            ota.wait()
    key = raw_input("press 'y' to flash filesystem\n")
    if ('y' == key):
        print "uploading website.espfs"
        ota = subprocess.Popen(("curl -i -X POST %s/flashraw.cgi --data-binary \"@website.espfs\"" % ip_addr),
                            stdout = subprocess.PIPE)
        print(curl.stdout.read())
        ota.wait()
    
    print "completed."
        
if  __name__ == "__main__":
    if len(sys.argv) < 2:
        print 'Usage: %s esp_ip' % sys.argv[0]
    else:
        main(sys.argv[1])
