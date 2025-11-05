/*
 * ESP32 Chopper Dome Logics v5.2 - 2025/07
 * Optimized for ESP32-C3 Mini with Eyes & Periscope
 * www.printed-droid.com

 * * IMPORTANT: ESP32-C3 has only 2 RMT channels!
 * This version uses bit-banging for additional strips to avoid RMT conflicts.
 * USE FASTLED LIBRARY 3.9.0!
 
 * NEW FEATURES IN v5.2:
 * - Settings Persistence: All configurations are automatically saved
 *   and reloaded after a restart (using the Preferences library).
 * - Random Mode: Each component (Main Dome, Eyes, Periscope) can be
 *   set to a mode where patterns, colors, and speeds are
 *   changed automatically and randomly.
 * - Configurable LED Count: Easy adjustment of LED counts at the top of the sketch.
 *   Features:
 * - Non-blocking LED control with millis() timing
 * - Serial command interface for runtime configuration
 * - Configurable patterns for main dome (19 LEDs)
 * - Individual control for Eyes (up to 7 LEDs each) and Periscope (1 LED)
 * - 15 predefined colors + custom RGB support
 * - Smooth transitions and effects
 * - Help system via serial monitor
 * - Layered Patterns: Combines multiple patterns for more complex effects.
 * - Color Palettes: Use up to 5 colors for dynamic, multi-color animations.
 * - Smooth Transitions: Patterns now crossfade smoothly into one another.
 * - Settings Persistence for new features.
 * - Short/Long Press Button Control on IO0.
 * - 5 User-Savable Presets for complete lighting states.
 * - Serial commands to save, load, and set a startup preset.
 * - Layered patterns, color palettes, and smooth transitions.
 * - All documentation and output in English.
 * - Authentic "Chopper" eye animation mode (alternating blue, solid yellow).
 * - 5 Pre-configured default user presets (User 1 to User 5).
 * - Serial command to restore default presets.
 
 * * Hardware:
 * - ESP32-C3 Mini (Select "Lolin C3 Mini")
 * - 5x WS2812 LED strips
 * - Main dome: 19 LEDs on IO5
 * - Eye 1: up to 7 LEDs on IO6
 * - Eye 2: up to 7 LEDs on IO7
 * - Eye 3: up to 7 LEDs on IO10
 * - Periscope: 1 LED on IO4
 */

#include <FastLED.h>
#include <Preferences.h>
#include "esp_log.h"

// ## HARDWARE & CONFIGURATION ##
#define NUM_LEDS_MAIN       19
#define NUM_LEDS_EYE        7
#define NUM_LEDS_PERISCOPE  1
#define PALETTE_SIZE        5
#define NUM_PRESETS         5

#define BUTTON_PIN          0
#define LONG_PRESS_TIME     3000 // ms for a long press

// Pin definitions for ESP32-C3 Mini
#define LED_PIN_MAIN      5
#define LED_PIN_EYE1      6
#define LED_PIN_EYE2      7
#define LED_PIN_EYE3      10
#define LED_PIN_PERISCOPE 4

// Timing
#define RANDOM_INTERVAL 15000
#define TRANSITION_TIME 250
#define CHOPPER_EYE_INTERVAL 400 // Blink interval in ms for chopper mode

// Global objects
Preferences preferences;
CRGB leds_main[NUM_LEDS_MAIN];
CRGB leds_main_buffer[NUM_LEDS_MAIN];
CRGB leds_eye1[NUM_LEDS_EYE];
CRGB leds_eye2[NUM_LEDS_EYE];
CRGB leds_eye3[NUM_LEDS_EYE];
CRGB leds_periscope[NUM_LEDS_PERISCOPE];

// Predefined colors
const CRGB COLORS[15] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Orange, CRGB::Purple, CRGB::Cyan, CRGB::White, CRGB::Pink, CRGB::Lime, CRGB::Aqua, CRGB::Magenta, CRGB::Navy, CRGB::Maroon, CRGB::Olive};
const char* COLOR_NAMES[15] = {"red", "green", "blue", "yellow", "orange", "purple", "cyan", "white", "pink", "lime", "aqua", "magenta", "navy", "maroon", "olive"};

// Pattern types
enum PatternType {PATTERN_ORIGINAL, PATTERN_BLINK, PATTERN_FADE, PATTERN_RAINBOW, PATTERN_CHASE, PATTERN_SPARKLE, PATTERN_BREATHE, PATTERN_SOLID, PATTERN_LAYER, PATTERN_PALETTE};

// Configuration structures
struct MainConfig { PatternType pattern; CRGB color1; CRGB color2; CRGB colorPalette[PALETTE_SIZE]; bool usePalette; uint16_t speed; uint8_t brightness; bool enabled; bool isRandom; bool chopperEyeMode; };
struct ComponentConfig { CRGB color1; CRGB color2; uint16_t blinkInterval; bool twoColor; bool enabled; bool isRandom; };
struct Preset { MainConfig main; ComponentConfig components[4]; };

// Global configurations
MainConfig mainConfig;
ComponentConfig componentConfigs[4];

// Timing & State variables
unsigned long lastMainUpdate=0, lastRainbowUpdate=0, transitionStartTime=0, pressStartTime=0, lastChopperEyeUpdate = 0;
unsigned long lastComponentUpdate[4]={0}, lastRandomUpdate[5]={0};
uint8_t mainState=0, paletteState=0, rainbowIndex=0;
int currentMode = 0; // 0 = Chopper Default, 1-5 = User Presets
bool componentStates[4]={false}, originalPhase=true, inTransition=false, buttonActive=false, chopperEye1State = true;

// Serial command buffer
const byte SERIAL_BUFFER_SIZE = 128;
char serialBuffer[SERIAL_BUFFER_SIZE];
byte bufferPosition = 0;


// ################### FORWARD DECLARATIONS ###################
void processSerialCommand(char* command);
void processMainCommand(char* params);
void processComponentCommand(char* params, bool isPeriscope);
void setMainPattern(const char* pattern);
void setMainColor(char* colorStr, int colorNum);
void setPaletteColor(int index, char* colorStr);
void setComponentColor(int index, char* colorStr, int colorNum);
void setMainSpeed(int speed);
void setComponentSpeed(int index, int speed);
void setMainBrightness(int brightness);
CRGB parseColor(char* colorStr);
CRGB getRandomPaletteColor();
void strToLower(char* str);
char* trimWhitespace(char* str);
void updateRandomModes();
void updateMainPattern();
void updateComponent(int index);
void updateChopperEyes();
CRGB* getComponentLeds(int index);
const char* getPatternName(PatternType pattern);
void printHelp();
void printColors();
void printPatterns();
void printStatus();
void resetToDefaults();
void factoryResetPresets();
void saveMainConfig();
void loadMainConfig();
void saveComponentConfig(int index);
void loadComponentConfig(int index);
void savePreset(int index, Preset& p);
void loadPreset(int index);
void handleButton();
void activateChopperDefault();


