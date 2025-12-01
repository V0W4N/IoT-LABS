#include "my_servo.h"

// Internal Servo object storage (one per ServoMotor instance)
// Using a simple static array approach for C compatibility
static Servo gServoObjects[12];  // Max 12 servos on most Arduinos
static bool gServoSlotUsed[12] = {false};

// Helper to get servo slot index from pin
static int8_t getServoSlot(uint8_t pin) {
    for (uint8_t i = 0; i < 12; i++) {
        if (!gServoSlotUsed[i]) continue;
        // Check if this slot is attached to the pin
        if (gServoObjects[i].attached()) {
            return i;
        }
    }
    return -1;
}

static int8_t allocateServoSlot() {
    for (uint8_t i = 0; i < 12; i++) {
        if (!gServoSlotUsed[i]) {
            gServoSlotUsed[i] = true;
            return i;
        }
    }
    return -1;
}

static void freeServoSlot(int8_t slot) {
    if (slot >= 0 && slot < 12) {
        gServoSlotUsed[slot] = false;
    }
}

// Store slot index in servo structure (using pin as lookup isn't reliable)
// We'll use a simple map approach
static int8_t gPinToSlot[70] = {-1};  // Assuming max 70 pins

static int16_t constrain_angle(int16_t angle, int16_t minA, int16_t maxA) {
    if (angle < minA) return minA;
    if (angle > maxA) return maxA;
    return angle;
}

void servo_init(ServoMotor* servo, uint8_t pin) {
    servo_init_custom(servo, pin, 544, 2400);
}

void servo_init_custom(ServoMotor* servo, uint8_t pin, uint16_t minPulseUs, uint16_t maxPulseUs) {
    servo->pin = pin;
    servo->currentAngle = 90;
    servo->targetAngle = 90;
    servo->minAngle = 0;
    servo->maxAngle = 180;
    servo->minPulseUs = minPulseUs;
    servo->maxPulseUs = maxPulseUs;
    servo->speed = 0;  // Instant movement by default
    servo->attached = false;
    servo->moving = false;
    servo->lastUpdateTime = 0;
    servo->updateIntervalMs = 20;  // 50Hz update rate
    
    gPinToSlot[pin] = -1;
}

bool servo_attach(ServoMotor* servo) {
    if (servo->attached) {
        return true;
    }
    
    int8_t slot = allocateServoSlot();
    if (slot < 0) {
        return false;
    }
    
    gPinToSlot[servo->pin] = slot;
    gServoObjects[slot].attach(servo->pin, servo->minPulseUs, servo->maxPulseUs);
    servo->attached = true;
    
    // Move to initial position
    gServoObjects[slot].write(servo->currentAngle);
    
    return true;
}

void servo_detach(ServoMotor* servo) {
    if (!servo->attached) {
        return;
    }
    
    int8_t slot = gPinToSlot[servo->pin];
    if (slot >= 0) {
        gServoObjects[slot].detach();
        freeServoSlot(slot);
        gPinToSlot[servo->pin] = -1;
    }
    
    servo->attached = false;
    servo->moving = false;
}

bool servo_is_attached(const ServoMotor* servo) {
    return servo->attached;
}

void servo_set_angle(ServoMotor* servo, int16_t angle) {
    angle = constrain_angle(angle, servo->minAngle, servo->maxAngle);
    
    servo->targetAngle = angle;
    servo->currentAngle = angle;
    servo->moving = false;
    
    if (servo->attached) {
        int8_t slot = gPinToSlot[servo->pin];
        if (slot >= 0) {
            gServoObjects[slot].write(angle);
        }
    }
}

void servo_set_angle_smooth(ServoMotor* servo, int16_t angle, uint8_t speed) {
    angle = constrain_angle(angle, servo->minAngle, servo->maxAngle);
    
    servo->targetAngle = angle;
    servo->speed = speed;
    
    if (speed == 0) {
        // Instant movement
        servo_set_angle(servo, angle);
    } else {
        servo->moving = (servo->currentAngle != angle);
        servo->lastUpdateTime = millis();
    }
}

