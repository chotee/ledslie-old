/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

 #include <SPI.h>
#include "Arduino.h"

#define col_per_elem 8   // Every LED element has 8 columns.
#define elem_pcb_count 6  // Every PCB has 6 elements
#define pcb_line_count 3  // There are 3 PCBs in one line
#define display_line_count 3 // A display as 3 lines
#define elem_line_count elem_pcb_count*pcb_line_count // Number of elements in a line
#define elem_display_count elem_line_count*display_line_count // number of elements in the whole display
#define display_size elem_display_count*col_per_elem // Number of bytes to represent a whole display.

#define BAUD_RATE 115200

#define HEARTBEAT_PIN 1
#define LATCH_PIN 2

uint8_t bitmap[display_size]; //

void setup()
{
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(HEARTBEAT_PIN, OUTPUT);
  SPI.begin();
  SPI.beginTransaction(SPISettings(15000000, MSBFIRST, SPI_MODE0));
  // Init the whole bitmap to 0 first.
  for (int16_t i=0; i<display_size; i++) {
    bitmap[i] = 0x0;
  }
  Serial.begin(BAUD_RATE);  // Setup the serial line to read from
  Serial.println("Ready\n");
}

void toggle_pin(uint8_t pin_nr) {
    if(digitalRead(pin_nr)) {
        digitalWrite(pin_nr, LOW);
    } else {
        digitalWrite(pin_nr, HIGH);
    }
}

void transfer(uint8_t data) {
    SPI.transfer(data);
    Serial.println(data);
}

void update_display() {
    // Iterate over each of the columns
    for(uint8_t c=0; c < col_per_elem; c++) {
        // Shift the bytes into the display
        uint8_t col_select = ~(1<<c);
        // Shift in one byte for each element in the display
        for(int16_t e = elem_display_count; e >= 0; --e) {
            transfer(bitmap[e*col_per_elem+c]);
            if(e % 6 == 0) { // Add the column selector after each 6 bytes
                transfer(col_select);
            }
        }
        // After shifting in all data for that column, clock it in with the
        // LATCH_PIN that's normally kept high.
        digitalWrite(LATCH_PIN, LOW);
        digitalWrite(LATCH_PIN, HIGH);
    }
}

void loop()
{
    SPI.transfer(0xC);
    // update_display();
    //toggle_pin(HEARTBEAT_PIN);
}
