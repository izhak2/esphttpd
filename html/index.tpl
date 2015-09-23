<html>
<head>
	<link rel="stylesheet" type="text/css" href="style.css">
    <script type="text/javascript" src="140medley.min.js"></script>
    <script type="text/javascript">
    
    var xhr=j();
    var currAp="%currSsid%";
    
    function createInputForAp(ap) {
        if (ap.essid=="" && ap.rssi==0) return;
        var div=document.createElement("div");
        div.id="apdiv";
        var rssi=document.createElement("div");
        var rssiVal=-Math.floor(ap.rssi/51)*32;
        rssi.className="icon";
        rssi.style.backgroundPosition="0px "+rssiVal+"px";
        var encrypt=document.createElement("div");
        var encVal="-64";
        if (ap.enc=="0") encVal="0";
        if (ap.enc=="1") encVal="-32";
        encrypt.className="icon";
        encrypt.style.backgroundPosition="-32px "+encVal+"px";
        var input=document.createElement("input");
        input.type="radio";
        input.name="essid";
        input.value=ap.essid;
        input.style.display="block";
        if (currAp==ap.essid) input.checked="1";
        input.id="opt-"+ap.essid;
        var label=document.createElement("label");
        label.htmlFor="opt-"+ap.essid;
        label.textContent=ap.essid;
        div.appendChild(input);
        div.appendChild(rssi);
        div.appendChild(encrypt);
        div.appendChild(label);
        return div;
    }
    function getSelectedEssid() {
        var e=document.forms.wifiform.elements;
        for (var i=0; i<e.length; i++) {
            if (e[i].type=="radio" && e[i].checked) return e[i].value;
        }
        return currAp;
    }
    function scanAPs() {
        xhr.open("GET", "wifiscan.cgi");
        xhr.onreadystatechange=function() {
            if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
                var data=JSON.parse(xhr.responseText);
                currAp=getSelectedEssid();
                if (data.result.inProgress=="0" && data.result.APs.length>1) {
                    $("#aps").innerHTML="";
                    for (var i=0; i<data.result.APs.length; i++) {
                        if (data.result.APs[i].essid=="" && data.result.APs[i].rssi==0) continue;
                        $("#aps").appendChild(createInputForAp(data.result.APs[i]));
                    }
                    window.setTimeout(scanAPs, 20000);
                } else {
                    window.setTimeout(scanAPs, 1000);
                }
            }
        }
        xhr.send();
    }
    window.onload=function(e) {
        scanAPs();
    };
    function handleClick() {
        f = document.createElement("form");
        f.method="post";
        f.action="led.cgi";
        i = document.createElement("input");
        i.type="hidden";
        i.name="switch";
        i.value="1";
        f.appendChild(i);
        document.body.appendChild(f);
        f.submit();
    }
    </script>
</head>
<body>
<h1>ESP8266 Sample Page</h1>
<div class="content">
    <div class=round>
        <input type=checkbox id=onoff onchange='handleClick(this);' %ledStatus%/>
        <div class=back>
            <label class=but for=onoff>
                <span class=on>I</span>
                <span class=off>0</span>
            </label>
        </div>
    </div>
</div>
<div class="box">
	<a class="button" href="#popup1">Wifi Settings</a>
</div>
<div id="popup1" class="overlay">
	<div class="popup">
		<h2>Wifi Settings</h2>
		<a class="close" href>x</a>
		<div class="content">
            <p>
            Current WiFi mode: %WiFiMode%
            Note: %WiFiapwarn%
            </p>
            <form name="wifiform" action="connect.cgi" method="post">
            To connect to a WiFi network, please select one of the detected networks<br>
            <div id="aps">Scanning...</div>
            <br>
            WiFi password, if applicable: <br />
            <input type="text" name="passwd" val="%WiFiPasswd%" style="display:block">
            <br/>
            <input type="submit" name="connect" value="Connect!" style="display:block">
            </form>
            </p>
        </div>    
	</div>
</div>
<div class="footer" style="position:absolute;right:5px;bottom:5px;text-align:center">Boot partition - (%boot%)</div>
</body>
</html>