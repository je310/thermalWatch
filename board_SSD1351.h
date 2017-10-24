/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#define SPI_DRIVER (&SPID1)
#define SPI_PORT GPIOA
#define SCK_PAD  5  //PA5
#define MISO_PAD 6 //PA6
#define MOSI_PAD 12 //PA7

#define CS_PORT     GPIOA
#define RESET_PORT  GPIOA
#define DNC_PORT    GPIOB
#define CS_PAD     11        // PB5 --  0 = chip selected
#define RESET_PAD  8        // PA10 -- 0 = reset
#define DNC_PAD    6        // PA9 -- control=0, data=1 -- DNC or D/C

// SPI setup ajust " SPI_BaudRatePrescaler_X" to set SPI speed.
// Peripherial Clock 42MHz SPI2 SPI3
// Peripherial Clock 84MHz SPI1                                SPI1        SPI2/3
#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000) //  42 MHz      21 MHZ
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008) //  21 MHz      10.5 MHz
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010) //  10.5 MHz    5.25 MHz
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018) //  5.25 MHz    2.626 MHz
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020) //  2.626 MHz   1.3125 MHz
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028) //  1.3125 MHz  656.25 KHz
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030) //  656.25 KHz  328.125 KHz
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038) //  328.125 KHz 164.06 KHz
static SPIConfig spi_cfg = {
        NULL,
        CS_PORT,
        CS_PAD,
        SPI_BaudRatePrescaler_4 //AJUST SPEED HERE..
};

static inline void init_board(GDisplay *g) {
    (void) g;
    //g->board = 0;
                //Set up the pins..
        palSetPadMode(SPI_PORT, SCK_PAD,  PAL_MODE_ALTERNATE(5));
        palSetPadMode(SPI_PORT, MOSI_PAD,  PAL_MODE_ALTERNATE(5));
        palSetPadMode(SPI_PORT, MISO_PAD,  PAL_MODE_ALTERNATE(5));
        palSetPadMode(RESET_PORT, RESET_PAD, PAL_MODE_OUTPUT_PUSHPULL);
        palSetPadMode(CS_PORT, CS_PAD, PAL_MODE_OUTPUT_PUSHPULL);
        palSetPadMode(DNC_PORT, DNC_PAD, PAL_MODE_OUTPUT_PUSHPULL);
                //Set pins.
        palSetPad(CS_PORT, CS_PAD);
        palSetPad(RESET_PORT, RESET_PAD);
        palClearPad(DNC_PORT, DNC_PAD);
                //Start SPI1 with our config.
                spiStart(SPI_DRIVER, &spi_cfg);

}

static inline void post_init_board(GDisplay *g) {
    (void) g;
}


static inline void setpin_reset(GDisplay *g, bool_t state) {
    (void) g;
    palWritePad(RESET_PORT, RESET_PAD, !state);
}
static inline void set_backlight(GDisplay *g, uint8_t percent) {
    (void) g;
    (void) percent;
}
static inline void acquire_bus(GDisplay *g) {
    (void) g;
    spiSelect(SPI_DRIVER);

}

static inline void release_bus(GDisplay *g) {
    (void) g;
    spiUnselect(SPI_DRIVER);
}


static inline void write_cmd(GDisplay *g, uint8_t index) {
    static uint8_t  sindex;
    (void) g;
    spiStart(SPI_DRIVER, &spi_cfg);
    palClearPad(DNC_PORT, DNC_PAD);
    sindex = index;
    spiSend(SPI_DRIVER, 1, &sindex);
}

static inline void write_data(GDisplay *g, uint8_t data) {
    static uint8_t  sdata;
    (void) g;
    spiStart(SPI_DRIVER, &spi_cfg);
    palSetPad(DNC_PORT, DNC_PAD);
    sdata = data;
    spiSend(SPI_DRIVER, 1, &sdata);
}

static inline void write_data_multi(GDisplay *g,size_t size, uint8_t *data) {
  spiStart(SPI_DRIVER, &spi_cfg);
    palSetPad(DNC_PORT, DNC_PAD);
    //sdata = data;
    spiSelect(SPI_DRIVER);
    spiSend(SPI_DRIVER, size, data);
    spiUnselect(SPI_DRIVER);
}

static inline void setreadmode(GDisplay *g) {
   (void) g;
}

static inline void setwritemode(GDisplay *g) {
   (void) g;
}
static inline uint16_t read_data(GDisplay *g) {
    (void) g;
    return 0;
}

void pushBuffer(pixel_t* surface){

  write_cmd(GDISP, 0x15); // Set Column address
      write_data(GDISP, 0); // start
      write_data(GDISP, 127); // end

      write_cmd(GDISP, 0x75); // set row address
      write_data(GDISP,0); // start
      write_data(GDISP, 95 ); // end

      write_data_multi(GDISP,128*96*2, surface);
}


#endif /* _GDISP_LLD_BOARD_H */
