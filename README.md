# ESP32 Chopper Dome Logics v5.2
**Advanced ESP32-C3 Mini controller for Star Wars Chopper droid dome and eye lighting**

## 🤖 Project Overview

This enhanced controller brings your Chopper droid's dome and eye lighting to life with smooth animations, customizable patterns, and authentic "Chopper" eye animations. Designed for builders who demand professional results with maximum flexibility.

### Key Features

- **🎨 Main Dome Control** - 19 WS2812B LEDs with 10 different animation patterns
- **👁️ Triple Eye System** - Three 7-LED eye arrays with independent control
- **🔭 Periscope LED** - Single LED with synchronized or independent control
- **✨ Authentic Chopper Mode** - Movie-accurate alternating blue eyes with solid yellow third eye
- **💾 Settings Persistence** - All configurations automatically saved and restored after restart
- **🎲 Random Mode** - Automatic pattern, color, and speed randomization per component
- **🎨 Color Palette System** - 5-color palettes for dynamic multi-color animations
- **🌈 15 Predefined Colors** - Plus custom RGB support (0-255,0-255,0-255)
- **🔘 Physical Button Control** - Short press cycles presets, long press toggles power
- **💫 Smooth Transitions** - Crossfade effects between pattern changes
- **📝 Serial Command Interface** - Real-time configuration via serial monitor
- **🔧 5 User Presets** - Save and recall complete lighting states
- **⚙️ Configurable LED Counts** - Easy adjustment at sketch top

---

## 📝 Changelog

### Version 5.2 (2025-07)

**Settings Persistence, Random Mode & Button Control**

This major update introduces persistent storage, randomization features, and physical button control with 5 pre-configured user presets.

#### 🎯 New Features

**1. Settings Persistence**
- All configurations automatically saved using ESP32 Preferences library
- Settings survive power cycles and restarts
- No manual save commands required
- Independent storage for each configuration parameter

**2. Random Mode**
- Each component (Main Dome, Eyes 1-3, Periscope) can be set to random mode
- Automatically changes patterns, colors, and speeds every 15 seconds
- Serial feedback shows what was randomized
- Enable/disable per component: `main random on`, `eye 1 random on`, etc.

**3. Physical Button Control (IO0)**
- **Short Press (<3s)**: Cycle through modes
  - Mode 0: Chopper Default (unchangeable authentic look)
  - Modes 1-5: User Presets (fully customizable)
- **Long Press (>3s)**: Toggle all lights ON/OFF
- Mode cycles: Default → User 1 → User 2 → User 3 → User 4 → User 5 → Default

**4. User Preset System**
- 5 fully customizable user presets (User 1 to User 5)
- Save current configuration: `save <1-5>`
- Load saved preset: `load <1-5>`
- Set startup mode: `startup <0-5>` (0=Default, 1-5=User)
- Restore factory presets: `reset presets`

**5. Pre-Configured Default Presets**
- **User 1 (Agitated/Working)**: Fast sparkle dome, dual-color blinking eyes
- **User 2 (Angry/Attack)**: Red breathing dome, red eyes/periscope
- **User 3 (Happy/Celebratory)**: Rainbow dome, cyan/yellow eyes
- **User 4 (Stealth/Undercover)**: Dim navy fade, slow blue eyes
- **User 5 (Chopper Eyes + Palette)**: Authentic eyes with warm palette dome

**6. Chopper Eye Mode**
- Authentic "Chopper" animation: alternating blue on Eyes 1 & 2
- Solid yellow on Eye 3
- Enable: `main eyemode chopper`
- Disable: `main eyemode default`
- 400ms blink interval for movie accuracy

**7. Layered Patterns & Color Palettes**
- **Layer Pattern**: Combines breathe effect with sparkle overlay
- **Palette Pattern**: Smooth cycling through 5 custom colors
- Configure palette: `main palette <1-5> <color>`
- Toggle palette mode for chase/sparkle: `main palettemode on`

**8. Enhanced Serial Interface**
- Help system: `help` command shows all available commands
- Status display: `status` shows complete configuration
- Color list: `colors` shows 15 predefined color names
- Pattern list: `patterns` shows all 10 available patterns
- Reset command: `reset` returns to firmware defaults

#### 🔧 Hardware Configuration