// ################### SETUP ###################
void setup() {
  esp_log_level_set("rmt", ESP_LOG_NONE);
  Serial.begin(115200);
  delay(1000);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  randomSeed(esp_random());
  
  Serial.println("\n==================================================");
  Serial.println("    Welcome to Chopper Dome Logics v5.2");
  Serial.println("    (Presets, Button & Chopper Eyes)");
  Serial.println("             www.printed-droid.com");
  Serial.println("==================================================");

  preferences.begin("chopper-v53-en", false);

  if (!preferences.getBool("presetsInited", false)) {
    factoryResetPresets();
    preferences.putBool("presetsInited", true);
    Serial.println("✓ First boot: Factory presets have been created.");
  }
  
int startupMode = preferences.getUChar("startupMode", 0); // Default to loading Mode 0
  currentMode = startupMode;
  if (currentMode == 0) {
    activateChopperDefault();
    Serial.println("✓ Loaded Chopper Default on startup.");
  } else {
    loadPreset(currentMode);
    Serial.println("✓ Loaded User Preset " + String(currentMode) + " on startup.");
  }
  
  Serial.println("\nInitializing LED strips...");
  FastLED.addLeds<WS2812B, LED_PIN_MAIN, GRB>(leds_main, NUM_LEDS_MAIN);
  FastLED.addLeds<WS2812B, LED_PIN_EYE1, GRB>(leds_eye1, NUM_LEDS_EYE);
  FastLED.addLeds<WS2812B, LED_PIN_EYE2, GRB>(leds_eye2, NUM_LEDS_EYE);
  FastLED.addLeds<WS2812B, LED_PIN_EYE3, GRB>(leds_eye3, NUM_LEDS_EYE);
  FastLED.addLeds<WS2812B, LED_PIN_PERISCOPE, GRB>(leds_periscope, NUM_LEDS_PERISCOPE);
  
  FastLED.setBrightness(mainConfig.brightness);
  FastLED.clear();
  FastLED.show();
  Serial.println("✓ Button on IO0 initialized.");
  Serial.println("✓ System ready! Enter 'help' for available commands.\n");
}


// ################### MAIN LOOP ###################
void loop() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') { if (bufferPosition > 0) { serialBuffer[bufferPosition] = '\0'; processSerialCommand(serialBuffer); bufferPosition = 0; serialBuffer[0] = '\0'; } } 
    else if (inChar != '\r') { if (bufferPosition < SERIAL_BUFFER_SIZE - 1) serialBuffer[bufferPosition++] = inChar; }
  }
  
  handleButton();
  updateRandomModes();
  
  if (mainConfig.enabled) {
    updateMainPattern();
  } else {
    fill_solid(leds_main, NUM_LEDS_MAIN, CRGB::Black);
  }
  
  if (mainConfig.chopperEyeMode) {
    updateChopperEyes();
    if (componentConfigs[3].enabled) { updateComponent(3); } 
    else { fill_solid(leds_periscope, NUM_LEDS_PERISCOPE, CRGB::Black); }
  } else {
    for (int i = 0; i < 4; i++) {
      if (componentConfigs[i].enabled) { updateComponent(i); } 
      else { fill_solid(getComponentLeds(i), (i < 3 ? NUM_LEDS_EYE : NUM_LEDS_PERISCOPE), CRGB::Black); }
    }
  }
  
  FastLED.show();
}


// ################### BUTTON & PRESET LOGIC ###################
void handleButton() {
  bool currentState = (digitalRead(BUTTON_PIN) == LOW);
  if (currentState && !buttonActive) { pressStartTime = millis(); buttonActive = true; }
  if (!currentState && buttonActive) {
    unsigned long pressDuration = millis() - pressStartTime;
    if (pressDuration >= LONG_PRESS_TIME) {
      bool newState = !mainConfig.enabled;
      mainConfig.enabled = newState;
      for(int i=0; i<4; i++) componentConfigs[i].enabled = newState;
      Serial.println(String("Button LONG PRESS: Lights toggled ") + (newState ? "ON" : "OFF"));
      saveMainConfig();
      for(int i=0; i<4; i++) saveComponentConfig(i);
    } else {
      // --- SHORT PRESS: Cycle through all modes ---
      currentMode++;
      if (currentMode > NUM_PRESETS) { // Cycle from 5 back to 0 (Default)
        currentMode = 0;
      }

      if (currentMode == 0) {
        Serial.println("Button SHORT PRESS: Loading Chopper Default Mode");
        activateChopperDefault();
      } else {
        Serial.println("Button SHORT PRESS: Loading User Preset " + String(currentMode));
        loadPreset(currentMode);
      }
    }
    buttonActive = false;
  }
}

void activateChopperDefault() {
  currentMode = 0;
  // This defines the unchangeable "Chopper Default" look
  mainConfig = {PATTERN_CHASE, CRGB::Red, CRGB::Black, {}, false, 75, 180, true, false, true};
  
  // Make sure eyes are enabled so the chopperEyeMode can control them
  for (int i=0; i<3; i++) {
    componentConfigs[i].enabled = true;
  }
  // Periscope can be independent
  componentConfigs[3].enabled = true;
  componentConfigs[3].blinkInterval = 1000;
  componentConfigs[3].color1 = CRGB::Cyan;
  componentConfigs[3].twoColor = false;


  FastLED.setBrightness(mainConfig.brightness);
  
  // Trigger a smooth transition to this new state
  memcpy(leds_main_buffer, leds_main, sizeof(leds_main));
  transitionStartTime = millis();
  inTransition = true;
}

void savePreset(int index, Preset& p) {
  if (index < 1 || index > NUM_PRESETS) return;
  String key = "preset" + String(index);
  preferences.putBytes(key.c_str(), &p, sizeof(Preset));
}

