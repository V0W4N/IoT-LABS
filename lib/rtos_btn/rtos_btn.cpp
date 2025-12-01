#include "rtos_btn.h"

// Static task function for button monitoring
void RTOSButton::monitorTaskFunction(void* pvParameters) {
    RTOSButton* rtosBtn = static_cast<RTOSButton*>(pvParameters);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Take mutex to protect button state access
        if (xSemaphoreTake(rtosBtn->mutex, portMAX_DELAY) == pdTRUE) {
            rtosBtn->button->checkState();
            xSemaphoreGive(rtosBtn->mutex);
        }
        
        vTaskDelayUntil(&lastWakeTime, rtosBtn->updatePeriod);
    }
}

RTOSButton::RTOSButton(int pin, bool pullup)
    : button(nullptr), mutex(nullptr), monitorTask(nullptr), 
      updatePeriod(pdMS_TO_TICKS(10)), initialized(false) {
    
    // Create button instance
    button = new ButtonUtils(pin, pullup);
    
    // Create mutex for thread-safe access
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        // Mutex creation failed
        delete button;
        button = nullptr;
    }
}

RTOSButton::~RTOSButton() {
    stop();
    
    if (mutex != nullptr) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
    
    if (button != nullptr) {
        delete button;
        button = nullptr;
    }
}

bool RTOSButton::start(TickType_t updatePeriodMs, UBaseType_t priority) {
    if (initialized || button == nullptr || mutex == nullptr) {
        return false;
    }
    
    updatePeriod = updatePeriodMs;
    
    // Create monitoring task
    BaseType_t result = xTaskCreate(
        monitorTaskFunction,
        "BtnMonitor",
        128,  // Stack size in words
        this,
        priority,
        &monitorTask
    );
    
    if (result == pdPASS) {
        initialized = true;
        return true;
    }
    
    return false;
}

void RTOSButton::stop() {
    if (!initialized) {
        return;
    }
    
    if (monitorTask != nullptr) {
        vTaskDelete(monitorTask);
        monitorTask = nullptr;
    }
    
    initialized = false;
}

bool RTOSButton::isPressed() {
    if (button == nullptr || mutex == nullptr) {
        return false;
    }
    
    bool pressed = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        pressed = button->checkState();
        xSemaphoreGive(mutex);
    }
    
    return pressed;
}

bool RTOSButton::consumePress() {
    if (button == nullptr || mutex == nullptr) {
        return false;
    }
    
    bool consumed = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        consumed = button->consumePress();
        xSemaphoreGive(mutex);
    }
    
    return consumed;
}

int RTOSButton::getPressCount() {
    if (button == nullptr || mutex == nullptr) {
        return 0;
    }
    
    int count = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        count = button->getPressCount();
        xSemaphoreGive(mutex);
    }
    
    return count;
}

void RTOSButton::resetPressCount() {
    if (button == nullptr || mutex == nullptr) {
        return;
    }
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        button->resetPressCount();
        xSemaphoreGive(mutex);
    }
}

void RTOSButton::setDebounceDelay(unsigned long delayMs) {
    if (button == nullptr || mutex == nullptr) {
        return;
    }
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        button->setDebounceDelay(delayMs);
        xSemaphoreGive(mutex);
    }
}

