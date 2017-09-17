/*
 ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "ch.h"
#include "hal.h"
#include "ch_test.h"
#include "stdlib.h"
#include "string.h"
#include "gfx.h"
#include "time.h"

#define __FPU_USED

#define SCB_DEMCR (*(volatile unsigned *)0xE000EDFC)
#define CPU_RESET_CYCLECOUNTER do { SCB_DEMCR = SCB_DEMCR | 0x01000000; \
    DWT->CYCCNT = 0; \
    DWT->CTRL = DWT->CTRL | 1 ; } while(0)

static THD_WORKING_AREA(screenThrWA, 1024);
static THD_FUNCTION(updateScreen, arg);


#define THERM_CS_PORT    GPIOA
#define THERM_CS_PAD     8
#define THERM_SPI_PORT  GPIOB
#define THERM_SCK_PAD   3
#define THERM_MOSI_PAD  5
#define THERM_MISO_PAD  4

#define SPI_PORT GPIOA
#define SCK_PAD  5  //PA5
#define MISO_PAD 6 //PA6
#define MOSI_PAD 7 //PA7

#define CS_PORT     GPIOA
#define RESET_PORT  GPIOA
#define DNC_PORT    GPIOA
#define CS_PAD     8        // PB5 --  0 = chip selected
#define RESET_PAD  10        // PA10 -- 0 = reset
#define DNC_PAD    9        // PA9 -- control=0, data=1 -- DNC or D/C

#define VOSPI_FRAME_SIZE (164)

#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000) //  42 MHz      21 MHZ
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008) //  21 MHz      10.5 MHz
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010) //  10.5 MHz    5.25 MHz
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018) //  5.25 MHz    2.626 MHz
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020) //  2.626 MHz   1.3125 MHz
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028) //  1.3125 MHz  656.25 KHz
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030) //  656.25 KHz  328.125 KHz
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038) //  328.125 KHz 164.06 KHz
static SPIConfig spi_cfg_therm = {NULL,
CS_PORT,
                                  CS_PAD, SPI_CR1_CPOL | SPI_CR1_CPHA |
                                  SPI_BaudRatePrescaler_4,0
};

//vars
GDisplay* pixmap;
RTCDateTime timespec;
RTCAlarm alarmspec;

//therm vars
bool shouldThermal = 1;
int frameAvailable = 0;
int bufferToReadFrom = 1;
int imageReading = 0;
int packet_number = 0;
//char waste[160];
//char thermImLoc[60][80][2];
int count = 0;

int min = 99999;
int max = 0;

static uint8_t lepton_frame_packet[VOSPI_FRAME_SIZE];
static uint8_t tx[VOSPI_FRAME_SIZE] = {0, };
static short lepton_image[3][30][160];
//static short SB[96][128];
int lost_frame_counter = 0;
int last_frame_number;
int frame_complete = 0;
int start_image = 0;
int need_resync = 0;
int last_crc;
int new_frame = 0;
int frame_counter = 0;
int renders =0;

uint8_t hist[100];
int histCount = 0;

//functions

void lepton();
void lepton3();

static int uitoa(unsigned int value, char * buf, int max);
void displayWatch();
void enterStandby();

//char hist[256];

int main(void) {

  DBGMCU->CR &= ~(DBGMCU_CR_TRACE_IOEN);
  halInit();
  chSysInit();
  palSetPadMode(THERM_SPI_PORT, THERM_SCK_PAD, PAL_MODE_ALTERNATE(6));
  palSetPadMode(THERM_SPI_PORT, THERM_MOSI_PAD, PAL_MODE_ALTERNATE(6));
  palSetPadMode(THERM_SPI_PORT, THERM_MISO_PAD, PAL_MODE_ALTERNATE(6));
  palSetPadMode(THERM_CS_PORT, THERM_CS_PAD, PAL_MODE_OUTPUT_PUSHPULL);

  palSetPadMode(SPI_PORT, SCK_PAD,  PAL_MODE_ALTERNATE(5));
  palSetPadMode(SPI_PORT, MOSI_PAD,  PAL_MODE_ALTERNATE(5));
  palSetPadMode(SPI_PORT, MISO_PAD,  PAL_MODE_ALTERNATE(5));
  palSetPadMode(CS_PORT, CS_PAD, PAL_MODE_OUTPUT_PUSHPULL);
  //palSetPadMode(GPIOB, 3, PAL_MODE_OUTPUT_PUSHPULL);
  //palSetPadMode(DNC_PORT, DNC_PAD, PAL_MODE_OUTPUT_PUSHPULL);
//
 // AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
  gfxInit();
  //spiStop(&SPID1);
  pixmap = gdispPixmapCreate(128, 96);
  displayWatch();
  chThdSleepMilliseconds(1000);
  spiStart(&SPID3, &spi_cfg_therm);
  //spiUnselect(&SPID3);
  //spiSelect(&SPID3);
  //spiUnselect(&SPID3);
  chThdSleepMilliseconds(300);
  thread_t *tp = chThdCreateStatic(screenThrWA,
                                    sizeof(screenThrWA),
                                    LOWPRIO,  /* Initial priority.    */
                                    updateScreen,    /* Thread function.     */
                                    NULL);       /* Thread parameter.    */
  while(1){
  lepton3();
  }
