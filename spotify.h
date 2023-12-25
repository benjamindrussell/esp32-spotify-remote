#ifndef SPOTIFY_H_INCLUDED
#define SPOTIFY_H_INCLUDED

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <base64.h>
#include <WiFiClientSecure.h>

typedef struct{
	String auth_code;
	String access_token;
	String refresh_token;
	bool auth_code_set;
	bool access_token_set;
    WiFiClientSecure client;
} spotify_client;

enum web_page {
	HOME,
	ERROR
};

const String WIFI_SSID = "Russell Family Network";
const String WIFI_PASSWORD = "thewifipassword";
const String CLIENT_ID = "974b8ee0b9f84500b4dc5340cf7b1416";
const String CLIENT_SECRET = "11bfca0a5d7c451499d2fda8133652df";
const String REDIRECT_URI = "http://192.168.1.146/callback";


void init_spotify_client(spotify_client *spotify);
void get_access_token(spotify_client *spotify);

#endif