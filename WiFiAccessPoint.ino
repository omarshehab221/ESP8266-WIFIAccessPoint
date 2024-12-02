#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

// WiFi Access Point credentials
const char *apSSID = "ESPap";
const char *apPassword = "thereisnospoon";

WebSocketsServer webSocket(81);  // WebSocket server on port 81
#define MAX_MESSAGES 50
String messages[MAX_MESSAGES];
int messageCount = 0;

// Web server on port 80
ESP8266WebServer server(80); // Web Server on port 80

// Variables to track the LED state
bool ledState = false;
bool isBlinking = false;
unsigned long previousMillis = 0;
unsigned long blinkInterval = 200;  // 200ms for blink duration
int blinkCount = 0;
int maxBlinks = 6;  // Blink 3 times (on/off counts as 2)

String connectedSSID = "";

// Track the number of connected devices
int previousStationCount = 0;

// Variables for WiFi scan and connection
String networksJson = "[]";
bool scanning = false;

// Root route handler
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP8266 LED Controller</title>";
  html += "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">";
  html += "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>";
  html += "<link href=\"https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap\" rel=\"stylesheet\">";
  html += "<style>* { margin: 0; padding: 0; font-family: \"Inter\", sans-serif; } html { -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; }";
  html += "body { min-width: 100%; min-height: 100svh; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 1.5rem; }";
  html += "h1 { font-weight: 900; font-size: 2rem; line-height: 2rem; margin: 0; } span.state { font-size: 1.25rem; }";
  html += "a { display: block; min-width: fit-content; text-decoration: none; color: #000000; padding: 1rem 1.5rem; border: 2px solid #000000; border-radius: 0.5rem; cursor: pointer; }";
  html += "a.primary { color:#ffffff; background-color:#000000; border:none; }";
  html += "button { display: block; min-width: fit-content; font-size: 1rem; color: #000000; background-color: transparent; padding: 1rem 1.5rem; border: 2px solid #000000; border-radius: 0.5rem; cursor: pointer; text-align: left; }";
  html += "button.primary { color:#ffffff; background-color:#000000; border:none; }";
  html += "div.buttons { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 1rem; min-width: fit-content; }</style>";

  // Add JavaScript for handling the toggle
  html += "<script>function toggleLED() {fetch('/toggle').then(response => response.json()).then(data => {";
  html += "document.getElementById('ledState').textContent = data.ledState ? 'ON' : 'OFF';});}";
  html += "function blinkLED() {fetch('/blink').then(response => response.json()).then(data => {console.log(`${data.ledState}`)});}</script></head>";

  html += "<body><span class=\"state\">" + (WiFi.status() == WL_CONNECTED ? ("Connected to <strong>" + WiFi.SSID() + "</strong>") : "<strong>Not connected to the internet</strong>") + "</span><h1>ESP8266 LED Controller</h1>";
  html += "<div class=\"buttons\"><div style=\"grid-column: span 2 / span 2;display: flex;width: 100%;justify-content: space-between;align-items: center;\">";
  html += "<div style=\"display: flex;gap: 0.25rem;font-size: 1.125rem;\"><span>LED State:</span><span id=\"ledState\" style=\"font-weight: 700\">" + String(ledState ? "ON" : "OFF") + "</span></div>";
  html += "<button class=\"primary\" onclick=\"toggleLED()\" style=\"padding: 0.75rem 1.25rem;\">Toggle LED</button></div>";
  html += "<button onclick=\"blinkLED()\">Blink LED</button><a href=\"/scan\">Scan Networks</a>";
  html += "<a href=\"/hidden\">Connect to Hidden Network</a><a href=\"/chat\">ChatRoom</a></div></body></html>";

  server.send(200, "text/html", html);
}

// Toggle the LED state
void toggleLED() {
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);  // LOW = LED ON
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", String("{\"ledState\":") + (ledState ? "true" : "false") + "}");
}

// Get the LED state
void getLEDState() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", String("{\"ledState\":") + (ledState ? "true" : "false") + "}");
}

// Blink the LED
void blinkLED() {
  digitalWrite(LED_BUILTIN, HIGH);
  isBlinking = true;
  blinkInterval = 100;
  blinkCount = 0;
  maxBlinks = 15;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", String("{\"ledState\": \"blinking\"}"));
}