//  spiReceive(&SPID3, 1, &waste);
//  spiStop(&SPID3);

//  while(1){

//    //chThdSleepMilliseconds(567);
//  }

  chThdSleepMilliseconds(5);
  while (true) {

  }
}

static THD_FUNCTION(updateScreen, arg) {

  while(1){
    if(frameAvailable){
      prepareFrame();
      frameAvailable = 0;
    }
    chThdYield();
    //chThdSleepMilliseconds(1);
  }

}

void lepton3(){

  //this variable is picking which section of the image buffer is being written to.
  //ufortunately lepton is silly and we need to wait till packet 20 to know which is the actual section.
  //for now we use this local variable and just increment to see what the usual pattern is.
  static localSeg =0;

  int frame_number;

  //read a packet.
  spiSelect(&SPID3);
  spiExchange(&SPID3, VOSPI_FRAME_SIZE, tx, lepton_frame_packet);
  //check to see xFxx in the ID bytes. bail if so.
  if(lepton_frame_packet[0]& 0x0f == 0x0f) return;

  //the packet must be valid.
  frame_number = lepton_frame_packet[1]; // this is the packet number only within the segment.
  hist[histCount%100] = frame_number;
  histCount++;
  if(localSeg != 3){
    int i = 0;
    if(frame_number < 60 && frame_number %2 == 0){ //even packets
      lost_frame_counter = 0;
      for(i=0;i<80;i++)
      {
          lepton_image[localSeg][frame_number/2][i] = (lepton_frame_packet[2*i+4] << 8 | lepton_frame_packet[2*i+5]);
      }
    }
    if(frame_number < 60 && frame_number %2 == 1){ //odd packets
      lost_frame_counter = 0;
      for(i=0;i<80;i++)
      {
          lepton_image[localSeg][frame_number/2][i+80] = (lepton_frame_packet[2*i+4] << 8 | lepton_frame_packet[2*i+5]);
      }
    }
  }
  if(localSeg!=0){
    int k = 0;
    k++;
  }

  //if we get garbage for too long wait so we can start from the beginning
  lost_frame_counter++;
  if(lost_frame_counter>300)
  {
      need_resync = 1;
      lost_frame_counter = 0;

  }

  if(need_resync)
  {
      chThdSleepMilliseconds(300);
      need_resync = 0;
  }

  if(frame_number == 59){
    localSeg++;
  }
  if(localSeg == 4){
    localSeg = 0;
    frameAvailable = 1;
  }



}
#ifdef LEPTON1
void lepton() {
  static short old [5];
  int i;
      int frame_number;
      //spiStart(&SPID1, &spi_cfg_therm);
      spiSelect(&SPID3);
      spiExchange(&SPID3, VOSPI_FRAME_SIZE, tx, lepton_frame_packet);
      //spiUnselect(&SPID1);
      //spiStop(&SPID1);
      count ++;

      if(((lepton_frame_packet[0]&0xf) != 0x0f))
      {
          if(lepton_frame_packet[1] == 0  )
          {
              if(last_crc != (lepton_frame_packet[3]<<8 | lepton_frame_packet[4]))
              {
                  new_frame = 1;
              }
              last_crc = lepton_frame_packet[3]<<8 | lepton_frame_packet[4];
          }
          frame_number = lepton_frame_packet[1];
          hist[count%256] = frame_number;
          if(frame_number < 60 )
          {
              lost_frame_counter = 0;
              for(i=0;i<80;i++)
              {
                  lepton_image[frame_number][i] = (lepton_frame_packet[2*i+4] << 8 | lepton_frame_packet[2*i+5]);
              }
          }
          if( frame_number == 59)
          {
              frame_complete = 1;
              last_frame_number = 0;
          }
      }
      else chThdSleepMilliseconds(20);

      lost_frame_counter++;
      if(lost_frame_counter>300)
      {
          need_resync = 1;
          lost_frame_counter = 0;

      }

      if(need_resync)
      {
          chThdSleepMilliseconds(300);
          need_resync = 0;
      }


      if(frame_complete)
      {


        static bool flip =1;
        flip = !flip;

        //prepareFrame() ;
        //spiStop(&SPID1);
        //if(flip) palSetPad(GPIOB, 3);
        //else palClearPad(GPIOB, 3);
          if(new_frame)
          {
              frame_counter++;
              frameAvailable = 1;
              {
                  //print_image_binary();
              }
              new_frame = 0;
              chThdSleepMilliseconds(20);

          }
          frame_complete = 0;
      }
}

