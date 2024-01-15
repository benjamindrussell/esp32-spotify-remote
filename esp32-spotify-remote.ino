/**
 * The esp32 spotify remote is designed to be used with the flipper zero to
 * control the user's spotify acocunt 
 * 
 * This repository: https://github.com/benjamindrussell/esp32-spotify-remote
 * Flipper app repository: https://github.com/benjamindrussell/flipper-spotify-remote
 * 
 * @file esp32-spotify-remote.ino
 * @author Ben Russell
 */

#include "spotify.h"
#include <WebServer.h>

spotify_client spotify;
WebServer server(80);

void setup(){
    Serial.begin(115200);
    delay(2000);

	spotify_init_client(&spotify);

	// connect to wifi
	WiFi.begin(spotify.credentials.wifi_ssid, spotify.credentials.wifi_password);
	while (WiFi.status() != WL_CONNECTED){
		delay(500);
	}

	// send ip over serial to flipper zero
	spotify.ip_address = WiFi.localIP().toString();
	spotify.redirect_uri = "http://" + spotify.ip_address + "/callback";
	spotify.wifi_connected = true;

	// assign server callbacks
	server.on("/", handle_on_root);
	server.on("/callback", handle_authorization);
	server.begin();
}

void loop(){
	if(Serial.available() > 0){
		int input = get_input();
		switch(input){
			case REMOTE_LAUNCH:
				spotify.remote_launched = true;
				spotify.auth_code_set = false;
				spotify.access_token_set = false;
				if(spotify.ip_address == "0.0.0.0"){
					Serial.println("IP: " + spotify.ip_address + " Not connected, make sure ssid and password are correct");
				} else {
					Serial.println("IP: Go to " + spotify.ip_address + " in your browser");
				}
				break;
			case BACK_BUTTON:
				spotify.remote_launched = false;
				break;
			default:
				break;
		}
	}

	if(!spotify.remote_launched){
		//  do nothing until remote is launched
	} else if(!spotify.wifi_connected){
		/**
		 * @todo error handling
		*/
	} else if(!spotify.auth_code_set){
		// if the auth code isn't set, user will have to access the web server
		server.handleClient();

	} else if(spotify.access_token_set && (millis() - spotify.start_time) / 1000 > spotify.expire_time) { 
		// if code is set but expired then refresh tokens
		spotify.access_token_set = false;
		spotify_refresh_tokens(&spotify);	

	} else if (!spotify.access_token_set){
		spotify_get_tokens(&spotify);
		if (spotify.access_token_set){
			// turn off repeat and shuffle on connect
			spotify_init_repeat_state(&spotify);
			spotify_init_shuffle_state(&spotify);
		}

	} else {
		spotify_make_request(&spotify, get_input());
	}

	// while getting code and tokens, delay two seconds, otherwise no delay
	delay(spotify.poll_rate);
}

/**
 * Provides correct html page
 *
 * @param spotify client struct containing program state
 * @param page member of enum web_page that specifes the correct page
 * @return String containing html code
 */
String get_html_page(spotify_client *spotify, int page){
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
	html += "client_id=" + spotify->credentials.client_id + "&";
	html += "redirect_uri=" + spotify->redirect_uri + "&";
	html += "scope=user-library-read user-read-playback-state user-modify-playback-state\">Log in to spotify</a>\n";
	html += "    </main>\n";
	html += "  </body>\n";
	html += "</html>\n";

	return html;
}

void handle_on_root(){
	// Serial.println("Handling root...");
	server.send(200, "text/html", get_html_page(&spotify, HOME));
}

void handle_authorization(){
	// Serial.println("Handling authorization...");

	// if code parameter is empty, serve error page, else save value of code parameter to auth_code
	if(server.arg("code") == ""){
		server.send(200, "text/html", get_html_page(&spotify, ERROR));
	} else {
		spotify.auth_code = server.arg("code");
		spotify.auth_code_set = true;
		server.send(200, "text/html", "Authorization complete, you may close this tab");
	}
}

/**
 * Get input from user via serial from flipper zero
 *
 * @return integer representing action chosen on flipper
 */
int get_input(){
	char message[16];
	while (Serial.available() > 0){
		static unsigned int message_pos = 0;

		char in_byte = Serial.read();
		if (in_byte != '\n' && (message_pos < 16 - 1)){
			message[message_pos] = in_byte;
			message_pos++;
		} else {
			message[message_pos] = '\0';
			message_pos = 0;
		}
	}
	return atoi(message);
}