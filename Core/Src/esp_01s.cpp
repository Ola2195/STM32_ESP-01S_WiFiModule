/*
 * esp_01s.cpp
 *
 *  Created on: May 30, 2024
 *      Author: Ola
 */


#include "esp_01s.hpp"

#include "main.h"
#include "stm32f3xx_hal.h"


extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart2;



#define PRINTF_LEN 1024
static char buf_printf[PRINTF_LEN];

// Sends to console
#define print_debug(f_, ...) snprintf(buf_printf, PRINTF_LEN, (f_), ##__VA_ARGS__); \
HAL_UART_Transmit(&huart2, (uint8_t*)buf_printf, strlen(buf_printf), 1000);


// Sends to WiFi and to console
#define print_wifi(f_, ...) snprintf(buf_printf, PRINTF_LEN-6, (f_), ##__VA_ARGS__); \
HAL_UART_Transmit(&huart3, (uint8_t *)buf_printf, strlen(buf_printf), 1000);



#define BR 1
#define NOBR 0
#define RELAY_TIME 200

#define RESPONSE_OUTOFTIME 0
#define RESPONSE_INTIME 1
#define HANDLE 2

// IPD definitely takes 8 places, plus size, after ':' are trash
#define MAX_SIZE_OF_TRASH 16

// Maximum time to wait for a callback [ms]
#define  MAX_CALLBACK_TIME 7000

#define IPDFOUND_SIZE 		100

#define SSID 			"Xiaomi 11 Lite 5G NE"
#define PASSWORD		"LubiePlacki"

#define ESP_PIN			GPIO_PIN_4
#define ESP_PIN_PORT	GPIOC




esp01s::esp01s(void) :
		counter1kHz(0), prevCounter1kHz(0), write_buffer(0), read_buffer(0), esp_double_used(0),
		first_request(0), esp_request(0), bracet(0) {
}

void esp01s::ESP_Counter(void) {
	counter1kHz++;
	if ((counter1kHz - prevCounter1kHz) == 10 && prevCounter1kHz != 0) {
		if(strlen((char*)esp_ring_buffer[write_buffer]) > 0) {
			// Reset search variables +IPD,c,r: to search again
			recog_IPD = 0;
			recog_size = 0;
			bracet = 0;
			if(!control
					&& ((write_buffer == BACKUP_BUFFOR_SIZE-1 && read_buffer!=0)
					|| (write_buffer<BACKUP_BUFFOR_SIZE-1 && write_buffer+1 != read_buffer))) {
				esp_request++;
				control = true;
			}
		}
	}
}


void esp01s::ESP_Interrupt(void) {
	if(!resetState) {
		int esp_request = 0;
		if(*esp_recv_buffer_pointer != '\0') {
			if(control) {
				if(write_buffer == BACKUP_BUFFOR_SIZE-1)
					write_buffer = 0;
				else
					write_buffer++;
				esp_ring_buffer[write_buffer][0] = *esp_recv_buffer_pointer;
				*esp_recv_buffer_pointer = '\0';
				esp_recv_buffer_pointer = esp_ring_buffer[write_buffer];
				control = false;
			}

			/*
			 * IPD handling
			 */

			// We load the data once its size is found
			if (recog_IPD == IPDFOUND_SIZE) {
				recog_size--;
				// End of data loading
				if (recog_size < 1) {
					// Reset search variables +IPD,c,r: to search again
					recog_IPD = 0;
					recog_size = 0;
					bracet = 0;
					if(!control && ((write_buffer == BACKUP_BUFFOR_SIZE-1 && read_buffer!=0)
							|| (write_buffer<BACKUP_BUFFOR_SIZE-1 && write_buffer+1 != read_buffer))) {
						esp_request++;
						control = true;
					}
				}
			}
			// Recognise the "+IPD," sequence
			else if (recog_IPD < 4 && *esp_recv_buffer_pointer == IPDstring[recog_IPD]) {
				recog_IPD++;
			}
			else {
				if (recog_IPD == 4 || recog_IPD == 5 || recog_IPD == 6) {
					recog_IPD++;	  		// The 4th and 5th characters are ‘,’ and the channel number 0-4 (1 character)
				}
				// The size starts with the 6th character and ends with ":"
				else if (recog_IPD > 6 && *esp_recv_buffer_pointer != ':') {
					// ASCII cyfry sprawdz czy OK
					if (*esp_recv_buffer_pointer >= 48	&& *esp_recv_buffer_pointer <= 57) {	// ASCII digits
						recog_IPD++;
						recog_size *= 10; 		// replace with INT
						recog_size += *esp_recv_buffer_pointer - 48;
					} else {
						recog_IPD = 0;
						recog_size = 0;
					}
				}
				else if (recog_IPD > 6 && *esp_recv_buffer_pointer == ':' && recog_size!=0 && recog_IPD != IPDFOUND_SIZE) {
					recog_IPD = IPDFOUND_SIZE;
				}
				// Discontinue the search for IPD
				else if (recog_IPD > 0 && recog_IPD < 4 && *esp_recv_buffer_pointer != IPDstring[recog_IPD]) {
					recog_IPD = 0;
					recog_size = 0;
				}
			}

			if(*esp_recv_buffer_pointer != '\0') {
				esp_recv_buffer_pointer++;
				*esp_recv_buffer_pointer = '\0';
			}

			prevCounter1kHz = counter1kHz; 		// For counting the time since the previous mark
		}
		esp_request += esp_request;

		// Error Memory leak
		if (esp_recv_buffer_pointer - esp_ring_buffer[write_buffer] >= BUFFER_LEN) {
			print_debug("Memory leak buffer too short - HAL_UART_RxCpltCallback interrupt!\r\nReadBuffer: %i\r\nWriteBuffer: %i, %s\r\n", read_buffer, write_buffer, esp_ring_buffer[write_buffer]);
			ESP_Reset();
		} else {
			HAL_UART_Receive_IT(&huart3, (uint8_t*) esp_recv_buffer_pointer, 1);
		}
	}
}

