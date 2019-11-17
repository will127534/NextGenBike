// Blink app
//
// Blinks the LEDs on Buckler

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
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
#include "APA102.h"

// LED array
static uint8_t LEDS[3] = {BUCKLER_SD_MOSI,BUCKLER_SD_MISO, BUCKLER_LED2};


uint32_t Wheel(uint8_t WheelPos){
    float angle = (float)WheelPos/255.0 * 360;
    
    float red, green, blue;

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







int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // configure leds
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  for (int i=0; i<3; i++) {
    error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
    APP_ERROR_CHECK(error_code);
  }
  printf("Code initialized\n");

  // loop forever
	int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
	uint32_t color = 0xFF0000;      // 'On' color (starts red)


  while (1) {
  rainbow(5);
  rainbowCycle(5);
  theaterChaseRainbow(5);
  /*
	  SetPixelColor(head, color); // 'On' pixel at head
	  SetPixelColor(tail, 0);     // 'Off' pixel at tail
	  PixelShow();                     // Refresh strip
	  nrf_delay_ms(10);                   // Pause 20 milliseconds (~50 FPS)

	  if(++head >= 72) {         // Increment head index.  Off end of strip?
	    head = 0;                       //  Yes, reset head index to start
	    if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
	      color = 0xFF0000;             //   Yes, reset to red
	  }
	  if(++tail >= 72) tail = 0; // Increment, reset tail index
	 */
	
  }
}

