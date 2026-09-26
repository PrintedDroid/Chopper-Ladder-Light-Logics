/*
 * Arduino Nano Chopper Dome Logics v4.2 - 2025/10
 * Adapted from ESP32 version for Arduino Nano
 * www.printed-droid.com
 
 * USE FASTLED v3.9.0 !!!

 * Key changes from ESP32:
 * - Uses EEPROM instead of Preferences for storage
 * - Adjusted pin mappings for Arduino Nano
 * - Removed ESP32-specific features
 * - Simplified memory usage for Nano's limited RAM
 * 
 * Hardware:
 * - Arduino Nano
 * - 5x WS2812 LED strips
 * - Main dome (Ladder Light): 19 LEDs on D9
 * - Eye 1: 7 LEDs on D10
 * - Eye 2: 7 LEDs on D11  
 * - Eye 3: 7 LEDs on D12
 * - Periscope: 1 LED on D13
 * - Button on D2
 */

#include <FastLED.h>
#include <EEPROM.h>

// ## HARDWARE & CONFIGURATION ##
#define NUM_LEDS_MAIN       19
#define NUM_LEDS_EYE        7
#define NUM_LEDS_PERISCOPE  1
#define NUM_PRESETS         5

#define BUTTON_PIN          2
#define LONG_PRESS_TIME     1000 // ms for a long press

// Pin definitions for Arduino Nano
#define LED_PIN_MAIN        9   // Ladder Light
#define LED_PIN_EYE1        10
#define LED_PIN_EYE2        11
#define LED_PIN_EYE3        12
#define LED_PIN_PERISCOPE   13

// Timing
#define RANDOM_INTERVAL 15000
#define TRANSITION_TIME 250

// EEPROM addresses
#define EEPROM_SIGNATURE    0x42  // Signature byte to check if EEPROM is initialized
#define EEPROM_ADDR_SIG     0
#define EEPROM_ADDR_MODE    1
#define EEPROM_ADDR_MAIN    10
#define EEPROM_ADDR_COMP    50
#define EEPROM_ADDR_PRESET  100  // Presets start here

// Global LED arrays
CRGB leds_main[NUM_LEDS_MAIN];
CRGB leds_main_buffer[NUM_LEDS_MAIN];
CRGB leds_eye1[NUM_LEDS_EYE];
CRGB leds_eye2[NUM_LEDS_EYE];
CRGB leds_eye3[NUM_LEDS_EYE];
CRGB leds_periscope[NUM_LEDS_PERISCOPE];

// Predefined colors
const CRGB COLORS[15] = {
  CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Orange,
  CRGB::Purple, CRGB::Cyan, CRGB::White, CRGB::Pink, CRGB::Lime,
  CRGB::Aqua, CRGB::Magenta, CRGB::Navy, CRGB::Maroon, CRGB::Olive
};

const char COLOR_NAMES[15][8] PROGMEM = {
  "red", "green", "blue", "yellow", "orange",
  "purple", "cyan", "white", "pink", "lime",
  "aqua", "magenta", "navy", "maroon", "olive"
};

// Pattern types
enum PatternType {
  PATTERN_ORIGINAL, PATTERN_BLINK, PATTERN_FADE, PATTERN_RAINBOW,
  PATTERN_CHASE, PATTERN_SPARKLE, PATTERN_BREATHE, PATTERN_SOLID
};

// Simplified configuration structures for Arduino Nano
struct MainConfig {
  PatternType pattern;
  CRGB color1;
  CRGB color2;
  uint16_t speed;
  uint8_t brightness;
  bool enabled;
  bool isRandom;
};

struct ComponentConfig {
  CRGB color1;
  CRGB color2;
  uint16_t blinkInterval;
  bool twoColor;
  bool enabled;
  bool isRandom;
};

// Global configurations
MainConfig mainConfig;
ComponentConfig componentConfigs[4];

// Timing & State variables
unsigned long lastMainUpdate = 0;
unsigned long lastRainbowUpdate = 0;
unsigned long transitionStartTime = 0;
unsigned long pressStartTime = 0;
unsigned long lastComponentUpdate[4] = {0};
unsigned long lastRandomUpdate[5] = {0};

uint8_t mainState = 0;
uint8_t rainbowIndex = 0;
int currentMode = 0;

bool componentStates[4] = {false};
bool originalPhase = true;
bool inTransition = false;
bool buttonPressed = false;
bool buttonActive = false;

// Serial command buffer
char serialBuffer[64];
byte bufferPosition = 0;