#endif
//#define old
#ifdef old
void prepareFrame() {
  char* pixloc = (char*)gdispPixmapGetBits(pixmap);
  int i = 0;
  int pixels = 96 * 128;

  renders++;

  int localMax = 0;
  int localMin = 99999;
  int minMinMax = max -min;
  for (i = 0; i < 60; i++) {
    int j = 0;
    for (j = 0; j < 80; j++) {

      if(lepton_image[i][j]>localMax)localMax =lepton_image[i][j] ;
      if(lepton_image[i][j]<localMin)localMin =lepton_image[i][j] ;
      float value = lepton_image[i][j] ;
      value = (value - min)/minMinMax;
      uint8_t red, green, blue,upper,lower;
      float fgreen = 510*(0.5-fabs(value-0.5));
      float fred = 510*(value-0.5);
      float fblue = 510*(0.5-value);
      if (fred > 255)fred =255;
      if (fblue < 0) fblue = 0;
      if (fred < 0)fred =0;
      if (fblue > 255) fblue = 255;
      red = fred;
      blue = fblue;
      green = fgreen;
      uint16_t colour = (((31*(red+4))/255)<<11) |
                     (((63*(green+2))/255)<<5) |
                     ((31*(blue+4))/255);
      colour = (colour << 8) + (colour >>8);
      gdispGDrawPixel(pixmap, j, i, colour);

//      pixloc[(i*128+j)*2] = colour << 8;
//      pixloc[(i*128+j)*2+1] = colour;
    }

  }
  min = localMin;
  max = localMax;

  pushBuffer(pixloc);
}

#else
void prepareFrame() {
  char* pixloc = (char*)gdispPixmapGetBits(pixmap);
  int i = 0;
  int pixels = 96 * 128;

  renders++;

  int localMax = 0;
  int localMin = 99999;
  int minMinMax = max -min;
  for (i = 0; i < 60; i++) {
    int j = 0;
    for (j = 0; j < 80; j++) {

      if(lepton_image[i][j]>localMax)localMax =lepton_image[i][j] ;
      if(lepton_image[i][j]<localMin)localMin =lepton_image[i][j] ;
      int value = lepton_image[i][j] ;
      value = ((value - min)<<8)/minMinMax;
      if(value > 255)value = 255;
      if(value <0)value = 0;
      int red, green, blue,upper,lower;
      green = (128-abs(value-128))<<1;
      red = (value-128)<<1;
      blue = (128-value)<<1;
      if (red > 255)red =255;
      if (blue < 0) blue = 0;
      if (red < 0)red =0;
      if (blue > 255) blue = 255;
//      red = fred;
//      blue = fblue;
//      green = fgreen;
      uint16_t colour = (((31*(red+4))/255)<<11) |
                     (((63*(green+2))/255)<<5) |
                     ((31*(blue+4))/255);
      colour = (colour << 8) + (colour >>8);
      gdispGDrawPixel(pixmap, j, i, colour);

//      pixloc[(i*128+j)*2] = colour << 8;
//      pixloc[(i*128+j)*2+1] = colour;
    }

  }
  min = localMin;
  max = localMax;

  pushBuffer(pixloc);
}
#endif
char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
                  "Oct", "Nov", "Dec"};