- **Main Dome**: 19 LEDs on IO5 (WS2812B)
- **Eye 1**: 7 LEDs on IO6 (WS2812B)
- **Eye 2**: 7 LEDs on IO7 (WS2812B)
- **Eye 3**: 7 LEDs on IO10 (WS2812B)
- **Periscope**: 1 LED on IO4 (WS2812B)
- **Button**: IO0 (internal pullup, active LOW)

#### ⚠️ Important Notes

- **ESP32-C3 has only 2 RMT channels!**
- This version uses bit-banging for additional strips to avoid RMT conflicts
- **USE FASTLED LIBRARY 3.9.0!**
- ESP32-C3 Mini board selection: "Lolin C3 Mini"

---

## 🔧 Hardware Requirements

### Core Components
- **ESP32-C3 Mini** development board (Lolin C3 Mini or compatible)
- **5x WS2812B LED Strips/Rings:**
  - 19 LEDs for Main Dome
  - 3x 7 LEDs for Eyes
  - 1 LED for Periscope
- **Momentary Push Button** - NO (Normally Open) for mode control
- **5V Power Supply** - Adequate current for all LEDs (minimum 2-3A recommended)
  - Calculation: (19 + 7 + 7 + 7 + 1) × 60mA = ~2.5A maximum
- **Level Shifter** (optional but recommended) - 3.3V to 5V for data lines

### Recommended Assembly
- **Printed Droid Chopper Dome Kit** - Professional dome and electronics carrier
- **Wire Management** - Proper routing for 5 independent LED strips
- **Power Distribution** - Common 5V bus with adequate gauge wire

---

## 📋 Pin Configuration

ESP32-C3 Mini Pin Assignments:

### LED Control Pins
- **Main Dome (19 LEDs):**
  - DATA: GPIO5

- **Eye 1 (7 LEDs):**
  - DATA: GPIO6

- **Eye 2 (7 LEDs):**
  - DATA: GPIO7

- **Eye 3 (7 LEDs):**
  - DATA: GPIO10

- **Periscope (1 LED):**
  - DATA: GPIO4

### Input Pin
- **Mode Button:**
  - PIN: GPIO0 (IO0)
  - Configuration: INPUT_PULLUP (active LOW)
  - Short press: Cycle modes
  - Long press (>3s): Toggle power

---

## 🚀 Installation

### Method 1: Arduino IDE Setup

1. **Install ESP32 board support:**
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Board Manager → Search "ESP32" → Install

2. **Board Configuration for ESP32-C3:**
   - Board: "ESP32C3 Dev Module" or "Lolin C3 Mini"
   - Upload Speed: "921600"
   - USB CDC On Boot: "Enabled"
   - Flash Size: "4MB"
   - Partition Scheme: "Default 4MB with spiffs"

3. **Required Libraries:**
   Install via Arduino Library Manager:
   - **FastLED** - Version 3.9.0 (IMPORTANT: Do not use newer versions due to ESP32-C3 RMT limitations)

4. **Upload the Sketch:**
   - Open `Chopper_ladderlights_ESP32C3Mini_v5.2.ino`
   - Select correct COM port
   - Click Upload
   - Open Serial Monitor at **115200 baud**

### Method 2: Upload Pre-Compiled Binary

1. Download latest `.bin` file from releases
2. Use ESP Flash Download Tool or esptool.py
3. Flash at address 0x0
4. Example esptool command:
   ```bash
   esptool.py --chip esp32c3 --port /dev/ttyUSB0 write_flash -z 0x0 Chopper_v5.2.bin
   ```

---

## 🎮 Physical Button Controls

The controller features a physical button on **GPIO0 (IO0)** with two functions:

### Short Press (<3 seconds)
Cycles through all 6 modes sequentially:

| Mode | Name | Description |
| :--- | :--- | :--- |
| 0 | **Chopper Default** | Unchangeable authentic Chopper look (red chase dome, alternating blue eyes, yellow third eye, cyan periscope) |
| 1 | **User Preset 1** | Agitated/Working mode (customizable) |
| 2 | **User Preset 2** | Angry/Attack mode (customizable) |
| 3 | **User Preset 3** | Happy/Celebratory mode (customizable) |
| 4 | **User Preset 4** | Stealth/Undercover mode (customizable) |
| 5 | **User Preset 5** | Chopper Eyes + Palette (customizable) |

After mode 5, cycles back to mode 0.

