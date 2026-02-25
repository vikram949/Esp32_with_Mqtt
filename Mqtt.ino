#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <Preferences.h>

#define LED_BUILTIN 2
#endif

const char* mqtt_server = "your_mqtt_server";
WiFiClient espClient;
PubSubClient client(espClient);

WebServer server(80);
Preferences preferences;

String deviceID = "LIGHT_" + String((uint32_t)ESP.getEfuseMac());
String topic = "lights/" + deviceID;

bool configMode = false;
String scannedNetworks = "";
bool networksScanDone = false;

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WiFi Setup</title>
  <style>
    body { 
      font-family: Arial, sans-serif; 
      margin: 0; 
      padding: 20px; 
      background: #f0f0f0; 
    }
    .container { 
      max-width: 400px; 
      margin: 0 auto; 
      background: white; 
      padding: 20px; 
      border-radius: 10px; 
      box-shadow: 0 2px 10px rgba(0,0,0,0.1); 
    }
    h2 { color: #333; text-align: center; }
    select, input { 
      width: 100%; 
      padding: 10px; 
      margin: 10px 0; 
      border: 1px solid #ddd; 
      border-radius: 5px; 
      box-sizing: border-box; 
    }
    button { 
      background: #4CAF50; 
      color: white; 
      border: none; 
      cursor: pointer; 
    }
    button:hover { background: #45a049; }
    .refresh-btn { 
      background: #2196F3; 
      margin-bottom: 10px; 
    }
    .refresh-btn:hover { background: #1976D2; }
  </style>
</head>
<body>
  <div class="container">
    <h2>WiFi Setup</h2>
    <button class="refresh-btn" onclick="refreshNetworks()">Refresh Networks</button>
    <form action="/setup" method="POST">
      <select name="ssid" id="networkSelect" required>
        <option value="">Loading networks...</option>
      </select>
      <input type="password" name="pass" placeholder="WiFi Password">
      <button type="submit">Connect</button>
    </form>
  </div>
  
  <script>
    function loadNetworks() {
      fetch('/networks')
        .then(response => response.text())
        .then(data => {
          document.getElementById('networkSelect').innerHTML = data;
        })
        .catch(error => {
          document.getElementById('networkSelect').innerHTML = '<option value="">Error loading networks</option>';
        });
    }
    
    function refreshNetworks() {
      document.getElementById('networkSelect').innerHTML = '<option value="">Scanning...</option>';
      fetch('/scan')
        .then(() => {
          setTimeout(loadNetworks, 3000);
        });
    }
    
    // Load networks when page loads
    setTimeout(loadNetworks, 1000);
  </script>
</body>
</html>
)rawliteral";

void saveCredentials(String ssid, String pass) {
  Serial.println("Saving credentials...");
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", pass);
  preferences.end();
  Serial.println("Credentials saved successfully!");
}

bool connectToWiFi(String ssid, String pass) {
  Serial.println("Attempting WiFi connection...");
  Serial.println("SSID: " + ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\nWiFi Connection Failed!");
    return false;
  }
}

bool loadStoredCredentials() {
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("password", "");
  preferences.end();
  
  if (ssid.length() > 0 && ssid.length() < 32) {
    Serial.println("Found stored credentials for: " + ssid);
    return connectToWiFi(ssid, pass);
  } else {
    Serial.println("No valid stored credentials");
    return false;
  }
}

void scanNetworksAsync() {
  Serial.println("Starting network scan...");
  
  // Temporarily switch to STA mode for scanning
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  
  int n = WiFi.scanNetworks(true); // Async scan
  if (n == WIFI_SCAN_RUNNING) {
    Serial.println("Scan started...");
  }
}

void checkScanResults() {
  int n = WiFi.scanComplete();
  
  if (n >= 0) {
    Serial.println("Scan complete. Found " + String(n) + " networks");
    scannedNetworks = "<option value=''>-- Select Network --</option>";
    
    for (int i = 0; i < n && i < 15; i++) {
      String ssid = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);
      
      if (ssid.length() > 0 && ssid.length() < 32) {
        scannedNetworks += "<option value='" + ssid + "'>" + ssid + " (" + String(rssi) + "dBm)</option>";
      }
    }
    
    networksScanDone = true;
    WiFi.scanDelete();
    
    // Switch back to AP mode
    WiFi.mode(WIFI_AP);
  }
}

void startConfigMode() {
  configMode = true;
  Serial.println("Starting Configuration Mode...");
  
  WiFi.mode(WIFI_AP);
  bool success = WiFi.softAP("Smart_Bulb_Setup", "12345678");
  
  if (success) {
    Serial.println("AP Started Successfully!");
    Serial.println("Connect to: Smart_Bulb_Setup");
    Serial.println("Password: 12345678");
    Serial.println("Go to: http://192.168.4.1");
  }
  
  // Setup routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });
  
  server.on("/networks", HTTP_GET, []() {
    if (networksScanDone) {
      server.send(200, "text/html", scannedNetworks);
    } else {
      server.send(200, "text/html", "<option value=''>Scanning networks...</option>");
    }
  });
  
  server.on("/scan", HTTP_GET, []() {
    networksScanDone = false;
    scanNetworksAsync();
    server.send(200, "text/plain", "Scan started");
  });
  
  server.on("/setup", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    
    if (ssid.length() == 0) {
      server.send(400, "text/html", "<h2>Error: Please select a network!</h2>");
      return;
    }
    
    server.send(200, "text/html", 
      "<h2>Connecting...</h2><p>Please wait while connecting to: " + ssid + "</p>"
      "<script>setTimeout(() => window.close(), 10000);</script>");
    
    delay(1000);
    
    // Save and connect
    saveCredentials(ssid, pass);
    server.stop();
    WiFi.softAPdisconnect(true);
    
    if (connectToWiFi(ssid, pass)) {
      Serial.println("Connection successful! Restarting...");
      delay(2000);
      ESP.restart();
    } else {
      Serial.println("Connection failed. Restarting config mode...");
      delay(2000);
      startConfigMode();
    }
  });
  
  server.begin();
  
  // Start initial network scan
  delay(1000);
  scanNetworksAsync();
}

