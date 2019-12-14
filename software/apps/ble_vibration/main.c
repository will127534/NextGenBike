// Robot Template app
//
// Framework for creating applications that control the Kobuki robot

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_spi.h"
#include "nrf_serial.h"
#include "nrf_twi_mngr.h"
#include "nrfx_gpiote.h"

#include "buckler.h"
#include "display.h"
#include "kobukiActuator.h"
#include "kobukiSensorPoll.h"
#include "kobukiSensorTypes.h"
#include "kobukiUtilities.h"
#include "mpu9250.h"
#include "simple_ble.h"
#include "Adafruit_DRV2605.h"

#include "bike_break.h"

// LED and button array
//static uint8_t LEDS[2] = {BUCKLER_LED0, BUCKLER_LED1};
static uint8_t LEDS[2] = {NRF_GPIO_PIN_MAP(0, 24), NRF_GPIO_PIN_MAP(0, 25)};
static uint8_t BUTTONS[2] = {NRF_GPIO_PIN_MAP(0, 3), NRF_GPIO_PIN_MAP(0, 4)};
static bool turned = false;

// I2C manager
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

// global variables
typedef enum {
  IDLE,
  RIGHT_TURN,
  LEFT_TURN,
  ARRIVED
} states;

volatile uint8_t direction = 0;
static uint8_t brake = 0;

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
        // c0:98:e5:49:xx:xx
        .platform_id       = 0x49,    // used as 4th octect in device BLE address
        .device_id         = 0x0001, // TODO: replace with your lab bench number
        .adv_name          = "BIKE", // used in advertisements if there is room
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
static simple_ble_char_t direction_char = {.uuid16 = 0xEDA1};
// static simple_ble_char_t brake_char = {.uuid16 = 0xAEEF};
static simple_ble_char_t brake_char = {.uuid16 = 0xEDA2};

simple_ble_app_t* simple_ble_app;

extern void ble_evt_write(ble_evt_t const* p_ble_evt) {
    // TODO: logic for each characteristic and related state changes
  // uint8_t* data = (p_ble_evt->evt).gap_evt.params.adv_report.data.p_data;
  uint8_t* data = (p_ble_evt -> evt).gatts_evt.params.write.data;
  direction = *data;
  printf("direction = %d\n", direction);
}

void test(uint8_t a){
  brake = a;
  // direction = a;
  printf("brake = %d\n", brake);
  // simple_ble_notify_char(&direction_char);
  simple_ble_notify_char(&brake_char);
}

void print_state(states current_state){
	switch(current_state){
	case IDLE:
		display_write("IDLE", DISPLAY_LINE_0);
		break;
  case RIGHT_TURN:
    display_write("RIGHT TURN", DISPLAY_LINE_0);
    break;
  case LEFT_TURN:
    display_write("LEFT TURN", DISPLAY_LINE_0);
    break;
  case ARRIVED:
    display_write("ARRIVED", DISPLAY_LINE_0);
    break;
    }
}