### Long Press (>3 seconds)
Toggles ALL lights (dome + eyes + periscope) ON or OFF simultaneously.

**Serial Feedback:**
```
Button SHORT PRESS: Loading User Preset 3
Button LONG PRESS: Lights toggled ON
```

---

## 4. Serial Commands (Runtime Configuration)

The controller provides a comprehensive serial command interface at **115200 baud**. All commands are case-insensitive.

**Format:** `[component] [subcommand] [value]`

### 4.1. Utility Commands

| Command | Function |
| :--- | :--- |
| `help` | Shows complete command reference with examples |
| `status` | Displays current configuration of all components |
| `patterns` | Lists all 10 available main dome patterns |
| `colors` | Shows 15 predefined color names |
| `reset` | Reset all settings to firmware defaults (except presets) |

### 4.2. Mode & Preset Commands

| Command | Function | Example |
| :--- | :--- | :--- |
| `load default` | Activate unchangeable Chopper Default look | `load default` |
| `load <1-5>` | Load a custom User Preset | `load 3` |
| `save <1-5>` | Save current config to User Preset slot | `save 2` |
| `startup <0-5>` | Set mode to load on boot (0=Default, 1-5=User) | `startup 1` |
| `reset presets` | Restore the 5 factory default User Presets | `reset presets` |

**Note:** You cannot save over the Chopper Default (mode 0) - it is read-only.

### 4.3. Main Dome Commands

All main dome commands start with `main`:

| Command | Function | Valid Values | Example |
| :--- | :--- | :--- | :--- |
| `main pattern <name>` | Set animation pattern | See section 4.4 | `main pattern rainbow` |
| `main eyemode <mode>` | Set eye animation mode | `chopper`, `default` | `main eyemode chopper` |
| `main color1 <color>` | Set primary color | Color name or RGB | `main color1 blue` |
| `main color2 <color>` | Set secondary/layer color | Color name or RGB | `main color2 255,0,0` |
| `main palette <1-5> <color>` | Set palette color slot | Slot 1-5, color | `main palette 3 purple` |
| `main palettemode <state>` | Use palette for chase/sparkle | `on`, `off` | `main palettemode on` |
| `main speed <ms>` | Set animation speed | 10-5000 ms | `main speed 100` |
| `main brightness <val>` | Set master brightness | 1-255 | `main brightness 180` |
| `main random <state>` | Enable/disable random mode | `on`, `off` | `main random on` |
| `main on` / `main off` | Enable/disable main dome LEDs | - | `main on` |

**Examples:**
```
main pattern chase
main color1 red
main speed 75
main brightness 200
main eyemode chopper
main palette 1 orange
main palette 2 yellow
main palettemode on
main random on
```

### 4.4. Available Patterns

| Pattern | Description |
| :--- | :--- |
| `original` | Classic LED-by-LED fill effect with color alternation |
| `blink` | Simple blinking between color1 and color2 |
| `fade` | Smooth fade in and out of color1 |
| `rainbow` | Moving rainbow effect across the strip |
| `chase` | Single pixel chasing on background of color2 |
| `sparkle` | Random pixels sparkling on background of color2 |
| `breathe` | Smooth pulsing "breathing" effect with color1 |
| `solid` | Solid, non-animated display of color1 |
| `layer` | Breathe effect (color1) with sparkle overlay (color2) |
| `palette` | Smoothly cycles through the 5 palette colors |

**Palette Mode:**
When `palettemode on` is set, the `chase` and `sparkle` patterns will randomly select colors from your 5-color palette instead of using color1.

### 4.5. Eye Commands (1-3)

Control individual eyes with `eye <1-3> <subcommand>`:

| Command | Function | Valid Values | Example |
| :--- | :--- | :--- | :--- |
| `eye <1-3> color1 <color>` | Set primary color | Color name or RGB | `eye 1 color1 blue` |
| `eye <1-3> color2 <color>` | Set secondary color | Color name or RGB | `eye 2 color2 yellow` |
| `eye <1-3> speed <ms>` | Set blink speed | 10-5000 ms | `eye 3 speed 250` |
| `eye <1-3> single` | Set single color blinking mode | - | `eye 1 single` |
| `eye <1-3> dual` | Set dual color blinking mode | - | `eye 2 dual` |
| `eye <1-3> random <state>` | Enable/disable random mode | `on`, `off` | `eye 1 random on` |
| `eye <1-3> on` / `off` | Enable/disable eye LEDs | - | `eye 3 off` |

