#include "display_read_com.h"

/********************** IO functions **************************/

void Init_IO(void)
{
    uint z;
    
    // general init
    stdio_init_all();
    // init pins
    // test pins
    gpio_init(PIN_LED);     // GP25
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_init(PIN_TEST10);  // GP10
    gpio_set_dir(PIN_TEST10, GPIO_OUT);
    gpio_init(PIN_TEST11);  // GP11
    gpio_set_dir(PIN_TEST11, GPIO_OUT);
    
    // pins to read display communication
    gpio_init(PIN_nWR);     // GP22
    gpio_set_dir(PIN_nWR, GPIO_IN);
    gpio_init(PIN_nCS);     // GP20
    gpio_set_dir(PIN_nCS, GPIO_IN);
    gpio_init(PIN_Data);     // GP21
    gpio_set_dir(PIN_Data, GPIO_IN);
    
    gpio_init(PIN_RESET);   // GP18
    gpio_set_dir(PIN_RESET, GPIO_OUT);

    // define pin status
    gpio_put(PIN_LED, 1);
    gpio_put(PIN_RESET, 0);
    
    gpio_put(PIN_TEST10, 1);
    gpio_put(PIN_TEST11, 1);    
    
    // init .._buffer
    strcpy(rx_buffer, version);
    for (z = 0; z < BUFFER_SIZE; z++) 
    {    
        tx_buffer[z] = 0;
        rx_buffer[z] = 0;
        in_buffer[z] = 0;
        send_buffer1[z] = 0;
        send_buffer2[z] = 0;
    }
    tx_in_p = 0;
    tx_out_p = 0;
    rx_in_p = 0;
    rx_out_p = 0;
    rx_timeout = 0;
    in_in_p = 0;

    // analyse
    pin_nWR_old = 1;
    cnt_bit_in = 0;
    in_data = 0;
    mode_w = 0;
    mode_c = 0;
    cnt_main_loop = 0;
    cnt_reset_loop = 0;

    // Enable the watchdog, requiring the watchdog to be updated every 200ms or the chip will reboot
    // second arg is when set to pause on debug which means the watchdog will pause when stepping through code
    watchdog_enable(200, 0);
}


void check_IO(void)
{
    pin_nWR = gpio_get(PIN_nWR);
    pin_nCS = gpio_get(PIN_nCS);
    pin_Data = gpio_get(PIN_Data);
    
    if (!pin_nCS)
    {
        // receive data
        if (pin_nWR && (pin_nWR != pin_nWR_old))
        {
            // new data bit
            if (pin_Data) in_data = in_data | 0x01;
            cnt_bit_in++;
            if ((mode_w == 2) && (cnt_bit_in == 4))
            {
                // data received
                in_buffer[in_in_p++] = in_data;
                if (in_in_p > BUFFER_SIZE) in_in_p = 0;
                cnt_bit_in = 0;
                in_data = 0;
            }
            else if ((mode_w == 1) && (cnt_bit_in == 6))
            {
                // address received
                mode_w = 2;
                in_buffer[in_in_p++] = in_data;
                if (in_in_p > BUFFER_SIZE) in_in_p = 0;
                cnt_bit_in = 0;
                in_data = 0;
            }
            else if (!mode_w && !mode_c && (cnt_bit_in == 3))
            {
                // mode received
                if (in_data == 0x05)
                {
                    // write mode
                    mode_w = 1;
                    in_buffer[in_in_p++] = 'W';
                    if (in_in_p > BUFFER_SIZE) in_in_p = 0;
                }
                if (in_data == 0x04)
                {
                    // command mode - currently not used further
                    mode_c = 1;
            //        in_buffer[in_in_p++] = 'C';               - currently not used further
            //        if (in_in_p > BUFFER_SIZE) in_in_p = 0;   - currently not used further
                }
                cnt_bit_in = 0;
                in_data = 0;
            }
            else if ((mode_c == 1) && (cnt_bit_in == 5))
            {
                mode_c = 2;
            //    in_buffer[in_in_p++] = in_data;           - currently not used further
            //    if (in_in_p > BUFFER_SIZE) in_in_p = 0;   - currently not used further
                cnt_bit_in = 0;
                in_data = 0;
            }
            else if ((mode_c == 2) && (cnt_bit_in == 4))
            {
            //    in_buffer[in_in_p++] = in_data;           - currently not used further
            //    if (in_in_p > BUFFER_SIZE) in_in_p = 0;   - currently not used further
                cnt_bit_in = 0;
                in_data = 0;
            }
            else
            {
                in_data = in_data << 1;
            }
        }
        pin_nWR_old = pin_nWR;
        cnt_main_loop = 0;
    }
    else
    {
        pin_nWR_old = 1;
        cnt_bit_in = 0;
        in_data = 0;
        mode_w = 0;
        mode_c = 0;
        cnt_main_loop++;
        watchdog_update();
        
        if (cnt_main_loop == 450)
        {
            convert_w_data();
            UART_Rx();
            UART_Tx();
            
            gpio_put(PIN_TEST11, 1);
        }
        else if (cnt_main_loop > 450)
        {
            UART_Rx();
            UART_Tx();
            if (rx_in_p != rx_out_p)
            {
                if (rx_in_p > (rx_out_p + 12))
                {
                    // received a 13 byte for a request
                    parse_RX();
                }
            }
            cnt_main_loop = 450;
        }
    }
    
    if (cnt_reset_loop > 0)
    {
        cnt_reset_loop--;
        if (cnt_reset_loop == 1)
        {
            gpio_put(PIN_RESET, 0);
            cnt_reset_loop = 0;
        }
    }
}


