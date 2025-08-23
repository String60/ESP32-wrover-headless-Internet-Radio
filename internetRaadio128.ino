#include <WiFiManager.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"

// --- Pin definitions ---

// I2S audio output pins
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC  26

// Touch sensor pins
#define TOUCH_NEXT_PIN      32
#define TOUCH_PREV_PIN      33
#define TOUCH_PLAYPAUSE_PIN 34

// Legacy status LEDs
#define LED_PIN_VOL    2
#define LED_PIN_STATUS 4

// RGB LED pins
#define RGB_RED_PIN    12
#define RGB_GREEN_PIN  13
#define RGB_BLUE_PIN   14

// --- Debug control ---
#define DEBUG 1   // set to 0 to disable Serial printing

// --- URLs and network info ---
const char* configUrl = "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/config.json";

const char* userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)";

const char* genreUrls[] = {
  "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/British.json",
  "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/Australian.json",
  "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/Groups.json",
  "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/Decades.json",
  "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/Comedy.json",
  "https://raw.githubusercontent.com/String60/esp32-radio-streams/main/Other.json"
};

const char* genreNames[] = {
  "British", "Australian", "Groups", "Decades", "Comedy", "Other"
};

const int genreCount = sizeof(genreUrls) / sizeof(genreUrls[0]);

// --- Global objects and variables ---
Audio audio;
WiFiManager wifiManager;

struct RadioStation {
  String name;
  String url;
};

std::vector<RadioStation> stations;

int currentChannel = 0;
bool isPlaying = false;
bool playing = false;
bool wifiConnected = false;

unsigned long touchHoldStart = 0;

bool volumeMode = false;
bool genreMode = false;
unsigned long volumeModeActivatedAt = 0;
unsigned long genreModeActivatedAt = 0;

const int longPressDuration = 1000;
const int volumeModeTimeout = 10000;
const int genreModeTimeout = 10000;

int currentGenreIndex = 0;

unsigned long rgbBlinkStart = 0;
bool rgbBlinking = false;
int rgbBlinkDuration = 500;

enum RGBState { 
  RGB_OFF, 
  RGB_PLAYING, 
  RGB_PAUSED, 
  RGB_VOLUME, 
  RGB_CHANNEL_CHANGE, 
  RGB_WIFI, 
  RGB_GENRE_MODE, 
  RGB_GENRE_CHANGE 
};
RGBState rgbState = RGB_OFF;

// === Setup function ===
void setup() {
  #if DEBUG
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Internet Radio Starting ===");
  #endif

  pinMode(TOUCH_NEXT_PIN, INPUT);
  pinMode(TOUCH_PREV_PIN, INPUT);
  pinMode(TOUCH_PLAYPAUSE_PIN, INPUT);

  pinMode(LED_PIN_VOL, OUTPUT);
  pinMode(LED_PIN_STATUS, OUTPUT);
  digitalWrite(LED_PIN_VOL, LOW);
  digitalWrite(LED_PIN_STATUS, LOW);

  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  setRGBColor(0, 0, 0);

  // --- Audio setup ---
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);                    
  audio.setBufsize(8*1024, 128*1024);   // tuned for stable streaming

  // Connect to WiFi
  #if DEBUG
  Serial.println("Connecting to WiFi...");
  #endif
  while (!wifiManager.autoConnect("AutoConnectAP")) {
    blinkStatusLED();
  }
  wifiConnected = true;
  digitalWrite(LED_PIN_STATUS, HIGH);
  WiFi.setSleep(false);   // <<< important for stable streaming

  #if DEBUG
  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
  #endif

  setRGBState(RGB_WIFI);

  #if DEBUG
  Serial.println("Fetching configuration...");
  #endif
  fetchConfig();

  #if DEBUG
  Serial.println("Fetching initial stream list...");
  #endif
  if (fetchStreamList(genreUrls[currentGenreIndex])) {
    #if DEBUG
    Serial.print("Stream list loaded for genre: ");
    Serial.println(genreNames[currentGenreIndex]);
    #endif
    playChannel(currentChannel);
  } else {
    #if DEBUG
    Serial.println("Failed to fetch initial stream list.");
    #endif
  }
}

// === Main loop function ===
void loop() {
  audio.loop();      // always run this first
  yield();           // let WiFi background tasks run

  handleRGBBlink();  
  handleButtons();   // moved button code into separate function
}