**Note:** When `main eyemode chopper` is active, eye color/speed commands are ignored and the authentic Chopper animation runs instead.

**Examples:**
```
eye 1 color1 yellow
eye 1 color2 blue
eye 1 dual
eye 1 speed 150
eye 2 random on
```

### 4.6. Periscope Commands

Control the periscope LED with `periscope <subcommand>`:

| Command | Function | Valid Values | Example |
| :--- | :--- | :--- | :--- |
| `periscope color1 <color>` | Set primary color | Color name or RGB | `periscope color1 cyan` |
| `periscope color2 <color>` | Set secondary color | Color name or RGB | `periscope color2 white` |
| `periscope speed <ms>` | Set blink speed | 10-5000 ms | `periscope speed 500` |
| `periscope single` | Set single color blinking mode | - | `periscope single` |
| `periscope dual` | Set dual color blinking mode | - | `periscope dual` |
| `periscope random <state>` | Enable/disable random mode | `on`, `off` | `periscope random on` |
| `periscope on` / `off` | Enable/disable periscope LED | - | `periscope on` |

**Examples:**
```
periscope color1 cyan
periscope speed 1000
periscope single
```

### 4.7. Color Formats

Colors can be specified in three ways:

**1. Predefined Color Names (15 colors):**
```
red, green, blue, yellow, orange, purple, cyan, white,
pink, lime, aqua, magenta, navy, maroon, olive
```

**2. RGB Values:**
```
255,0,0      → Pure red
0,255,0      → Pure green
128,0,255    → Purple
```

**3. Special Keywords:**
```
black, off   → Turn LEDs off (CRGB::Black)
```

**Examples:**
```
main color1 red
main color2 128,64,255
eye 1 color1 off
periscope color1 cyan
```

---

## 🎨 Pre-Configured User Presets

The controller ships with 5 factory default user presets demonstrating different "moods" and use cases. These can be customized and overwritten.

### User Preset 1: Agitated / Working
**Theme:** Busy, active state

| Component | Setting | Value |
| :--- | :--- | :--- |
| Main Dome | Pattern | Sparkle |
| | Color 1 | White |
| | Color 2 | Blue |
| | Speed | 80 ms |
| | Brightness | 200 |
| Eye 1 & 2 | Color 1 | Yellow |
| | Color 2 | Blue |
| | Mode | Dual |
| | Speed | 150 ms |
| Eye 3 | Color 1 | Orange |
| | Mode | Single |
| Periscope | Color 1 | White |
| | Speed | 300 ms |

### User Preset 2: Angry / Attack Mode
**Theme:** Aggressive, menacing state

| Component | Setting | Value |
| :--- | :--- | :--- |
| Main Dome | Pattern | Breathe |
| | Color 1 | Red |
| | Speed | 50 ms |
| | Brightness | 255 |
| All Eyes | Color 1 | Red |
| | Speed | 200 ms |
| Periscope | Color 1 | Red |

### User Preset 3: Happy / Celebratory
**Theme:** Bright, colorful state

| Component | Setting | Value |
| :--- | :--- | :--- |
| Main Dome | Pattern | Rainbow |
| | Speed | 50 ms |
| | Brightness | 220 |
| Eye 1 & 2 | Color 1 | Cyan |
| | Color 2 | Yellow |
| | Mode | Dual |
| | Speed | 250 ms |
| Eye 3 | Color 1 | Lime |
| Periscope | Color 1 | Magenta |

### User Preset 4: Stealth / Undercover
**Theme:** Dim, minimal visibility

| Component | Setting | Value |
| :--- | :--- | :--- |
| Main Dome | Pattern | Fade |
| | Color 1 | Navy |
| | Speed | 500 ms |
| | Brightness | 80 |
| Eye 1 & 2 | Color 1 | Blue |
| | Speed | 1500 ms |
| Eye 3 | Disabled | - |
| Periscope | Disabled | - |

### User Preset 5: Chopper Eyes + Palette Dome
**Theme:** Authentic Chopper animation with warm dome

