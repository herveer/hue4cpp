# Interactive Demo Application - User Guide

## Overview

The **interactive_demo** application is a comprehensive, menu-driven console application that showcases all features of the hue4cpp library. It's designed to be both a practical tool for testing your Hue setup and a learning resource for understanding how to use the library.

## Features

### 🔗 Bridge Connection
- Automatic bridge discovery on your local network
- Interactive authentication flow with link button
- Persistent credential storage (credentials are saved to `hue4cpp_demo_config.txt`)
- Load saved credentials automatically on subsequent runs

### 💡 Light Control
- List all lights with current states and capabilities
- Select individual lights for detailed control
- Control features (when supported by the light):
  - Turn on/off with smooth transitions
  - Adjust brightness (0-100%)
  - Set RGB colors
  - Set color temperature (2000K-6500K)
  - Trigger alert effects (blinking)
  - Refresh light state

### 🔍 Sensor Monitoring
- List all sensors with types and states
- View detailed sensor information
- Supported sensor types:
  - Motion sensors (detect motion)
  - Temperature sensors (°C readings)
  - Light level sensors
  - Button sensors (dimmer switches, tap dials)
  - Camera motion sensors
  - Bell buttons (doorbells)
  - Rotary sensors (dials/knobs)
  - Geolocation sensors
  - Tamper detection sensors

### 📡 Live Activity Monitor
- Real-time event streaming via Server-Sent Events (SSE)
- Monitor all Hue activity as it happens:
  - Light state changes (on/off, brightness, color)
  - Sensor state changes (motion, temperature, buttons)
  - Bridge connection status

## Building

The interactive demo is built automatically when you build the hue4cpp examples:

```bash
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

The executable will be located at:
- Linux/macOS: `build/examples/interactive_demo`
- Windows: `build/examples/Release/interactive_demo.exe` or `build/examples/Debug/interactive_demo.exe`

## Running the Application

### First Run

1. Make sure your Philips Hue bridge is connected to the same network as your computer
2. Run the application:
   ```bash
   ./examples/interactive_demo
   ```
3. From the main menu, select option **1. Connect to Bridge**
4. Choose **2. Discover new bridge**
5. The app will search for bridges on your network
6. Select your bridge from the list
7. When prompted, **press the link button on your physical Hue bridge**
8. Press Enter in the application
9. The app will authenticate and save the credentials

### Subsequent Runs

On subsequent runs, simply:
1. Run the application
2. Select option **1. Connect to Bridge**
3. Choose **1. Load saved connection**
4. The app will automatically load and use your saved credentials

## Usage Examples

### Controlling a Light

1. From the main menu, select **3. Control a Light**
2. Select the light you want to control
3. Use the control menu to:
   - Turn the light on or off
   - Adjust brightness
   - Change colors
   - Set color temperature
   - Trigger alerts

Example workflow:
```
Choose: 3
Select a light: 1
Choose: 1    (Turn On)
Choose: 4    (Set Brightness)
Enter brightness (0-100): 75
Choose: 5    (Set Color)
Enter Red (0-255): 0
Enter Green (0-255): 0
Enter Blue (0-255): 255
```

### Monitoring Live Activity

1. From the main menu, select **6. Live Activity Monitor**
2. The app will start streaming real-time events
3. Try changing lights or triggering sensors using the Hue app or physical buttons
4. Watch the events appear in real-time
5. Press Enter to stop monitoring

Example output:
```
[14:23:15.234] Light State Changed - ID: abc123
    Power: ON
    Brightness: 75%
[14:23:18.567] Sensor State Changed - ID: def456
[14:23:20.123] Light State Changed - ID: abc123
    Power: OFF
```

### Viewing Sensor Details

1. From the main menu, select **4. List All Sensors**
2. Review the list of sensors with their types
3. From the main menu, select **5. View Sensor Details**
4. Select a sensor to view detailed information

Example:
```
Select a sensor: 1

ID: abc123-motion
Type: Motion
Enabled: Yes

Motion State:
  Motion Detected: NO
  Data Valid: Yes
```

## Configuration File

The application saves connection credentials to `hue4cpp_demo_config.txt` in the current directory. This file contains:
- Bridge IP address
- Bridge ID
- Authentication key

**Security Note**: This file contains sensitive credentials. In a production application, you should use OS-specific secure storage (Keychain on macOS, Credential Manager on Windows, Secret Service on Linux).

## Tips

- **Navigation**: Use numeric keys to navigate menus
- **Back button**: Most pages have a "0. Back" option
- **Clear display**: The app automatically clears the screen between pages for better readability
- **Error handling**: If an operation fails, an error message will be displayed
- **Real-time updates**: Use the Live Activity Monitor to verify that your changes are being applied
- **Capability awareness**: The light control menu only shows options supported by your specific light

## Learning from the Code

The interactive demo is designed to be readable and well-documented. Key learning points:

1. **Configuration Management** (lines 108-169): Shows how to save/load credentials
2. **Bridge Discovery** (showConnectPage): Demonstrates the discovery and authentication flow
3. **Light Control** (showLightControlPage): Shows comprehensive light control with capability detection
4. **Sensor Access** (showSensorsPage, showSensorDetailsPage): Demonstrates sensor enumeration and type-specific data access
5. **Event Streaming** (showLiveActivityPage): Shows how to use SSE for real-time monitoring

## Troubleshooting

### "No bridges found"
- Ensure your bridge is powered on and connected to the network
- Verify your computer is on the same network as the bridge
- Try the discovery process again

### "Authentication failed"
- Make sure you pressed the physical link button on the bridge
- The button press must occur within 30 seconds of the authentication attempt
- Try the authentication process again

### "Operation failed" errors
- Check that the light/sensor is still accessible
- Verify the bridge connection is still active
- Try refreshing the light state

### Saved configuration doesn't work
- The bridge IP may have changed (DHCP)
- Delete `hue4cpp_demo_config.txt` and reconnect
- Consider setting a static IP for your bridge

## Next Steps

After exploring the interactive demo, check out these resources:

1. **API Documentation**: See the header files in `include/hue4cpp/` for detailed API documentation
2. **Focused Examples**: Review individual example files for specific features:
   - `basic_control.cpp` - Simple light control
   - `color_control.cpp` - Advanced color operations
   - `state_monitoring.cpp` - SSE event handling
   - `sensor_monitoring.cpp` - Sensor-specific code
3. **Integration**: Use the patterns from the demo in your own applications
4. **ROADMAP.md**: Check the roadmap for upcoming features

## Contributing

If you find issues or have suggestions for improving the interactive demo, please open an issue or submit a pull request on GitHub!