void loadPreset(int index) {
  if (index < 1 || index > NUM_PRESETS) return;
  currentMode = index; // Set the global mode to the loaded preset
  Preset loadedPreset;
  String key = "preset" + String(index);
  if (preferences.getBytes(key.c_str(), &loadedPreset, sizeof(Preset))) {
    memcpy(&mainConfig, &loadedPreset.main, sizeof(MainConfig));
    memcpy(&componentConfigs, &loadedPreset.components, sizeof(ComponentConfig) * 4);
    FastLED.setBrightness(mainConfig.brightness);
    memcpy(leds_main_buffer, leds_main, sizeof(leds_main));
    transitionStartTime = millis();
    inTransition = true;
    Serial.println("Successfully loaded preset " + String(index));
  } else {
    Serial.println("Preset " + String(index) + " is empty. Nothing loaded.");
  }
}

void factoryResetPresets() {
  Serial.println("Creating factory default user presets...");
  Preset p; // A temporary Preset object we will fill with data

  // ===================================================================================
  // --- Preset 1: User 1 (Agitated / Working) ---
  // A busy look with a fast, sparkling dome and dual-color blinking eyes.
  // ===================================================================================
  p.main.pattern        = PATTERN_SPARKLE;
  p.main.color1         = CRGB::White;
  p.main.color2         = CRGB::Blue;
  p.main.speed          = 80;
  p.main.brightness     = 200;
  p.main.enabled        = true;
  p.main.isRandom       = false;
  p.main.chopperEyeMode = false;
  p.main.usePalette     = false;
  
  p.components[0] = {CRGB::Yellow, CRGB::Blue, 150, true, true, false}; // Eye 1
  p.components[1] = p.components[0];                                   // Eye 2 (same as 1)
  p.components[2] = {CRGB::Orange, CRGB::Black, 150, false, true, false}; // Eye 3
  p.components[3] = {CRGB::White, CRGB::Black, 300, false, true, false};  // Periscope
  savePreset(1, p);

  // ===================================================================================
  // --- Preset 2: User 2 (Angry / Attack Mode) ---
  // An aggressive look, everything is red. The dome breathes menacingly.
  // ===================================================================================
  p.main.pattern        = PATTERN_BREATHE;
  p.main.color1         = CRGB::Red;
  p.main.color2         = CRGB::Black;
  p.main.speed          = 50;
  p.main.brightness     = 255;
  p.main.enabled        = true;
  p.main.isRandom       = false;
  p.main.chopperEyeMode = false;
  p.main.usePalette     = false;

  p.components[0] = {CRGB::Red, CRGB::Black, 200, false, true, false}; // Eye 1
  p.components[1] = p.components[0];                                   // Eye 2
  p.components[2] = p.components[0];                                   // Eye 3
  p.components[3] = p.components[0];                                   // Periscope
  savePreset(2, p);

  // ===================================================================================
  // --- Preset 3: User 3 (Happy / Celebratory) ---
  // Bright and colorful. The dome has a rainbow, and the eyes blink in happy colors.
  // ===================================================================================
  p.main.pattern        = PATTERN_RAINBOW;
  p.main.color1         = CRGB::White; // Not used by rainbow, but set for consistency
  p.main.color2         = CRGB::Black;
  p.main.speed          = 50;
  p.main.brightness     = 220;
  p.main.enabled        = true;
  p.main.isRandom       = false;
  p.main.chopperEyeMode = false;
  p.main.usePalette     = false;
  
  p.components[0] = {CRGB::Cyan, CRGB::Yellow, 250, true, true, false}; // Eye 1
  p.components[1] = p.components[0];                                  // Eye 2
  p.components[2] = {CRGB::Lime, CRGB::Black, 250, false, true, false};  // Eye 3
  p.components[3] = {CRGB::Magenta, CRGB::Black, 500, false, true, false}; // Periscope
  savePreset(3, p);
  
  // ===================================================================================
  // --- Preset 4: User 4 (Stealth / Undercover) ---
  // Dim and slow. Most components are off.
  // ===================================================================================
  p.main.pattern        = PATTERN_FADE;
  p.main.color1         = CRGB::Navy;
  p.main.color2         = CRGB::Black;
  p.main.speed          = 500;
  p.main.brightness     = 80;
  p.main.enabled        = true;
  p.main.isRandom       = false;
  p.main.chopperEyeMode = false;
  p.main.usePalette     = false;
  
  p.components[0] = {CRGB::Blue, CRGB::Black, 1500, false, true, false}; // Eye 1
  p.components[1] = p.components[0];                                    // Eye 2
  p.components[2].enabled = false;                                      // Eye 3 off
  p.components[3].enabled = false;                                      // Periscope off
  savePreset(4, p);

  // ===================================================================================
  // --- Preset 5: User 5 (Chopper Eyes + Palette Dome) ---
  // A special combination: The authentic Chopper eyes with a warm, flickering dome.
  // ===================================================================================
  p.main.pattern        = PATTERN_PALETTE;
  p.main.color1         = CRGB::White; // Not used by palette
  p.main.color2         = CRGB::Black;
  p.main.speed          = 200;
  p.main.brightness     = 200;
  p.main.enabled        = true;
  p.main.isRandom       = false;
  p.main.chopperEyeMode = true; // Use authentic eye animation
  p.main.usePalette     = true; // This pattern uses the palette below
  p.main.colorPalette[0] = CRGB::Orange;
  p.main.colorPalette[1] = CRGB::Yellow;
  p.main.colorPalette[2] = CRGB::White;
  p.main.colorPalette[3] = CRGB::DarkOrange;
  p.main.colorPalette[4] = CRGB::Bisque;
  
  // Settings for Eyes 1, 2, 3 are ignored when chopperEyeMode is true.
  p.components[0] = {};
  p.components[1] = {};
  p.components[2] = {};
  p.components[3] = {CRGB::Cyan, CRGB::Black, 1000, false, true, false}; // Periscope is independent
  savePreset(5, p);

  Serial.println("Default user presets 1-5 created and saved.");
}