// ################### FORWARD DECLARATIONS ###################
void processSerialCommand(char* command);
void processMainCommand(char* params);
void processComponentCommand(char* params, bool isPeriscope);
void setMainPattern(const char* pattern);
void setMainColor(char* colorStr, int colorNum);
void setComponentColor(int index, char* colorStr, int colorNum);
void setMainSpeed(int speed);
void setComponentSpeed(int index, int speed);
void setMainBrightness(int brightness);
CRGB parseColor(char* colorStr);
void updateRandomModes();
void updateMainPattern();
void updateComponent(int index);
CRGB* getComponentLeds(int index);
const char* getPatternName(PatternType pattern);
void printHelp();
void printColors();
void printPatterns();
void printStatus();
void resetToDefaults();
void saveMainConfig();
void loadMainConfig();
void saveComponentConfig(int index);
void loadComponentConfig(int index);
void savePreset(int index);
void loadPreset(int index);
void handleButton();
void initializeEEPROM();
char* trimWhitespace(char* str);
void strToLower(char* str);

// ################### SETUP ###################
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(0));
  
  Serial.println(F("\n=================================================="));
  Serial.println(F("    Welcome to Chopper Dome Logics v4.2"));
  Serial.println(F("          Arduino Nano Version"));
  Serial.println(F("         www.printed-droid.com"));
  Serial.println(F("=================================================="));
  
  // Check EEPROM initialization
  if (EEPROM.read(EEPROM_ADDR_SIG) != EEPROM_SIGNATURE) {
    Serial.println(F("Initializing EEPROM..."));
    initializeEEPROM();
  }
  
  // Load startup mode
  currentMode = EEPROM.read(EEPROM_ADDR_MODE);
  if (currentMode > NUM_PRESETS) currentMode = 0;
  
  if (currentMode == 0) {
    resetToDefaults();
    Serial.println(F("Loaded default configuration"));
  } else {
    loadPreset(currentMode);
    Serial.print(F("Loaded User Preset "));
    Serial.println(currentMode);
  }
  
  Serial.println(F("\nInitializing LED strips..."));
  FastLED.addLeds<WS2812B, LED_PIN_MAIN, GRB>(leds_main, NUM_LEDS_MAIN);
  FastLED.addLeds<WS2812B, LED_PIN_EYE1, GRB>(leds_eye1, NUM_LEDS_EYE);
  FastLED.addLeds<WS2812B, LED_PIN_EYE2, GRB>(leds_eye2, NUM_LEDS_EYE);
  FastLED.addLeds<WS2812B, LED_PIN_EYE3, GRB>(leds_eye3, NUM_LEDS_EYE);
  FastLED.addLeds<WS2812B, LED_PIN_PERISCOPE, GRB>(leds_periscope, NUM_LEDS_PERISCOPE);
  
  FastLED.setBrightness(mainConfig.brightness);
  FastLED.clear();
  FastLED.show();
  
  Serial.println(F("System ready! Enter 'help' for commands.\n"));
}

// ################### MAIN LOOP ###################
void loop() {
  // Handle serial input
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      if (bufferPosition > 0) {
        serialBuffer[bufferPosition] = '\0';
        processSerialCommand(serialBuffer);
        bufferPosition = 0;
      }
    } else if (inChar != '\r' && bufferPosition < 63) {
      serialBuffer[bufferPosition++] = inChar;
    }
  }
  
  handleButton();
  updateRandomModes();
  
  if (mainConfig.enabled) {
    updateMainPattern();
  } else {
    fill_solid(leds_main, NUM_LEDS_MAIN, CRGB::Black);
  }
  
  for (int i = 0; i < 4; i++) {
    if (componentConfigs[i].enabled) {
      updateComponent(i);
    } else {
      int numLeds = (i < 3) ? NUM_LEDS_EYE : NUM_LEDS_PERISCOPE;
      fill_solid(getComponentLeds(i), numLeds, CRGB::Black);
    }
  }
  
  FastLED.show();
}

// ################### BUTTON HANDLING ###################
void handleButton() {
  bool currentState = (digitalRead(BUTTON_PIN) == LOW);
  
  if (currentState && !buttonActive) {
    pressStartTime = millis();
    buttonActive = true;
  }
  
  if (!currentState && buttonActive) {
    unsigned long pressDuration = millis() - pressStartTime;
    
    if (pressDuration >= LONG_PRESS_TIME) {
      // Long press - toggle all lights
      bool newState = !mainConfig.enabled;
      mainConfig.enabled = newState;
      for(int i = 0; i < 4; i++) {
        componentConfigs[i].enabled = newState;
      }
      Serial.print(F("Lights toggled "));
      Serial.println(newState ? F("ON") : F("OFF"));
      saveMainConfig();
      for(int i = 0; i < 4; i++) {
        saveComponentConfig(i);
      }
    } else {
      // Short press - cycle modes
      currentMode++;
      if (currentMode > NUM_PRESETS) {
        currentMode = 0;
      }
      
      if (currentMode == 0) {
        Serial.println(F("Loading Default Mode"));
        resetToDefaults();
      } else {
        Serial.print(F("Loading User Preset "));
        Serial.println(currentMode);
        loadPreset(currentMode);
      }
      
      EEPROM.write(EEPROM_ADDR_MODE, currentMode);
    }
    buttonActive = false;
  }
}

