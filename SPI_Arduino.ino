#include <SPI.h>
#include <Wire.h>
#include "Adafruit_LSM303.h"

int marker = 0;

unsigned char NOT_RDY = 'n';
unsigned char RDY = 'r';
unsigned char TXRX = 't';
unsigned char DONE = 'd';

volatile boolean isr_flag;

byte hold;
byte state;
int16_t data;

int acc_pin[3] = {0, 1, 2};
int16_t acc_data[3] = {0};

bool data_hold = false;

byte buf0;
byte buf1;
byte buf2;
byte buf3;
byte buf4;
byte buf5;

void setup(void) {
    //  Slave out
    pinMode(MISO, OUTPUT);
    //  SPI -> slave mode
    SPCR |= _BV(SPE);
    //  Activate interrupts
    SPCR |= _BV(SPIE);
    //  Ready up the arduino for sensing
    state = NOT_RDY;
    SPDR = state;

    //  Accelerometer
    pinMode(acc_pin[0], INPUT); pinMode(acc_pin[1], INPUT);
    pinMode(acc_pin[2], INPUT);
}

ISR (SPI_STC_vect) {
    uint8_t hold = SPDR;
    //  SPDR has changed!! Make sure SPDR = (state || data) depending on what is happening
    switch(hold) {
        case 'n':
            // If rpi was not ready, change SPDR back to NOT_RDY
            if (state == NOT_RDY) {
                SPDR = state;
                data_hold = false;
            }
            break;
        case 'r':
            // If arduino not ready, change SPDR = NOT_RDY 
            if (state == NOT_RDY) {
                SPDR = state;
                data_hold = false;
            // If arduino is ready, set state to TXRX for data transmission
            // Turn on data_hold to negate entry into data aquisition
            } else if (state == RDY) {
                state = TXRX;
                SPDR = state;
                data_hold = true;
            // Otherwise, something probably went wrong, so start over by going into
            // not ready state and set dala_hold = false ---> basically just forget 
            // the incident and take a new data set for the next attempt.
            } else {
                state = NOT_RDY;
                SPDR = state;
                data_hold = false;
            }
            break;
        case 't':
            // Transmit all bytes of data out if rpi and arduino are in txrx state
            if (state == TXRX) {
                switch(marker) {
                    case 0:
                        SPDR = buf0;
                        marker++;
                        break;
                    case 1:
                        SPDR = buf1;
                        marker++;
                        break;
                    case 2:
                        SPDR = buf2;
                        marker++;
                        break;
                    case 3:
                        SPDR = buf3;
                        marker++;
                        break;
                    case 4:
                        SPDR = buf4;
                        marker++;
                        break;
                    case 5:
                        SPDR = buf5;
                        marker++;
                        break;
                    case 6:
                        SPDR = DONE;
                        break;
                }
            } else {
                state = NOT_RDY;
                SPDR = state;
                state = NOT_RDY;
                SPDR = state;
                data_hold = false;
            }
            break;
        case 'd':
            marker=0;
            state = NOT_RDY;
            SPDR = state;
            data_hold = false;
            break;
    }
    isr_flag = true;
}

void loop(void) {
    if (isr_flag) {
        isr_flag = false;
    } else if ((!data_hold) && (state == NOT_RDY)) {
        acc_read();
        prep_data();
        marker=0;
    }
}

void acc_read() {
    acc_data[0] = analogRead(acc_pin[0]);
    acc_data[1] = analogRead(acc_pin[1]);
    acc_data[2] = analogRead(acc_pin[2]);
}

void prep_data() {
    data = (int16_t)acc_data[0];
    buf0 = data & 0xFF;
    buf1 = (data >> 8) & 0xFF;
    data = (int16_t)acc_data[1];
    buf2 = data & 0xFF;
    buf3 = (data >> 8) & 0xFF;
    data = (int16_t)acc_data[2];
    buf4 = data & 0xFF;
    buf5 = (data >> 8) & 0xFF;
    
    state = RDY;
    SPDR = state;
}
