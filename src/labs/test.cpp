//#define LAB_TEST

#ifdef LAB_TEST

#include <Arduino.h>
#include "uart_io_fix.h"
#include "interactive_scanf.h"
#include "config.h"


void setup()
{
    initUartIOFixESP32();
    Serial.begin(SERIAL_BAUD_RATE);
    printf("Test Lab started");
}


void loop()
{
    char buffer[32];

    interactiveScanf(buffer, 32, "Enter command: ");

    //scanf(SCANF_READ_TEXT_FORMAT, buffer); 
    // Поток не лочится в принципе, а просто выдаёт постоянно резудьтат буффера
    // Может работать параллельно c delay и другими функциями
    // Если в буффере ничего нет, всё равно выдаёт пустую строку и не блокирует выполнение
    // Если в буффере есть данные, выдаёт их
    // Очищается буффер по символу новой строки \n при выдаче данных

    //printf("\nCommand result %s\n", buffer);
    delay(1000);
    
}



#endif