// ################### SERIAL COMMANDS ###################
void processSerialCommand(char* command) {
  char* params = trimWhitespace(command);
  strToLower(params);
  
  if (strcmp(params, "help") == 0) {
    printHelp();
  } else if (strcmp(params, "status") == 0) {
    printStatus();
  } else if (strcmp(params, "colors") == 0) {
    printColors();
  } else if (strcmp(params, "patterns") == 0) {
    printPatterns();
  } else if (strcmp(params, "reset") == 0) {
    resetToDefaults();
  } else if (strncmp(params, "save ", 5) == 0) {
    int index = atoi(params + 5);
    if(index > 0 && index <= NUM_PRESETS) {
      savePreset(index);
      Serial.print(F("Saved to preset "));
      Serial.println(index);
    }
  } else if (strncmp(params, "load ", 5) == 0) {
    int index = atoi(params + 5);
    if(index >= 0 && index <= NUM_PRESETS) {
      if(index == 0) {
        resetToDefaults();
      } else {
        loadPreset(index);
      }
      currentMode = index;
      EEPROM.write(EEPROM_ADDR_MODE, currentMode);
    }
  } else if (strncmp(params, "startup ", 8) == 0) {
    int index = atoi(params + 8);
    if (index >= 0 && index <= NUM_PRESETS) {
      EEPROM.write(EEPROM_ADDR_MODE, index);
      Serial.print(F("Startup mode set to "));
      Serial.println(index);
    }
  } else if (strncmp(params, "main ", 5) == 0) {
    processMainCommand(params + 5);
  } else if (strncmp(params, "eye ", 4) == 0) {
    processComponentCommand(params + 4, false);
  } else if (strncmp(params, "periscope ", 10) == 0) {
    processComponentCommand(params + 10, true);
  } else {
    Serial.println(F("Unknown command"));
  }
}

void processMainCommand(char* params) {
  params = trimWhitespace(params);
  
  if (strncmp(params, "pattern ", 8) == 0) {
    setMainPattern(params + 8);
    mainConfig.isRandom = false;
  } else if (strncmp(params, "color1 ", 7) == 0) {
    setMainColor(params + 7, 1);
    mainConfig.isRandom = false;
  } else if (strncmp(params, "color2 ", 7) == 0) {
    setMainColor(params + 7, 2);
    mainConfig.isRandom = false;
  } else if (strncmp(params, "speed ", 6) == 0) {
    setMainSpeed(atoi(params + 6));
    mainConfig.isRandom = false;
  } else if (strncmp(params, "brightness ", 11) == 0) {
    setMainBrightness(atoi(params + 11));
  } else if (strcmp(params, "on") == 0) {
    mainConfig.enabled = true;
    Serial.println(F("Main dome enabled"));
  } else if (strcmp(params, "off") == 0) {
    mainConfig.enabled = false;
    Serial.println(F("Main dome disabled"));
  } else if (strncmp(params, "random ", 7) == 0) {
    mainConfig.isRandom = (strcmp(params + 7, "on") == 0);
    Serial.print(F("Main random mode "));
    Serial.println(mainConfig.isRandom ? F("ON") : F("OFF"));
  }
  saveMainConfig();
}

void processComponentCommand(char* params, bool isPeriscope) {
  int index = -1;
  char* command;
  
  if (isPeriscope) {
    index = 3;
    command = trimWhitespace(params);
  } else {
    int eyeNum = params[0] - '1';
    if (eyeNum >= 0 && eyeNum <= 2) {
      index = eyeNum;
      command = trimWhitespace(params + 1);
    }
  }
  
  if (index == -1) return;
  
  if (strncmp(command, "color1 ", 7) == 0) {
    setComponentColor(index, command + 7, 1);
    componentConfigs[index].isRandom = false;
  } else if (strncmp(command, "color2 ", 7) == 0) {
    setComponentColor(index, command + 7, 2);
    componentConfigs[index].isRandom = false;
  } else if (strncmp(command, "speed ", 6) == 0) {
    setComponentSpeed(index, atoi(command + 6));
    componentConfigs[index].isRandom = false;
  } else if (strcmp(command, "single") == 0) {
    componentConfigs[index].twoColor = false;
  } else if (strcmp(command, "dual") == 0) {
    componentConfigs[index].twoColor = true;
  } else if (strcmp(command, "on") == 0) {
    componentConfigs[index].enabled = true;
  } else if (strcmp(command, "off") == 0) {
    componentConfigs[index].enabled = false;
  } else if (strncmp(command, "random ", 7) == 0) {
    componentConfigs[index].isRandom = (strcmp(command + 7, "on") == 0);
  }
  
  saveComponentConfig(index);
}

