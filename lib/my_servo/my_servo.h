#ifndef MY_SERVO_H
#define MY_SERVO_H

#include <Arduino.h>
#include <Servo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Servo motor configuration and state structure
 */
typedef struct {
    uint8_t pin;              // PWM pin
    
    int16_t currentAngle;     // Current angle in degrees (0-180)
    int16_t targetAngle;      // Target angle in degrees
    int16_t minAngle;         // Minimum angle limit
    int16_t maxAngle;         // Maximum angle limit
    
    uint16_t minPulseUs;      // Minimum pulse width in microseconds
    uint16_t maxPulseUs;      // Maximum pulse width in microseconds
    
    uint8_t speed;            // Movement speed (degrees per update, 0 = instant)
    bool attached;            // Servo attached flag
    bool moving;              // Currently moving flag
    
    uint32_t lastUpdateTime;  // Last position update timestamp
    uint16_t updateIntervalMs; // Update interval for smooth movement
} ServoMotor;

/**
 * @brief Initialize servo motor with default settings (0-180 degrees)
 * @param servo Pointer to servo structure
 * @param pin PWM pin number
 */
void servo_init(ServoMotor* servo, uint8_t pin);

/**
 * @brief Initialize servo motor with custom pulse width range
 * @param servo Pointer to servo structure
 * @param pin PWM pin number
 * @param minPulseUs Minimum pulse width (default 544)
 * @param maxPulseUs Maximum pulse width (default 2400)
 */
void servo_init_custom(ServoMotor* servo, uint8_t pin, uint16_t minPulseUs, uint16_t maxPulseUs);

/**
 * @brief Attach servo to pin and enable PWM output
 * @param servo Pointer to servo structure
 * @return true if attached successfully
 */
bool servo_attach(ServoMotor* servo);

/**
 * @brief Detach servo and disable PWM output
 * @param servo Pointer to servo structure
 */
void servo_detach(ServoMotor* servo);

/**
 * @brief Check if servo is attached
 * @param servo Pointer to servo structure
 * @return true if attached
 */
bool servo_is_attached(const ServoMotor* servo);

/**
 * @brief Set servo angle immediately
 * @param servo Pointer to servo structure
 * @param angle Target angle in degrees
 */
void servo_set_angle(ServoMotor* servo, int16_t angle);

/**
 * @brief Set servo angle with smooth movement
 * @param servo Pointer to servo structure
 * @param angle Target angle in degrees
 * @param speed Degrees per update step (0 = instant)
 */
void servo_set_angle_smooth(ServoMotor* servo, int16_t angle, uint8_t speed);

/**
 * @brief Get current servo angle
 * @param servo Pointer to servo structure
 * @return Current angle in degrees
 */
int16_t servo_get_angle(const ServoMotor* servo);

/**
 * @brief Get target servo angle
 * @param servo Pointer to servo structure
 * @return Target angle in degrees
 */
int16_t servo_get_target(const ServoMotor* servo);

/**
 * @brief Check if servo is currently moving
 * @param servo Pointer to servo structure
 * @return true if moving towards target
 */
bool servo_is_moving(const ServoMotor* servo);

/**
 * @brief Update servo position (call in loop for smooth movement)
 * @param servo Pointer to servo structure
 * @return true if position was updated
 */
bool servo_update(ServoMotor* servo);

/**
 * @brief Set angle limits for servo
 * @param servo Pointer to servo structure
 * @param minAngle Minimum angle
 * @param maxAngle Maximum angle
 */
void servo_set_limits(ServoMotor* servo, int16_t minAngle, int16_t maxAngle);

/**
 * @brief Move servo by relative amount
 * @param servo Pointer to servo structure
 * @param delta Angle change (positive = increase, negative = decrease)
 */
void servo_move_by(ServoMotor* servo, int16_t delta);

/**
 * @brief Set servo to minimum angle
 * @param servo Pointer to servo structure
 */
void servo_go_min(ServoMotor* servo);

/**
 * @brief Set servo to maximum angle
 * @param servo Pointer to servo structure
 */
void servo_go_max(ServoMotor* servo);

/**
 * @brief Set servo to center position
 * @param servo Pointer to servo structure
 */
void servo_go_center(ServoMotor* servo);

/**
 * @brief Stop smooth movement at current position
 * @param servo Pointer to servo structure
 */
void servo_stop(ServoMotor* servo);

/**
 * @brief Get angle error (target - current)
 * @param servo Pointer to servo structure
 * @return Angle error in degrees
 */
int16_t servo_get_error(const ServoMotor* servo);

/**
 * @brief Set update interval for smooth movement
 * @param servo Pointer to servo structure
 * @param intervalMs Update interval in milliseconds
 */
void servo_set_update_interval(ServoMotor* servo, uint16_t intervalMs);

/**
 * @brief Write raw pulse width to servo
 * @param servo Pointer to servo structure
 * @param pulseUs Pulse width in microseconds
 */
void servo_write_microseconds(ServoMotor* servo, uint16_t pulseUs);

/**
 * @brief Get current pulse width
 * @param servo Pointer to servo structure
 * @return Current pulse width in microseconds
 */
uint16_t servo_read_microseconds(const ServoMotor* servo);

#ifdef __cplusplus
}

// C++ wrapper for internal Servo object access
namespace ServoInternal {
    Servo* getServoObject(ServoMotor* servo);
}
#endif

#endif // MY_SERVO_H


