# ESP-01S WiFi Module on STM32

Handling the ESP-01S WiFi module using an STM32 microcontroller. The implementation is written in C++ and provides a comprehensive set of functionalities to initialize, manage, and communicate with the ESP-01S module over UART. The code is designed to be integrated into an STM32 project using the STM32CubeMX generated setup.

## Features

1. **Initialization and Reset**
   - Functions to initialize and reset the ESP-01S module.
     
2. **WiFi Connectivity**
   - Configuring the ESP-01S module to connect to a specified WiFi network.
   - Handling multiple connection attempts and checking for successful connections.

3. **UART Communication**
   - UART interrupt handling for receiving data from the ESP-01S.
   - Buffer management to handle incoming data and avoid memory leaks.
   - Sending AT commands to the ESP-01S and processing the responses.

4. **Data Processing**
   - Parsing incoming data for specific patterns (e.g., "+IPD" indicating incoming data).
   - Managing buffers to handle multiple data packets efficiently.

## File Structure

- `esp_01s.hpp` - Header file with class definition and function prototypes.
- `esp_01s.cpp` - Main source file implementing the functionalities of the ESP-01S module.