// ################### SETTERS ###################
void setMainPattern(const char* pattern) {
  PatternType newPattern = mainConfig.pattern;
  
  if (strcmp(pattern, "original") == 0) newPattern = PATTERN_ORIGINAL;
  else if (strcmp(pattern, "blink") == 0) newPattern = PATTERN_BLINK;
  else if (strcmp(pattern, "fade") == 0) newPattern = PATTERN_FADE;
  else if (strcmp(pattern, "rainbow") == 0) newPattern = PATTERN_RAINBOW;
  else if (strcmp(pattern, "chase") == 0) newPattern = PATTERN_CHASE;
  else if (strcmp(pattern, "sparkle") == 0) newPattern = PATTERN_SPARKLE;
  else if (strcmp(pattern, "breathe") == 0) newPattern = PATTERN_BREATHE;
  else if (strcmp(pattern, "solid") == 0) newPattern = PATTERN_SOLID;
  else {
    Serial.println(F("Unknown pattern"));
    return;
  }
  
  if (mainConfig.pattern != newPattern) {
    memcpy(leds_main_buffer, leds_main, sizeof(leds_main));
    transitionStartTime = millis();
    inTransition = true;
    mainConfig.pattern = newPattern;
    mainState = 0;
    originalPhase = true;
    Serial.print(F("Pattern set to: "));
    Serial.println(pattern);
  }
}

void setMainColor(char* colorStr, int colorNum) {
  CRGB color = parseColor(trimWhitespace(colorStr));
  if (colorNum == 1) {
    mainConfig.color1 = color;
  } else {
    mainConfig.color2 = color;
  }
  Serial.print(F("Main color "));
  Serial.print(colorNum);
  Serial.print(F(" set to: "));
  Serial.println(colorStr);
}

void setComponentColor(int index, char* colorStr, int colorNum) {
  CRGB color = parseColor(trimWhitespace(colorStr));
  if (colorNum == 1) {
    componentConfigs[index].color1 = color;
  } else {
    componentConfigs[index].color2 = color;
  }
}

void setMainSpeed(int speed) {
  if (speed < 10 || speed > 5000) {
    Serial.println(F("Speed must be 10-5000"));
    return;
  }
  mainConfig.speed = speed;
  Serial.print(F("Speed: "));
  Serial.println(speed);
}

void setComponentSpeed(int index, int speed) {
  if (speed < 10 || speed > 5000) return;
  componentConfigs[index].blinkInterval = speed;
}

void setMainBrightness(int brightness) {
  if (brightness < 1 || brightness > 255) {
    Serial.println(F("Brightness must be 1-255"));
    return;
  }
  mainConfig.brightness = brightness;
  FastLED.setBrightness(brightness);
  Serial.print(F("Brightness: "));
  Serial.println(brightness);
}

// ################### COLOR PARSING ###################
CRGB parseColor(char* colorStr) {
  // Check predefined colors
  for (int i = 0; i < 15; i++) {
    char colorName[8];
    strcpy_P(colorName, COLOR_NAMES[i]);
    if (strcmp(colorStr, colorName) == 0) {
      return COLORS[i];
    }
  }
  
  if (strcmp(colorStr, "black") == 0 || strcmp(colorStr, "off") == 0) {
    return CRGB::Black;
  }
  
  // Parse RGB format
  char* p1 = strchr(colorStr, ',');
  if (p1) {
    char* p2 = strchr(p1 + 1, ',');
    if (p2) {
      *p1 = '\0';
      *p2 = '\0';
      int r = atoi(colorStr);
      int g = atoi(p1 + 1);
      int b = atoi(p2 + 1);
      if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
        return CRGB(r, g, b);
      }
    }
  }
  
  Serial.println(F("Invalid color"));
  return CRGB::Black;
}

