#include <ArduinoJson.h>
#include <base64.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>

typedef struct{
	String auth_code;
	String access_token;
	String refresh_token;
	bool auth_code_set;
	bool access_token_set;
} spotify_client;

enum web_page {
	HOME,
	ERROR
};

const String WIFI_SSID = "";
const String WIFI_PASSWORD = "";
const String CLIENT_ID = "";
const String CLIENT_SECRET = "";
const String REDIRECT_URI = "http://192.168.1.146/callback";

spotify_client spotify;
WebServer server(80);

void setup(){
    Serial.begin(115200);
    delay(4000);

	init_spotify_client(&spotify);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED){
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi network");

	server.on("/", handle_on_root);
	server.on("/callback", handle_authorization);
	server.begin();
}

void loop(){
	if (spotify.auth_code_set){
		if(spotify.access_token_set){

		} else {
			get_access_tokens();
		}
	} else {
		server.handleClient();
	}

	delay(10000);
}

void init_spotify_client(spotify_client *spotify){
	spotify->auth_code = "";
	spotify->access_token = "";
	spotify->refresh_token = "";
	spotify->auth_code_set = false;
	spotify->access_token_set = false;
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

void get_access_tokens(){
	HTTPClient http;

    String credentials = CLIENT_ID + ":" + CLIENT_SECRET;
    String encoded_credentials =  base64::encode(credentials);

	http.begin("https://accounts.spotify.com/api/token");
	http.addHeader("Authorization", "Basic " + encoded_credentials);
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	String post_body = "grant_type=authorization_code&code=" + spotify.auth_code + "&redirect_uri=" + REDIRECT_URI;

	int http_code = http.POST(post_body);

	if (http_code > 0){
		String json = http.getString();
		Serial.println("HTTP Code: " + http_code);
		Serial.println(json);
		if(http_code == 200){
			DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, json);
            if (error){
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                return;
            }

            spotify.access_token = String((const char*)doc["access_token"]);
            spotify.access_token_set = true;
            spotify.refresh_token = String((const char*)doc["refresh_token"]);

            Serial.println("Got access and refresh tokens");
            Serial.println("Access token: " + spotify.access_token);
            Serial.println("Refresh Token: " + spotify.refresh_token);
		} else {
			Serial.println("Failed to get tokens");
            Serial.println(json);
		}
	} else {
		Serial.println("HTTP Request Failed");
	}
}