// ################### SERIAL COMMAND PROCESSING ###################
void processSerialCommand(char* command) {
  char* params = trimWhitespace(command);
  strToLower(params);
  if (strcmp(params, "reset presets") == 0) { factoryResetPresets(); }
  else if (strcmp(params, "load default") == 0) { activateChopperDefault(); }
  else if (strncmp(params, "save ", 5) == 0) { 
    int index = atoi(params + 5);
    if(index > 0 && index <= NUM_PRESETS) {
      Preset p; 
      memcpy(&p.main, &mainConfig, sizeof(MainConfig)); 
      memcpy(&p.components, &componentConfigs, sizeof(ComponentConfig)*4);
      savePreset(index, p);
    } else {
      Serial.println("Error: Can only save to User Presets 1-5.");
    }
  }
  else if (strncmp(params, "load ", 5) == 0) { loadPreset(atoi(params + 5)); }
  else if (strncmp(params, "startup ", 8) == 0) {
    int index = atoi(params + 8);
    if (index >= 0 && index <= NUM_PRESETS) { 
      preferences.putUChar("startupMode", index); 
      Serial.println("Startup mode set to " + String(index) + ". (0=Default, 1-5=User)"); 
    } else { 
      Serial.println("Error: Mode index must be between 0 and 5."); 
    }
  }
  else if (strncmp(params, "main ", 5) == 0) processMainCommand(trimWhitespace(params + 5));
  else if (strncmp(params, "eye ", 4) == 0) processComponentCommand(trimWhitespace(params + 4), false);
  else if (strncmp(params, "periscope ", 10) == 0) processComponentCommand(trimWhitespace(params + 10), true);
  else if (strcmp(params, "status") == 0) printStatus();
  else if (strcmp(params, "colors") == 0) printColors();
  else if (strcmp(params, "patterns") == 0) printPatterns();
  else if (strcmp(params, "reset") == 0) resetToDefaults();
  else if (strcmp(params, "help") == 0) printHelp();
  else Serial.println("Unknown command. Type 'help' for available commands.");
}

void processMainCommand(char* params) {
  bool needsSave = true;
  if (strncmp(params, "pattern ", 8) == 0) { setMainPattern(trimWhitespace(params + 8)); mainConfig.isRandom = false; }
  else if (strncmp(params, "color1 ", 7) == 0) { setMainColor(trimWhitespace(params + 7), 1); mainConfig.isRandom = false; }
  else if (strncmp(params, "color2 ", 7) == 0) { setMainColor(trimWhitespace(params + 7), 2); mainConfig.isRandom = false; }
  else if (strncmp(params, "eyemode ", 8) == 0) {
      char* mode = trimWhitespace(params + 8);
      if (strcmp(mode, "chopper") == 0) { mainConfig.chopperEyeMode = true; Serial.println("Chopper eye mode enabled."); } 
      else { mainConfig.chopperEyeMode = false; Serial.println("Default eye mode enabled."); }
  } else if (strncmp(params, "palette ", 8) == 0) {
      char* p = trimWhitespace(params + 8); int idx = atoi(p) - 1; char* c = strchr(p, ' ');
      if (c != nullptr && idx >= 0 && idx < PALETTE_SIZE) setPaletteColor(idx, trimWhitespace(c + 1));
      else Serial.println("Invalid format. Use: main palette <1-5> <color>");
      mainConfig.isRandom = false;
  } else if (strncmp(params, "palettemode ", 12) == 0) {
      char* state = trimWhitespace(params + 12); mainConfig.usePalette = (strcmp(state, "on") == 0);
      Serial.println(String("Palette mode for chase/sparkle ") + (mainConfig.usePalette ? "enabled" : "disabled")); mainConfig.isRandom = false;
  } else if (strncmp(params, "speed ", 6) == 0) { setMainSpeed(atoi(params + 6)); mainConfig.isRandom = false; }
  else if (strncmp(params, "brightness ", 11) == 0) { setMainBrightness(atoi(params + 11)); }
  else if (strcmp(params, "on") == 0) { mainConfig.enabled = true; Serial.println("Main dome enabled"); }
  else if (strcmp(params, "off") == 0) { mainConfig.enabled = false; Serial.println("Main dome disabled"); }
  else if (strncmp(params, "random ", 7) == 0) {
    char* state = trimWhitespace(params + 7); mainConfig.isRandom = (strcmp(state, "on") == 0);
    Serial.println(String("Main dome random mode ") + (mainConfig.isRandom ? "enabled" : "disabled"));
  } else { Serial.println("Unknown main command. See 'help'."); needsSave = false; }
  if (needsSave) saveMainConfig();
}

void processComponentCommand(char* params, bool isPeriscope) {
    int index = -1; char* command; String componentNameStr;
    if (isPeriscope) { index = 3; command = params; componentNameStr = "Periscope"; } 
    else {
        int eyeNum = params[0] - '1';
        if (eyeNum >= 0 && eyeNum <= 2) { index = eyeNum; command = trimWhitespace(params + 1); componentNameStr = "Eye " + String(index + 1); } 
        else { Serial.println("Eye number must be 1-3"); return; }
    }
    if (index == -1) return;
    bool manualOverride = true;
    if (strncmp(command, "color1 ", 7) == 0) setComponentColor(index, trimWhitespace(command + 7), 1);
    else if (strncmp(command, "color2 ", 7) == 0) setComponentColor(index, trimWhitespace(command + 7), 2);
    else if (strncmp(command, "speed ", 6) == 0) setComponentSpeed(index, atoi(command + 6));
    else if (strcmp(command, "single") == 0) { componentConfigs[index].twoColor = false; Serial.println(componentNameStr + " set to single color"); }
    else if (strcmp(command, "dual") == 0) { componentConfigs[index].twoColor = true; Serial.println(componentNameStr + " set to dual color"); }
    else if (strcmp(command, "on") == 0) { componentConfigs[index].enabled = true; Serial.println(componentNameStr + " enabled"); manualOverride = false; }
    else if (strcmp(command, "off") == 0) { componentConfigs[index].enabled = false; Serial.println(componentNameStr + " disabled"); manualOverride = false; }
    else if (strncmp(command, "random ", 7) == 0) {
        char* state = trimWhitespace(command + 7); componentConfigs[index].isRandom = (strcmp(state, "on") == 0);
        Serial.println(componentNameStr + " random mode " + (componentConfigs[index].isRandom ? "enabled" : "disabled")); manualOverride = false;
    } else { Serial.println("Valid commands: color1, color2, speed, single, dual, random on/off, on, off"); return; }
    if (manualOverride) componentConfigs[index].isRandom = false;
    saveComponentConfig(index);
}