void parse_RX(void)
{
    bool found_request_data1 = 0;
    bool found_request_data2 = 0;
    bool found_request_reset = 0;
    uint8_t a;
    
    // there are 13 char in rx_buffer - compare 
    do 
    {
        if (!memcmp(&rx_buffer[rx_out_p], in_request_data1_str, 13)) 
        {
            // request found
            found_request_data1 = 1;
        }
        else if (!memcmp(&rx_buffer[rx_out_p], in_request_data2_str, 13)) 
        {
            // request found
            found_request_data2 = 1;
        }
        else if (!memcmp(&rx_buffer[rx_out_p], in_request_reset_str, 13)) 
        {
            // request found
            found_request_reset = 1;
        }
        rx_out_p++;
        if (rx_out_p > BUFFER_SIZE) rx_out_p = 0; 
    } while ((!found_request_data1) && (!found_request_data2) && (!found_request_reset) && (rx_in_p > (rx_out_p + 12)));
    
    if (found_request_data1)
    {
        // valid request received
        // send send_buffer
        for (a = 0; a < 13; a++)
        {
            tx_buffer[tx_in_p++] = send_buffer1[a];
            if (tx_in_p > BUFFER_SIZE) tx_in_p = 0;
        }
    }
    else if (found_request_data2)
    {
        // valid request received
        // send send_buffer
        for (a = 0; a < 13; a++)
        {
            tx_buffer[tx_in_p++] = send_buffer2[a];
            if (tx_in_p > BUFFER_SIZE) tx_in_p = 0;
        }
    }
    else if (found_request_reset)
    {
        // valid request received
        // send reset for approx. 2,28ms
        gpio_put(PIN_RESET, 1);
        cnt_reset_loop = 2000;
        
    }
}


/********************** end IO functions **************************/


/********************** helper function **************************/


uint8_t sseg_to_number(uint8_t sseg)
{
    uint8_t n;
    
    switch (sseg)
    {
        case 0x5f: n = '0';
            break;
        case 0x50: n = '1';
            break;
        case 0x6b: n = '2';
            break;
        case 0x79: n = '3';
            break;
        case 0x74: n = '4';
            break;
        case 0x3d: n = '5';
            break;
        case 0x3f: n = '6';
            break;
        case 0x58: n = '7';
            break;
        case 0x7f: n = '8';
            break;
        case 0x7d: n = '9';
            break;
        default:   n = ' ';
    }
    
    return n;
}


