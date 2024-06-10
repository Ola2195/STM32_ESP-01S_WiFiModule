# ESP-01S WiFi Module on STM32

## Description
This project demonstrates how to integrate and control an ESP-01S WiFi module using an STM32 microcontroller. The implementation is done in C++ and includes initialization, communication, and handling of the WiFi module using Hayes AT commands.

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

## Required Configuration
Configurations can be set up using CubeMX, an initialization code generation tool provided by STMicroelectronics. Users can configure the peripherals and pin settings graphically in CubeMX, and the tool generates initialization code for the STM32 microcontroller accordingly.

### USART (This project uses the USART3)
- **Peripheral:** USART3
- **Baud Rate:** 115200 baud
- **Word Length:** 8 bits
- **Stop Bits:** 1
- **Parity:** None
- **Mode:** Full duplex (transmit and receive)
- **Over Sampling:** 16
- **One Bit Sampling:** Disabled
- **Global Interrupts:** Enabled

### Timer Configuration (TIM17):
- **Peripheral:** TIM17
- **Prescaler:** 11
- **Counter Period:** 59999 (to achieve an interrupt every 100 ms)
- **Counter Mode:** Up
- **Auto-Reload Preload:** Disabled
- **Global Interrupts:** Enabled

### GPIO Configuration (PC4):
- **Pin:** PC4
- **Mode:** Output
- **Output Type:** Push-pull
- **Pull-up/Pull-down:** None
- **Speed:** High speed

### Example Usage in the Project:
- USART3 is used for communication with the ESP-01S module.
- TIM17 is configured to generate interrupts at a frequency of 10 Hz (every 100 ms) to handle timing operations.
- GPIO pin PC4 is utilized to control the power supply to the ESP-01S module.
- Adding an additional UART2 allows debugging code using the `print_debug` macro.

## Interrupt Handling and Callbacks
Interrupts for UART3 and TIM17 are enabled to handle asynchronous events efficiently. To handle them in the `main.cpp` file, add the lines shown below in the appropriate places.

```cpp
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        module.ESP_Interrupt();
    }
}
```
This callback function is invoked whenever a character is received via UART3. It calls the `ESP_Interrupt()` function of the module object to handle incoming data from the ESP-01S module.

```cpp
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(htim->Instance == TIM17){
        module.ESP_Counter();
    }
}
```
This callback function is executed when the period set for TIM17 elapses, triggering a periodic interrupt. It calls the `ESP_Counter()` function of the module object, likely used for timing-related operations in the project.

```cpp
HAL_TIM_Base_Start_IT(&htim17);
```
This line of code starts TIM17 with interrupts enabled, ensuring that the timer can trigger periodic interrupts as configured.