// ################### SETTER & PARSER FUNCTIONS ###################
void setMainPattern(const char* pattern) {
  PatternType oldPattern = mainConfig.pattern, newPattern = oldPattern;
  if (strcmp(pattern, "original") == 0) newPattern = PATTERN_ORIGINAL; else if (strcmp(pattern, "blink") == 0) newPattern = PATTERN_BLINK;
  else if (strcmp(pattern, "fade") == 0) newPattern = PATTERN_FADE; else if (strcmp(pattern, "rainbow") == 0) newPattern = PATTERN_RAINBOW;
  else if (strcmp(pattern, "chase") == 0) newPattern = PATTERN_CHASE; else if (strcmp(pattern, "sparkle") == 0) newPattern = PATTERN_SPARKLE;
  else if (strcmp(pattern, "breathe") == 0) newPattern = PATTERN_BREATHE; else if (strcmp(pattern, "solid") == 0) newPattern = PATTERN_SOLID;
  else if (strcmp(pattern, "layer") == 0) newPattern = PATTERN_LAYER; else if (strcmp(pattern, "palette") == 0) newPattern = PATTERN_PALETTE;
  else { Serial.println("Unknown pattern. Type 'patterns' for a list."); return; }
  if (oldPattern != newPattern) {
    memcpy(leds_main_buffer, leds_main, sizeof(leds_main)); transitionStartTime = millis(); inTransition = true;
    mainConfig.pattern = newPattern; mainState = 0; originalPhase = true;
    Serial.println(String("Main pattern set to: ") + pattern);
  }
}

void setMainColor(char* colorStr, int colorNum) {
  CRGB color = parseColor(colorStr);
  if (color == CRGB::Black && strcmp(colorStr, "black") != 0 && strcmp(colorStr, "off") != 0) return;
  if (colorNum == 1) mainConfig.color1 = color; else mainConfig.color2 = color;
  Serial.println("Main color " + String(colorNum) + " set to: " + colorStr);
}

void setPaletteColor(int index, char* colorStr) {
    CRGB color = parseColor(colorStr);
    if (color == CRGB::Black && strcmp(colorStr, "black") != 0 && strcmp(colorStr, "off") != 0) return;
    mainConfig.colorPalette[index] = color;
    Serial.println("Palette color " + String(index + 1) + " set to: " + colorStr);
}

void setComponentColor(int index, char* colorStr, int colorNum) {
  CRGB color = parseColor(colorStr);
  if (color == CRGB::Black && strcmp(colorStr, "black") != 0 && strcmp(colorStr, "off") != 0) return;
  String name = (index < 3) ? "Eye " + String(index + 1) : "Periscope";
  if (colorNum == 1) componentConfigs[index].color1 = color; else componentConfigs[index].color2 = color;
  Serial.println(name + " color " + String(colorNum) + " set to: " + colorStr);
}

void setMainSpeed(int speed) {
  if (speed < 10 || speed > 5000) { Serial.println("Speed must be between 10 and 5000 ms"); return; }
  mainConfig.speed = speed;
  Serial.println("Main speed set to: " + String(speed) + " ms");
}

void setComponentSpeed(int index, int speed) {
  if (speed < 10 || speed > 5000) { Serial.println("Speed must be between 10 and 5000 ms"); return; }
  componentConfigs[index].blinkInterval = speed;
  String name = (index < 3) ? "Eye " + String(index + 1) : "Periscope";
  Serial.println(name + " speed set to: " + String(speed) + " ms");
}

void setMainBrightness(int brightness) {
  if (brightness < 1 || brightness > 255) { Serial.println("Brightness must be between 1 and 255"); return; }
  mainConfig.brightness = brightness;
  FastLED.setBrightness(brightness);
  Serial.println("Main brightness set to: " + String(brightness));
}

CRGB parseColor(char* colorStr) {
  for (int i=0; i<15; i++) if (strcmp(colorStr, COLOR_NAMES[i]) == 0) return COLORS[i];
  if (strcmp(colorStr, "black") == 0 || strcmp(colorStr, "off") == 0) return CRGB::Black;
  char* p1 = strchr(colorStr, ',');
  if (p1) { char* p2 = strchr(p1 + 1, ','); if (p2) { *p1='\0'; *p2='\0'; int r=atoi(colorStr),g=atoi(p1+1),b=atoi(p2+1); *p1=',';*p2=','; if (r>=0&&r<=255&&g>=0&&g<=255&&b>=0&&b<=255) return CRGB(r,g,b); } }
  Serial.println("Invalid color. Use a name (e.g., red) or RGB format (e.g., 255,100,0).");
  return CRGB::Black;
}

CRGB getRandomPaletteColor() { return mainConfig.colorPalette[random8(PALETTE_SIZE)]; }

