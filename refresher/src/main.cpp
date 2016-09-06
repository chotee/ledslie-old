/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include "Arduino.h"

#define BAUD_RATE 115200

// bitmaps * strings * rows * elements
#define elem_count 18
#define chain_count 3
#define chain_size 8*elem_count
#define bitmap_size chain_count*chain_size
#define data_size 2*bitmap_size
uint8_t bitmaps[data_size];

bool led_state = 0;
uint8_t  active_bitmap;
uint16_t read_counter;
uint16_t disp_counter;

void setup()
{
  for (int16_t i=0; i<data_size; i++) {
    bitmaps[i] = 0;
  }
  active_bitmap = 0;
  read_counter  = 0;
  disp_counter  = 0;

  pinMode(LED_BUILTIN, OUTPUT);  // initialize LED digital pin as an output.
  Serial.begin(BAUD_RATE);  // Setup the serial line to read from
  Serial.println("Ready");
}

void toggle_led() {
    if(led_state) {
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
    led_state = ~led_state;
}

void read_serial() {
    uint8_t bytes_pending = Serial.available();
    if(bytes_pending) {
        uint16_t data_start = active_bitmap*bitmap_size + read_counter;
        Serial.readBytes(bitmaps+data_start, bytes_pending);
    }
}

void update_row(uint8_t row_nr) {
    // bits 0, 1, 2 are the data bits. One for each chain.
    #define enable_bit = 4;
    #define clock_bit = 5;
    uint8_t *chain_offsets[chain_count];
    for(uint8_t c=0; c<chain_count; c++) { // For each chain, where does that data begin?
        chain_offsets[c] = bitmaps + active_bitmap*bitmap_size + c*chain_size;
    }
    uint8_t output_byte = 0;
    for(uint8_t e=0; e<elem_count; e++) {  // Walk over each byte in the chain
        output_byte = (output_byte>>chain_count)<<chain_count; // make the first 3 bits zero
        uint8_t read_byte[chain_count]; // Store the bytes to work with. One for each chain.
        for(uint8_t c=0; c<chain_count; c++) { // Walk over each of the chains.
            read_byte[c] = *(chain_offsets[c] + e); // Grab that byte for the chain.
        }

        for(uint8_t b=0; b<8; b++) { // walk over each of the bits in the byte.
            for(uint8_t c=0; c<chain_count; c++) { // Walk over each chain
                // Check if bit b in the byte of that chain is true. if so, set
                // the bit corresponding with chain c to true. Assumes
                output_byte |= (read_byte[c] & 1<<b ? 1 : 0) << c;  // set that one bit.

            }
        }
    }
}

void update_display() {
    for(uint8_t row_counter=0; row_counter<8; row_counter++) {
        update_row(row_counter);
    }

}

void loop()
{
    // turn the LED on (HIGH is the voltage level)
    toggle_led();
    read_serial();
    update_display();
}