char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
void displayWatch() {
  char pps_str[25];
  coord_t height, width, rx, ry, rcx, rcy;
  color_t random_color;
  font_t font;

  gdispSetOrientation(GDISP_ROTATE_90);

  width = gdispGetWidth();
  height = gdispGetHeight();
  RTCDateTime timespec;
  rtcGetTime(&RTCD1, &timespec);
  struct tm time;
  uint32_t msec;
  rtcConvertDateTimeToStructTm(&timespec, &time, &msec);
  //get time as string
  char bufH[5];
  memset(bufH, 0, sizeof(bufH));
  uitoa(time.tm_hour, bufH, sizeof(bufH));
  char bufM[4];
  memset(bufM, 0, sizeof(bufM));
  uitoa(time.tm_min, bufM, sizeof(bufM));

  strcat(bufH, ":");
  if (time.tm_min < 10)
    strcat(bufH, "0");
  strcat(bufH, bufM);

  //calc date string
  char bufferDate[25];
  memset(bufferDate, 0, sizeof(bufferDate));
  strcat(bufferDate, days[time.tm_wday]);
  strcat(bufferDate, " ");
  char mday[4];
  memset(mday, 0, sizeof(mday));
  int intMday = time.tm_mday;
  uitoa(time.tm_mday, mday, sizeof(mday));
  strcat(bufferDate, mday);
  //intMday = 24;
  if (intMday == 1)
    strcat(bufferDate, "st");
  else if (intMday == 2)
    strcat(bufferDate, "nd");
  else if (intMday == 3)
    strcat(bufferDate, "rd");
  else if (intMday == 21)
    strcat(bufferDate, "st");
  else if (intMday == 22)
    strcat(bufferDate, "nd");
  else if (intMday == 23)
    strcat(bufferDate, "rd");
  else if (intMday == 31)
    strcat(bufferDate, "st");
  else
    strcat(bufferDate, "th");
  strcat(bufferDate, " ");
  strcat(bufferDate, months[time.tm_mon]);
  //time
  font = gdispOpenFont("DejaVuSans32_aa");
  //gdispDrawStringBox(0, 10, width, 30, "14:30", font, White, justifyCenter);
  //gdispFillString(0, 10,  "14:30",  font, 0xffff, 0x0000);
  gdispGFillStringBox(pixmap, 0, 10, width, 30, bufH, font, 0xffff, 0x0000,
                      justifyCenter);
  //Date
  font = gdispOpenFont("DejaVuSans16");
  gdispGDrawStringBox(pixmap, 0, 43, width, 30, bufferDate, font, White,
                      justifyCenter);

  //reminder
  font = gdispOpenFont("UI1");
  gdispGDrawStringBox(pixmap, 0, 66, width, 30, "Mum's b'day is in 4 days",
                      font, White, justifyCenter);

  //battery
  gdispGFillArea(pixmap, 110, 0, 17, 6, 0xffff);

  gdispGFillArea(pixmap, 111, 1, 15, 4, 0x0F00);

  //alarm symbol

  gdispGDrawCircle(pixmap, 98, 4, 2, 0xffff);
  gdispGDrawLine(pixmap, 98, 4, 98 + 3, 4 - 3, 0xFFFF);
  gdispGDrawLine(pixmap, 98, 4, 98 - 3, 4 - 3, 0xFFFF);

  //debug count
  static int count = 0;
  static int mspf = 0;
  count++;
  if (count % 20 == 0) {
    mspf = DWT->CYCCNT / (168000 * 20);
    CPU_RESET_CYCLECOUNTER
    ;
  }
  char charCount[4];
  memset(charCount, 0, sizeof(charCount));
  uitoa(mspf, charCount, sizeof(charCount));
  font = gdispOpenFont("UI1");
  gdispGFillArea(pixmap, 0, 85, 30, 11, 0x0F00);
  gdispGDrawStringBox(pixmap, 0, 85, 30, 11, charCount, font, White,
                      justifyCenter);

  //copy fBuffer to display
  char* pixloc = (char*)gdispPixmapGetBits(pixmap);
  int i = 0;
  int pixels = 96 * 128;
  for (i = 0; i < pixels; i++) {
    char temp = pixloc[2 * i];
    pixloc[2 * i] = pixloc[2 * i + 1];
    pixloc[2 * i + 1] = temp;
  }
  pushBuffer(pixloc);

  //gdispBlitArea(0, 0, 128, 96, gdispPixmapGetBits(pixmap));
}

static int uitoa(unsigned int value, char * buf, int max) {
  int n = 0;
  int i = 0;
  unsigned int tmp = 0;

  if (!buf)
    return -3;

  if (2 > max)
    return -4;

  i = 1;
  tmp = value;
  if (0 > tmp) {
    tmp *= -1;
    i++;
  }
  for (;;) {
    tmp /= 10;
    if (0 >= tmp)
      break;
    i++;
  }
  if (i >= max) {
    buf[0] = '?';
    buf[1] = 0x0;
    return 2;
  }

  n = i;
  tmp = value;
  if (0 > tmp) {
    tmp *= -1;
  }
  buf[i--] = 0x0;
  for (;;) {
    buf[i--] = (tmp % 10) + '0';
    tmp /= 10;
    if (0 >= tmp) {
      break;
    }
  }
  if (-1 != i) {
    buf[i--] = '-';
  }

  return n;
}

void enterStandby() {
  //enter standby mode
  //chSysLock();
  //PWR->CR1 |= PWR_CR_CWUF | PWR_CR_CSBF;
  //    PWR->CR3 |= PWR_CR3_EWUP;
  //    PWR->CR1 |= PWR_CR1_LPMS_STANDBY;
  //    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  //    __WFI();

}

