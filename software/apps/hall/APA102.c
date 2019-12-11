#include <APA102.h>


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "buckler.h"
#include "nrf_drv_spi.h"


uint8_t g_rOffset = (DOTSTAR_BGR & 3);

uint8_t g_gOffset = ((DOTSTAR_BGR >> 2) & 3);

uint8_t g_bOffset = ((DOTSTAR_BGR >> 4) & 3);



uint16_t g_NumLEDs = 35;                 // Number of pixels

uint8_t g_PixelsDataArray[35*3] = {0};       // LED RGB values (3 bytes ea.)



void sw_spi_out(uint8_t n) { // Bitbang SPI write

  for(uint8_t i=8; i--; n <<= 1) {

    if(n & 0x80)  nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,30));

    else          nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(0,30));

    nrf_delay_us(5);
     nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,31));
    nrf_delay_us(5);
     nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(0,31));


  }
}




void PixelShow(void) {



  if(!g_PixelsDataArray) return;



  uint8_t *ptr = &g_PixelsDataArray[0];

  uint8_t i;            // -> LED data

  uint16_t n   = g_NumLEDs;              // Counter

  uint16_t b16 = (uint16_t)g_PixelBrightness; // Type-convert for fixed-point math



  // Soft (bitbang) SPI

    for(i=0; i<4; i++) sw_spi_out(0);    // Start-frame marker

    if(g_PixelBrightness) {                     // Scale pixel brightness on output

      do {                               // For each pixel...

        sw_spi_out(0xE3);                //  Pixel start

        for(i=0; i<3; i++) sw_spi_out((*ptr++ * b16) >> 8); // Scale, write

      } while(--n);

    } else {                             // Full brightness (no scaling)

      do {                               // For each pixel...

        sw_spi_out(0xE3);                //  Pixel start

        for(i=0; i<3; i++) sw_spi_out(*ptr++); // R,G,B

      } while(--n);

    }

    for(i=0; i<4; i++) sw_spi_out(0xFF);



}



void ClearPixels() { // Write 0s (off) to full pixel buffer

  memset(g_PixelsDataArray, 0, g_NumLEDs * 3);

}



void SetPixelColorRGB(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {

  if(n < g_NumLEDs) {

    uint8_t *p = &g_PixelsDataArray[n * 3];

    p[g_rOffset] = r;

    p[g_gOffset] = g;

    p[g_bOffset] = b;

  }

}



void SetPixelColor(uint16_t n, uint32_t c) {

  if(n < g_NumLEDs) {

    uint8_t *p = &g_PixelsDataArray[n * 3];

    p[g_rOffset] = (uint8_t)(c >> 16);

    p[g_gOffset] = (uint8_t)(c >>  8);

    p[g_bOffset] = (uint8_t)c;

  }

}