// ################### LED ANIMATION & HELPERS ###################
void updateMainPattern() {
  CRGB patternBuffer[NUM_LEDS_MAIN]; unsigned long currentTime = millis();
  switch (mainConfig.pattern) {
    case PATTERN_ORIGINAL: memcpy(patternBuffer, leds_main, sizeof(leds_main)); if (currentTime - lastMainUpdate >= mainConfig.speed) { if(originalPhase) patternBuffer[mainState]=mainConfig.color1; else patternBuffer[mainState]=mainConfig.color2; mainState++; if (mainState >= NUM_LEDS_MAIN) { mainState=0; originalPhase=!originalPhase; } lastMainUpdate=currentTime; } break;
    case PATTERN_BLINK: if (currentTime-lastMainUpdate >= mainConfig.speed) { mainState=!mainState; fill_solid(patternBuffer,NUM_LEDS_MAIN,mainState?mainConfig.color1:mainConfig.color2); lastMainUpdate=currentTime; } else { memcpy(patternBuffer,leds_main,sizeof(leds_main)); } break;
    case PATTERN_FADE: if (currentTime-lastMainUpdate >= mainConfig.speed/255) { uint8_t b=sin8(mainState*2); fill_solid(patternBuffer,NUM_LEDS_MAIN,mainConfig.color1.nscale8_video(b)); mainState++; lastMainUpdate=currentTime; } else { memcpy(patternBuffer,leds_main,sizeof(leds_main)); } break;
    case PATTERN_RAINBOW: if (currentTime-lastRainbowUpdate >= mainConfig.speed/10) { fill_rainbow(patternBuffer,NUM_LEDS_MAIN,rainbowIndex,7); rainbowIndex++; lastRainbowUpdate=currentTime; } else { memcpy(patternBuffer,leds_main,sizeof(leds_main)); } break;
    case PATTERN_CHASE: if (currentTime-lastMainUpdate >= mainConfig.speed) { fill_solid(patternBuffer,NUM_LEDS_MAIN,mainConfig.color2); patternBuffer[mainState]=mainConfig.usePalette?getRandomPaletteColor():mainConfig.color1; mainState=(mainState+1)%NUM_LEDS_MAIN; lastMainUpdate=currentTime; } else { memcpy(patternBuffer,leds_main,sizeof(leds_main)); } break;
    case PATTERN_SPARKLE: if (currentTime-lastMainUpdate >= mainConfig.speed) { fill_solid(patternBuffer,NUM_LEDS_MAIN,mainConfig.color2); for(int i=0;i<3;i++) { patternBuffer[random(NUM_LEDS_MAIN)]=mainConfig.usePalette?getRandomPaletteColor():mainConfig.color1; } lastMainUpdate=currentTime; } else { memcpy(patternBuffer,leds_main,sizeof(leds_main)); } break;
    case PATTERN_BREATHE: { uint8_t b=ease8InOutQuad(beatsin8(10,0,255)); fill_solid(patternBuffer,NUM_LEDS_MAIN,mainConfig.color1.nscale8_video(b)); } break;
    case PATTERN_SOLID: fill_solid(patternBuffer,NUM_LEDS_MAIN,mainConfig.color1); break;
    case PATTERN_LAYER: { uint8_t b=ease8InOutQuad(beatsin8(10,0,255)); fill_solid(patternBuffer,NUM_LEDS_MAIN,mainConfig.color1.nscale8_video(b)); if(random8()<20) patternBuffer[random(NUM_LEDS_MAIN)]=mainConfig.color2; } break;
    case PATTERN_PALETTE: { CRGB c1=mainConfig.colorPalette[paletteState], c2=mainConfig.colorPalette[(paletteState+1)%PALETTE_SIZE]; uint8_t b=ease8InOutCubic(beatsin8(10,0,255)); fill_solid(patternBuffer,NUM_LEDS_MAIN,blend(c1,c2,b)); if(b==255&&beatsin8(10,0,255)<128) paletteState=(paletteState+1)%PALETTE_SIZE; } break;
  }
  if (inTransition) {
    uint32_t elapsed = millis()-transitionStartTime;
    if (elapsed >= TRANSITION_TIME) { inTransition=false; memcpy(leds_main,patternBuffer,sizeof(leds_main)); } 
    else { uint8_t b=map(elapsed,0,TRANSITION_TIME,0,255); for(int i=0;i<NUM_LEDS_MAIN;i++) leds_main[i]=blend(leds_main_buffer[i],patternBuffer[i],b); }
  } else { memcpy(leds_main,patternBuffer,sizeof(leds_main)); }
}

void updateComponent(int index) {
  unsigned long currentTime = millis();
  if (currentTime - lastComponentUpdate[index] >= componentConfigs[index].blinkInterval) {
    CRGB* leds = getComponentLeds(index); int num_leds = (index<3)?NUM_LEDS_EYE:NUM_LEDS_PERISCOPE;
    CRGB color = componentStates[index]?(componentConfigs[index].twoColor?componentConfigs[index].color1:componentConfigs[index].color1):(componentConfigs[index].twoColor?componentConfigs[index].color2:CRGB::Black);
    fill_solid(leds, num_leds, color); componentStates[index]=!componentStates[index]; lastComponentUpdate[index]=currentTime;
  }
}

void updateChopperEyes() {
  if (componentConfigs[2].enabled) { fill_solid(leds_eye3, NUM_LEDS_EYE, CRGB::Yellow); } 
  else { fill_solid(leds_eye3, NUM_LEDS_EYE, CRGB::Black); }
  unsigned long currentTime = millis();
  if (currentTime - lastChopperEyeUpdate >= CHOPPER_EYE_INTERVAL) {
    lastChopperEyeUpdate = currentTime; chopperEye1State = !chopperEye1State;
    if (componentConfigs[0].enabled) { fill_solid(leds_eye1, NUM_LEDS_EYE, chopperEye1State ? CRGB::Blue : CRGB::Black); } else { fill_solid(leds_eye1, NUM_LEDS_EYE, CRGB::Black); }
    if (componentConfigs[1].enabled) { fill_solid(leds_eye2, NUM_LEDS_EYE, !chopperEye1State ? CRGB::Blue : CRGB::Black); } else { fill_solid(leds_eye2, NUM_LEDS_EYE, CRGB::Black); }
  }
}

void updateRandomModes() {
  unsigned long currentTime=millis();
  if (mainConfig.isRandom&&mainConfig.enabled) { if(currentTime-lastRandomUpdate[0]>=RANDOM_INTERVAL){ setMainPattern(getPatternName((PatternType)random(PATTERN_BLINK,PATTERN_SOLID+1))); mainConfig.color1=COLORS[random(15)]; mainConfig.color2=(random(2)==1)?CRGB::Black:COLORS[random(15)]; mainConfig.speed=random(50,301); mainConfig.usePalette=(random(4)==0); Serial.println(String("[RANDOM] Main Dome: New pattern -> ")+getPatternName(mainConfig.pattern)); lastRandomUpdate[0]=currentTime; } }
  for(int i=0;i<4;i++){ if(componentConfigs[i].isRandom&&componentConfigs[i].enabled){ if(currentTime-lastRandomUpdate[i+1]>=RANDOM_INTERVAL){ componentConfigs[i].color1=COLORS[random(15)]; componentConfigs[i].color2=COLORS[random(15)]; componentConfigs[i].blinkInterval=random(100,501); componentConfigs[i].twoColor=(random(3)>0); String n=(i<3)?"Eye "+String(i+1):"Periscope"; Serial.println("[RANDOM] "+n+": New random settings applied."); lastRandomUpdate[i+1]=currentTime; } } }
}

CRGB* getComponentLeds(int index) {
  switch(index) { case 0: return leds_eye1; case 1: return leds_eye2; case 2: return leds_eye3; case 3: return leds_periscope; default: return nullptr; }
}

const char* getPatternName(PatternType p) {
  switch(p){ case PATTERN_ORIGINAL:return"original";case PATTERN_BLINK:return"blink";case PATTERN_FADE:return"fade";case PATTERN_RAINBOW:return"rainbow";case PATTERN_CHASE:return"chase";case PATTERN_SPARKLE:return"sparkle";case PATTERN_BREATHE:return"breathe";case PATTERN_SOLID:return"solid";case PATTERN_LAYER:return"layer";case PATTERN_PALETTE:return"palette";default:return"unknown"; }
}