void esp01s::ESP_Reset() {
	// Cut off the power
//	HAL_GPIO_WritePin(ESP_PIN_PORT, ESP_PIN, GPIO_PIN_RESET);

	// Waiting for the card to detect the voltage change
	HAL_Delay(1000);

	// Reset of variables related to the WiFi-server status
	firstLoop = false;
	atStatus = false;
	wifiStatus = false;
	serverStatus = false;
	control = false;
	resetState = true;

	// Reset of variables related to server connection and receiving data
	counter1kHz = 0;
	prevCounter1kHz = 0;
	write_buffer = 0;
	read_buffer = 0;
	esp_double_used = 0;
	memset((char*)esp_ring_buffer, '\0', sizeof(esp_ring_buffer));
	memset((char*)esp_double_used_buffer, '\0', sizeof(esp_double_used_buffer));
	esp_request = 0;
	bracet = 0;

	// Restore the power
//	HAL_GPIO_WritePin(ESP_PIN_PORT, ESP_PIN, GPIO_PIN_SET);
	HAL_Delay(1000);
}


void esp01s::AT_Init() {
	resetState = false;
	atStatus = false;
	wifiStatus = false;
	serverStatus = false;

	esp_recv_buffer_pointer = esp_ring_buffer[write_buffer];
	esp_request = 0;

	// Enable UART interrupts
	HAL_UART_Receive_IT(&huart3, (uint8_t*) esp_recv_buffer_pointer, 1);

	first_request = true;
	ESP_ServiceRequest("AT+RST", 1000, BR); 	// Reset the module
	ESP_ServiceRequest("AT", 0, BR);			// Test if AT system works correctly
	ESP_ServiceRequest("AT+CWMODE=1", 0, BR);	// Set AP’s info, 1 client mode
	ESP_ServiceRequest("AT+CWQAP", 0, BR);		// Disconnect from the AP is currently connected to
	ESP_ServiceRequest("AT+RST", 1000, BR); 	// Reset the module
	ESP_ServiceRequest("AT+CIPSTAMAC=\"A4:CF:12:EF:A7:40\"", 1000, BR);	// Set MAC address
	first_request = false;
	if(!resetState)
		atStatus = true;
}


void esp01s::WiFi_Init() {
	print_debug("\r\nTrying to connect WIFI network...\r\n");
	char CWJAPrequest[100];
	snprintf(CWJAPrequest, 100, "AT+CWJAP=\"%s\",\"%s\"", SSID, PASSWORD);
	ESP_ServiceRequest(CWJAPrequest, 7000, BR); 		// Connect a SSID with supplied password

	if(strstr((char *)esp_ring_buffer[read_buffer-1], "CONNECTED") || strstr((char *)esp_ring_buffer[read_buffer-1], "ready")
			|| strstr((char *)esp_ring_buffer[read_buffer-2], "CONNECTED") || strstr((char *)esp_ring_buffer[read_buffer-2], "ready")
			|| strstr((char *)esp_ring_buffer[read_buffer-3], "CONNECTED") || strstr((char *)esp_ring_buffer[read_buffer-3], "ready")) {
		ESP_ServiceRequest("AT+CIFSR", 2000, BR); 		// Get local IP address
		ESP_ServiceRequest("AT+CIPMUX=1", 0, BR); 		// Enable multiplex mode
		print_debug("WIFI init done!\r\n");
		wifiStatus = true;
	} else {
		print_debug("WIFI init failed!\r\n");
	}
}


