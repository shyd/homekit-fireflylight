/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2020 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
////////////////////////////////////////////////////////////
//                                                        //
//    HomeSpan: A HomeKit implementation for the ESP32    //
//    ------------------------------------------------    //
//                                                        //
// Example 6: One working on/off LED and one working     //
//            dimmable LED, both based on the LightBulb   //
//            Service                                     //
//                                                        //
////////////////////////////////////////////////////////////


#include "HomeSpan.h" 
#include "DEV_LED.h"          

TaskHandle_t h_HK_poll;

void HK_poll(void * pvParameters);

void setup() {

  // Example 6 changes Example 5 so that LED #2 is now dimmable, instead of just on/off.  This requires us to create a new
  // derived Service we will name "DEV_DimmableLED"  Instead of creating a new file to store this definition, we will simply
  // tack it on to the end of the DEV_LED.h file that includes the code we created in Example 5 to control an on/off LED.
  // Grouping similar-style Services in one ".h" file makes it easier to organize your custom devices.

  // As usual, all previous comments have been deleted and only new changes from the previous example are shown.

  // NOTE: The Arduino/ESP32 code base does not include the function analogWrite() which is typically used to create a PWM
  // output to drive the brightness of an LED.  The ESP32 code base itself includes a set of functions to create PWM output
  // and the ESP32 chip has built-in PWM functionality specifically for this purpose.  There are numerous libraries
  // you can download that mimics or reproduces analogWrite() in some form or another.  HomeSpan conveniently comes with
  // it own version of a wrapper around the ESP32 PWM classes that make it very easy to define PWM "channel," attach a pin,
  // and set the PWM level (or duty cycle) from 0-100%.  These functions are encapsualted in the LedPin class, as defined in
  // extras/PwmPin.h.  We will include this file in our updated DEV_LED.h for use with DEV_DimmableLED.
  
  Serial.begin(115200);

  homeSpan.enableOTA();
  homeSpan.begin(Category::Lighting,"HomeSpan LEDs");
  
  new SpanAccessory(); 
  
    new Service::AccessoryInformation();    
      new Characteristic::Name("LED String 1");    
      new Characteristic::Manufacturer("Schuett"); 
      new Characteristic::SerialNumber("123-ABC");  
      new Characteristic::Model("Ikea Vissvass");   
      new Characteristic::FirmwareRevision(HOMESPAN_VERSION);  
      new Characteristic::Identify();             
      
    new Service::HAPProtocolInformation();          
      new Characteristic::Version("1.1.0");         
  
    new DEV_DimmableLED(33, LEDC_CHANNEL_0, LEDC_TIMER_0);        // NEW! create a dimmable (PWM-driven) LED attached to pin 17.  See new code at end of DEV_LED.h
    
  new SpanAccessory(); 
  
    new Service::AccessoryInformation();    
      new Characteristic::Name("LED String 2");    
      new Characteristic::Manufacturer("Schuett"); 
      new Characteristic::SerialNumber("123-ABC");  
      new Characteristic::Model("Ikea Vissvass");   
      new Characteristic::FirmwareRevision(HOMESPAN_VERSION);  
      new Characteristic::Identify();             
      
    new Service::HAPProtocolInformation();          
      new Characteristic::Version("1.1.0");         
  
    new DEV_DimmableLED(25, LEDC_CHANNEL_1, LEDC_TIMER_1);

  new SpanAccessory(); 
  
    new Service::AccessoryInformation();    
      new Characteristic::Name("LED String 3");    
      new Characteristic::Manufacturer("Schuett"); 
      new Characteristic::SerialNumber("123-ABC");  
      new Characteristic::Model("Ikea Vissvass");   
      new Characteristic::FirmwareRevision(HOMESPAN_VERSION);  
      new Characteristic::Identify();             
      
    new Service::HAPProtocolInformation();          
      new Characteristic::Version("1.1.0");         
  
    new DEV_DimmableLED(26, LEDC_CHANNEL_2, LEDC_TIMER_2);

    xTaskCreatePinnedToCore(
                      HK_poll,    /* Task function. */
                      "HK_poll",  /* name of task. */
                      10000,       /* Stack size of task */
                      NULL,        /* parameter of the task */
                      1,           /* priority of the task */
                      &h_HK_poll, /* Task handle to keep track of created task */
                      0);          /* pin task to core 0 */  
    
    delay(1000);
 
} // end of setup()

//////////////////////////////////////

void HK_poll(void * pvParameters){

  while(1){
    homeSpan.poll();
  }   //loop
}     //task

void loop(){
  
  //homeSpan.poll();
  
} // end of loop()