// ################### UTILITY & INFO COMMANDS ###################
void printHelp() {
  Serial.println("\n=== ESP32 Chopper Dome Logics v5.2 - Command Help ===");
  Serial.println("\n--- PHYSICAL CONTROLS (BUTTON ON IO0) ---");
  Serial.println("\n--- MODES ---");
  Serial.println("  The system has one unchangeable 'Default Mode' (0) and 5 'User Presets' (1-5).");
  Serial.println("  The button cycles through: Default -> User 1 -> ... -> User 5 -> Default");
  Serial.println("  SHORT PRESS (<1s) : Cycle through modes (0-5).");
  Serial.println("  LONG PRESS  (>1s) : Toggle all lights ON or OFF.");

  Serial.println("\n--- UTILITY COMMANDS ---");
  Serial.println("  status     - Show current configuration of all components.");
  Serial.println("  patterns   - Show list of available main dome patterns.");
  Serial.println("  colors     - Show list of predefined color names.");
  Serial.println("  reset      - Reset all settings (except presets) to firmware defaults.");
  Serial.println("  help       - Shows this help text.\n");

  Serial.println("\n--- MODE & PRESET COMMANDS ---");
  Serial.println("  load default      - Activate the unchangeable Chopper Default look.");
  Serial.println("  load <1-5>        - Load a custom User Preset.");
  Serial.println("  save <1-5>        - Save current config to a User Preset slot (cannot save over default).");
  Serial.println("  startup <0-5>     - Set mode to load on boot (0=Default, 1-5=User).");
  Serial.println("  reset presets     - Restore the 5 factory default User Presets.");

  Serial.println("\n--- MAIN DOME COMMANDS ---");
  Serial.println("  main pattern <name>         - Set animation pattern (see 'patterns').");
  Serial.println("  main eyemode <chopper/def>  - Set authentic 'Chopper' or default eye pattern.");
  Serial.println("  main color1 <color>         - Set primary color.");
  Serial.println("  main color2 <color>         - Set secondary/layer color.");
  Serial.println("  main palette <1-5> <color>  - Set a color in the palette.");
  Serial.println("  main palettemode <on/off>   - Use palette for chase/sparkle patterns.");
  Serial.println("  main speed <ms>             - Set animation speed (10-5000).");
  Serial.println("  main brightness <1-255>     - Set master brightness.");
  Serial.println("  main random <on/off>        - Enable/disable random mode.");
  Serial.println("  main <on/off>               - Enable/disable main dome LEDs.");
  Serial.println("  > Example: main palette 3 255,0,255");

  Serial.println("\n--- EYE (1-3) & PERISCOPE COMMANDS ---");
  Serial.println("  eye <1-3> <...>           - Controls for eyes (e.g., eye 1 color1 blue).");
  Serial.println("  > Sub-commands: color1, color2, speed, single, dual, random on/off, on, off");
  Serial.println("  eye <1-3> color1 <color> - Set primary color for specified eye");
  Serial.println("  eye <1-3> color2 <color> - Set secondary color");
  Serial.println("  eye <1-3> speed <ms>     - Set blink speed (10-5000)");
  Serial.println("  eye <1-3> single/dual    - Set single or dual color blinking mode");
  Serial.println("  eye <1-3> random on/off  - Enable/disable automatic random settings");
  Serial.println("  eye <1-3> on/off         - Enable/disable specified eye LEDs");

  Serial.println("  periscope <...>          - Controls for periscope (e.g., periscope speed 500).");
  Serial.println("  periscope color1 <color> - Set primary color");
  Serial.println("  periscope color2 <color> - Set secondary color");
  Serial.println("  periscope speed <ms>     - Set blink speed (10-5000)");
  Serial.println("  periscope single/dual    - Set single or dual color blinking mode");
  Serial.println("  periscope random on/off  - Enable/disable automatic random settings");
  Serial.println("  periscope on/off         - Enable/disable periscope LED");

  Serial.println("\n--- COLOR FORMATS ---");
  Serial.println("  - By name: red, blue, green, etc. (see 'colors')");
  Serial.println("  - By RGB value: e.g., 255,0,0 for red");
  Serial.println("  - Special names: black, off\n");
}

void printColors() {
  Serial.println("\nAvailable predefined colors:");
  for (int i=0;i<15;i++) Serial.println(String("  - ")+COLOR_NAMES[i]);
  Serial.println("\nAlso supports RGB format (e.g., '100,50,255') and 'black' or 'off'.\n");
}

void printPatterns() {
  Serial.println("\nAvailable patterns for the main dome:");
  Serial.println("  original - The classic LED-by-LED fill effect.");
  Serial.println("  blink    - Simple blinking between color1 and color2.");
  Serial.println("  fade     - Smooth fade in and out of color1.");
  Serial.println("  rainbow  - A moving rainbow effect across the strip.");
  Serial.println("  chase    - A single pixel chasing on a background of color2.");
  Serial.println("  sparkle  - Random pixels sparkling on a background of color2.");
  Serial.println("  breathe  - A smooth pulsing 'breathing' effect with color1.");
  Serial.println("  solid    - A solid, non-animated display of color1.");
  Serial.println("  layer    - (NEW) Breathe effect (color1) with sparkle overlay (color2).");
  Serial.println("  palette  - (NEW) Smoothly cycles through the 5 palette colors.\n");
}