void convert_w_data(void)
{
    uint z, a = 0;
    uint8_t d0, d1;
    bool found = 0;
    
    // search for start-char
    do
    {
        if ((in_buffer[a] == 'W') && (in_buffer[a + 1] == 0))
        {
            found = 1;
        }
        a++;
    } while ((a < in_in_p) && (!found));

    if (found)
    {
        // adjust
        a++;    
        // now adjusted
    
        if ((a + 20) < in_in_p)
        {
            // rest of data is long enough

            // reset send_buffer(x)
            for (z = 0; z < 13; z++)
            {
                send_buffer1[z] = 0;
                send_buffer2[z] = 0;
            }

            // fixed send_string send_buffer1
            send_buffer1[0] = start_byte;    // fixed start byte
            send_buffer1[1] = device_id;     // request/response device id   - range 0x10 .. 0x17
            send_buffer1[2] = response_data1_byte; // requested data id = 0xa0 - response 0xa1
            send_buffer1[3] = length_byte;   // data length (fixed) - request and response

            // fixed send_string send_buffer2
            send_buffer2[0] = start_byte;    // fixed start byte
            send_buffer2[1] = device_id;     // request/response device id   - range 0x10 .. 0x17
            send_buffer2[2] = response_data2_byte; // requested data id = 0xa2 - response 0xa3
            send_buffer2[3] = length_byte;   // data length (fixed) - request and response

            // 1st value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[4] = sseg_to_number(d1 & 0x7F);
            // save bit 7 
            if (d1 & 0x80) send_buffer2[4] = send_buffer2[4] | 0x80;
           
            // 2nd value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[5] = sseg_to_number(d1 & 0x7F);
            // save bit 7 
            if (d1 & 0x80) send_buffer2[4] = send_buffer2[4] | 0x40;
            
            // 3rd value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[6] = sseg_to_number(d1 & 0x7F);
            // copy bit 7 too
            if (d1 & 0x80) send_buffer1[6] = send_buffer1[6] | 0x80;
            
            // 4th value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            // save bit 7 
            if (d1 & 0x80) send_buffer2[4] = send_buffer2[4] | 0x10;
            // write value
            // add info kW vs. W to send_buffer1[7]
            if (in_buffer[a] & 0x02)
            {
                send_buffer1[7] = sseg_to_number(d1 & 0x7F) | 0x80;
            }
            else
            {
                send_buffer1[7] = sseg_to_number(d1 & 0x7F);
            }
                        
            // in-between values - use only bits 0..3
            send_buffer2[5] = in_buffer[a++];
            send_buffer2[6] = in_buffer[a++];
            send_buffer2[7] = in_buffer[a++];
            send_buffer2[8] = in_buffer[a++];
            send_buffer2[9] = in_buffer[a++];
            
            // 5st value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[8] = sseg_to_number(d1 & 0x7F);
            // save bit 7 
            if (d1 & 0x80) send_buffer2[4] = send_buffer2[4] | 0x08;
            
            // 6nd value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[9] = sseg_to_number(d1 & 0x7F);
            // save bit 7 
            if (d1 & 0x80) send_buffer2[4] = send_buffer2[4] | 0x04;
            
            // 7rd value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[10] = sseg_to_number(d1 & 0x7F);
            // copy bit 7 too
            if (d1 & 0x80) send_buffer1[10] = send_buffer1[10] | 0x80;
            
            // 8th value
            d0 = in_buffer[a++] & 0x0F;
            d1 = in_buffer[a++] & 0x0F;
            d1 = d1 << 4;
            d1 = d1 | d0;
            send_buffer1[11] = sseg_to_number(d1 & 0x7F);
            // save bit 7 
            if (d1 & 0x80) send_buffer2[4] = send_buffer2[4] | 0x01;
            
            // calculate checksum send_buffer1
            d0 = 0;
            for (a = 0; a < 12; a++)
            {
                d0 += send_buffer1[a];
            }
            send_buffer1[12] = d0;
            
            // calculate checksum send_buffer2
            d0 = 0;
            for (a = 0; a < 12; a++)
            {
                d0 += send_buffer2[a];
            }
            send_buffer2[12] = d0;
        }
    }
    in_in_p = 0;    
}
    
/********************** end helper functions **************************/


/********************** UART functions **************************/

void Init_UART(void)
{
    uart_init(UART1_ID, BAUD_RATE);
    uart_set_format(UART1_ID, 8, 1, UART_PARITY_NONE);
    uart_set_translate_crlf(UART1_ID, false);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
}

void UART_Rx(void)
{
    if (uart_is_readable(UART1_ID))   // new char in rx fifo ?
    {
        rx_buffer[rx_in_p++] = uart_getc(UART1_ID);
        if (rx_in_p > BUFFER_SIZE) rx_in_p = 0;
        rx_timeout = 0;
    }
    else
    {
        rx_timeout++;
        if (rx_timeout > 3800)
        {
            rx_in_p = 0;
            rx_out_p = 0;
            rx_timeout = 3800;
        }
    }
}
 
void UART_Tx(void)
{
    if (tx_in_p != tx_out_p)
    {
        if (uart_is_writable(UART1_ID))
        {
            uart_putc_raw(UART1_ID, tx_buffer[tx_out_p++]);  // update UART transmit data register
            if (tx_out_p > BUFFER_SIZE) tx_out_p = 0;
        }
    }
}

/********************** end UART functions **************************/


int main() 
{
    Init_IO();
    Init_UART();
    
    while (1) 
    {
        /* used for testing
        gpio_put(PIN_TEST10, 1);
        gpio_put(PIN_TEST11, 1);
        gpio_put(PIN_TEST10, 0);
        gpio_put(PIN_TEST11, 0);
        */

        check_IO();
    }
}
