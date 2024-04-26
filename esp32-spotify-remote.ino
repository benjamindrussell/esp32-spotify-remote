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
    Serial.println("HI"); // let the flipper know esp32 is plugged in

	WiFi.begin(spotify.wifi_ssid, spotify.wifi_password); // connect to wifi
	
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
		server.send(200, "text/html", get_html_page(DONE));
	}
}

/**
 * Provides correct html page
 *
 * @param page member of enum web_page that specifes the correct page
 * @return String containing html code
 */
String get_html_page(int page){
    String html= "";
    html += "<!DOCTYPE html>\n";
    html += "<html lang=\"en\">\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>Flipper Zero | Spotify</title>\n";
    html += "    <link rel=\"icon\" type=\"image/png\" href=\"https://cdn.flipperzero.one/qFlipper_macOS_256px_ugly.png\">\n";
    html += "    <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n";
    html += "    <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>\n";
    html += "    <link href=\"https://fonts.googleapis.com/css2?family=Jersey+10&display=swap\" rel=\"stylesheet\">\n";
    html += "    <style>\n";
    html += "        body {\n";
    html += "            background: #FF8200;\n";
    html += "            background-image: url(\"https://flipperzero.one/img/tild6362-3266-4764-a536-643963633836__pattern_preview.png\");\n";
    html += "            margin: 0;\n";
    html += "            padding: 0;\n";
    html += "            font-family: 'Jersey 10';\n";
    html += "        }\n";

    html += "        #logos {\n";
    html += "            display: flex;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "            gap: 50px;\n";
    html += "        }\n";

    html += "        #button {\n";
    html += "            border: none;\n";
    html += "            border-radius: 0;\n";
    html += "            background-color: #000000;\n";
    html += "            color: #ffffff;\n";
    html += "            font-family: 'Jersey 10';\n";
    html += "            font-size: 30px;\n";
    html += "            padding: 10px 20px;\n";
    html += "            text-decoration: none;\n";
    html += "        }\n";

    html += "        #button:hover {\n";
    html += "            cursor: pointer;\n";
    html += "        }\n";
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div id=\"content\" style=\"background-color: #ffffff; padding: 40px;\">\n";
    html += "        <div id=\"logos\">\n";
    html += "            <img\n";
    html += "                src=\"https://cdn.flipperzero.one/transparent.png\"\n";
    html += "                style=\"width: 100px;\"\n";
    html += "            />\n";
    html += "            <img \n";
    html += "                src=\"https://upload.wikimedia.org/wikipedia/commons/thumb/1/19/Spotify_logo_without_text.svg/1024px-Spotify_logo_without_text.svg.png\"\n";
    html += "                style=\"width: 75px;\"\n";
    html += "            />\n";
    html += "        </div>\n";
    html += "        <h1 style=\"font-size: 60px; text-align: center;\">Spotify Remote</h1>\n";
    html += "        <div id=\"login\" style=\"display: flex; justify-content: center;\">\n";
    if(page == ERROR) {
        html += "    <h2 style=\"text-align: center;\">Oops! login failed, please try again.</h2>\n";
    } else if(page == DONE){
        html +="     <h2 style=\"text-align: center;\">Login successful, you may now close this tab</h2>\n";
    }
    if(page == HOME || page == ERROR){
	html += "        <a id=\"button\" href=\"https://accounts.spotify.com/authorize?";
	html += "response_type=code&";
	html += "client_id=" + spotify.client_id + "&";
	html += "redirect_uri=" + spotify.redirect_uri + "&";
	html += "scope=user-library-read user-read-playback-state user-modify-playback-state\">Login</a>\n";
    }
    html += "        </div>\n";
    html += "    </div>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

/**
 * Will parse Serial data and correctly handle inputs
 * 
 * @param input string recieved over Serial
*/
void handle_input(const char *input_string){
    int action = input_string[0] - '0'; // first byte should be a number in the Action enum
    String data;
    if(action >= PREVIOUS && action <= REPEAT){
        spotify.request = action;
    } else if(action == REMOTE_LAUNCH){
        spotify.remote_launched = true;
    } else if(action == BACK_BUTTON){
        spotify.remote_launched = false;
        spotify.auth_code = "";
        spotify.access_token = "";
    }

}

/**
 * freeRTOS task dedicated to listening for Serial input
 * 
 * @param parameter unused but required for freeRTOS
*/
void read_input(void* parameter){
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
                handle_input(buffer);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    xTaskCreate(
        read_input, // function name
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
        if (spotify.ip_address == "0.0.0.0"){
            Serial.println("IP: " + spotify.ip_address + " Not connected, make sure ssid and password are correct");
        } else {
            Serial.println("IP: Go to " + spotify.ip_address + " in your browser");
        }
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