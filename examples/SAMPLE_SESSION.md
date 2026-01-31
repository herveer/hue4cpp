# Interactive Demo - Sample Session

This file shows what a typical session with the interactive demo looks like.

## Welcome Screen

```
========================================
  HUE4CPP Interactive Demo
  Version 0.1.0
========================================

This is a comprehensive demo application that showcases
all features of the hue4cpp library.

Features:
  - Bridge discovery and authentication
  - Persistent credential storage
  - Complete light control
  - Sensor monitoring
  - Real-time event streaming


Press Enter to continue...
```

## Main Menu

```
================================================================================
                           HUE4CPP - Interactive Demo
================================================================================

Library Version: 0.1.0

Connected to: Philips hue (192.168.1.10)

--------------------------------------------------------------------------------
Main Menu:

  1. Connect to Bridge
  2. List All Lights
  3. Control a Light
  4. List All Sensors
  5. View Sensor Details
  6. Live Activity Monitor
  0. Exit
--------------------------------------------------------------------------------

Choose an option: 
```

## Bridge Connection (First Time)

```
================================================================================
                              Connect to Bridge
================================================================================

1. Load saved connection
2. Discover new bridge
0. Back to main menu

Choose: 2

Discovering bridges...

Found 1 bridge(s):
  1. Philips hue (192.168.1.10)

Select bridge (1-1): 1

Authentication required.
Please press the LINK BUTTON on your bridge now...

Press Enter to continue...

Authenticating...
✓ Authentication successful!
✓ Configuration saved to: hue4cpp_demo_config.txt

Press Enter to continue...
```

## List All Lights

```
================================================================================
                                  All Lights
================================================================================

Fetching lights...

Found 5 light(s):

1. Living Room Ceiling
   ID: abc123-light-1
   State: ON
   Brightness: 75%
   Capabilities: On/Off, Brightness, Color, Color Temp

2. Bedroom Light
   ID: abc123-light-2
   State: OFF
   Capabilities: On/Off, Brightness, Color Temp

3. Kitchen Strip
   ID: abc123-light-3
   State: ON
   Brightness: 100%
   Capabilities: On/Off, Brightness, Color

4. Desk Lamp
   ID: abc123-light-4
   State: ON
   Brightness: 50%
   Capabilities: On/Off, Brightness

5. Bathroom Light
   ID: abc123-light-5
   State: OFF
   Capabilities: On/Off, Color Temp

Press Enter to continue...
```

## Control a Light

```
================================================================================
                      Controlling: Living Room Ceiling
================================================================================

Current State:
  Power: ON
  Brightness: 75%
  Color (XY): x=0.3127, y=0.3290
  Color Temp: 2700K

--------------------------------------------------------------------------------
Controls:
  1. Turn On
  2. Turn Off
  3. Toggle
  4. Set Brightness
  5. Set Color (RGB)
  6. Set Color Temperature
  7. Alert (Blink)
  8. Refresh State
  0. Back to Main Menu
--------------------------------------------------------------------------------

Choose: 5

Enter Red (0-255): 0
Enter Green (0-255): 0
Enter Blue (0-255): 255
✓ Color set to RGB(0,0,255)

Press Enter to continue...
```

## List All Sensors

```
================================================================================
                                 All Sensors
================================================================================

Fetching sensors...

Found 8 sensor(s):

1. Sensor
   ID: sensor-motion-1
   Type: Motion
   Enabled: Yes

2. Sensor
   ID: sensor-temp-1
   Type: Temperature
   Enabled: Yes

3. Sensor
   ID: sensor-light-1
   Type: Light Level
   Enabled: Yes

4. Sensor
   ID: sensor-button-1
   Type: Button
   Enabled: Yes

5. Sensor
   ID: sensor-motion-2
   Type: Motion
   Enabled: Yes

6. Sensor
   ID: sensor-temp-2
   Type: Temperature
   Enabled: Yes

7. Sensor
   ID: sensor-light-2
   Type: Light Level
   Enabled: Yes

8. Sensor
   ID: sensor-button-2
   Type: Button
   Enabled: Yes

Press Enter to continue...
```

## View Sensor Details

```
================================================================================
                              Sensor Details
================================================================================

Select a sensor:
  1. Motion (sensor-motion-1)
  2. Temperature (sensor-temp-1)
  3. Light Level (sensor-light-1)
  4. Button (sensor-button-1)
  5. Motion (sensor-motion-2)
  6. Temperature (sensor-temp-2)
  7. Light Level (sensor-light-2)
  8. Button (sensor-button-2)
  0. Back

Choose: 2

================================================================================
                              Sensor Details
================================================================================

ID: sensor-temp-1
Type: Temperature
Enabled: Yes

Temperature State:
  Temperature: 21.50 °C
  Data Valid: Yes

Press Enter to continue...
```

## Live Activity Monitor

```
================================================================================
                           Live Activity Monitor
================================================================================

Starting real-time event monitoring...
Try controlling lights or triggering sensors via the Hue app.

Press Enter to stop monitoring...
--------------------------------------------------------------------------------

[14:23:15.234] Light State Changed - ID: abc123-light-1
    Power: OFF
[14:23:18.567] Sensor State Changed - ID: sensor-motion-1
[14:23:20.123] Light State Changed - ID: abc123-light-3
    Power: ON
    Brightness: 50.0%
[14:23:25.890] Light State Changed - ID: abc123-light-1
    Power: ON
    Brightness: 100.0%
[14:23:28.456] Sensor State Changed - ID: sensor-button-1

Monitoring stopped.

Press Enter to continue...
```

## Exit

```
================================================================================
                                   Goodbye!
================================================================================

Thank you for using hue4cpp!

```

---

## Notes

- All screen transitions are smooth with automatic screen clearing
- Error messages are displayed with ✗ prefix for visibility
- Success messages use ✓ prefix
- The interface is designed to be intuitive and self-explanatory
- Navigation is always numeric, making it easy to use
- Each page has a consistent header format for professional appearance
