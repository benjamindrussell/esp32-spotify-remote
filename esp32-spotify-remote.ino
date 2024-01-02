#include <ArduinoJson.h>
#include <base64.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>

#define BTN1 0
#define BTN2 4

typedef struct{
	String auth_code;
	String access_token;
	int start_time;
	int expire_time;
	String refresh_token;
	bool auth_code_set;
	bool access_token_set;
	int request;
} spotify_client;

enum web_page {
	HOME,
	ERROR
};

enum request {
	NONE,
	NEXT,
	PREVIOUS,
	VOL_UP,
	VOL_DOWN,
	PLAY,
	PAUSE,
};

const String WIFI_SSID = "Russell Family Network";
const String WIFI_PASSWORD = "thewifipassword";
const String CLIENT_ID = "974b8ee0b9f84500b4dc5340cf7b1416";
const String CLIENT_SECRET = "11bfca0a5d7c451499d2fda8133652df";
const String REDIRECT_URI = "http://192.168.1.145/callback";

spotify_client spotify;
WebServer server(80);

void IRAM_ATTR next_ISR(){
	spotify.request = NEXT;
}

void IRAM_ATTR previous_ISR(){
	spotify.request = PREVIOUS;
}

void IRAM_ATTR play_ISR(){
	spotify.request = PLAY;
}

void IRAM_ATTR pause_ISR(){
	spotify.request = PAUSE;
}

void setup(){
    Serial.begin(115200);
    delay(4000);
	pinMode(BTN1, INPUT_PULLUP);
	pinMode(BTN2, INPUT_PULLUP);
	attachInterrupt(BTN1, play_ISR, FALLING);
	attachInterrupt(BTN2, pause_ISR, FALLING);

	init_spotify_client(&spotify);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED){
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi network");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); 

	server.on("/", handle_on_root);
	server.on("/callback", handle_authorization);
	server.begin();
}

int poll_rate = 5000;
void loop(){
	if (spotify.auth_code_set){
		Serial.println("Auth Code set");
		if(spotify.access_token_set && (millis() - spotify.start_time) / 1000 > spotify.expire_time){
			Serial.println("Access token expired, fetching new token...");
			spotify.access_token_set = false;
			refresh_tokens();
		}
		if(spotify.access_token_set){
			Serial.println("Both Set");
			switch(spotify.request){
				case NEXT:
					spotify_remote_next();
					spotify.request = NONE;
					break;
				case PREVIOUS:
					spotify_remote_previous();
					spotify.request = NONE;
					break;
				case PLAY:
					spotify_remote_play();
					spotify.request = NONE;
					break;
				case PAUSE:
					spotify_remote_pause();
					spotify.request = NONE;
					break;
				default:
					break;
			}
		} else {
			Serial.println("Getting tokens");
			get_tokens();
		}
	} else {
		server.handleClient();
	}
	delay(poll_rate);
}

void init_spotify_client(spotify_client *spotify){
	spotify->auth_code = "";
	spotify->access_token = "";
	spotify->refresh_token = "";
	spotify->start_time = 0;
	spotify->expire_time = 0;
	spotify->auth_code_set = false;
	spotify->access_token_set = false;
	spotify->request = NONE;
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

void get_tokens(){
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
			spotify.start_time = millis();
            spotify.refresh_token = String((const char*)doc["refresh_token"]);
			spotify.expire_time = int(doc["expires_in"]);

            Serial.println("Got access and refresh tokens1");
            Serial.println("Access token: " + spotify.access_token);
            Serial.println("Refresh Token: " + spotify.refresh_token);
			poll_rate = 100;
		} else {
			Serial.println("Failed to get tokens");
            Serial.println(json);
		}
	} else {
		Serial.println("HTTP Request Failed");
	}

	http.end();
}

void refresh_tokens(){
	HTTPClient http;

    String credentials = CLIENT_ID + ":" + CLIENT_SECRET;
    String encoded_credentials =  base64::encode(credentials);

	http.begin("https://accounts.spotify.com/api/token");
	http.addHeader("Authorization", "Basic " + encoded_credentials);
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	String post_body = "grant_type=refresh_token&refresh_token=" + spotify.refresh_token;

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
			spotify.start_time = millis();
            spotify.refresh_token = String((const char*)doc["refresh_token"]);
			spotify.expire_time = int(doc["expires_in"]);

            Serial.println("Got access and refresh tokens2");
            Serial.println("Access token: " + spotify.access_token);
            Serial.println("Refresh Token: " + spotify.refresh_token);
			poll_rate = 100;
		} else {
			Serial.println("Failed to refresh tokens3");
            Serial.println(json);
		}
	} else {
		Serial.println("HTTP Request Failed");
	}

	http.end();
}

void spotify_remote_next(){
	Serial.println("Next");
	HTTPClient http;

	http.begin("https://api.spotify.com/v1/me/player/next");
	http.addHeader("Authorization", "Bearer " + spotify.access_token);
	http.addHeader("Content-Type", "application/json");

	int http_code = http.POST("{}");

	if(http_code > 0){
		// String json = http.getString();
		Serial.println("HTTP Code: " + http_code);
		// Serial.println(json);
	} else {
		Serial.println("HTTP Request Failed");
	}

	http.end();
}

void spotify_remote_previous(){
	Serial.println("Previous");
	HTTPClient http;

	http.begin("https://api.spotify.com/v1/me/player/previous");
	http.addHeader("Authorization", "Bearer " + spotify.access_token);
	http.addHeader("Content-Type", "application/json");

	int http_code = http.POST("{}");

	if(http_code > 0){
		// String json = http.getString();
		Serial.println("HTTP Code: " + http_code);
		// Serial.println(json);
	} else {
		Serial.println("HTTP Request Failed");
	}

	http.end();
}

void spotify_remote_play(){
	Serial.println("Play");
	HTTPClient http;
	http.begin("https://api.spotify.com/v1/me/player/play");
	http.addHeader("Authorization", "Bearer " + spotify.access_token);
	http.addHeader("Content-Type", "application/json");

	int http_code = http.PUT("{}");

	if(http_code > 0){
		// String json = http.getString();
		Serial.println("HTTP Code: " + http_code);
		// Serial.println(json);
	} else {
		Serial.println("HTTP Request Failed");
	}

	http.end();
}

void spotify_remote_pause(){
	Serial.println("pause");
	HTTPClient http;
	http.begin("https://api.spotify.com/v1/me/player/pause");
	http.addHeader("Authorization", "Bearer " + spotify.access_token);
	// http.addHeader("Content-Type", "application/json");

	int http_code = http.PUT("{}");

	if(http_code > 0){
		// String json = http.getString();
		Serial.println("HTTP Code: " + http_code);
		// Serial.println(json);
	} else {
		Serial.println("HTTP Request Failed");
	}

	http.end();
}