// ################### LED UPDATES ###################
void updateMainPattern() {
  CRGB patternBuffer[NUM_LEDS_MAIN];
  unsigned long currentTime = millis();
  
  switch (mainConfig.pattern) {
    case PATTERN_ORIGINAL:
      memcpy(patternBuffer, leds_main, sizeof(leds_main));
      if (currentTime - lastMainUpdate >= mainConfig.speed) {
        if(originalPhase) {
          patternBuffer[mainState] = mainConfig.color1;
        } else {
          patternBuffer[mainState] = mainConfig.color2;
        }
        mainState++;
        if (mainState >= NUM_LEDS_MAIN) {
          mainState = 0;
          originalPhase = !originalPhase;
        }
        lastMainUpdate = currentTime;
      }
      break;
      
    case PATTERN_BLINK:
      if (currentTime - lastMainUpdate >= mainConfig.speed) {
        mainState = !mainState;
        fill_solid(patternBuffer, NUM_LEDS_MAIN, mainState ? mainConfig.color1 : mainConfig.color2);
        lastMainUpdate = currentTime;
      } else {
        memcpy(patternBuffer, leds_main, sizeof(leds_main));
      }
      break;
      
    case PATTERN_FADE:
      if (currentTime - lastMainUpdate >= mainConfig.speed / 255) {
        uint8_t brightness = sin8(mainState * 2);
        fill_solid(patternBuffer, NUM_LEDS_MAIN, mainConfig.color1.nscale8_video(brightness));
        mainState++;
        lastMainUpdate = currentTime;
      } else {
        memcpy(patternBuffer, leds_main, sizeof(leds_main));
      }
      break;
      
    case PATTERN_RAINBOW:
      if (currentTime - lastRainbowUpdate >= mainConfig.speed / 10) {
        fill_rainbow(patternBuffer, NUM_LEDS_MAIN, rainbowIndex, 7);
        rainbowIndex++;
        lastRainbowUpdate = currentTime;
      } else {
        memcpy(patternBuffer, leds_main, sizeof(leds_main));
      }
      break;
      
    case PATTERN_CHASE:
      if (currentTime - lastMainUpdate >= mainConfig.speed) {
        fill_solid(patternBuffer, NUM_LEDS_MAIN, mainConfig.color2);
        patternBuffer[mainState] = mainConfig.color1;
        mainState = (mainState + 1) % NUM_LEDS_MAIN;
        lastMainUpdate = currentTime;
      } else {
        memcpy(patternBuffer, leds_main, sizeof(leds_main));
      }
      break;
      
    case PATTERN_SPARKLE:
      if (currentTime - lastMainUpdate >= mainConfig.speed) {
        fill_solid(patternBuffer, NUM_LEDS_MAIN, mainConfig.color2);
        for(int i = 0; i < 3; i++) {
          patternBuffer[random(NUM_LEDS_MAIN)] = mainConfig.color1;
        }
        lastMainUpdate = currentTime;
      } else {
        memcpy(patternBuffer, leds_main, sizeof(leds_main));
      }
      break;
      
    case PATTERN_BREATHE:
      {
        uint8_t brightness = beatsin8(10, 0, 255);
        fill_solid(patternBuffer, NUM_LEDS_MAIN, mainConfig.color1.nscale8_video(brightness));
      }
      break;
      
    case PATTERN_SOLID:
      fill_solid(patternBuffer, NUM_LEDS_MAIN, mainConfig.color1);
      break;
  }
  
  // Handle transitions
  if (inTransition) {
    uint32_t elapsed = millis() - transitionStartTime;
    if (elapsed >= TRANSITION_TIME) {
      inTransition = false;
      memcpy(leds_main, patternBuffer, sizeof(leds_main));
    } else {
      uint8_t blendAmount = map(elapsed, 0, TRANSITION_TIME, 0, 255);
      for(int i = 0; i < NUM_LEDS_MAIN; i++) {
        leds_main[i] = blend(leds_main_buffer[i], patternBuffer[i], blendAmount);
      }
    }
  } else {
    memcpy(leds_main, patternBuffer, sizeof(leds_main));
  }
}

void updateComponent(int index) {
  unsigned long currentTime = millis();
  if (currentTime - lastComponentUpdate[index] >= componentConfigs[index].blinkInterval) {
    CRGB* leds = getComponentLeds(index);
    int numLeds = (index < 3) ? NUM_LEDS_EYE : NUM_LEDS_PERISCOPE;
    
    CRGB color;
    if (componentStates[index]) {
      color = componentConfigs[index].color1;
    } else {
      color = componentConfigs[index].twoColor ? componentConfigs[index].color2 : CRGB::Black;
    }
    
    fill_solid(leds, numLeds, color);
    componentStates[index] = !componentStates[index];
    lastComponentUpdate[index] = currentTime;
  }
}

void updateRandomModes() {
  unsigned long currentTime = millis();
  
  if (mainConfig.isRandom && mainConfig.enabled) {
    if(currentTime - lastRandomUpdate[0] >= RANDOM_INTERVAL) {
      mainConfig.pattern = (PatternType)random(PATTERN_BLINK, PATTERN_SOLID + 1);
      mainConfig.color1 = COLORS[random(15)];
      mainConfig.color2 = (random(2) == 1) ? CRGB::Black : COLORS[random(15)];
      mainConfig.speed = random(50, 301);
      Serial.println(F("[RANDOM] Main updated"));
      lastRandomUpdate[0] = currentTime;
    }
  }
  
  for(int i = 0; i < 4; i++) {
    if(componentConfigs[i].isRandom && componentConfigs[i].enabled) {
      if(currentTime - lastRandomUpdate[i + 1] >= RANDOM_INTERVAL) {
        componentConfigs[i].color1 = COLORS[random(15)];
        componentConfigs[i].color2 = COLORS[random(15)];
        componentConfigs[i].blinkInterval = random(100, 501);
        componentConfigs[i].twoColor = (random(3) > 0);
        Serial.print(F("[RANDOM] Component "));
        Serial.print(i);
        Serial.println(F(" updated"));
        lastRandomUpdate[i + 1] = currentTime;
      }
    }
  }
}

CRGB* getComponentLeds(int index) {
  switch(index) {
    case 0: return leds_eye1;
    case 1: return leds_eye2;
    case 2: return leds_eye3;
    case 3: return leds_periscope;
    default: return nullptr;
  }
}

