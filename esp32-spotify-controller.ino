#include <WiFi.h>
#include <WebServer.h>

const String WIFI_SSID = "Russell Family Network";
const String WIFI_PASSWORD = "thewifipassword";
const String CLIENT_ID = "974b8ee0b9f84500b4dc5340cf7b1416";
const String CLIENT_SECRET = "11bfca0a5d7c451499d2fda8133652df";
const String REDIRECT_URI = "http://192.168.1.146/callback";

String get_home_page();
void handle_on_root();
void handle_authorization();

// Create server object
WebServer server(80);

IPAddress ip(192, 168, 1, 146);  // Set your desired IP address
IPAddress gateway(192, 168, 1, 1);  // Set your gateway IP address
IPAddress subnet(255, 255, 255, 0);  // Set your subnet mask

void setup() {
  Serial.begin(115200);

  
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi\n Ip is: ");
    Serial.println(WiFi.localIP());

  // Initialize server
  server.on("/", handle_on_root);
  server.on("/callback", handle_authorization);
  server.begin();
}

void loop() {
  server.handleClient();
}

String get_home_page(){
  String html = "";
  html += "<!DOCTYPE html>\n";
  html += "<html lang=\"en\">\n";
  html += "  <head>\n";
  html += "    <meta charset=\"UTF-8\">\n";
  html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "    <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n";
  html += "    <title>Flipper Zero | Spotify</title>\n";
  html += "  </head>\n";
  html += "  <body>\n";
  html += "    <main>\n";
  html += "        <h1>Need Authorization Code</h1>\n";
  html += "        <a href=\"https://accounts.spotify.com/authorize?";
  html +=                   "response_type=code&";
  html +=                   "client_id=" + CLIENT_ID + "&";
  html +=                   "redirect_uri=" + REDIRECT_URI + "&";
  html +=                   "scope=user-library-read user-read-playback-state user-modify-playback-state\">Log in to spotify</a>\n";
  html += "    </main>\n";
  html += "  </body>\n";
  html += "</html>\n";

  return html;  
}

void handle_on_root(){
  Serial.println("Handling root...");
  server.send(200, "text/html", get_home_page()); 
}

void handle_authorization(){
  Serial.print("Handling authorization...");
  Serial.print(server.arg("code"));
  server.send(200, "text/html", "Authorization complete, you may close this tab");
}
