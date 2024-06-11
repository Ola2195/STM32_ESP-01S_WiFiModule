/*
 * esp_01s.hpp
 *
 *  Created on: May 30, 2024
 *      Author: Ola
 */


#ifndef APPLICATION_USER_ESP01S_H_
#define APPLICATION_USER_ESP01S_H_

#define BACKUP_BUFFOR_SIZE 	15
#define BUFFER_LEN 			700

#include <string.h>
#include <stdio.h>
#include <cstdint>
#include "stdlib.h"


class esp01s {
	public:
		/*
		 * State flags.
		 */
		bool firstLoop;
		bool atStatus;
		bool wifiStatus;
		bool serverStatus;
		bool resetState;

		/*
		 * @brief	The method is used to initialize the ESP-01S WiFi module.
		 */
		esp01s(void);

		/*
		 * @brief	Timer interrupt handling.
		 * 			Responsible for calculating the time since the last message received/sent.
		 * 			If 0.1s has elapsed since the last signal from the WiFi module,
		 * 			the interrupt flag is automatically set and the received data is processed.
		 */
		void ESP_Counter(void);

		/*
		 * @brief	Handling the UART interrupt responsible for receiving data.
		 */
		void ESP_Interrupt(void);

		/*
		 * @brief	Resetting the WiFi module and all related variables.
		 */
		void ESP_Reset();

		/*
		 * @brief	ESP initialization with Hayes AT commands.
		 */
		void AT_Init();

		/*
		 * @brief	WiFi initialization with AT Hayes commands.
		 */
		void WiFi_Init();

	private:
		/*
		 * Variables used to count down the time since the last signal from the WiFi module.
		 */
		volatile uint32_t counter1kHz, prevCounter1kHz;

		/*
		 * Interruption received flag.
		 * If set to 'true', the interpretation of the data received from the module begins.
		 */
		volatile uint8_t esp_request;

		/*
		 * Backup buffer flag.
		 */
		volatile uint8_t esp_double_used;

		/*
		 * Pointer to the character received in the interruption.
		 */
		volatile char *esp_recv_buffer_pointer;

		/*
		 * Data buffers.
		 */
		volatile char esp_ring_buffer[BACKUP_BUFFOR_SIZE][BUFFER_LEN],	// Backup buffers, used when the main buffer is being processed
					  esp_double_used_buffer[BUFFER_LEN];				// Backup buffer used in situations where two IPDs have been written to one buffer

		/*
		 * The data buffer works on the principle of the buffer ring.
		 * For this reason, it is necessary to know which buffer is currently being read and which is being written.
		 */
		volatile uint8_t write_buffer, read_buffer;

		/* Properties needed to UART Callback. */
		uint8_t recog_IPD;
		uint16_t recog_size;
		char IPDstring[6] = "+IPD,";
		bool control;
		int bracet;
		bool sentWait;
		bool first_request;


		/*
		 * @brief	Hayes AT command support.
		 * @param	request		AT Hayes request.
		 * @param	waitTime	The time between sending and waiting for a response.
		 * @param	endl		Whether to send end of line after request
		 * 						0	in WS mode,
		 * 						1	in offline mode, or HTTP server without WS.
		 * @retval	State of success of the request
		 */
		void ESP_ServiceRequest(const char *request, uint16_t waitTime, uint8_t endl);

		/*
		 * @brief	End of reading received data.
		 * @param	status	TODO
		 */
		uint8_t ESP_FinishBufferProcess(int status);

		/*
		 * TODO
		 */
		void ESP_HandleIPDSupport();
};

#endif /* APPLICATION_USER_ESP01S_H_ */
