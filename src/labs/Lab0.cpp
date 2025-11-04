//#define LAB_0
#ifdef LAB_0

#include <Arduino.h>
#include "my_led.h"
#include "my_btn.h"
#include "milis_utils.h"
#include "config.h"

#define LED_PIN 13
#define BUTTON_PIN DEFAULT_BUTTON_PIN


#define SHOW_OFF

// Lab 0: LED state change application with button press

LedUtils led(LED_PIN);
ButtonUtils button(BUTTON_PIN);

void setup() {
    // No initialization needed - constructors already configured pins
}

int mil = 0;
void loop() {
    #ifdef SHOW_OFF
        executePeriodicallyRef(mil, 50, [](){
            button.update();
            if (button.btnPressed())
                led.toggle();    
        });
    #else
        button.update(); 
        if (button.btnPressed())
            led.toggle();    
        delay(50); //Debounce time: 50 ms
    #endif
}

#endif