void callback(char* topics, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("MQTT: " + message);
  
  if (String(topics) == topic) {
    if (message == "1") {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("LED ON");
    } else if (message == "0") {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("LED OFF");
    } else if (message == "config") {
      Serial.println("Config mode requested");
      startConfigMode();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Bulb Starting ===");
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.println("Device ID: " + deviceID);
  Serial.println("MQTT Topic: " + topic);
  
  // Clear any corrupted EEPROM data
  preferences.begin("wifi", false);
  if (preferences.getString("ssid", "").indexOf('\0') != -1 || 
      preferences.getString("ssid", "").length() > 32) {
    Serial.println("Clearing corrupted WiFi data...");
    preferences.clear();
  }
  preferences.end();
  
  // Try to connect with stored credentials
  if (!loadStoredCredentials()) {
    startConfigMode();
  } else {
    // Setup MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }
}

void reconnectMQTT() {
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(deviceID.c_str())) {
      Serial.println("Connected!");
      client.subscribe(topic.c_str());
    } else {
      Serial.println("Failed");
    }
  }
}

void loop() {
  static String serialData = "";
  static unsigned long lastMQTTAttempt = 0;
  
  // Handle config mode
  if (configMode) {
    server.handleClient();
    
    // Check scan results
    if (!networksScanDone) {
      checkScanResults();
    }
    
    // Handle serial commands in config mode
    if (Serial.available()) {
      serialData = Serial.readStringUntil('\n');
      serialData.trim();
      
      if (serialData == "scan") {
        Serial.println("Manual scan requested");
        networksScanDone = false;
        scanNetworksAsync();
      } else if (serialData == "networks") {
        Serial.println("Available networks:");
        Serial.println(scannedNetworks);
      }
      serialData = "";
    }
    
    return;
  }
  
  // Handle serial commands in normal mode
  if (Serial.available()) {
    serialData = Serial.readStringUntil('\n');
    serialData.trim();
    
    if (serialData == "config" || serialData == "setup") {
      startConfigMode();
    } else if (serialData == "restart") {
      ESP.restart();
    } else if (serialData == "clear") {
      preferences.begin("wifi", false);
      preferences.clear();
      preferences.end();
      Serial.println("WiFi credentials cleared");
      ESP.restart();
    } else if (serialData == "status") {
      Serial.println("WiFi: " + String(WiFi.status()));
      Serial.println("IP: " + WiFi.localIP().toString());
      Serial.println("MQTT: " + String(client.connected()));
    }
    serialData = "";
  }
  
  // Handle MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected() && millis() - lastMQTTAttempt > 5000) {
      reconnectMQTT();
      lastMQTTAttempt = millis();
    }
    client.loop();
  } else {
    // WiFi disconnected, try to reconnect
    Serial.println("WiFi disconnected, attempting reconnect...");
    if (!loadStoredCredentials()) {
      startConfigMode();
    }
  }
  
  yield();
}
