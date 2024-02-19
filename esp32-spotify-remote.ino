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

#include <WebServer.h>
#include <WiFi.h>

#include "spotify.h"

SpotifyClient spotify;
WebServer server(80);



/**
 * Connect to wifi and save IP
*/
void start_wifi(){
	// connect to wifi
	WiFi.begin(spotify.wifi_ssid, spotify.wifi_password);
	
	while (WiFi.status() != WL_CONNECTED); // wait for connection

	// save ip address to send to Flipper Zero
	spotify.ip_address = WiFi.localIP().toString();
	spotify.redirect_uri = "http://" + spotify.ip_address + "/callback";
}

/**
 * Starts server in station mode and assigns callbacks to each url
*/
void start_server(){
    server.on("/", handle_on_root);
	server.on("/callback", handle_authorization);
	server.begin();
}

/**
 * Serve home html page
*/
void handle_on_root(){
	server.send(200, "text/html", get_html_page(HOME));
}

/**
 * Get acccess code from callback url
*/
void handle_authorization(){
	// if access code argument is empty, serve error page, else save value of code parameter to auth_code
	if(server.arg("code") == ""){
		server.send(200, "text/html", get_html_page(ERROR));
	} else {
		spotify.auth_code = server.arg("code");
		server.send(200, "text/html", "Authorization complete, you may close this tab");
	}
}

/**
 * Provides correct html page
 *
 * @param page member of enum web_page that specifes the correct page
 * @return String containing html code
 */
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
	html += "client_id=" + spotify.client_id + "&";
	html += "redirect_uri=" + spotify.redirect_uri + "&";
	html += "scope=user-library-read user-read-playback-state user-modify-playback-state\">Log in to spotify</a>\n";
	html += "    </main>\n";
	html += "  </body>\n";
	html += "</html>\n";

	return html;
}

/**
 * Will parse Serial data and correctly handle inputs
 * 
 * @param input string recieved over Serial
*/
void parse_input(const char *input_string){
    int action = input_string[0] - '0'; // first byte should be a number in the Action enum
    String data;
    if(strlen(input_string) > 1){
        if(action == CREDENTIALS){
            data = String (input_string + 1); // cast as a string and cut off the identifying byte
            spotify.wifi_ssid = data.substring(0, data.indexOf('+')); // '+' delimiter is in between ssid and password
            spotify.wifi_password = data.substring(data.indexOf('+') + 1);
        }
    } else if (action < REMOTE_LAUNCH) {
        spotify.request = action;
    } else if(action == REMOTE_LAUNCH){ // reset auth code values and send ip to flipper to be displayed to the user
        spotify.remote_launched = true;
        spotify.auth_code = "";
        spotify.access_token = "";
        if (spotify.ip_address == "0.0.0.0"){
            Serial.println("IP: " + spotify.ip_address + " Not connected, make sure ssid and password are correct");
        } else {
            Serial.println("IP: Go to " + spotify.ip_address + " in your browser");
        }
    } else if(action == BACK_BUTTON){
        spotify.remote_launched = false;
    }
}

/**
 * freeRTOS task dedicated to listening for Serial input
 * 
 * @param parameter unused but required for freeRTOS
*/
void handle_input(void* parameter){
    int buffer_size = 128;
    char buffer[buffer_size]; 
    int buffer_index = 0;

    for(;;){
        if(Serial.available() > 0){
            char in_byte = Serial.read();
            if (in_byte != '\n' && buffer_index < buffer_size) { // read until delimiter or max buffer size
                buffer[buffer_index] = in_byte;
                buffer_index++;
            } else { // null terminate string and handle the data
                buffer[buffer_index] = '\0';
                buffer_index = 0;
                parse_input(buffer);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    xTaskCreate(
        handle_input, // function name
        "Handle Input", // task name for debugging,
        1000, // stack size
        NULL, // task parameters
        1, // task priority
        NULL // task handle
    );
    spotify_init_client(&spotify);
    start_wifi();
    start_server();
}

void loop() {
    if(!spotify.remote_launched){
        // do nothing until remote is launched
    } else if(WiFi.status() != WL_CONNECTED){
        // if wifi isn't connected, try again
        start_wifi();
        start_server();
    } else if(spotify.auth_code.length() == 0){
        // if the auth code isn't set, user will have to access the web server
        server.handleClient();
    } else if(spotify.access_token.length() == 0){
        // make request to spotify api for access token
        spotify_get_tokens(&spotify);
    } else {
        // make request to spotify api correspinding to action chosen on Flipper Zero
        spotify_make_request(&spotify);
        spotify.request = NONE;
    }
}