// ################### EEPROM FUNCTIONS ###################
void initializeEEPROM() {
  EEPROM.write(EEPROM_ADDR_SIG, EEPROM_SIGNATURE);
  EEPROM.write(EEPROM_ADDR_MODE, 0);
  resetToDefaults();
  saveMainConfig();
  for(int i = 0; i < 4; i++) {
    saveComponentConfig(i);
  }
}

void saveMainConfig() {
  int addr = EEPROM_ADDR_MAIN;
  EEPROM.write(addr++, mainConfig.pattern);
  EEPROM.write(addr++, mainConfig.color1.r);
  EEPROM.write(addr++, mainConfig.color1.g);
  EEPROM.write(addr++, mainConfig.color1.b);
  EEPROM.write(addr++, mainConfig.color2.r);
  EEPROM.write(addr++, mainConfig.color2.g);
  EEPROM.write(addr++, mainConfig.color2.b);
  EEPROM.write(addr++, mainConfig.speed >> 8);
  EEPROM.write(addr++, mainConfig.speed & 0xFF);
  EEPROM.write(addr++, mainConfig.brightness);
  EEPROM.write(addr++, mainConfig.enabled);
  EEPROM.write(addr++, mainConfig.isRandom);
}

void loadMainConfig() {
  int addr = EEPROM_ADDR_MAIN;
  mainConfig.pattern = (PatternType)EEPROM.read(addr++);
  mainConfig.color1.r = EEPROM.read(addr++);
  mainConfig.color1.g = EEPROM.read(addr++);
  mainConfig.color1.b = EEPROM.read(addr++);
  mainConfig.color2.r = EEPROM.read(addr++);
  mainConfig.color2.g = EEPROM.read(addr++);
  mainConfig.color2.b = EEPROM.read(addr++);
  mainConfig.speed = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
  mainConfig.brightness = EEPROM.read(addr++);
  mainConfig.enabled = EEPROM.read(addr++);
  mainConfig.isRandom = EEPROM.read(addr++);
  
  // Validate loaded values
  if(mainConfig.pattern > PATTERN_SOLID) mainConfig.pattern = PATTERN_ORIGINAL;
  if(mainConfig.speed < 10 || mainConfig.speed > 5000) mainConfig.speed = 50;
  if(mainConfig.brightness == 0) mainConfig.brightness = 100;
}

void saveComponentConfig(int index) {
  int addr = EEPROM_ADDR_COMP + (index * 12);
  EEPROM.write(addr++, componentConfigs[index].color1.r);
  EEPROM.write(addr++, componentConfigs[index].color1.g);
  EEPROM.write(addr++, componentConfigs[index].color1.b);
  EEPROM.write(addr++, componentConfigs[index].color2.r);
  EEPROM.write(addr++, componentConfigs[index].color2.g);
  EEPROM.write(addr++, componentConfigs[index].color2.b);
  EEPROM.write(addr++, componentConfigs[index].blinkInterval >> 8);
  EEPROM.write(addr++, componentConfigs[index].blinkInterval & 0xFF);
  EEPROM.write(addr++, componentConfigs[index].twoColor);
  EEPROM.write(addr++, componentConfigs[index].enabled);
  EEPROM.write(addr++, componentConfigs[index].isRandom);
}

void loadComponentConfig(int index) {
  int addr = EEPROM_ADDR_COMP + (index * 12);
  componentConfigs[index].color1.r = EEPROM.read(addr++);
  componentConfigs[index].color1.g = EEPROM.read(addr++);
  componentConfigs[index].color1.b = EEPROM.read(addr++);
  componentConfigs[index].color2.r = EEPROM.read(addr++);
  componentConfigs[index].color2.g = EEPROM.read(addr++);
  componentConfigs[index].color2.b = EEPROM.read(addr++);
  componentConfigs[index].blinkInterval = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
  componentConfigs[index].twoColor = EEPROM.read(addr++);
  componentConfigs[index].enabled = EEPROM.read(addr++);
  componentConfigs[index].isRandom = EEPROM.read(addr++);
  
  // Validate
  if(componentConfigs[index].blinkInterval < 10 || componentConfigs[index].blinkInterval > 5000) {
    componentConfigs[index].blinkInterval = (index < 3) ? 250 : 500;
  }
}