void esp01s::ESP_ServiceRequest(const char *request, uint16_t waitTime, uint8_t endl) {
	if(resetState)		return;

	// Make sure there is no overdue readable data
	while(esp_request>0 || (!control && !first_request)) {
		while(esp_request>0)		ESP_FinishBufferProcess(HANDLE);
	}

	int status;
	print_wifi(request); 	// Sends a WiFi request
	print_debug("\r\n%s", request);		// Prints the request to the console

	// Only if the function is started with the BR parameter, Print end of line.
	if (endl)
		HAL_UART_Transmit(&huart3, (uint8_t*) "\r\n", 2, 100);

	// Waiting a certain amount of time for a callback
	if (waitTime > 0)
		HAL_Delay(waitTime);

	// Maximum time to wait for a callback
	uint16_t timer = 0;
	while (esp_request == 0 && timer < MAX_CALLBACK_TIME) {
		HAL_Delay(1); 			// Wait
		timer++;
	}

	if(timer < MAX_CALLBACK_TIME) {
		status = RESPONSE_INTIME;
	} else {			// If the waiting time is exceeded
		status = RESPONSE_OUTOFTIME;
		print_debug("	[OUT OF TIME WAITING FOR RESPONSE]{%s}\r\n", esp_ring_buffer[read_buffer]);
		if(first_request) {		// Reset if it happened after the card was started correctly
			ESP_Reset();
		}
	}

	int result = 0;
	do{
		result = ESP_FinishBufferProcess(status);
	} while(result != 0);
}


uint8_t esp01s::ESP_FinishBufferProcess(int status) {
	int secondBuffer = 0;
	int moreIPD = 0;

	// If an error message is received, reset the MCU
	if(strstr((char *)esp_ring_buffer[read_buffer], "busy") || (strstr((char *)esp_ring_buffer[read_buffer], "ERROR") && !wifiStatus))
		ESP_Reset();

	// Display the received data and pass it on for interpretation
	if(status == RESPONSE_INTIME || status == HANDLE) {
		print_debug("\r\n{%s}\r\n", esp_ring_buffer[read_buffer]);
	}

	// If more than one IPD came to the buffor, handle them all
	while(esp_double_used > 0) {
		moreIPD++;
		memcpy((char *)esp_ring_buffer[read_buffer], (char *)esp_double_used_buffer, BUFFER_LEN);
		esp_double_used = 0;
		ESP_HandleIPDSupport();
		print_debug("\r\n{%s}\r\n", esp_ring_buffer[read_buffer]);
	}

	// Move the read indicator
	if(read_buffer == BACKUP_BUFFOR_SIZE-1)
		read_buffer = 0;
	else
		read_buffer++;

	// Reduce the amount of analysis flag
	if(esp_request != 0)
		esp_request--;

	// If you have not finished analyzing all objects, it will let you know that it will be the next one to read
	if(esp_request > 0) {
		secondBuffer = 1;
//		prevCounter1kHz = counter1kHz; 		// Reset buffer processing time
	}
	return secondBuffer;
}

/*
 *
 */
void esp01s::ESP_HandleIPDSupport() {
	char *incommingPayload = strstr((char*)  esp_ring_buffer[read_buffer], "+IPD,");

	// Remove garbage found before JSON
	char* colon = strchr((char*) esp_ring_buffer[read_buffer], ':');
	colon++;
	while(colon && *colon!='{' && colon-incommingPayload < MAX_SIZE_OF_TRASH) {
		*colon = ' ';
		colon++;
	}

	// Checking if there is a next IPD in the string
	char *nextIPD = strchr((char*) esp_ring_buffer[read_buffer], ':');
	nextIPD++;
	int IPDrecog = 0;
	for(int counter = 0; counter < sizeof( esp_ring_buffer[read_buffer])-(nextIPD-(char*) esp_ring_buffer[read_buffer]); counter++) {
		if(*nextIPD == IPDstring[IPDrecog])
			IPDrecog++;
		if(IPDrecog == 5) {
			esp_double_used++;
			nextIPD -= 5;
			break;
		}
		nextIPD++;
	}

	// Copying the remaining IPD to the backup buffer
	if(esp_double_used > 0) {
		memcpy((char*)esp_double_used_buffer, nextIPD, BUFFER_LEN);
		*nextIPD = '\0';
	}
}
