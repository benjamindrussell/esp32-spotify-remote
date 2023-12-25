#include "spotify.h"

String get_html_page(int page);
void handle_on_root();
void handle_authorization();

spotify_client spotify;
WebServer server(80);

void setup(){
	Serial.begin(115200);
	init_spotify_client(&spotify);

	init_wifi();

	server.on("/", handle_on_root);
	server.on("/callback", handle_authorization);
	server.begin();
}

void loop(){
	if(spotify.auth_code_set){
		if(spotify.access_token_set){
			Serial.println("Auth code and access token set");
		} else {
			get_access_token(&spotify);
		}
	} else {
		server.handleClient();
	}
}

void init_wifi(){
	IPAddress ip(192, 168, 1, 146);		
	IPAddress gateway(192, 168, 1, 1);	
	IPAddress subnet(255, 255, 255, 0); 

	WiFi.config(ip, gateway, subnet);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.println("Connecting to WiFi...");
	}

	Serial.println("Connected to WiFi\n Ip is: ");
	Serial.println(WiFi.localIP());
}

String get_html_page(int page){
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
	if(page == HOME){
		html += "        <h1>Login to Spotify</h1>\n";
	} else if (page == ERROR){
		html += "        <h1>Login Failed, Try Again</h1>\n";
	}
	html += "        <a href=\"https://accounts.spotify.com/authorize?";
	html += "response_type=code&";
	html += "client_id=" + CLIENT_ID + "&";
	html += "redirect_uri=" + REDIRECT_URI + "&";
	html += "scope=user-library-read user-read-playback-state user-modify-playback-state\">Log in to spotify</a>\n";
	html += "    </main>\n";
	html += "  </body>\n";
	html += "</html>\n";

	return html;
}

void handle_on_root(){
	Serial.println("Handling root...");
	server.send(200, "text/html", get_html_page(HOME));
}

void handle_authorization(){
	Serial.println("Handling authorization...");
	if(server.arg("code") == ""){
		server.send(200, "text/html", get_html_page(ERROR));
	} else {
		spotify.auth_code = server.arg("code");
		spotify.auth_code_set = true;
		server.send(200, "text/html", "Authorization complete, you may close this tab");
	}
}