/**
 * Contains info for the esp32 spotify remote such as named constants, structs containing app state,
 * and declarations of functionsn relating to spotify api calls
 * 
 * @file spotify.h
 * @author Ben Russell
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define CLIENT_ID ""
#define CLIENT_SECRET ""

enum web_page {
	HOME,
	ERROR
};

enum input {
	NONE,
	PREVIOUS,
	NEXT,
	PLAY,
	PAUSE,
	SHUFFLE,
	REPEAT,
	REMOTE_LAUNCH,
	BACK_BUTTON,
};

struct credentials{
	String wifi_ssid;	
	String wifi_password;
	String client_id; 
	String client_secret;
};

typedef struct{
	String auth_code; // code acquired from login
	String access_token; // code included in requests
	String refresh_token; // used to generate new access token
	String repeat_state; // off or context
	String redirect_uri;
	String ip_address; // ip address of esp32
	int start_time; // time token was acquired
	int expire_time; // amount of time until token expires
	int request; // next action to call
	int poll_rate; // delay in loop
	bool auth_code_set;
	bool access_token_set;
	bool shuffle_state;
	bool remote_launched;
	bool wifi_connected;
	struct credentials credentials; // contains wifi and spotify app credentials
} spotify_client;

void spotify_init_client(spotify_client *spotify);
void spotify_init_credentials(struct credentials *credentials);

int spotify_get_tokens(spotify_client *spotify);
int spotify_refresh_tokens(spotify_client *spotify);

int spotify_init_shuffle_state(spotify_client *spotify);
int spotify_init_repeat_state(spotify_client *spotify);

int spotify_previous(spotify_client *spotify, HTTPClient &http);
int spotify_next(spotify_client *spotify, HTTPClient &http);
int spotify_play(spotify_client *spotify, HTTPClient &http);
int spotify_pause(spotify_client *spotify, HTTPClient &http);
int spotify_toggle_shuffle_state(spotify_client *spotify, HTTPClient &http);
int spotify_toggle_repeat_state(spotify_client *spotify, HTTPClient &http);

int spotify_make_request(spotify_client *spotify, int request);
