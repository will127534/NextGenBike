#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"

#include "buckler.h"
#include "US5881LUA.h"
#include "APA102.h"
#include "font8x8_basic.h"
#include "image.h"


// LED array
static uint8_t LEDS[3] = {NRF_GPIO_PIN_MAP(0,30),NRF_GPIO_PIN_MAP(0,31), BUCKLER_LED2};
volatile bool new_degree = 0;
volatile bool new_rpm = 0;
uint32_t Wheel(uint32_t WheelPos){
    float angle = (float)WheelPos;
    
    float red, green, blue;
    if (angle == 70){
      red = 255;
      green = 255;
      blue = 255;
    }
    if (angle<60) {red = 255; green = (angle*4.25-0.01); blue = 0;} else
    if (angle<120) {red = ((120.0-angle)*4.25-0.01); green = 255; blue = 0;} else
    if (angle<180) {red = 0, green = 255; blue = ((angle-120.0)*4.25-0.01);} else
    if (angle<240) {red = 0, green = ((240-angle)*4.25-0.01); blue = 255;} else
    if (angle<300) {red = ((angle-240.0)*4.25-0.01), green = 0; blue = 255;} else
                   {red = 255, green = 0; blue = ((360.0-angle)*4.25-0.01);}
    //printf("%f,%f,%f,%f\n",angle,red,green,blue);


    uint32_t r,g,b;
    r = red;
    g = green;
    b = blue;
    return r<<16|g<<8|b;
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<72; i++) {
      SetPixelColor(i, c);
      PixelShow();
      nrf_delay_ms(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<72; i++) {
      SetPixelColor(i, Wheel((i+j) & 255));
    }
    PixelShow();
    nrf_delay_ms(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< 72; i++) {
      SetPixelColor(i, Wheel(((i * 256 / 72) + j) & 255));
    }
    PixelShow();
    nrf_delay_ms(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < 72; i=i+3) {
        SetPixelColor(i+q, c);    //turn every third pixel on
      }
      PixelShow();

      nrf_delay_ms(wait);

      for (int i=0; i < 72; i=i+3) {
        SetPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < 72; i=i+3) {
          SetPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        PixelShow();

        nrf_delay_ms(wait);

        for (int i=0; i < 72; i=i+3) {
          SetPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}






void TIMER4_IRQHandler(void) {
  // This should always be the first line of the interrupt handler!
  // It clears the event so that it doesn't happen again
  NRF_TIMER4->EVENTS_COMPARE[0] = 0;
  // if(degreeCount<picWidth)
  //      printf("degree %ld\n",degreeCount);
  NRF_TIMER4->CC[0]=read_timer()+degreeWidth;
  if(degreeCount>= 360){
    degreeCount -= 360;
  }
  degreeCount+=1;
  new_degree = 1;

  // Place your interrupt handler code here

}

void GPIOTE_IRQHandler(void) {
    NRF_GPIOTE->EVENTS_IN[0] = 0;
    //printf("interrupt\n");
    // nrf_gpio_pin_toggle(BUCKLER_LED0);
  	// printf("old time:%ld\n",oldTime);
  	rpm = 30*16000000/(read_timer() - oldTime);
    new_rpm = 1;
  	degreeWidth = (read_timer() - oldTime)/(picWidth);
  	// printf("total time for one revolution:%ld\n",(read_timer()-oldTime));
  	degreeCount = 0;
    new_degree = 1;
  	NRF_TIMER4->TASKS_CLEAR = 1;
  	NRF_TIMER4->TASKS_START = 1;
  	oldTime = read_timer();//clear timer
  	NRF_TIMER4->CC[0]=degreeWidth-1;
  	//printf("RPM: %ld\n", rpm);
}
uint8_t char_array [35][360] = {0};
void write_LED(char c, int start, uint8_t color){
  char *fontB;
  fontB=font8x8_basic[c];//E
  //printf("Start=%d,%d\n",start,start+6);
  for(int i=start+6;i>=start;i--){
      for(int j=30;j>=23;j--){
        char_array[j][i]=(fontB[7-(j-23)]>>(6-(i-start)))&1;
        if(char_array[j][i] == 1){
          char_array[j][i] = color;
        }
      }
  }
}

// void write_LED_string(char str[], int size, int start, uint8_t color){
//   for(int =0;i<size;i++){ 
//     char c=str[i];
//     char *fontB;
//      fontB=font8x8_basic[c];//E
//      //printf("Start=%d,%d\n",start,start+6);
//      for(int i=start+6;i>=start;i--){
//          for(int j=30;j>=23;j--){
//            char_array[j][i]=(fontB[7-(j-23)]>>(6-(i-start)))&1;
//            if(char_array[j][i] == 1){
//              char_array[j][i] = color;
//            }
//          }
//      }
//   }
// }

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;
  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");
  NVIC_EnableIRQ(GPIOTE_IRQn);
  NVIC_SetPriority(GPIOTE_IRQn,0);

  NRF_GPIOTE->CONFIG[0] |= 0x1D00; // input
  NRF_GPIOTE->CONFIG[0] |= 0x20000; // HiToLO
  NRF_GPIOTE->CONFIG[0] |= 1;//event mode
  NRF_GPIOTE->INTENSET |=1;//interrupt
  for (int i=0; i<3; i++) {
    nrf_gpio_cfg_output(LEDS[i]);
  }
  nrf_gpio_cfg_input(NRF_GPIO_PIN_MAP(0,29),NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,3));
  nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(0,2));
  nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,2));


  // You can use the NRF GPIO library to test your interrupt
  //nrf_gpio_pin_dir_set(BUCKLER_LED0, NRF_GPIO_PIN_DIR_OUTPUT);

  nrf_delay_ms(1000); 

  // Don't forget to initialize your timer library
  virtual_timer_init();
  setup();

  /*
  char *fontA;
  fontA=font8x8_basic[72];//H
  char *fontB;
  fontB=font8x8_basic[69];//E

  for(int j=22;j>=15;j--){
    for(int i=10;i<17;i++){
      char_array[j][i]=(fontA[8-(j-14)]>>(i-10))&1;
      printf("%d", char_array[j][i]);
    }
    for(int i=20;i<27;i++){
      char_array[j][i]=(fontB[8-(j-14)]>>(i-20))&1;
      printf("%d", char_array[j][i]);
    }
  }

*/
  // write_LED('D', 7,255/11*1);
  // write_LED('L', 7+8*1,255/11*2);
  // write_LED('R', 7+8*2,255/11*3);
  // write_LED('O', 7+8*3,255/11*4);
  // write_LED('W', 7+8*4,255/11*5);
  // write_LED(',', 7+8*5,255/11*6);
  // write_LED('O', 7+8*6,255/11*7);
  // write_LED('L', 7+8*7,255/11*8);
  // write_LED('L', 7+8*8,255/11*9);
  // write_LED('E', 7+8*9,255/11*10);
  // write_LED('H', 7+8*10,255/11*11);
  // printf("done\n");
  // write_LED(",", 47);
  // write_LED("W", 55);
  
  //while(1);
  //  font=font8x8_basic[76];//L
  // for(int i=10;i<17;i++){
  //   for(int j=27;j<35;j++){
  //     char_array[i][j]=(font[j-27]>>(i-10))&1;
  //   }
  // }
  
  // loop forever
   // printf("Time: %d\n", oldTime);
  //uint32_t count = 0;

  while (1) {
    
    // if(new_rpm){
    //   write_LED('0'+(rpm/1)%10, 60,255/11*1);
    //   write_LED('0'+(rpm/10)%10, 60+8*1,255/11*2);
    //   write_LED('0'+(rpm/100)%10, 60+8*2,255/11*3);
    //   write_LED('0'+(rpm/1000)%10, 60+8*3,255/11*4);
    //   write_LED('=', 60+8*4,255/11*5);
    //   write_LED('M', 60+8*5,255/11*6);
    //   write_LED('P', 60+8*6,255/11*7);
    //   write_LED('R', 60+8*7,255/11*8);
    //   new_rpm = 0;
    // }

    /*
      for (int i=0; i < 35; i++) {
           SetPixelColor(i, Wheel(count%360));
           count ++;
      }
      int count=read_timer();
      PixelShow();
      printf("%d\n", read_timer()-count);
      nrf_delay_ms(100); 
      */
    if(new_degree){
      //printf("new_degree %d\n", degreeCount);
      // if(degreeCount == 180){
      // for (int i=0; i < 35; i++) {
      //     SetPixelColor(i, Wheel(280));
      //   }
      //   PixelShow();
      // }
      // if(degreeCount == 270){
      // for (int i=0; i < 35; i++) {
      //     SetPixelColor(i, Wheel(200));
      //   }
      //   PixelShow();
      // }
      // if(degreeCount == 90){
      // for (int i=0; i < 35; i++) {
      //     SetPixelColor(i, Wheel(30));
      //   }
      //   PixelShow();
      // }
      // if(degreeCount == 10){
      // for (int i=0; i < 35; i++) {
      //     SetPixelColor(i, Wheel(70));
      //   }
      //   PixelShow();
      // }
      // else{
      for (int i=0; i < 35; i++) {
        // uint8_t res=char_array[i][degreeCount];
        //  if(res!=0)
        //   SetPixelColor(i, Wheel(res));
        //  else{
        //    //SetPixelColorRGB(i, res*255,res*255,res*255);
        //   SetPixelColor(i,0);
        //  }
        // if(degreeCount==0)
        //   SetPixelColor(i, Wheel(330));
        // else if (degreeCount==20)
        // {
        //   /* code */
        //   SetPixelColor(i, Wheel(10));
        // }
        // else
        //   SetPixelColor(i,0);
        SetPixelColorRGB(i, PIC1_Red[degreeCount][i],PIC1_Green[degreeCount][i],PIC1_Blue[degreeCount][i]);
        
      }
      PixelShow();

      // }

      new_degree = 0;
    }
  }
}