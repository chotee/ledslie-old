#include <stdio.h>
#include <stdint.h>


#define col_per_elem 8   // Every LED element has 8 columns.
#define elem_pcb_count 6  // Every PCB has 6 elements
#define pcb_line_count 3  // There are 3 PCBs in one line
#define display_line_count 1 // A display as 3 lines
#define elem_line_count elem_pcb_count*pcb_line_count // Number of elements in a line
#define elem_display_count elem_line_count*display_line_count // number of elements in the whole display
#define display_size elem_display_count*col_per_elem // Number of bytes to represent a whole display.

// SPI CLOCK_PIN 13
// SPI DATA_PIN 11
#define LATCH_PIN 9  // I latch the data of a single column to display it.
#define HEARTBEAT_PIN 5 // I toggle high/low when a full cycle is complete
#define SERIAL_LOAD_PIN 6 // I toggle high when serial is being read

//#define BAUD_RATE 115200
#define BAUD_RATE 9600

#define LOW 0
#define HIGH 1

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

// While one Bitmap is being displayed, the other gets filled.
uint8_t bitmapA[display_size]; // Reserve space for two bitmaps.
uint8_t bitmapB[display_size]; // The second bitmap.
uint8_t *bitmap_display = bitmapA;
uint8_t *bitmap_load = bitmapB;
uint16_t load_counter = 0;

void setup()
{
  // pinMode(LATCH_PIN, OUTPUT);
  // pinMode(HEARTBEAT_PIN, OUTPUT);
  // pinMode(SERIAL_LOAD_PIN, OUTPUT);
  // SPI.begin();
  // SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));
  // Init the whole bitmap to 0 first.
  int16_t i;
  for (i=0; i<display_size; i++) {
    bitmapA[i] = 0x00;
    bitmapB[i] = 0x00;
  }
  // Serial.begin(BAUD_RATE);  // Setup the serial line to read from
  // Serial.println("Ledslie Refresher Ready");
}

void swap_bitmaps() {
    uint8_t *t = bitmap_display;
    bitmap_display = bitmap_load;
    bitmap_load = t;
    load_counter = 0;


}

uint8_t digitalRead(uint8_t pin_nr) {
    return 0;
}

void digitalWrite(uint8_t pin_nr, uint8_t value) {
    printf("Set PIN %d to %d\n", pin_nr, value);
}

void toggle_pin(uint8_t pin_nr) {
    if(digitalRead(pin_nr)) {
        digitalWrite(pin_nr, LOW);
    } else {
        digitalWrite(pin_nr, HIGH);
    }
}

void transfer(uint8_t data) {
    //SPI.transfer(data);
    //Serial.println(data);
    printf("SPI Transfer: 0x%02x ", data);
    printf("b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data));
    printf("\n");
}

void update_column(
        uint8_t col // elements column that we're trying to fill
    ) {
    // I send over all the data of one column of all the elements in the display
    uint8_t col_select = ~(1<<col); // column selection Byte
    // Shift the bytes into the display
    //Serial.println("-");
    int16_t e;
    for(e = elem_display_count-1; e >= 0; --e) {
        //Serial.println(e);
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

void show(uint8_t c) {
    printf("%#08x\n", c);
}


void read_input() {
    digitalWrite(SERIAL_LOAD_PIN, HIGH);
    // for(uint8_t t = Serial.available(); t>0; --t) {
    uint8_t c;
    scanf("%c", &c);
    c = c - '0';
    show(c);
    uint16_t i = 0;
    for(i = 0; i<=display_size; i++) {
        bitmap_load[i] = c;
    }
    swap_bitmaps();
        // bitmap_load[load_counter] = Serial.read();
        // // load_counter++;
        // // if(load_counter == display_size) {
        // //     swap_bitmaps();
        // // }
    // }
    digitalWrite(SERIAL_LOAD_PIN, LOW);
}

void loop()
{
    //SPI.transfer(0xC);
    uint8_t c;
    for(c = 0; c < col_per_elem; c++) {
        update_column(c);
        read_input();
    }
    toggle_pin(HEARTBEAT_PIN); // Pin to measure software performance.
}


int main()
{
    setup();
    while(1) {
        loop();
    }
}
