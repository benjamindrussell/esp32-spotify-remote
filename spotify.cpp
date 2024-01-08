/**
 * Functions relating to spotify api calls 
 * 
 * @file spotify.cpp
 * @author Ben Russell
 */

#include "spotify.h"

/**
 * Initializes all the fields in the spotify_client struct
 * 
 * @param spotify spotify client struct containing program state
 */
void spotify_init_client(spotify_client *spotify){
	spotify->auth_code = "";
	spotify->access_token = "";
	spotify->refresh_token = "";
	spotify->repeat_state = "off";
	spotify->redirect_uri = "";
	spotify->start_time = 0;
	spotify->expire_time = 0;
	spotify->poll_rate = 5000;
	spotify->request = NONE;
	spotify->auth_code_set = false;
	spotify->access_token_set = false;
	spotify->shuffle_state = false;
	spotify_init_credentials(&(spotify->credentials));	
}

/**
 * Initializes all fields of the credentials struct
 * 
 * @param credentials all credentials specified by the user
 */
void spotify_init_credentials(struct credentials *credentials){
	credentials->wifi_ssid = WIFI_SSID;
	credentials->wifi_password = WIFI_PASSWORD;
	credentials->client_id = CLIENT_ID;
	credentials->client_secret = CLIENT_SECRET;
}

/**
 * After auth code is acquired, get access token and refresh token
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request, or -1 on json deserialization error
 */
int spotify_get_tokens(spotify_client *spotify){
	HTTPClient http;

	// Base 64 encoded string that contains the client ID and client secret key. 
	// The field must have the format: Authorization: Basic <base64 encoded client_id:client_secret>
    String credentials = spotify->credentials.client_id + ":" + spotify->credentials.client_secret;
    String encoded_credentials =  base64::encode(credentials);

	http.begin("https://accounts.spotify.com/api/token");
	http.addHeader("Authorization", "Basic " + encoded_credentials);
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	String post_body = "grant_type=authorization_code&code=" + spotify->auth_code + "&redirect_uri=" + spotify->redirect_uri;

	int http_code = http.POST(post_body);

	if (http_code > 0){
		String json = http.getString();
		if(http_code == 200){
			DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, json);
            if (error){
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                return -1;
            }

			// parse json response and get tokens
            spotify->access_token = String((const char*)doc["access_token"]);
            spotify->refresh_token = String((const char*)doc["refresh_token"]);
            spotify->access_token_set = true;

			// mark start time and time to expire
			spotify->start_time = millis();
			spotify->expire_time = int(doc["expires_in"]);

			spotify->poll_rate = 0;
			Serial.println("OK");
        }
	}

	http.end();
	return http_code;
}

/**
 * Use refresh token to get new access token adn refresh token
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request, or -1 on json deserialization error
 */
int spotify_refresh_tokens(spotify_client *spotify){
	HTTPClient http;

	// Base 64 encoded string that contains the client ID and client secret key. 
	// The field must have the format: Authorization: Basic <base64 encoded client_id:client_secret>
    String credentials = spotify->credentials.client_id + ":" + spotify->credentials.client_secret;
    String encoded_credentials =  base64::encode(credentials);

	http.begin("https://accounts.spotify.com/api/token");
	http.addHeader("Authorization", "Basic " + encoded_credentials);
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	String post_body = "grant_type=refresh_token&refresh_token=" + spotify->refresh_token;

	int http_code = http.POST(post_body);

	if (http_code > 0){
		String json = http.getString();
		if(http_code == 200){
			DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, json);
            if (error){
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                return -1;
            }

			// parse json response and get tokens
            spotify->access_token = String((const char*)doc["access_token"]);
            spotify->refresh_token = String((const char*)doc["refresh_token"]);
            spotify->access_token_set = true;

			// mark start time and time to expire
			spotify->start_time = millis();
			spotify->expire_time = int(doc["expires_in"]);

			Serial.println("OK");
		}
	}

	http.end();
	return http_code;
}