// Scan for WiFi networks
void scanNetworks() {
  if (scanning) {
    server.send(503, "application/json", "{\"error\":\"Already scanning\"}");
    return;
  }

  scanning = true;
  int n = WiFi.scanNetworks();
  // networksJson = "[";

  // for (int i = 0; i < n; ++i) {
  //   networksJson += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"open\":" + (WiFi.encryptionType(i) == AUTH_OPEN ? "true" : "false") + "}";
  //   if (i < n - 1) networksJson += ",";
  // }

  // networksJson += "]";
  scanning = false;

  // String contentType = server.header("Content-Type");

  // if (contentType == "text/html") {
  // Generate HTML response
  String html = "<!DOCTYPE html><html><head><title>Available Networks</title>";
  html += "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">";
  html += "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>";
  html += "<link href=\"https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap\" rel=\"stylesheet\">";
  html += "<style>* { margin: 0; padding: 0; font-family: \"Inter\", sans-serif; } html { -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; }";
  html += "body { min-width: 100%; min-height: 100svh; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 1.5rem; }";
  html += "div.container { display: flex; flex-direction: column; gap: 1rem; min-width: 30rem; }";
  html += "h1 { font-weight: 900; font-size: 2rem; line-height: 2rem; margin: 0; }";
  html += "div.input-wrapper { display: flex; flex-direction: column; gap: 0.25rem; width: 100%; }";
  html += "input { display: block; min-width: fit-content; text-decoration: none; color: #000000; padding: 0.5rem 0.75rem; border: 2px solid #000000; border-radius: 0.5rem }";
  html += "th { text-align: left; font-weight: 600; font-size: 0.9rem; color: #6c6c6c; } th:last-of-type { text-align: center; }";
  html += "col { min-width: 12rem; } td { padding: 0.4rem 0; } td > button { margin: 0 auto; }";
  html += "button { display: block; min-width: fit-content; font-size: 1rem; color: #000000; background-color: transparent; padding: 0.75rem 0.9rem; border: 2px solid #000000; border-radius: 0.5rem; cursor: pointer; }";
  html += "button.primary { color:#ffffff; background-color:#000000; border:none; }</style>";
  html += "<script>function connectToNetwork(ssid, passwordIndex) { document.getElementById(`connect${passwordIndex}`).textContent = \"Connecting...\";";
  html += "const password = document.getElementById(`password${passwordIndex}`).value;";
  html += "fetch(\"/connect\", { method: \"POST\", headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, ";
  html += "body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}` })";
  html += ".then(request => request.json()).then(data => { document.getElementById(`connect${passwordIndex}`).textContent = data.status; setTimeout(() => {window.location.href = '/';}, 1000); })}</script></head>";
  html += "<body><h1>Available Networks</h1><table><colgroup><col /><col /><col /><col class=\"actions\"></colgroup>";
  html += "<thead><tr><th>SSID</th><th>RSSI</th><th>Access</th><th>Actions</th></tr></thead><tbody>";

  for (int i = 0; i < n; ++i) {
    html += "<tr><td style=\"font-weight: 700;\">" + WiFi.SSID(i) + "</td><td style=\"font-size: 0.8rem;\">" + String(WiFi.RSSI(i)) + "</td>";
    html += "<td>" + String(WiFi.encryptionType(i) == AUTH_OPEN ? " Open" : " <input type=\"password\" id=\"password" + String(i) + "\" name=\"password\" placeholder=\"Enter the network password\" />") + "</td>";
    html += "<td><button id=\"connect" + String(i) + "\" class=\"primary\" onclick=\"connectToNetwork('" + WiFi.SSID(i) + "', " + String(i) + ")\">Connect</button></td></tr>";
  }

  html += "</tbody></table></body></html>";

  scanning = false;

  server.send(200, "text/html", html);

  // } else {
  //   // Default to JSON response
  //   server.sendHeader("Access-Control-Allow-Origin", "*");
  //   server.send(200, "application/json", networksJson);
  // }
}

