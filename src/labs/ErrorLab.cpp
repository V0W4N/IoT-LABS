//#define ERROR_LAB

#ifdef ERROR_LAB


#include <Arduino.h>
#include "utils/error_led_utils.h"
#include "configs/system_config.h"


void setup()
{
    defaultErrorLed.blinkForever(LED_ERROR_BLINK_FOREVER_DELAY);
}

void loop()
{
    
}

#endif
