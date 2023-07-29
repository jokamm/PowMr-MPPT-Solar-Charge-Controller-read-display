/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef DISPLAY_READ_COM_H
#define	DISPLAY_READ_COM_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

// leave uart0 free for stdio
#define UART1_ID uart1
#define BAUD_RATE 9600
#define UART1_TX_PIN 8
#define UART1_RX_PIN 9
#define BUFFER_SIZE 254

#define start_byte            0x3a
#define device_id             0x14        // range 0x10 .. 0x17
#define request_data1_byte    0xa0
#define response_data1_byte   0xa1
#define request_data2_byte    0xa2
#define response_data2_byte   0xa3
#define request_reset_byte    0xa4
#define length_byte           0x08
/*                request  data1 data2 reset
 * device_id = 10 --> crc = 0xf2  0xf4  0xf6
 * device_id = 11 --> crc = 0xf3  0xf5  0cf7
 * device_id = 12 --> crc = 0xf4  0xf6  0xf8
 * device_id = 13 --> crc = 0xf5  0xf7  0xf9
 * device_id = 14 --> crc = 0xf6  0xf8  0xfa
 * device_id = 15 --> crc = 0xf7  0xf9  0xfb
 * device_id = 16 --> crc = 0xf8  0xfa  0xfc
 * device_id = 17 --> crc = 0xf9  0xfb  0xfd
 * set crc_byte accordingly */
#define crc_data1_byte 0xf6
#define crc_data2_byte 0xf8
#define crc_reset_byte 0xfa

// info
char version[] = " Display_read_com Build 014 20230409 ID14";

// for testing
const uint PIN_LED = 25;
const uint PIN_TEST10 = 10;
const uint PIN_TEST11 = 11;

// connection to display
const uint PIN_nWR = 22;
const uint PIN_nCS = 20;
const uint PIN_Data = 21;
bool pin_nWR;
bool pin_nCS;
bool pin_Data;

const uint PIN_RESET = 18;

// buffers
uint8_t tx_buffer[BUFFER_SIZE + 1];
uint tx_in_p;
uint tx_out_p;
uint8_t rx_buffer[BUFFER_SIZE + 19];
uint rx_in_p;
uint rx_out_p;
uint rx_timeout;
uint8_t in_buffer[BUFFER_SIZE + 1];
uint in_in_p;
uint8_t send_buffer1[BUFFER_SIZE + 1];
uint8_t send_buffer2[BUFFER_SIZE + 1];

// analyse
uint cnt_bit_in;
bool pin_nWR_old;
uint8_t in_data;
uint mode_w;
uint mode_c;
uint cnt_main_loop;
uint cnt_reset_loop;

uint8_t in_request_data1_str[] = {start_byte, device_id, request_data1_byte, length_byte, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, crc_data1_byte}; 
uint8_t in_request_data2_str[] = {start_byte, device_id, request_data2_byte, length_byte, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, crc_data2_byte}; 
uint8_t in_request_reset_str[] = {start_byte, device_id, request_reset_byte, length_byte, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, crc_reset_byte}; 

// functions
void Init_IO(void);
void check_IO(void);
void parse_RX(void);
uint8_t sseg_to_number(uint8_t);
void convert_w_data(void);
void Init_UART(void);
void UART_Rx(void);
void UART_Tx(void);


#endif	/* DISPLAY_READ_COM_H */
 