int16_t servo_get_angle(const ServoMotor* servo) {
    return servo->currentAngle;
}

int16_t servo_get_target(const ServoMotor* servo) {
    return servo->targetAngle;
}

bool servo_is_moving(const ServoMotor* servo) {
    return servo->moving;
}

bool servo_update(ServoMotor* servo) {
    if (!servo->moving || servo->speed == 0) {
        return false;
    }
    
    uint32_t now = millis();
    if ((now - servo->lastUpdateTime) < servo->updateIntervalMs) {
        return false;
    }
    servo->lastUpdateTime = now;
    
    int16_t diff = servo->targetAngle - servo->currentAngle;
    
    if (diff == 0) {
        servo->moving = false;
        return false;
    }
    
    int16_t step = servo->speed;
    if (abs(diff) < step) {
        step = abs(diff);
    }
    
    if (diff > 0) {
        servo->currentAngle += step;
    } else {
        servo->currentAngle -= step;
    }
    
    // Clamp to limits
    servo->currentAngle = constrain_angle(servo->currentAngle, servo->minAngle, servo->maxAngle);
    
    // Update physical servo
    if (servo->attached) {
        int8_t slot = gPinToSlot[servo->pin];
        if (slot >= 0) {
            gServoObjects[slot].write(servo->currentAngle);
        }
    }
    
    // Check if reached target
    if (servo->currentAngle == servo->targetAngle) {
        servo->moving = false;
    }
    
    return true;
}

void servo_set_limits(ServoMotor* servo, int16_t minAngle, int16_t maxAngle) {
    servo->minAngle = minAngle;
    servo->maxAngle = maxAngle;
    
    // Clamp current values to new limits
    servo->currentAngle = constrain_angle(servo->currentAngle, minAngle, maxAngle);
    servo->targetAngle = constrain_angle(servo->targetAngle, minAngle, maxAngle);
}

void servo_move_by(ServoMotor* servo, int16_t delta) {
    int16_t newAngle = servo->currentAngle + delta;
    servo_set_angle(servo, newAngle);
}

void servo_go_min(ServoMotor* servo) {
    servo_set_angle(servo, servo->minAngle);
}

void servo_go_max(ServoMotor* servo) {
    servo_set_angle(servo, servo->maxAngle);
}

void servo_go_center(ServoMotor* servo) {
    int16_t center = (servo->minAngle + servo->maxAngle) / 2;
    servo_set_angle(servo, center);
}

void servo_stop(ServoMotor* servo) {
    servo->targetAngle = servo->currentAngle;
    servo->moving = false;
}

int16_t servo_get_error(const ServoMotor* servo) {
    return servo->targetAngle - servo->currentAngle;
}

void servo_set_update_interval(ServoMotor* servo, uint16_t intervalMs) {
    servo->updateIntervalMs = intervalMs;
}

void servo_write_microseconds(ServoMotor* servo, uint16_t pulseUs) {
    if (!servo->attached) {
        return;
    }
    
    int8_t slot = gPinToSlot[servo->pin];
    if (slot >= 0) {
        gServoObjects[slot].writeMicroseconds(pulseUs);
        
        // Calculate approximate angle from pulse width
        int32_t range = servo->maxPulseUs - servo->minPulseUs;
        int32_t angleRange = servo->maxAngle - servo->minAngle;
        if (range > 0) {
            servo->currentAngle = servo->minAngle + 
                ((pulseUs - servo->minPulseUs) * angleRange) / range;
        }
    }
}

uint16_t servo_read_microseconds(const ServoMotor* servo) {
    if (!servo->attached) {
        return 0;
    }
    
    int8_t slot = gPinToSlot[servo->pin];
    if (slot >= 0) {
        return gServoObjects[slot].readMicroseconds();
    }
    return 0;
}

#ifdef __cplusplus
namespace ServoInternal {
    Servo* getServoObject(ServoMotor* servo) {
        if (!servo->attached) return nullptr;
        int8_t slot = gPinToSlot[servo->pin];
        if (slot >= 0) {
            return &gServoObjects[slot];
        }
        return nullptr;
    }
}
#endif

