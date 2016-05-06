/*  3 x 2-Byte Data Transmission
 *  # entries of data 2 bytes wide----- this code knows what type of data,
 *  and how much data it is getting. In the future, it will be nice to be
 *  able to establish different amounts of entires and different byte sizes
 *  for different slaves using some meta-data during startup of the system,
 *  a sort of configuration between master and slaves upon startup.
 */

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>

/*************************
  Pinout
 *************************

RPi  -->  Arduino
SCLK -->  SCLK - 13
MISO -->  MISO - 12
MOSI -->  MOSI - 11
CE0  -->  SS   - 10
GND  -->  GND

*/

/*************************
  Declare Global variables
 *************************/

#define _MAX_WAIT_LOOP 3
#define WAIT 10

int fd;

unsigned char NOT_RDY = 'n';
unsigned char RDY = 'r';
unsigned char TXRX = 't';
unsigned char DONE = 'd';

unsigned char ard_status;

int unsigned long time_start;
int unsigned long time_start_loop;

int16_t rxData[3];

unsigned char rxDatum;
unsigned char txDat;

std::fstream Rx_Data;

int ATTEMPT = 0;
int SUCCESS = 0;
int FAILURE = 0;

/*************************
  Declare Functions
 *************************/

int spiTxRx(unsigned char txDat);

/*******************************
  Main
  Setup SPI
  Open file spidev0.0 (chip enable 0) for read/write
  access with the file descriptor "fd"
  Configure transfer speed (1MHz)
  Start an endless loop that repeadedly send the unsigned unsigned characters
  in hello[] array to the Arduino and displays the 
  returned bytes
 *******************************/

int main(void) {

    fd = open("/dev/spidev0.0", O_RDWR);
    unsigned int speed = 1000000;
    ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    Rx_Data.open ("Rx_Data.txt", std::fstream::out);
    Rx_Data.close();

    time_start = time(NULL);

    while(ATTEMPT<=10000) {
        //  Check to see if the Arduino is ready and send self status
        time_start_loop = time(NULL);
        ard_status = spiTxRx(RDY);
//        std::cout << "Got ard_status = " << ard_status << std::endl;
        usleep(WAIT);
        if (ard_status == RDY) {
            ard_status = spiTxRx(TXRX);
            if (ard_status == TXRX) {
                //  Get the data
                usleep(WAIT);
                rxDatum = spiTxRx(TXRX);
                rxData[0] = rxDatum & 0xFF;
                usleep(WAIT);
                rxDatum = spiTxRx(TXRX);
                rxData[0] = (rxDatum << 8) | rxData[0];
                usleep(WAIT);
                rxDatum = spiTxRx(TXRX);
                rxData[1] = rxDatum & 0xFF;
                usleep(WAIT);
                rxDatum = spiTxRx(TXRX);
                rxData[1] = (rxDatum << 8) | rxData[1];
                usleep(WAIT);
                rxDatum = spiTxRx(TXRX);
                rxData[2] = rxDatum & 0xFF;
                usleep(WAIT);
                rxDatum = spiTxRx(TXRX);
                rxData[2] = (rxDatum << 8) | rxData[2];
                //  Store the data
                usleep(WAIT);
                ard_status = spiTxRx(DONE);
                if (ard_status == DONE) {
                    Rx_Data.open ("Rx_Data.txt", std::fstream::out | std::fstream::app);
                    Rx_Data << rxData[0] << std::endl;
                    Rx_Data.close();
                    ATTEMPT++;
                    SUCCESS++;
                } else {
                    ATTEMPT++;
                    FAILURE++;
//                    std::cout << "FAILURE!\n";
                }
            }
        } else if ((time(NULL)-time_start_loop)>=_MAX_WAIT_LOOP) {
            std::cout << "The arduino was not ready...quitting\n";
            break;
        }
    }
    std::cout << "Total time = " << time(NULL)-time_start << std::endl;
    std::cout << "ATTEMPT = " << ATTEMPT << std::endl;
    std::cout << "SUCCESS = " << SUCCESS << std::endl;
    std::cout << "FAILURE = " << FAILURE << std::endl;
}

int spiTxRx(unsigned char txDat) {
    unsigned char rxDat;

    struct spi_ioc_transfer spi;

    memset (&spi, 0, sizeof(spi));

    spi.tx_buf = (unsigned long)&txDat;
    spi.rx_buf = (unsigned long)&rxDat;
    spi.len    = 1;

    ioctl (fd, SPI_IOC_MESSAGE(1), &spi);

    return rxDat;
}