| Component | Setting | Value |
| :--- | :--- | :--- |
| Main Dome | Pattern | Palette |
| | Eye Mode | Chopper |
| | Speed | 200 ms |
| | Brightness | 200 |
| | Palette 1 | Orange |
| | Palette 2 | Yellow |
| | Palette 3 | White |
| | Palette 4 | Dark Orange |
| | Palette 5 | Bisque |
| Eyes 1-3 | Controlled by Chopper mode | Alternating blue + solid yellow |
| Periscope | Color 1 | Cyan |
| | Speed | 1000 ms |

---

## 🛠️ Troubleshooting

### Common Issues Quick Reference

| Problem | Quick Fix |
|---------|-----------|
| No serial output | Check baud rate (115200), verify USB connection |
| LEDs not working | Verify WS2812B wiring, check power supply (5V), add level shifter |
| Some strips don't light | ESP32-C3 uses bit-banging - check FastLED version (must be 3.9.0) |
| Random crashes | Check power supply stability, verify adequate current rating |
| Settings not saved | Wait for serial confirmation, don't power off immediately |
| Button not responding | Check IO0 connection, verify pullup resistor (internal enabled) |

### Hardware Issues

**LEDs Not Working:**
- ✅ Check WS2812B data line wiring (correct GPIO pins)
- ✅ Verify power supply (5V, adequate current - minimum 2-3A)
- ✅ Add 3.3V to 5V level shifter on data lines
- ✅ Check ground connection between ESP32 and LED power supply
- ✅ Test with simple FastLED blink sketch first

**Some Strips Work, Others Don't:**
- ✅ Verify FastLED library version is **exactly 3.9.0**
- ✅ ESP32-C3 has only 2 RMT channels - newer FastLED versions may fail
- ✅ Check individual strip power connections
- ✅ Verify correct GPIO pins in code (lines 63-67)

**Button Not Responding:**
- ✅ Verify button connected between IO0 and GND
- ✅ Use normally-open (NO) momentary switch
- ✅ Internal pullup is enabled - no external resistor needed
- ✅ Check for loose connections

### Software Issues

**Configuration Problems:**
```
status              ← Verify all settings
reset               ← Reset to firmware defaults
reset presets       ← Restore factory presets
load default        ← Load Chopper Default mode
```

**Serial Communication Issues:**
- ✅ Set baud rate to **115200** (not 9600!)
- ✅ Select correct COM port
- ✅ Check USB cable quality (data cable, not charge-only)
- ✅ Press EN/RST button if ESP32 doesn't respond
- ✅ Enable "USB CDC On Boot" in Arduino IDE board settings

**Settings Not Persisting:**
- ✅ Wait for serial confirmation after save commands
- ✅ Don't power off immediately after saving
- ✅ Check ESP32 flash is not full (unlikely with 4MB)
- ✅ Try `reset` command to clear corrupted preferences

---

## 📊 System Information

### Status Display

Type `status` in the serial monitor to see complete configuration:

```
=== Current System Configuration ===
  ACTIVE MODE: User Preset 3

Main Dome & Global:
  Enabled: Yes
  Random Mode: OFF
  Eye Mode: Default
  Pattern: rainbow
  Speed: 50 ms
  Brightness: 220
  Color 1 (RGB): 255,255,255
  Color 2 (RGB): 0,0,0
  Palette Mode: OFF
    Palette 1: 0,0,255
    ...

Eye 1:
  Enabled: Yes
  Random Mode: OFF
  Mode: Dual Color
  Speed: 250 ms
  Color 1 (RGB): 0,255,255
  Color 2 (RGB): 255,255,0
...
```

### Memory Usage
- **Flash Program Size**: ~50-60 KB
- **SRAM (Global Variables)**: ~8 KB
- **Preferences (Flash)**: ~2 KB per preset
- **Total Flash**: 4MB available, <1% used

### Performance Metrics
- **LED Update Rate**: ~60 FPS (limited by FastLED.show())
- **Serial Command Latency**: <5ms
- **Button Debounce**: None (handled by press duration)
- **Random Mode Interval**: 15 seconds per component

---

## 🎯 Advanced Configuration

### Customizing LED Counts

To adjust LED counts (e.g., different ring sizes), edit lines 53-55:

```cpp
#define NUM_LEDS_MAIN       19  // Change to your main dome LED count
#define NUM_LEDS_EYE        7   // Change to your eye LED count
#define NUM_LEDS_PERISCOPE  1   // Change to your periscope LED count
```