// === Handle button input ===
void handleButtons() {
  bool nextPressed = digitalRead(TOUCH_NEXT_PIN) == HIGH;
  bool prevPressed = digitalRead(TOUCH_PREV_PIN) == HIGH;
  bool playPausePressed = digitalRead(TOUCH_PLAYPAUSE_PIN) == HIGH;

  static unsigned long nextTouchStart = 0;
  static unsigned long prevTouchStart = 0;

  digitalWrite(LED_PIN_VOL, volumeMode ? HIGH : LOW);

  if (volumeMode && millis() - volumeModeActivatedAt > volumeModeTimeout) {
    volumeMode = false;
    #if DEBUG
    Serial.println("Volume mode timeout. Exiting.");
    #endif
    setRGBState(isPlaying ? RGB_PLAYING : RGB_PAUSED);
  }

  if (genreMode && millis() - genreModeActivatedAt > genreModeTimeout) {
    genreMode = false;
    #if DEBUG
    Serial.println("Genre mode timeout. Exiting.");
    #endif
    setRGBState(isPlaying ? RGB_PLAYING : RGB_PAUSED);
  }

  // --- Play/Pause ---
  if (playPausePressed) {
    if (touchHoldStart == 0) touchHoldStart = millis();
    else if (!volumeMode && !genreMode && millis() - touchHoldStart >= longPressDuration) {
      volumeMode = true;
      volumeModeActivatedAt = millis();
      #if DEBUG
      Serial.println("Entered volume mode.");
      #endif
      setRGBState(RGB_VOLUME);
      startRGBBlink();
    }
  } else {
    if (touchHoldStart != 0 && !volumeMode && !genreMode) {
      #if DEBUG
      Serial.println("Play/Pause button tapped.");
      #endif
      togglePlayPause();
    }
    touchHoldStart = 0;
  }

  // --- Next button ---
  if (nextPressed) {
    if (nextTouchStart == 0) nextTouchStart = millis();
  } else if (nextTouchStart != 0) {
    unsigned long held = millis() - nextTouchStart;
    nextTouchStart = 0;

    if (volumeMode) {
      audio.setVolume(min(audio.getVolume() + 1, 21));
      #if DEBUG
      Serial.print("Volume Up: "); Serial.println(audio.getVolume());
      #endif
    }
    else if (genreMode) {
      if (held >= longPressDuration) {
        genreMode = false;
        #if DEBUG
        Serial.println("Exiting genre mode.");
        #endif
        setRGBState(isPlaying ? RGB_PLAYING : RGB_PAUSED);
      }
    }
    else {
      if (held >= longPressDuration) {
        genreMode = true;
        genreModeActivatedAt = millis();
        #if DEBUG
        Serial.println("Entered genre mode.");
        #endif
        setRGBState(RGB_GENRE_MODE);
        startRGBBlink();
      } else {
        currentChannel = (currentChannel + 1) % stations.size();
        #if DEBUG
        Serial.println("Short Next press.");
        #endif
        playChannel(currentChannel);
        setRGBState(RGB_CHANNEL_CHANGE);
        startRGBBlink();
      }
    }
    volumeModeActivatedAt = millis();
  }

  // --- Previous button ---
  if (prevPressed) {
    if (prevTouchStart == 0) prevTouchStart = millis();
  } else if (prevTouchStart != 0) {
    unsigned long held = millis() - prevTouchStart;
    prevTouchStart = 0;

    if (volumeMode) {
      audio.setVolume(max(audio.getVolume() - 1, 0));
      #if DEBUG
      Serial.print("Volume Down: "); Serial.println(audio.getVolume());
      #endif
    }
    else if (genreMode) {
      if (held < longPressDuration) {
        currentGenreIndex = (currentGenreIndex + 1) % genreCount;
        #if DEBUG
        Serial.print("Genre switched to: "); Serial.println(genreNames[currentGenreIndex]);
        #endif
        genreModeActivatedAt = millis();
        if (fetchStreamList(genreUrls[currentGenreIndex])) {
          currentChannel = 0;
          playChannel(currentChannel);
        }
        setRGBState(RGB_GENRE_CHANGE);
        startRGBBlink();
      }
    }
    else {
      if (held >= longPressDuration) {
        currentGenreIndex = 0;
        #if DEBUG
        Serial.println("Long Prev press: Jumping to British genre.");
        #endif
        genreMode = true;
        genreModeActivatedAt = millis();
        if (fetchStreamList(genreUrls[currentGenreIndex])) {
          currentChannel = 0;
          playChannel(currentChannel);
          setRGBState(RGB_GENRE_CHANGE);
          startRGBBlink();
        } else {
          #if DEBUG
          Serial.println("Failed to fetch British genre.");
          #endif
          setRGBState(RGB_PAUSED);
        }
      } else {
        currentChannel = (currentChannel - 1 + stations.size()) % stations.size();
        #if DEBUG
        Serial.println("Short Prev press.");
        #endif
        playChannel(currentChannel);
        setRGBState(RGB_CHANNEL_CHANGE);
        startRGBBlink();
      }
    }
    volumeModeActivatedAt = millis();
  }
}