void savePreset(int index) {
  if(index < 1 || index > NUM_PRESETS) return;
  
  int addr = EEPROM_ADDR_PRESET + ((index - 1) * 60);
  
  // Save main config
  EEPROM.write(addr++, mainConfig.pattern);
  EEPROM.write(addr++, mainConfig.color1.r);
  EEPROM.write(addr++, mainConfig.color1.g);
  EEPROM.write(addr++, mainConfig.color1.b);
  EEPROM.write(addr++, mainConfig.color2.r);
  EEPROM.write(addr++, mainConfig.color2.g);
  EEPROM.write(addr++, mainConfig.color2.b);
  EEPROM.write(addr++, mainConfig.speed >> 8);
  EEPROM.write(addr++, mainConfig.speed & 0xFF);
  EEPROM.write(addr++, mainConfig.brightness);
  EEPROM.write(addr++, mainConfig.enabled);
  EEPROM.write(addr++, mainConfig.isRandom);
  
  // Save component configs
  for(int i = 0; i < 4; i++) {
    EEPROM.write(addr++, componentConfigs[i].color1.r);
    EEPROM.write(addr++, componentConfigs[i].color1.g);
    EEPROM.write(addr++, componentConfigs[i].color1.b);
    EEPROM.write(addr++, componentConfigs[i].color2.r);
    EEPROM.write(addr++, componentConfigs[i].color2.g);
    EEPROM.write(addr++, componentConfigs[i].color2.b);
    EEPROM.write(addr++, componentConfigs[i].blinkInterval >> 8);
    EEPROM.write(addr++, componentConfigs[i].blinkInterval & 0xFF);
    EEPROM.write(addr++, componentConfigs[i].twoColor);
    EEPROM.write(addr++, componentConfigs[i].enabled);
    EEPROM.write(addr++, componentConfigs[i].isRandom);
  }
}

void loadPreset(int index) {
  if(index < 1 || index > NUM_PRESETS) return;
  
  int addr = EEPROM_ADDR_PRESET + ((index - 1) * 60);
  
  // Load main config
  mainConfig.pattern = (PatternType)EEPROM.read(addr++);
  mainConfig.color1.r = EEPROM.read(addr++);
  mainConfig.color1.g = EEPROM.read(addr++);
  mainConfig.color1.b = EEPROM.read(addr++);
  mainConfig.color2.r = EEPROM.read(addr++);
  mainConfig.color2.g = EEPROM.read(addr++);
  mainConfig.color2.b = EEPROM.read(addr++);
  mainConfig.speed = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
  mainConfig.brightness = EEPROM.read(addr++);
  mainConfig.enabled = EEPROM.read(addr++);
  mainConfig.isRandom = EEPROM.read(addr++);
  
  // Load component configs
  for(int i = 0; i < 4; i++) {
    componentConfigs[i].color1.r = EEPROM.read(addr++);
    componentConfigs[i].color1.g = EEPROM.read(addr++);
    componentConfigs[i].color1.b = EEPROM.read(addr++);
    componentConfigs[i].color2.r = EEPROM.read(addr++);
    componentConfigs[i].color2.g = EEPROM.read(addr++);
    componentConfigs[i].color2.b = EEPROM.read(addr++);
    componentConfigs[i].blinkInterval = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    componentConfigs[i].twoColor = EEPROM.read(addr++);
    componentConfigs[i].enabled = EEPROM.read(addr++);
    componentConfigs[i].isRandom = EEPROM.read(addr++);
  }
  
  FastLED.setBrightness(mainConfig.brightness);
  memcpy(leds_main_buffer, leds_main, sizeof(leds_main));
  transitionStartTime = millis();
  inTransition = true;
}

// ################### UTILITY FUNCTIONS ###################
void resetToDefaults() {
  // Default main configuration
  mainConfig.pattern = PATTERN_CHASE;
  mainConfig.color1 = CRGB::Red;
  mainConfig.color2 = CRGB::Black;
  mainConfig.speed = 75;
  mainConfig.brightness = 180;
  mainConfig.enabled = true;
  mainConfig.isRandom = false;
  
  // Default eye configurations
  for(int i = 0; i < 3; i++) {
    componentConfigs[i].color1 = CRGB::Yellow;
    componentConfigs[i].color2 = CRGB::Blue;
    componentConfigs[i].blinkInterval = 250;
    componentConfigs[i].twoColor = true;
    componentConfigs[i].enabled = true;
    componentConfigs[i].isRandom = false;
  }
  
  // Default periscope configuration
  componentConfigs[3].color1 = CRGB::Cyan;
  componentConfigs[3].color2 = CRGB::Black;
  componentConfigs[3].blinkInterval = 500;
  componentConfigs[3].twoColor = false;
  componentConfigs[3].enabled = true;
  componentConfigs[3].isRandom = false;
  
  FastLED.setBrightness(mainConfig.brightness);
  saveMainConfig();
  for(int i = 0; i < 4; i++) {
    saveComponentConfig(i);
  }
  
  Serial.println(F("Reset to defaults"));
}

const char* getPatternName(PatternType pattern) {
  switch(pattern) {
    case PATTERN_ORIGINAL: return "original";
    case PATTERN_BLINK: return "blink";
    case PATTERN_FADE: return "fade";
    case PATTERN_RAINBOW: return "rainbow";
    case PATTERN_CHASE: return "chase";
    case PATTERN_SPARKLE: return "sparkle";
    case PATTERN_BREATHE: return "breathe";
    case PATTERN_SOLID: return "solid";
    default: return "unknown";
  }
}