// Connect to a WiFi network
void connectToNetwork() {
  if (!server.hasArg("ssid")) {
    server.send(400, "application/json", "{\"error\":\"Missing ssid\"}");
    return;
  }

  String ssid = server.arg("ssid");
  String password = server.hasArg("password") ? server.arg("password") : "";  // Allow empty passwords for open networks

  WiFi.mode(WIFI_AP_STA);

  // WiFi.softAPdisconnect(true);  // Stop the AP before connecting to the new network
  WiFi.begin(ssid.c_str(), password.isEmpty() ? nullptr : password.c_str());

  // Wait for connection for a few seconds
  // Optionally, print connection status in the serial monitor
  Serial.println("Attempting to connect to network:");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + (password.isEmpty() ? "(no password)" : password));

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  String response;
  if (WiFi.status() == WL_CONNECTED) {
    connectedSSID = ssid;
    response = "{\"status\":\"Connected\"}";
    if (!isBlinking) {
      digitalWrite(LED_BUILTIN, HIGH);
      isBlinking = true;
      blinkInterval = 500;
      blinkCount = 0;
      maxBlinks = 4;
    }
  } else {
    response = "{\"status\":\"Failed to connect\"}";
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

// Form for connecting to hidden networks
void handleHidden() {
  String html = "<!DOCTYPE html><html><head><title>Connect to Hidden Network</title>";
  html += "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">";
  html += "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>";
  html += "<link href=\"https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap\" rel=\"stylesheet\">";
  html += "<style>* { margin: 0; padding: 0; font-family: \"Inter\", sans-serif; } html { -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; }";
  html += "body { min-width: 100%; min-height: 100svh; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 1.5rem; }";
  html += "form { display: flex; flex-direction: column; gap: 1rem; min-width: 30rem; }";
  html += "h1 { font-weight: 900; font-size: 2rem; line-height: 2rem; margin: 0; }";
  html += "div.input-wrapper { display: flex; flex-direction: column; gap: 0.25rem; width: 100%; }";
  html += "input { display: block; min-width: fit-content; text-decoration: none; color: #000000; padding: 0.75rem 0.9rem; border: 2px solid #000000; border-radius: 0.5rem }";
  html += "button { display: block; min-width: fit-content; font-size: 1rem; color: #000000; background-color: transparent; padding: 0.75rem 0.9rem; border: 2px solid #000000; border-radius: 0.5rem; cursor: pointer; }";
  html += "button.primary { color:#ffffff; background-color:#000000; border:none; }</style>";
  html += "<script> function connectToNetwork(event) {";
  html += "  event.preventDefault();";
  html += "  const ssidInput = document.getElementById('ssid');";
  html += "  const ssid = ssidInput.value;";
  html += "  const passwordInput = document.getElementById('password');";
  html += "  const password = passwordInput.value;";
  html += "  fetch('/connect', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
  html += "    body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`";
  html += "  });";
  html += "  ssidInput.value = '';";
  html += "  passwordInput.value = '';";
  html += "  window.location.href = '/';";
  html += "  return false;";
  html += "}";
  html += "</script>";
  html += "</head><body><h1>Connect to Hidden Network</h1><form onSubmit='return connectToNetwork(event)'>";
  html += "<div class=\"input-wrapper\"><label for=\"ssid\">SSID:</label><input type=\"text\" id=\"ssid\" name=\"ssid\"  placeholder=\"Enter the network SSID\" required></div>";
  html += "<div class=\"input-wrapper\"><label for=\"password\">Password:</label><input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter the network password\"></div>";
  html += "<button type=\"submit\" class=\"primary\">Connect</button>";
  html += "</form></body></html>";

  server.send(200, "text/html", html);
}
// Chat Room
void handleChat() {
  String html = "<!DOCTYPE html><html><head><title>Chat Room</title>";
  html += "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">";
  html += "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>";
  html += "<link href=\"https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap\" rel=\"stylesheet\">";
  html += "<style>* { margin: 0; padding: 0; font-family: \"Inter\", sans-serif; } html { -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; }";
  html += "*, *::before, *::after { box-sizing: border-box; } body { min-width: 100vw !important; min-height: 100svh; display: flex; flex-direction: column; align-items: center; gap: 1.5rem; }";
  html += "form { display: flex; flex-direction: column; margin: 1.5rem; flex: 1; width: 100%; } div.input { display: flex; align-items: center; gap: 1rem; }";
  html += "div.header { display: flex; justify-content: space-between; gap: 1rem; padding: 0 1rem; } h1 { font-weight: 900; font-size: 2rem; line-height: 2rem; margin: 0; }";
  html += "input { display: block; min-width: fit-content; text-decoration: none; color: #000000; padding: 0.75rem 0.9rem; border: 2px solid #000000; border-radius: 0.5rem; }";
  html += ".grow-wrap > textarea, .grow-wrap::after { display: block; width: 100%; text-decoration: none; color: #000000; padding: 0.5rem 0.9rem; border: 2px solid #000000; border-radius: 0.5rem; resize: none; grid-area: 1 / 1 / 2 / 2; white-space: pre-wrap; word-break: break-word; }";
  html += ".grow-wrap { display: grid; flex: 1; } .grow-wrap::after { content: attr(data-replicated-value); white-space: pre-wrap; visibility: hidden; }";
  html += "textarea { outline: none; overflow: hidden; line-height: 1.5rem; }";
  html += "button { display: block; min-width: fit-content; font-size: 1rem; color: #000000; background-color: transparent; padding: 0.75rem 0.9rem; border: 2px solid #000000; border-radius: 0.5rem; cursor: pointer; }";
  html += "button.primary { color:#ffffff; background-color:#000000; border:none; } div.message { display: flex; flex-direction: column; gap: 0.5rem; min-width: fit-content }";
  html += "div.message > span.sender { margin-left: 0.5rem; font-weight: 600; font-size: 0.9rem; color: #6c6c6c; } div.message > div.content { padding: 0.75rem 1rem; background-color: #e6e6e6; color: #000000; min-width: 12rem; max-width: 16rem; white-space: pre-wrap; word-break: break-word; border-radius: 0.5rem; }</style>";
  // Add WebSocket JavaScript
  html += "<script>";
  html += "let ws = new WebSocket('ws://' + window.location.hostname + ':81/');";
  html += "ws.onmessage = function(event) {";
  html += "  const data = JSON.parse(event.data);";
  html += "  const name = document.getElementById('name').value || 'Anonymous';";
  html += "  const messageDiv = document.createElement('div');";
  html += "  messageDiv.className = 'message';";
  html += "  if (name === data.name) messageDiv.style['align-self'] = 'flex-end';";
  html += "  const contentStyle = name === data.name && 'background-color: #222; color: #fff';";
  html += "  messageDiv.innerHTML = `<span class='sender'>${data.name}</span> <div class='content' style='${contentStyle}'>${data.message}</div>`;";
  html += "  document.getElementById('messages').appendChild(messageDiv);";
  html += "};";
  
  // Add keyboard event handler
  html += "document.getElementById('message').addEventListener('keydown', function(event) {";
  html += "  if (event.key === 'Enter' && !event.shiftKey) {";
  html += "    event.preventDefault();";
  html += "    sendMessage(event);";
  html += "  }";
  html += "});";

  html += "function sendMessage(event) {";
  html += "  event.preventDefault();";
  html += "  const messageInput = document.getElementById('message');";
  html += "  const message = messageInput.value.trim();";
  html += "  if (!message) return;"; // Don't send empty messages
  html += "  const name = document.getElementById('name').value || 'Anonymous';";
  html += "  fetch('/broadcast', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
  html += "    body: `name=${encodeURIComponent(name)}&message=${encodeURIComponent(message)}`";
  html += "  });";
  html += "  messageInput.value = '';";
  html += "  document.querySelector('.grow-wrap').dataset.replicatedValue = '';";
  html += "  return false;";
  html += "}</script>";
  html += "</head><body><form class=\"container\" onsubmit=\"return sendMessage(event)\"><div class=\"header\"><h1>Chat Room</h1><div class=\"input\">";
  html += "<label for=\"message\">Display Name</label><input type=\"text\" id=\"name\" name=\"name\" placeholder=\"Enter a name for other to see\"></div></div>";
  html += "<div id=\"messages\" style=\"flex: 1; overflow-y: auto; padding: 1rem; display: flex; flex-direction: column; gap: 0.5rem;\"></div>";
  html += "<div class=\"input\" style=\"min-width: 30rem; align-items: end; padding: 0 1rem;\"><div class=\"grow-wrap\">";
  html += "<textarea type=\"text\" id=\"message\" name=\"message\" rows=\"1\" placeholder=\"Type a message...\" onInput=\"this.parentNode.dataset.replicatedValue = this.value\" required></textarea>";
  html += "</div><button type=\"submit\" class=\"primary\">Send</button></div></form>";
  html += "<script>document.querySelectorAll('textarea').forEach(el => { el.dispatchEvent(new Event('input', { bubbles: true, cancelable: true })); });</script></body></html>";

  server.send(200, "text/html", html);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[%u] Connected!\n", num);
        // Send existing messages to newly connected client
        for(int i = 0; i < messageCount; i++) {
          webSocket.sendTXT(num, messages[i]);
        }
      }
      break;
    case WStype_TEXT:
      // Handle incoming messages if needed
      break;
  }
}