**Note:** After changing these values, you must recompile and upload the sketch.

### Customizing Timing Constants

Edit lines 70-72 to adjust behavior:

```cpp
#define RANDOM_INTERVAL 15000      // Time between random changes (ms)
#define TRANSITION_TIME 250        // Crossfade duration (ms)
#define CHOPPER_EYE_INTERVAL 400   // Chopper eye blink speed (ms)
```

### Customizing Default Presets

The factory default presets are defined in the `factoryResetPresets()` function (lines 309-420). You can customize these before uploading to create your own "default" presets.

---

## 🚀 Future Expansion

### Planned Features
- Wi-Fi control via web interface
- Integration with other droid control systems
- Sound-reactive patterns
- Mobile app for preset management
- OTA (Over-The-Air) firmware updates

### Community Contributions
This is an open-source project. Contributions are welcome:
- Additional animation patterns
- New color palettes
- Hardware integration guides
- Bug fixes and optimizations

---

## 📞 Support

### Getting Help

1. **Check this README** for common solutions
2. **Review Serial Monitor** output (115200 baud) for error messages
3. **Test with factory presets** to verify hardware
4. **Use diagnostics commands**: `status`, `patterns`, `colors`

### Reporting Issues

When reporting problems, please include:
- ESP32-C3 board model
- FastLED library version (must be 3.9.0)
- Power supply specifications
- LED strip details (WS2812B type, LED count)
- Serial monitor output (115200 baud)
- Steps to reproduce the issue

---

## 📜 License & Credits

### Project Credits
- **Software Development**: Printed-Droid.com
- **Hardware Platform**: ESP32-C3 Mini (Espressif Systems)
- **LED Technology**: WS2812B addressable LEDs
- **Inspired by**: Star Wars Rebels - C1-10P "Chopper" Droid

### Open Source Libraries
- **FastLED 3.9.0**: High-performance LED control library
- **ESP32 Preferences**: EEPROM emulation for settings persistence
- **ESP32 Arduino Core**: ESP32-C3 support

### Special Thanks
- Star Wars Rebels creators for the amazing Chopper character
- R2-D2 Builders Club community
- ESP32 and FastLED development communities

### Disclaimer

⚠️ **IMPORTANT SAFETY NOTICE** ⚠️

This project involves electrical components and LED displays. Users are responsible for:

- Proper electrical safety and insulation
- Adequate power supply sizing and protection (minimum 2-3A for all LEDs)
- Safe assembly and operation
- Compliance with local electrical codes
- Testing all functions before final installation
- Proper heat dissipation (WS2812B LEDs can get warm at high brightness)

**BUILD AT YOUR OWN RISK.** Ensure proper knowledge of electronics and safety practices. The authors assume no responsibility for damage, injury, or malfunction resulting from use of this design.

**Power Supply Guidelines:**
- Use regulated 5V supply with adequate current rating
- Connect power directly to LED strips (not through ESP32)
- Use thick gauge wire for power distribution (minimum 22 AWG)
- Add bulk capacitor (1000µF) near ESP32 for stability
- Add 330Ω resistor on each data line (between ESP32 and first LED)

---

## 📸 Example Configurations

### Movie-Accurate Chopper Default
```
load default
```
- Red chase pattern on dome
- Alternating blue eyes (Eyes 1 & 2)
- Solid yellow third eye (Eye 3)
- Cyan blinking periscope

### Colorful Party Mode
```
load 3
```
or via serial commands:
```
main pattern rainbow
main brightness 220
eye 1 color1 cyan
eye 1 color2 yellow
eye 1 dual
eye 2 color1 cyan
eye 2 color2 yellow
eye 2 dual
periscope color1 magenta
save 3
```

### Stealth Mode
```
load 4
```
or via serial commands:
```
main pattern fade
main color1 navy
main brightness 80
main speed 500
eye 1 color1 blue
eye 1 speed 1500
eye 2 color1 blue
eye 2 speed 1500
eye 3 off
periscope off
save 4
```

---

**May the Force be with your build!** 🌟

*For the latest updates and community support, visit: [www.printed-droid.com](https://www.printed-droid.com)*

**Version**: v5.2
**Last Updated**: July 2025
**Compatible Hardware**: ESP32-C3 Mini + WS2812B LED Strips
**Required Library**: FastLED 3.9.0