// ################### INFO COMMANDS ###################
void printHelp() {
  Serial.println(F("\n=== Chopper Dome v4.2 Help ==="));
  Serial.println(F("\nBUTTON (D2):"));
  Serial.println(F("  Short press: Cycle modes (0-5)"));
  Serial.println(F("  Long press: Toggle all lights"));
  
  Serial.println(F("\nCOMMANDS:"));
  Serial.println(F("  help     - Show this help"));
  Serial.println(F("  status   - Show current config"));
  Serial.println(F("  patterns - List patterns"));
  Serial.println(F("  colors   - List colors"));
  Serial.println(F("  reset    - Reset to defaults"));
  
  Serial.println(F("\nPRESETS:"));
  Serial.println(F("  save <1-5>    - Save current to preset"));
  Serial.println(F("  load <0-5>    - Load preset (0=default)"));
  Serial.println(F("  startup <0-5> - Set startup mode"));
  
  Serial.println(F("\nMAIN DOME:"));
  Serial.println(F("  main pattern <name>"));
  Serial.println(F("  main color1 <color>"));
  Serial.println(F("  main color2 <color>"));
  Serial.println(F("  main speed <10-5000>"));
  Serial.println(F("  main brightness <1-255>"));
  Serial.println(F("  main random <on/off>"));
  Serial.println(F("  main <on/off>"));
  
  Serial.println(F("\nEYES (1-3):"));
  Serial.println(F("  eye <1-3> color1 <color>"));
  Serial.println(F("  eye <1-3> color2 <color>"));
  Serial.println(F("  eye <1-3> speed <10-5000>"));
  Serial.println(F("  eye <1-3> single/dual"));
  Serial.println(F("  eye <1-3> random <on/off>"));
  Serial.println(F("  eye <1-3> <on/off>"));
  
  Serial.println(F("\nPERISCOPE:"));
  Serial.println(F("  periscope color1 <color>"));
  Serial.println(F("  periscope color2 <color>"));
  Serial.println(F("  periscope speed <10-5000>"));
  Serial.println(F("  periscope single/dual"));
  Serial.println(F("  periscope random <on/off>"));
  Serial.println(F("  periscope <on/off>"));
}

void printColors() {
  Serial.println(F("\nAvailable colors:"));
  for (int i = 0; i < 15; i++) {
    Serial.print(F("  - "));
    char colorName[8];
    strcpy_P(colorName, COLOR_NAMES[i]);
    Serial.println(colorName);
  }
  Serial.println(F("\nAlso: black, off, RGB (e.g. 255,0,0)"));
}

void printPatterns() {
  Serial.println(F("\nAvailable patterns:"));
  Serial.println(F("  original - Classic fill"));
  Serial.println(F("  blink    - Simple blink"));
  Serial.println(F("  fade     - Smooth fade"));
  Serial.println(F("  rainbow  - Moving rainbow"));
  Serial.println(F("  chase    - Single pixel chase"));
  Serial.println(F("  sparkle  - Random sparkles"));
  Serial.println(F("  breathe  - Breathing effect"));
  Serial.println(F("  solid    - Static color"));
}

void printStatus() {
  Serial.println(F("\n=== Current Status ==="));
  Serial.print(F("Mode: "));
  if (currentMode == 0) {
    Serial.println(F("Default"));
  } else {
    Serial.print(F("User "));
    Serial.println(currentMode);
  }
  
  Serial.println(F("\nMain Dome:"));
  Serial.print(F("  Enabled: "));
  Serial.println(mainConfig.enabled ? F("Yes") : F("No"));
  Serial.print(F("  Random: "));
  Serial.println(mainConfig.isRandom ? F("ON") : F("OFF"));
  Serial.print(F("  Pattern: "));
  Serial.println(getPatternName(mainConfig.pattern));
  Serial.print(F("  Speed: "));
  Serial.print(mainConfig.speed);
  Serial.println(F(" ms"));
  Serial.print(F("  Brightness: "));
  Serial.println(mainConfig.brightness);
  
  for(int i = 0; i < 4; i++) {
    Serial.print(F("\n"));
    Serial.print(i < 3 ? F("Eye ") : F("Periscope"));
    if(i < 3) Serial.print(i + 1);
    Serial.println(F(":"));
    Serial.print(F("  Enabled: "));
    Serial.println(componentConfigs[i].enabled ? F("Yes") : F("No"));
    Serial.print(F("  Random: "));
    Serial.println(componentConfigs[i].isRandom ? F("ON") : F("OFF"));
    Serial.print(F("  Mode: "));
    Serial.println(componentConfigs[i].twoColor ? F("Dual") : F("Single"));
    Serial.print(F("  Speed: "));
    Serial.print(componentConfigs[i].blinkInterval);
    Serial.println(F(" ms"));
  }
}

// ################### STRING UTILITIES ###################
char* trimWhitespace(char* str) {
  while(isspace(*str)) str++;
  if(*str == 0) return str;
  char* end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  *(end + 1) = '\0';
  return str;
}

void strToLower(char* str) {
  for(char* p = str; *p; ++p) {
    *p = tolower(*p);
  }
}