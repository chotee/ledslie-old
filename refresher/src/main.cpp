/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>
#include <SPI.h>

#define col_per_elem 8   // Every LED element has 8 columns.
#define elem_pcb_count 6  // Every PCB has 6 elements
#define pcb_line_count 3  // There are 3 PCBs in one line
#define display_line_count 3 // A display as 3 lines
#define elem_line_count elem_pcb_count*pcb_line_count // Number of elements in a line
#define elem_display_count elem_line_count*display_line_count // number of elements in the whole display
#define display_size elem_display_count*col_per_elem // Number of bytes to represent a whole display.


#define LATCH_PIN 9  // I latch the data of a single column to display it.
#define HEARTBEAT_PIN 5 // I toggle high/low when a full cycle is complete
#define SERIAL_LOAD_PIN 6 // I toggle high when serial is being read

#define BAUD_RATE 115200

// While one Bitmap is being displayed, the other gets filled.
uint8_t bitmapA[display_size]; // Reserve space for two bitmaps.
uint8_t bitmapB[display_size]; // The second bitmap.
uint8_t *bitmap_display = bitmapA;
uint8_t *bitmap_load = bitmapB;
uint16_t load_counter = 0;

void setup()
{
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(HEARTBEAT_PIN, OUTPUT);
  pinMode(SERIAL_LOAD_PIN, OUTPUT);
  SPI.begin();
  SPI.beginTransaction(SPISettings(15000000, MSBFIRST, SPI_MODE0));
  // Init the whole bitmap to 0 first.
  for (int16_t i=0; i<display_size; i++) {
    bitmapA[i] = 0x5;
    bitmapB[i] = 0xB;
  }
  Serial.begin(BAUD_RATE);  // Setup the serial line to read from
  Serial.println("Ledslie Refresher Ready");
}

void swap_bitmaps() {
    uint8_t *t = bitmap_display;
    bitmap_display = bitmap_load;
    bitmap_load = t;
    load_counter = 0;
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
    //Serial.println(data);
}

void update_column(
        uint8_t col // elements column that we're trying to fill
    ) {
    // I send over all the data of one column of all the elements in the display
    uint8_t col_select = ~(1<<col); // column selection Byte
    // Shift the bytes into the display
    for(int16_t e = elem_display_count; e >= 0; --e) {
        // Shift in one byte for each element in the display
        transfer(bitmap_display[e*col_per_elem+col]);
        if(e % elem_pcb_count == 0) { // Add the col_select byte after each elem_pcb_count bytes
            transfer(col_select);
        }
    }
    // After shifting in all data for that column, clock it in with the
    // LATCH_PIN that's normally kept high.
    digitalWrite(LATCH_PIN, LOW);
    digitalWrite(LATCH_PIN, HIGH);
}

void read_input() {
    digitalWrite(SERIAL_LOAD_PIN, HIGH);
    for(uint8_t t = Serial.available(); t>0; --t) {
        bitmap_load[load_counter] = Serial.read();
        load_counter++;
        if(load_counter == display_size) {
            swap_bitmaps();
        }
    }
    digitalWrite(SERIAL_LOAD_PIN, LOW);
}

void loop()
{
    //SPI.transfer(0xC);
    for(uint8_t c=0; c < col_per_elem; c++) {
        update_column(c);
        read_input();
    }
    toggle_pin(HEARTBEAT_PIN); // Pin to measure software performance.
}