// handler called whenever an input pin BUTTON changes
void pin_change_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  switch(pin) {
    case NRF_GPIO_PIN_MAP(0, 3): {
      printf("here0\n");
      if (!nrfx_gpiote_in_is_set(BUTTONS[0])) {
        if(nrf_gpio_pin_out_read(LEDS[0]) == 1) {
          nrfx_gpiote_out_clear(LEDS[0]);
        } else {
          nrfx_gpiote_out_set(LEDS[0]);
        }
      }
      break;
    }

    case NRF_GPIO_PIN_MAP(0, 4): {
      printf("here1\n");
      if (!nrfx_gpiote_in_is_set(BUTTONS[1])) {
        if(nrf_gpio_pin_out_read(LEDS[1]) == 1) {
          nrfx_gpiote_out_clear(LEDS[1]);
        } else {
          nrfx_gpiote_out_set(LEDS[1]);
        }
      }
      break;
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

  /////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// BLE ////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////

  // Setup BLE
  simple_ble_app = simple_ble_init(&ble_config);

  simple_ble_add_service(&robot_service);

  // TODO: Register your characteristics
  simple_ble_add_characteristic(1, 1, 0, 0,
      sizeof(direction), (uint8_t*) &direction,
      &robot_service, &direction_char);
  printf("Added Direction characteristics\n");

  simple_ble_add_characteristic(1, 1, 1, 0,
      sizeof(brake), (uint8_t*) &brake,
      &robot_service, &brake_char);
  printf("Added Brake characteristics\n");

  // Start Advertising
  simple_ble_adv_only_name();

  // char * test = "blah";
  // simple_ble_adv_manuf_data(test, strlen(test));
  // simple_ble_adv_manuf_data(brake, strlen(brake));

  /////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// BLE ////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////



  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  nrf_gpio_pin_dir_set(BUCKLER_GROVE_D0, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(BUCKLER_GROVE_D1, NRF_GPIO_PIN_DIR_OUTPUT);

  // configure leds
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  for (int i=0; i<2; i++) {
    error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
    APP_ERROR_CHECK(error_code);
    nrfx_gpiote_out_clear(LEDS[i]);
  }


  // initialize display
  nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);
  nrf_drv_spi_config_t spi_config = {
    .sck_pin = BUCKLER_LCD_SCLK,
    .mosi_pin = BUCKLER_LCD_MOSI,
    .miso_pin = BUCKLER_LCD_MISO,
    .ss_pin = BUCKLER_LCD_CS,
    .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
    .orc = 0,
    .frequency = NRF_DRV_SPI_FREQ_4M,
    .mode = NRF_DRV_SPI_MODE_2,
    .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
  };
  error_code = nrf_drv_spi_init(&spi_instance, &spi_config, NULL, NULL);
  APP_ERROR_CHECK(error_code);
  display_init(&spi_instance);
  printf("Display initialized!\n");

  // initialize i2c master (two wire interface)
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = BUCKLER_SENSORS_SCL;
  i2c_config.sda = BUCKLER_SENSORS_SDA;
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  APP_ERROR_CHECK(error_code);

  mpu9250_init(&twi_mngr_instance);
  printf("IMU initialized!\n");

  begin(&twi_mngr_instance);
  printf("MPU-9250 initialized\n");

    //init bike state
  error_code = init_bike_state(5, 70, 5);
  APP_ERROR_CHECK(error_code);
  printf("Bike State initialized!\n");
    // configure two gpio buttons
  // input pin, trigger on releasing
  nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;
  error_code = nrfx_gpiote_in_init(BUTTONS[0], &in_config, pin_change_handler);
  nrfx_gpiote_in_event_enable(BUTTONS[0], true);

  in_config.pull = NRF_GPIO_PIN_PULLUP;
  error_code = nrfx_gpiote_in_init(BUTTONS[1], &in_config, pin_change_handler);
  nrfx_gpiote_in_event_enable(BUTTONS[1], true);

  APP_ERROR_CHECK(error_code);
  printf("initialized!\n");

  selectLibrary(1);

  states state = IDLE;

  // I2C trigger by sending 'go' command 

  // default, internal trigger when sending GO command

  setMode(DRV2605_MODE_EXTTRIGEDGE); 
  // loop forever
  uint8_t effect = 14;

  // loop forever, running state machine
  while (1) {
    brake = 0;
    // TODO: complete state machine
    switch(state){
      case(IDLE): {
        print_state(state);
        if(direction == 1){
          state = RIGHT_TURN;
        }else if(direction == 2){
          state = LEFT_TURN;
        }else if(direction == 3){
          state = ARRIVED;
        }else{
          state = IDLE;
        }

        break;
      }
      case(RIGHT_TURN): {
        
        print_state(state);
        setWaveform(0, effect);  // play effect 
        setWaveform(1, 0);       // end waveform

        // go(); // play the effect!
        nrf_gpio_pin_set(BUCKLER_GROVE_D1);
        nrf_delay_ms(3000);
        nrf_gpio_pin_clear(BUCKLER_GROVE_D1);
        direction = 0;

        state = IDLE;
        break;
      }
      case(LEFT_TURN): {
        
        print_state(state);
        setWaveform(0, effect);  // play effect 
        setWaveform(1, 0);       // end waveform
        // go(); // play the effect!
        nrf_gpio_pin_set(BUCKLER_GROVE_D0);
        nrf_delay_ms(3000);
        nrf_gpio_pin_clear(BUCKLER_GROVE_D0);
        direction = 0;

        state = IDLE;
        break;
      }
      case(ARRIVED): {
        print_state(state);
        setWaveform(0, effect);  // play effect 
        setWaveform(1, 0);       // end waveform
        // go(); // play the effect!
        nrf_gpio_pin_set(BUCKLER_GROVE_D0);
        nrf_gpio_pin_set(BUCKLER_GROVE_D1);
        nrf_delay_ms(3000);
        nrf_gpio_pin_clear(BUCKLER_GROVE_D0);
        nrf_gpio_pin_clear(BUCKLER_GROVE_D1);
        direction = 0;
        state = IDLE;

        test(5);
        break;
      }
    }


    //bike
    int x = read_bike_state();
    switch(x){
      case(0):{
        display_write("", DISPLAY_LINE_1);
        if(turned){
          nrfx_gpiote_out_clear(LEDS[0]);
          nrfx_gpiote_out_clear(LEDS[1]);
        }
        turned = false;
        break;
      }

      case(1):{
        display_write("LEFT", DISPLAY_LINE_1);
        turned = true;
        break;
      }

      case(2):{
        display_write("RIGHT", DISPLAY_LINE_1);
        turned = true;
        break;
      }

      case(10):{
        display_write("     BRAKE", DISPLAY_LINE_1);
        if(turned){
          nrfx_gpiote_out_clear(LEDS[0]);
          nrfx_gpiote_out_clear(LEDS[1]);
        }
        turned = false;
        break;
      }

      case(11):{
        display_write("LEFT BRAKE", DISPLAY_LINE_1);
        turned = true;
        break;
      }

      case(12):{
        display_write("     BRAKE RIGHT", DISPLAY_LINE_1);
        turned = true;
        break;
      }
    }
    // printf("here\n");
  }
}