/**
 * On connection, set shuffle to off
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_init_shuffle_state(spotify_client *spotify){
	HTTPClient http;

	http.begin("https://api.spotify.com/v1/me/player/shuffle?state=false");
	http.addHeader("Authorization", "Bearer " + spotify->access_token);

	int http_code = http.PUT("{}");

	http.end();
	return http_code;
}

/**
 * On connection, set repeat to off
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_init_repeat_state(spotify_client *spotify){
	HTTPClient http;

	http.begin("https://api.spotify.com/v1/me/player/repeat?state=off");
	http.addHeader("Authorization", "Bearer " + spotify->access_token);

	int http_code = http.PUT("{}");

	http.end();
	return http_code;
}

/**
 * Skip to previous song
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_previous(spotify_client *spotify, HTTPClient &http){
	http.begin("https://api.spotify.com/v1/me/player/previous");
	http.addHeader("Authorization", "Bearer " + spotify->access_token);
	http.addHeader("Content-Type", "application/json");

	int http_code = http.POST("{}");

	return http_code;
}

/**
 * Skip to next song
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_next(spotify_client *spotify, HTTPClient &http){
	http.begin("https://api.spotify.com/v1/me/player/next");
	http.addHeader("Authorization", "Bearer " + spotify->access_token);
	http.addHeader("Content-Type", "application/json");

	int http_code = http.POST("{}");

	return http_code;
}

/**
 * Play/resume track
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_play(spotify_client *spotify, HTTPClient &http){
	http.begin("https://api.spotify.com/v1/me/player/play");
	http.addHeader("Authorization", "Bearer " + spotify->access_token);
	http.addHeader("Content-Type", "application/json");

	int http_code = http.PUT("{}");

	return http_code;
}

/**
 * Pause track
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_pause(spotify_client *spotify, HTTPClient &http){
	http.begin("https://api.spotify.com/v1/me/player/pause");
	http.addHeader("Authorization", "Bearer " + spotify->access_token);

	int http_code = http.PUT("{}");

	return http_code;
}

/**
 * Toggle repeat between off and context
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_toggle_repeat_state(spotify_client *spotify, HTTPClient &http){
	if (spotify->repeat_state == "off"){
		http.begin("https://api.spotify.com/v1/me/player/repeat?state=context");
		spotify->repeat_state = "context";
	} else {
		http.begin("https://api.spotify.com/v1/me/player/repeat?state=off");
		spotify->repeat_state = "off";
	}
	http.addHeader("Authorization", "Bearer " + spotify->access_token);

	int http_code = http.PUT("{}");

	return http_code;
}

/**
 * On connection, set shuffle to off
 * 
 * @param spotify client struct containing program state
 * @return http status code, will be -1 on failed http request
 */
int spotify_toggle_shuffle_state(spotify_client *spotify, HTTPClient &http){
	if(spotify->shuffle_state){
		http.begin("https://api.spotify.com/v1/me/player/shuffle?state=false");
		spotify->shuffle_state = false;
	} else {
		http.begin("https://api.spotify.com/v1/me/player/shuffle?state=true");
		spotify->shuffle_state = true;
	}
	http.addHeader("Authorization", "Bearer " + spotify->access_token);

	int http_code = http.PUT("{}");

	return http_code;
}

/**
 * Abstracts all requests to a single function and 
 * avoids multiple initializations of http client 
 * 
 * @param spotify client struct containing program state
 * @param request int representing action to call
 * @return http status code, will be -1 on failed http request
 */
int spotify_make_request(spotify_client *spotify, int request){
	HTTPClient http;
	int http_code;

	switch (request){
		case PREVIOUS:
			http_code = spotify_previous(spotify, http);
			break;
		case NEXT:
			http_code = spotify_next(spotify, http);
			break;
		case PLAY:
			http_code = spotify_play(spotify, http);
			break;
		case PAUSE:
			http_code = spotify_pause(spotify, http);
			break;
		case SHUFFLE:
			http_code = spotify_toggle_shuffle_state(spotify, http);
			break;
		case REPEAT:
			http_code = spotify_toggle_repeat_state(spotify, http);
		default:
			http_code = -1;
			break;
	}

	http.end();
	return http_code;
}