// === RGB LED Functions ===
void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_RED_PIN, 255 - r);
  analogWrite(RGB_GREEN_PIN, 255 - g);
  analogWrite(RGB_BLUE_PIN, 255 - b);
}

void setRGBState(RGBState state) {
  rgbState = state;
  switch (state) {
    case RGB_OFF: setRGBColor(0,0,0); break;
    case RGB_PLAYING: setRGBColor(0,255,0); break;
    case RGB_PAUSED: setRGBColor(255,0,0); break;
    case RGB_VOLUME: break;
    case RGB_CHANNEL_CHANGE: break;
    case RGB_WIFI: setRGBColor(0,0,255); break;
    case RGB_GENRE_MODE: break;
    case RGB_GENRE_CHANGE: break;
  }
}

void startRGBBlink() {
  rgbBlinkStart = millis();
  rgbBlinking = true;
}

void handleRGBBlink() {
  if (!rgbBlinking) return;
  unsigned long elapsed = millis() - rgbBlinkStart;
  if (elapsed >= rgbBlinkDuration) {
    rgbBlinking = false;
    if (volumeMode) setRGBColor(255,0,255);
    else if (genreMode) setRGBColor(0,255,255);
    else setRGBState(isPlaying ? RGB_PLAYING : RGB_PAUSED);
  } else {
    if ((elapsed / 250) % 2 == 0) {
      switch (rgbState) {
        case RGB_VOLUME: setRGBColor(255,0,255); break;
        case RGB_CHANNEL_CHANGE: setRGBColor(0,255,0); break;
        case RGB_GENRE_MODE: setRGBColor(0,255,255); break;
        case RGB_GENRE_CHANGE: setRGBColor(0,0,255); break;
        default: break;
      }
    } else setRGBColor(0,0,0);
  }
}

// === Network and playback functions ===
bool fetchConfig() {
  HTTPClient http;
  http.begin(configUrl);
  http.addHeader("User-Agent", userAgent);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    http.end();
    if (error) {
      #if DEBUG
      Serial.println("Config JSON parse failed.");
      #endif
      return false;
    }
    if (doc.containsKey("volume")) audio.setVolume(constrain(doc["volume"],0,21));
    if (doc.containsKey("startChannel")) currentChannel = constrain(doc["startChannel"],0,stations.size()-1);
    return true;
  } else {
    #if DEBUG
    Serial.printf("Failed to fetch config. HTTP %d\n", httpCode);
    #endif
    http.end();
    return false;
  }
}

bool fetchStreamList(const char* url) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("User-Agent", userAgent);
  int httpCode = http.GET();
  if (httpCode != 200) {
    #if DEBUG
    Serial.printf("HTTP GET failed: %d\n", httpCode);
    #endif
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    #if DEBUG
    Serial.println("JSON parse failed.");
    #endif
    return false;
  }

  stations.clear();
  for (JsonObject obj : doc.as<JsonArray>()) {
    RadioStation s;
    s.name = obj["name"].as<String>();
    s.url = obj["url"].as<String>();
    stations.push_back(s);
  }
  #if DEBUG
  Serial.printf("Loaded %d stations.\n", stations.size());
  #endif
  return true;
}

void playChannel(int index) {
  if (index < 0 || index >= stations.size()) return;
  digitalWrite(LED_PIN_STATUS, LOW);
  delay(80);
  digitalWrite(LED_PIN_STATUS, HIGH);
  #if DEBUG
  Serial.printf("Connecting to stream #%d\n", index);
  Serial.println(stations[index].name);
  Serial.println(stations[index].url);
  #endif
  audio.connecttohost(stations[index].url.c_str(), userAgent);
  isPlaying = true;
  playing = true;
  setRGBState(RGB_PLAYING);
}

void stopChannel() {
  audio.stopSong();
  isPlaying = false;
  playing = false;
  digitalWrite(LED_PIN_STATUS, LOW);
  setRGBState(RGB_PAUSED);
}

void togglePlayPause() {
  if (isPlaying) stopChannel();
  else playChannel(currentChannel);
}

void blinkStatusLED() {
  digitalWrite(LED_PIN_STATUS, HIGH);
  delay(200);
  digitalWrite(LED_PIN_STATUS, LOW);
  delay(200);
}