// boradcast messages to all users
void broadcastMessage() {
  if (!server.hasArg("message") || !server.hasArg("name")) {
    server.send(400, "application/json", "{\"error\":\"Missing message or name\"}");
    return;
  }

  String name = server.arg("name");
  String message = server.arg("message");
  String formattedMessage = "{\"name\":\"" + name + "\",\"message\":\"" + message + "\"}";

  // Store message in circular buffer
  messages[messageCount % MAX_MESSAGES] = formattedMessage;
  messageCount++;

  // Broadcast to all connected clients
  webSocket.broadcastTXT(formattedMessage);
  
  server.send(200, "application/json", "{\"status\":\"sent\"}");
}

void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  server.sendHeader("Access-Control-Max-Age", "86400");

  server.send(204);
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  while (!Serial) {
    // Wait for Serial to initialize
  }
  Serial.println("Serial initialized.");

  // Initialize WiFi Access Point
  Serial.print("Configuring access point...");
  WiFi.softAP(apSSID, apPassword);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Initialize server routes
  server.on("/", handleRoot);
  server.on("/toggle", HTTP_GET, toggleLED);
  server.on("/state", HTTP_GET, getLEDState);
  server.on("/blink", HTTP_GET, blinkLED);
  server.on("/scan", HTTP_GET, scanNetworks);
  server.on("/hidden", HTTP_GET, handleHidden);
  server.on("/chat", HTTP_GET, handleChat);
  server.on("/broadcast", HTTP_POST, broadcastMessage);
  server.on("/connect", HTTP_POST, connectToNetwork);

  // Add OPTIONS handler for CORS preflight requests
  server.on("/toggle", HTTP_OPTIONS, handleOptions);
  server.on("/state", HTTP_OPTIONS, handleOptions);
  server.on("/blink", HTTP_OPTIONS, handleOptions);
  server.on("/scan", HTTP_OPTIONS, handleOptions);
  server.on("/hidden", HTTP_OPTIONS, handleOptions);
  server.on("/boradcast", HTTP_OPTIONS, handleOptions);
  server.on("/chat", HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");

  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // Ensure LED starts off
}

void loop() {
  server.handleClient();
  webSocket.loop();

  // Check for new connections
  int stationCount = WiFi.softAPgetStationNum();
  if (stationCount > previousStationCount && !isBlinking) {
    Serial.println("A device connected to the AP!");
    digitalWrite(LED_BUILTIN, HIGH);
    isBlinking = true;
    blinkInterval = 200;
    blinkCount = 0;
    maxBlinks = 6;
  }

  if (stationCount < previousStationCount && !isBlinking) {
    Serial.println("A device disconnected from the AP!");
    digitalWrite(LED_BUILTIN, HIGH);
    isBlinking = true;
    blinkInterval = 1000;
    blinkCount = 0;
    maxBlinks = 2;
  }

  previousStationCount = stationCount;

  // Handle LED blinking
  if (isBlinking) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle LED state

      blinkCount++;
      if (blinkCount >= maxBlinks) {
        isBlinking = false;                                // Stop blinking after max blinks
        digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);  // Restore LED state
      }
    }
  } else if (stationCount == 0) {
    digitalWrite(LED_BUILTIN, HIGH);  // Ensure LED is off when no devices are connected
  }
}
