
////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

CUSTOM_CHAR(Frequency, ac9e2a89-78fa-4279-9d0e-8873fdb30d3f, PR+PW+EV, UINT8, 120, 0, 240, false);
CUSTOM_CHAR(Inverted, ec2fa3e5-24d0-4e53-a620-8e6348e1cc23, PR+PW+EV, BOOL, 0, 0, 1, false);

#include "extras/PwmPin.h"                          // NEW! Include this HomeSpan "extra" to create LED-compatible PWM signals on one or more pins
#include <driver/ledc.h>


#define LEDC_DUTY_MAX        (8192-1)
#define LEDC_TEST_FADE_TIME    (500)

void initiateFade();

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
static bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    portBASE_TYPE taskAwoken = pdFALSE;

    if (param->event == LEDC_FADE_END_EVT) {
        SemaphoreHandle_t s = (SemaphoreHandle_t) user_arg;
        xSemaphoreGiveFromISR(s, &taskAwoken);
    }

    return (taskAwoken == pdTRUE);
}

// Here's the new code defining DEV_DimmableLED - changes from above are noted in the comments

struct DEV_DimmableLED : Service::LightBulb {       // Dimmable LED
  SpanCharacteristic *power;                        // reference to the On Characteristic
  SpanCharacteristic *level;                        // NEW! Create a reference to the Brightness Characteristic instantiated below
  SpanCharacteristic *freq;
  SpanCharacteristic *inverted;

  float fLevel = 0.0;

  uint32_t duty = 0;
  SemaphoreHandle_t semaphore = NULL;

    /*
    * Prepare and set configuration of timers
    * that will be used by LED Controller
    */
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
      .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
      .timer_num = LEDC_TIMER_0,            // timer index
      .freq_hz = 5000,                      // frequency of PWM signal
      .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
  };

  /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
    ledc_channel_config_t ledc_channel =
    {
          .gpio_num   = 33,
          .speed_mode = LEDC_LOW_SPEED_MODE,
          .channel    = LEDC_CHANNEL_0,
          .timer_sel  = LEDC_TIMER_0,
          .duty       = 0,
          .hpoint     = 0,
          .flags = {.output_invert = 0}
      };
  
  DEV_DimmableLED(int pin, ledc_channel_t channel, ledc_timer_t timer) : Service::LightBulb(){       // constructor() method

  /*
    LedPin=33: mode=0 channel=0, timer=0, freq=5000 Hz, resolution=13 bits
    LedPin=25: mode=0 channel=1, timer=0, freq=5000 Hz, resolution=13 bits
    LedPin=26: mode=0 channel=2, timer=0, freq=5000 Hz, resolution=13 bits
  */

  ledc_timer.timer_num = timer;
  ledc_channel.gpio_num = pin;
  ledc_channel.channel = channel;
  ledc_channel.timer_sel = timer;

    power=new Characteristic::On();     
    freq=new Characteristic::Frequency(12, true);
    freq->setDescription("Frequency");
    freq->setRange(1,255,1);
    inverted=new Characteristic::Inverted(0, true);
    inverted->setDescription("Inverted");
    level=new Characteristic::Brightness(50, true);       // NEW! Instantiate the Brightness Characteristic with an initial value of 50% (same as we did in Example 4)
    level->setRange(1,100,1);                       // NEW! This sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1% (different from Example 4 values)

    semaphore = xSemaphoreCreateBinary(); // to sync the loop function

    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);

    // Initialize fade service.
    ledc_fade_func_install(0);
    ledc_cbs_t callbacks = {
        .fade_cb = cb_ledc_fade_end_event
    };
    
    ledc_cb_register(ledc_channel.speed_mode, ledc_channel.channel, &callbacks, (void *) semaphore);

    initiateFade();
    
  } // end constructor

  boolean update(){                              // update() method

    // Here we set the brightness of the LED by calling ledPin->set(brightness), where brightness=0-100.
    // Note HomeKit sets the on/off status of a LightBulb separately from its brightness, which means HomeKit
    // can request a LightBulb be turned off, but still retains the brightness level so that it does not need
    // to be resent once the LightBulb is turned back on.
    
    // Multiplying the newValue of the On Characteristic ("power", which is a boolean) with the newValue of the
    // Brightness Characteristic ("level", which is an integer) is a short-hand way of creating the logic to
    // set the LED level to zero when the LightBulb is off, or to the current brightness level when it is on.
    
    //ledPin->set(power->getNewVal()*level->getNewVal());    
    fLevel = ((float)LEDC_DUTY_MAX)*((float)(power->getNewVal()*level->getNewVal()))/100.0f;

    return(true);                               // return true
  
  } // update

  void initiateFade() {
    ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, duty, freq->getVal()*100);
    ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel, LEDC_FADE_NO_WAIT);
  }

  void loop() {
    if(semaphore != NULL){
      if(xSemaphoreTake(semaphore, 0)) {
        if(duty == 0) {
          duty = (uint32_t)fLevel;
        } else {
          duty = 0;
        }
        
        if(ledc_channel.flags.output_invert != inverted->getVal()) {
          ledc_channel.flags.output_invert = inverted->getVal();
          ledc_channel_config(&ledc_channel);
        }

        initiateFade();
      }
    }
  }
};
      
//////////////////////////////////