void printStatus() {
    Serial.println("\n=== Current System Configuration ===");
    String modeString = (currentMode == 0) ? "Chopper Default" : "User Preset " + String(currentMode);
    Serial.println("  ACTIVE MODE: " + modeString);
    Serial.println("\nMain Dome & Global:");
    Serial.println("  Enabled: " + String(mainConfig.enabled ? "Yes" : "No"));
    Serial.println("  Random Mode: " + String(mainConfig.isRandom ? "ON" : "OFF"));
    Serial.println("  Eye Mode: " + String(mainConfig.chopperEyeMode ? "Chopper" : "Default"));
    Serial.println(String("  Pattern: ") + getPatternName(mainConfig.pattern));
    Serial.println("  Speed: " + String(mainConfig.speed) + " ms");
    Serial.println("  Brightness: " + String(mainConfig.brightness));
    Serial.println("  Color 1 (RGB): " + String(mainConfig.color1.r) + "," + String(mainConfig.color1.g) + "," + String(mainConfig.color1.b));
    Serial.println("  Color 2 (RGB): " + String(mainConfig.color2.r) + "," + String(mainConfig.color2.g) + "," + String(mainConfig.color2.b));
    Serial.println("  Palette Mode: " + String(mainConfig.usePalette ? "ON" : "OFF"));
    for(int i=0;i<PALETTE_SIZE;i++) Serial.println("    Palette "+String(i+1)+": "+String(mainConfig.colorPalette[i].r)+","+String(mainConfig.colorPalette[i].g)+","+String(mainConfig.colorPalette[i].b));
    for(int i=0;i<4;i++) {
        String name=(i<3)?"Eye "+String(i+1):"Periscope";
        Serial.println("\n" + name + ":");
        Serial.println("  Enabled: " + String(componentConfigs[i].enabled ? "Yes" : "No"));
        Serial.println("  Random Mode: " + String(componentConfigs[i].isRandom ? "ON" : "OFF"));
        Serial.println("  Mode: " + String(componentConfigs[i].twoColor ? "Dual Color" : "Single Color"));
        Serial.println("  Speed: " + String(componentConfigs[i].blinkInterval) + " ms");
        Serial.println("  Color 1 (RGB): "+String(componentConfigs[i].color1.r)+","+String(componentConfigs[i].color1.g)+","+String(componentConfigs[i].color1.b));
        Serial.println("  Color 2 (RGB): "+String(componentConfigs[i].color2.r)+","+String(componentConfigs[i].color2.g)+","+String(componentConfigs[i].color2.b));
    }
    Serial.println();
}

void resetToDefaults() {
  preferences.clear();
  loadMainConfig();
  for(int i=0; i<4; i++) loadComponentConfig(i);
  FastLED.setBrightness(mainConfig.brightness);
  saveMainConfig();
  for(int i=0; i<4; i++) saveComponentConfig(i);
  factoryResetPresets();
  preferences.putBool("presetsInited", true);
  Serial.println("All settings and presets have been reset to firmware defaults.");
}

// ################### PERSISTENCE HELPERS ###################
void saveMainConfig() {
  preferences.putUChar("main_pattern", mainConfig.pattern);
  preferences.putBytes("main_color1", &mainConfig.color1, sizeof(CRGB));
  preferences.putBytes("main_color2", &mainConfig.color2, sizeof(CRGB));
  preferences.putBytes("main_palette", &mainConfig.colorPalette, sizeof(mainConfig.colorPalette));
  preferences.putBool("main_usePlt", mainConfig.usePalette);
  preferences.putUShort("main_speed", mainConfig.speed);
  preferences.putUChar("main_bright", mainConfig.brightness);
  preferences.putBool("main_enabled", mainConfig.enabled);
  preferences.putBool("main_isRandom", mainConfig.isRandom);
  preferences.putBool("main_eyeMode", mainConfig.chopperEyeMode);
}

void loadMainConfig() {
  mainConfig.pattern = (PatternType)preferences.getUChar("main_pattern", PATTERN_ORIGINAL);
  mainConfig.color1 = CRGB::Red; preferences.getBytes("main_color1", &mainConfig.color1, sizeof(CRGB));
  mainConfig.color2 = CRGB::Black; preferences.getBytes("main_color2", &mainConfig.color2, sizeof(CRGB));
  if (preferences.getBytesLength("main_palette")==sizeof(mainConfig.colorPalette)) { preferences.getBytes("main_palette", &mainConfig.colorPalette, sizeof(mainConfig.colorPalette)); } 
  else { mainConfig.colorPalette[0]=CRGB::Blue; mainConfig.colorPalette[1]=CRGB::Red; mainConfig.colorPalette[2]=CRGB::Green; mainConfig.colorPalette[3]=CRGB::Yellow; mainConfig.colorPalette[4]=CRGB::Purple; }
  mainConfig.usePalette = preferences.getBool("main_usePlt", false);
  mainConfig.speed = preferences.getUShort("main_speed", 50);
  mainConfig.brightness = preferences.getUChar("main_bright", 100);
  mainConfig.enabled = preferences.getBool("main_enabled", true);
  mainConfig.isRandom = preferences.getBool("main_isRandom", false);
  mainConfig.chopperEyeMode = preferences.getBool("main_eyeMode", false);
}

void saveComponentConfig(int index) {
  String prefix = "c" + String(index) + "_";
  preferences.putBytes((prefix + "color1").c_str(), &componentConfigs[index].color1, sizeof(CRGB));
  preferences.putBytes((prefix + "color2").c_str(), &componentConfigs[index].color2, sizeof(CRGB));
  preferences.putUShort((prefix + "speed").c_str(), componentConfigs[index].blinkInterval);
  preferences.putBool((prefix + "twoColor").c_str(), componentConfigs[index].twoColor);
  preferences.putBool((prefix + "enabled").c_str(), componentConfigs[index].enabled);
  preferences.putBool((prefix + "isRandom").c_str(), componentConfigs[index].isRandom);
}

void loadComponentConfig(int index) {
  String prefix = "c" + String(index) + "_";
  CRGB defaultColor1 = (index<3)?CRGB::Yellow:CRGB::Cyan; CRGB defaultColor2 = (index<3)?CRGB::Blue:CRGB::Black;
  componentConfigs[index].color1 = defaultColor1; componentConfigs[index].color2 = defaultColor2;
  preferences.getBytes((prefix + "color1").c_str(), &componentConfigs[index].color1, sizeof(CRGB));
  preferences.getBytes((prefix + "color2").c_str(), &componentConfigs[index].color2, sizeof(CRGB));
  uint16_t defaultSpeed = (index<3)?250:500; bool defaultTwoColor = (index<3)?true:false;
  componentConfigs[index].blinkInterval = preferences.getUShort((prefix + "speed").c_str(), defaultSpeed);
  componentConfigs[index].twoColor = preferences.getBool((prefix + "twoColor").c_str(), defaultTwoColor);
  componentConfigs[index].enabled = preferences.getBool((prefix + "enabled").c_str(), true);
  componentConfigs[index].isRandom = preferences.getBool((prefix + "isRandom").c_str(), false);
}

// ################### STRING HELPERS ###################
char* trimWhitespace(char* str) {
  while (isspace((unsigned char)*str)) str++;
  if (*str == 0) return str;
  char* end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;
  *(end + 1) = '\0';
  return str;
}

void strToLower(char* str) {
  for (char* p = str; *p; ++p) *p = tolower(*p);
}

