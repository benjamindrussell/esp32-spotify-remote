/**
 * Contains info for the esp32 spotify remote such as named constants, structs containing app state,
 * and declarations of functions relating to spotify api calls
 * 
 * @file spotify.h
 * @author Ben Russell
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <HTTPClient.h>

#define CLIENT_ID ""
#define CLIENT_SECRET ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define WIFI_SSID_SIZE 32
#define WIFI_PASSWORD_SIZE 63

enum web_page {
	HOME,
	ERROR
};

// represents each possible action based on serial input
typedef enum {
	NONE,
	PREVIOUS,
	NEXT,
	PLAY,
	PAUSE,
	SHUFFLE,
	REPEAT,
	REMOTE_LAUNCH,
	BACK_BUTTON,
	CREDENTIALS,
} Action;

typedef struct{
	String wifi_ssid;	
	String wifi_password;
	String client_id; 
	String client_secret;
	String auth_code; // code acquired from login
	String access_token; // code included in requests
	String refresh_token; // used to generate new access token
	String repeat_state; // off or context
	String redirect_uri;
	String ip_address; // ip address of esp32
	int request; // next action to call
	bool shuffle_state;
	bool remote_launched;
} SpotifyClient;

void spotify_init_client(SpotifyClient *spotify);

int spotify_get_tokens(SpotifyClient *spotify);
int spotify_refresh_tokens(SpotifyClient *spotify);

int spotify_previous(SpotifyClient *spotify, HTTPClient &http);
int spotify_next(SpotifyClient *spotify, HTTPClient &http);
int spotify_play(SpotifyClient *spotify, HTTPClient &http);
int spotify_pause(SpotifyClient *spotify, HTTPClient &http);
int spotify_toggle_shuffle_state(SpotifyClient *spotify, HTTPClient &http);
int spotify_toggle_repeat_state(SpotifyClient *spotify, HTTPClient &http);

int spotify_make_request(SpotifyClient *spotify);