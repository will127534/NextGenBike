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
#include "simple_ble.h"

volatile uint8_t brake = 0;

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
        // c0:98:e5:49:xx:xx
        .platform_id       = 0x49,    // used as 4th octect in device BLE address
        .device_id         = 0x0012, // TODO: replace with your lab bench number
        .adv_name          = "POV_BLE", // used in advertisements if there is room

        .adv_interval      = MSEC_TO_UNITS(1000, UNIT_0_625_MS),
        .min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),
        .max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),
};

//4607eda0-f65e-4d59-a9ff-84420d87a4ca
static simple_ble_service_t robot_service = {{
    .uuid128 = {0xca,0xa4,0x87,0x0d,0x42,0x84,0xff,0xA9,
                0x59,0x4D,0x5e,0xf6,0xa0,0xed,0x07,0x46}
}};

// TODO: Declare characteristics and variables for your service
static simple_ble_char_t brake_char = {.uuid16 = 0xEDA1};

simple_ble_app_t* simple_ble_app;

extern void ble_evt_write(ble_evt_t const* p_ble_evt) {
    // TODO: logic for each characteristic and related state changes
  // uint8_t* data = (p_ble_evt->evt).gap_evt.params.adv_report.data.p_data;
  uint8_t* data = (p_ble_evt -> evt).gatts_evt.params.write.data;
  brake = *data;
  printf("ble send brake = %d\n", brake);
}

// LED array
static uint8_t LEDS[3] = {NRF_GPIO_PIN_MAP(0,30),NRF_GPIO_PIN_MAP(0,31), BUCKLER_LED2};
volatile bool new_degree = 0;
volatile bool new_rpm = 0;
volatile bool rpmBrake = 0;
volatile float speed = 0;

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

void TIMER4_IRQHandler(void) {
  // This should always be the first line of the interrupt handler!
  // It clears the event so that it doesn't happen again
  NRF_TIMER4->EVENTS_COMPARE[0] = 0;
  NRF_TIMER4->CC[0]=read_timer()+degreeWidth;
  degreeCount+=1;
  if(degreeCount>= 360){
    degreeCount -= 360;
  } 
  new_degree = 1;
}

void GPIOTE_IRQHandler(void) {
    NRF_GPIOTE->EVENTS_IN[0] = 0;
    originalRPM = rpm;
  	rpm = 30*16000000/(read_timer() - oldTime);
    if(originalRPM-rpm>50||rpm<10)
      rpmBrake = 1;
    else
      rpmBrake = 0;

    speed = rpm*10*2*3.14*60/63360.0; 
    new_rpm = 1;
  	degreeWidth = (read_timer() - oldTime)/(picWidth);
  	degreeCount = 0;
    new_degree = 1;
  	NRF_TIMER4->TASKS_CLEAR = 1;
  	NRF_TIMER4->TASKS_START = 1;
  	oldTime = read_timer();//clear timer
  	NRF_TIMER4->CC[0]=degreeWidth-1;
  	//printf("RPM: %ld\n", rpm);
}

uint8_t char_array [34][360] = {0};

void write_LED(char c, int start, uint8_t color){
  char *fontB;
  fontB=font8x8_basic[c];//E
  //printf("Start=%d,%d\n",start,start+6);

  for(int i=start+12;i>=start;i-=2){
      for(int j=29;j>=15;j-=2){
        char_array[j][i]=(fontB[7-(j-15)/2]>>(6-(i-start)/2))&1;

        if(char_array[j][i] == 1){
          char_array[j][i] = color;
          char_array[j-1][i]= color;
          char_array[j-1][i-1]= color;
          char_array[j][i-1]= color;
        }
        else
        {
          char_array[j][i] = 0;
          char_array[j-1][i]= 0;
          char_array[j-1][i-1]= 0;
          char_array[j][i-1]= 0;
        }
      }
  }
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;
  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");


/*
  ble_version_t version;
  error_code = sd_ble_version_get(&version);
  APP_ERROR_CHECK(error_code);
  printf("%d\n",version.company_id);
  printf("%d\n",version.subversion_number);
*/

  //while(1){}

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

  /////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// BLE ////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////

  // Setup BLE

  simple_ble_app = simple_ble_init(&ble_config);
  //simple_ble_adv_only_name();
  printf("Added Brake characteristics\n");
  
  simple_ble_add_service(&robot_service);




  simple_ble_add_characteristic(1, 1, 1, 0,
      sizeof(brake), (uint8_t*) &brake,
      &robot_service, &brake_char);
  

  // Start Advertising
  simple_ble_adv_only_name();

  // char * test = "blah";
  // simple_ble_adv_manuf_data(test, strlen(test));
  // simple_ble_adv_manuf_data(brake, strlen(brake));


  /////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// BLE ////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////


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
  printf("done\n");

  while (1) {
    
    if(new_rpm){
      write_LED('H', 180+18*6,255/11*6);
      write_LED('P', 180+18*5,255/11*6);
      write_LED('M', 180+18*4,255/11*6);
    	write_LED('0'+(int)(speed*10)%10, 180+18*3,255/11*1);
      write_LED('.', 180+18*2,255/11*2);
      write_LED('0'+((int)speed%10), 180+18*1,255/11*3);
      write_LED('0'+((int)speed/10)%10, 180+18*0,255/11*4);




      write_LED('0'+(rpm/1)%10, 15,255/11*1);
      write_LED('0'+(rpm/10)%10, 15+18*1,255/11*2);
      write_LED('0'+(rpm/100)%10, 15+18*2,255/11*3);
      write_LED('=', 15+18*3,255/11*5);
      write_LED('M', 15+18*4,255/11*6);
      write_LED('P', 15+18*5,255/11*7);
      write_LED('R', 15+18*6,255/11*8);
      

      new_rpm = 0;
    }


    if(new_degree){
      //printf("new_degree %d\n", degreeCount);
      for (int i=0; i < 34; i++) {
        if(rpmBrake){
          SetPixelColor(i, Wheel(330));
        }
        else if(brake == 2){
          SetPixelColorRGB(i, left_R[degreeCount][i],left_G[degreeCount][i],left_B[degreeCount][i]);
        }
        else if(brake == 3){
          SetPixelColorRGB(i, right_R[degreeCount][i],right_G[degreeCount][i],right_B[degreeCount][i]);
        }
        else{
          uint8_t res=char_array[i][degreeCount];
          if(res!=0)
            SetPixelColor(i, Wheel(res));
          else{
            SetPixelColor(i,0);
          }
        }
      }
              
      PixelShow();
      new_degree = 0;
   } 
  }
}