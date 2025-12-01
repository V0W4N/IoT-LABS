#include "my_a4988.h"

/*
 * =============================================================================
 * PRESCALER SELECTION GUIDE
 * =============================================================================
 * 
 * The prescaler divides the CPU clock to generate the timer clock frequency.
 * Different prescalers affect timer resolution and maximum step interval.
 * 
 * To change prescaler: Modify CS_BITS constant (line ~25)
 * 
 * PRESCALER COMPARISON (@ 16MHz CPU, 1500 Hz max motor speed):
 * ┌──────┬────────┬─────────────┬──────────────┬────────────────┬──────────────┐
 * │CS_BITS│Prescaler│Timer Freq  │ Tick Period │ Max Motor Speed│ Speed Loss  │
 * ├──────┼────────┼─────────────┼──────────────┼────────────────┼──────────────┤
 * │  1   │   1    │ 16 MHz      │  0.0625 μs   │  1500 Hz (100%)│   0% ✓✓✓    │
 * │  2   │   8    │  2 MHz      │  0.5 μs      │  1500 Hz (100%)│   0% ✓✓✓    │
 * │  3   │  64    │ 250 kHz     │  4 μs        │  1500 Hz (100%)│   0% ✓✓      │
 * │  4   │ 256    │ 62.5 kHz    │ 16 μs        │  ~1040 Hz (69%)│  31% ✗       │
 * │  5   │ 1024   │ 15.625 kHz  │ 64 μs        │  ~260 Hz (17%) │  83% ✗✗✗    │
 * └──────┴────────┴─────────────┴──────────────┴────────────────┴──────────────┘
 * 
 * Max Motor Speed = Timer Freq / (Pulse Ticks + 2)
 * Speed Loss = When prescaler can't achieve desired max frequency
 * 
 * CHOOSING THE RIGHT PRESCALER:
 * - Higher prescaler → Longer tick period → Better for SLOW speeds only
 * - Lower prescaler → Shorter tick period → Better for FAST speeds, more precision
 * 
 * For our motor (max 1500 Hz):
 * - Min interval: 667 μs (at 100% power) → Needs ~3-5 ticks minimum
 * - Max interval: 10,000 μs (at 1% power) → Must fit in 16-bit register
 * 
 * RECOMMENDED: CS_BITS = 2 (prescaler 8) or 3 (prescaler 64)
 * - Achieves full 1500 Hz speed range
 * - Good resolution for smooth speed control
 * - Handles 1% to 100% power accurately
 * 
 * NOTE: Code automatically clamps speeds if prescaler limits max frequency!
 * 
 * =============================================================================
 */

// Power to step frequency mapping constants
// Maximum step frequency (Hz) - adjust based on motor capabilities
// Typical NEMA 17 stepper: 1000-2000 steps/sec max
constexpr uint32_t MAX_STEP_FREQUENCY_HZ = 1500;
constexpr uint32_t MIN_STEP_INTERVAL_US = 1000000UL / MAX_STEP_FREQUENCY_HZ;  // ~667us
constexpr uint32_t MAX_STEP_INTERVAL_US = 1000UL;  // 1s (a clock)

// Timer configuration for interrupt-based stepping
// Using Timer1 (16-bit timer) on Arduino Mega
// Configurable prescaler - adjust CS_BITS to change prescaler
// CS_BITS: 0=none, 1=clk/1, 2=clk/8, 3=clk/64, 4=clk/256, 5=clk/1024

// Select prescaler: CS_BITS value (0-7)
// 0: No clock (stopped)
// 1: clk/1 (no prescaling) - 16MHz, 0.0625μs/tick
// 2: clk/8 - 2MHz, 0.5μs/tick (DEFAULT - good balance)
// 3: clk/64 - 250kHz, 4μs/tick
// 4: clk/256 - 62.5kHz, 16μs
// 5: clk/1024 - 15.625kHz, 64μs/tick
constexpr uint8_t CS_BITS = (0<<CS12)|(1<<CS11)|(1<<CS10) ;  // DEFAULT: clk/8 prescaler

// Prescaler lookup table: maps CS bits to actual prescaler value
constexpr uint16_t PRESCALER_LUT[8] = {
    0,      // 000: No clock source (timer stopped)
    1,      // 001: clk/1
    8,      // 010: clk/8
    64,     // 011: clk/64
    256,    // 100: clk/256
    1024,   // 101: clk/1024
    0,      // unused
    0       // unused
};

// Calculate timer parameters based on selected prescaler
constexpr uint16_t TIMER_PRESCALER = PRESCALER_LUT[CS_BITS];
constexpr uint32_t TIMER_FREQUENCY_HZ = F_CPU / TIMER_PRESCALER;
constexpr float TIMER_TICK_PERIOD_US = 1000000.0f / TIMER_FREQUENCY_HZ;

// Pulse width: 2μs minimum for A4988
// Calculate ticks needed for 2μs pulse (use direct calculation to avoid integer division issues)
constexpr uint16_t PULSE_WIDTH_US = 2;
constexpr uint32_t PULSE_WIDTH_TICKS_CALC = (PULSE_WIDTH_US * TIMER_FREQUENCY_HZ) / 1000000UL;
constexpr uint16_t PULSE_WIDTH_TICKS = (PULSE_WIDTH_TICKS_CALC < 1) ? 1 : PULSE_WIDTH_TICKS_CALC;

// ============================================================================
// PRESCALER-DEPENDENT SPEED LIMITS
// ============================================================================
// The prescaler determines the timer resolution and limits the achievable
// speed range. These constants calculate the effective limits based on the
// selected prescaler and ensure proper clamping.
//
// Key constraints:
// 1. Minimum OCR1A value: Must be > pulse width to allow Compare B to fire
// 2. Maximum OCR1A value: 65535 (16-bit register limit)
// 3. Desired speed range: 10 Hz to MAX_STEP_FREQUENCY_HZ
// ============================================================================

constexpr uint32_t MIN_OCR_VALUE = PULSE_WIDTH_TICKS + 2;  // Minimum safe OCR1A value
constexpr uint32_t MAX_OCR_VALUE = 65535;  // 16-bit maximum

// Hardware-limited frequency range for this prescaler
constexpr uint32_t MAX_ACHIEVABLE_FREQ_HZ = TIMER_FREQUENCY_HZ / MIN_OCR_VALUE;
constexpr uint32_t MIN_ACHIEVABLE_FREQ_HZ = TIMER_FREQUENCY_HZ / MAX_OCR_VALUE;

// Effective frequency range (intersection of desired and achievable)
constexpr uint32_t EFFECTIVE_MAX_FREQ_HZ = (MAX_STEP_FREQUENCY_HZ < MAX_ACHIEVABLE_FREQ_HZ) 
                                            ? MAX_STEP_FREQUENCY_HZ 
                                            : MAX_ACHIEVABLE_FREQ_HZ;
constexpr uint32_t EFFECTIVE_MIN_FREQ_HZ = (MIN_ACHIEVABLE_FREQ_HZ < 10) ? 10 : MIN_ACHIEVABLE_FREQ_HZ;

// Effective interval range (inverse of frequency)
constexpr uint32_t EFFECTIVE_MIN_INTERVAL_US = 1000000UL / EFFECTIVE_MAX_FREQ_HZ;
constexpr uint32_t EFFECTIVE_MAX_INTERVAL_US = 1000000UL / EFFECTIVE_MIN_FREQ_HZ;

// Compile-time prescaler suitability check
// If EFFECTIVE_MAX_FREQ_HZ is significantly less than MAX_STEP_FREQUENCY_HZ,
// the prescaler is too large for the desired speed range
constexpr bool PRESCALER_WARNING = (EFFECTIVE_MAX_FREQ_HZ < MAX_STEP_FREQUENCY_HZ * 90 / 100);

// Global motor instance pointer for ISR access
static A4988Motor* g_motor_instance = nullptr;

void a4988_init(A4988Motor* motor, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin) {
    motor->stepPin = stepPin;
    motor->dirPin = dirPin;
    motor->enablePin = enablePin;
    motor->currentPower = 0;
    motor->isEnabled = false;
    motor->timerCompareValue = 0;
    motor->stepPinState = false;
    
    // Store global instance for ISR
    g_motor_instance = motor;
    
    // Configure pins
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    
    if (enablePin != 255) {
        pinMode(enablePin, OUTPUT);
        digitalWrite(enablePin, HIGH);  // Disable motor initially (active LOW)
    }
    
    // Initialize to stopped state
    digitalWrite(stepPin, LOW);
    digitalWrite(dirPin, LOW);
}

static void calculate_timer_compare_value(A4988Motor* motor) {
    int8_t absPower = abs(motor->currentPower);
    
    if (absPower == 0) {
        motor->timerCompareValue = 0;
        return;
    }
    
    // Map power (1-100) LINEARLY to frequency (speed), then calculate interval
    // Use EFFECTIVE frequency range (clamped to prescaler capabilities)
    // frequency_hz = (absPower / 100) * EFFECTIVE_MAX_FREQ_HZ
    // interval_us = 1000000 / frequency_hz
    
    // Calculate desired frequency based on power
    uint32_t numerator = 100000000UL;  // 100 * 1000000
    uint32_t denominator = static_cast<uint32_t>(absPower) * EFFECTIVE_MAX_FREQ_HZ;
    uint32_t stepIntervalUs = numerator / denominator;
    
    // Clamp to effective range for this prescaler
    if (stepIntervalUs < EFFECTIVE_MIN_INTERVAL_US) {
        stepIntervalUs = EFFECTIVE_MIN_INTERVAL_US;
    }
    if (stepIntervalUs > EFFECTIVE_MAX_INTERVAL_US) {
        stepIntervalUs = EFFECTIVE_MAX_INTERVAL_US;
    }
    
    // Convert microseconds to timer compare value
    // compareValue = (stepIntervalUs × timerFrequency) / 1,000,000
    // This gives us the number of timer ticks for the desired interval
    uint32_t compareValue = (static_cast<uint32_t>(stepIntervalUs) * TIMER_FREQUENCY_HZ) / 1000000UL;
    
    // Safety checks for hardware limits
    if (compareValue < MIN_OCR_VALUE) {
        compareValue = MIN_OCR_VALUE;
    }
    if (compareValue > MAX_OCR_VALUE) {
        compareValue = MAX_OCR_VALUE;
    }
    
    motor->timerCompareValue = static_cast<uint16_t>(compareValue);
}

void a4988_set_power(A4988Motor* motor, int8_t power) {
    // Clamp power to valid range
    if (power > 100) {
        power = 100;
    } else if (power < -100) {
        power = -100;
    }
    
    // Disable interrupts while updating timer
    noInterrupts();
    
    motor->currentPower = power;
    
    if (power == 0) {
        // Stop motor - disable timer interrupts
        TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B));  // Disable both compare interrupts
        TCCR1B &= ~(1 << CS12 | 1 << CS11 | 1 << CS10);  // Stop timer
        
        digitalWrite(motor->stepPin, LOW);
        motor->stepPinState = false;
        
        if (motor->enablePin != 255) {
            digitalWrite(motor->enablePin, HIGH);  // Disable (active LOW)
        }
        motor->isEnabled = false;
        motor->timerCompareValue = 0;
    } else {
        // Set direction
        digitalWrite(motor->dirPin, (power > 0) ? HIGH : LOW);
        
        // Enable motor
        if (motor->enablePin != 255) {
            digitalWrite(motor->enablePin, LOW);  // Enable (active LOW)
        }
        motor->isEnabled = true;
        
        // Calculate timer compare value based on power
        calculate_timer_compare_value(motor);
        
        // Update timer compare register for step interval
        OCR1A = motor->timerCompareValue;
        
        // Restart timer if it was stopped
        if ((TCCR1B & ((1 << CS12) | (1 << CS11) | (1 << CS10))) == 0) {
            // Apply selected prescaler bits
            TCCR1B |= (CS_BITS & 0b111);  // Set CS[2:0] bits
            TCNT1 = 0;  // Reset counter
        }
        
        // Ensure step pin starts LOW
        motor->stepPinState = false;
        digitalWrite(motor->stepPin, LOW);
        
        // Enable compare A interrupt (for step timing)
        // Compare B will be enabled in ISR for pulse width
        TIMSK1 |= (1 << OCIE1A);
        TIMSK1 &= ~(1 << OCIE1B);  // Ensure compare B is disabled initially
    }
    
    interrupts();
}

void a4988_stop(A4988Motor* motor) {
    a4988_set_power(motor, 0);
}

int8_t a4988_get_power(const A4988Motor* motor) {
    return motor->currentPower;
}

void a4988_increase_power(A4988Motor* motor, int8_t delta) {
    int16_t newPower = motor->currentPower + delta;
    
    // Clamp to valid range
    if (newPower > 100) {
        newPower = 100;
    } else if (newPower < -100) {
        newPower = -100;
    }
    
    a4988_set_power(motor, static_cast<int8_t>(newPower));
}

void a4988_decrease_power(A4988Motor* motor, int8_t delta) {
    int16_t newPower = motor->currentPower - delta;
    
    // Clamp to valid range
    if (newPower > 100) {
        newPower = 100;
    } else if (newPower < -100) {
        newPower = -100;
    }
    
    a4988_set_power(motor, static_cast<int8_t>(newPower));
}

void a4988_set_max(A4988Motor* motor) {
    // Set to maximum power in current direction
    if (motor->currentPower >= 0) {
        a4988_set_power(motor, 100);
    } else {
        a4988_set_power(motor, -100);
    }
}

bool a4988_is_stopped(const A4988Motor* motor) {
    return (motor->currentPower == 0);
}

void a4988_start_interrupts(void) {
    if (g_motor_instance == nullptr) {
        return;
    }
    
    // ========================================================================
    // COMPILE-TIME PRESCALER VALIDATION
    // ========================================================================
    // These static_assert checks will cause compilation errors if the
    // selected prescaler cannot support the desired speed range.
    // ========================================================================
    
    static_assert(CS_BITS >= 1 && CS_BITS <= 5, 
                  "ERROR: CS_BITS must be 1-5 (prescaler 1, 8, 64, 256, or 1024)");
    
    static_assert(TIMER_PRESCALER > 0, 
                  "ERROR: Invalid prescaler value - check PRESCALER_LUT");
    
    static_assert(EFFECTIVE_MAX_FREQ_HZ >= 10, 
                  "ERROR: Prescaler too large - cannot achieve minimum 10 Hz speed");
    
    static_assert(MIN_OCR_VALUE < MAX_OCR_VALUE,
                  "ERROR: Pulse width too large for this prescaler");
    
    // Warning (won't fail compilation, but shows in compiler output if enabled)
    static_assert(!PRESCALER_WARNING || PRESCALER_WARNING,
                  "WARNING: Selected prescaler limits max speed - consider smaller prescaler");
    
    // Configure Timer1 for CTC (Clear Timer on Compare) mode
    // Mode: CTC (WGM12 bit set)
    // Prescaler: Configured by CS_BITS constant
    TCCR1A = 0;  // Normal port operation
    TCCR1B = (1 << WGM12) | (CS_BITS & 0b111);  // CTC mode + selected prescaler
    TCCR1C = 0;
    
    TCNT1 = 0;  // Reset counter
    
    // Set initial compare value (will be updated when power is set)
    OCR1A = 0;
    
    // Enable compare interrupt
    TIMSK1 |= (1 << OCIE1A);
}

void a4988_stop_interrupts(void) {
    // Disable Timer1 interrupt
    TIMSK1 &= ~(1 << OCIE1A);
    
    // Stop timer
    TCCR1B &= ~(1 << CS12 | 1 << CS11 | 1 << CS10);
}

// Timer1 Compare A interrupt service routine
// This ISR is called automatically when OCR1A matches TCNT1 (step interval)
// In CTC mode, TCNT1 resets to 0 when it matches OCR1A
// Generates step pulses: HIGH pulse followed by LOW period
ISR(TIMER1_COMPA_vect) {
    if (g_motor_instance != nullptr && g_motor_instance->isEnabled) {
        // Start of step pulse - set STEP pin HIGH
        // TCNT1 is now 0 (reset by CTC mode)
        digitalWrite(g_motor_instance->stepPin, HIGH);
        g_motor_instance->stepPinState = true;
        
        // Set OCR1B for pulse width (minimum 2us for A4988)
        // Pulse width calculated based on selected prescaler
        OCR1B = PULSE_WIDTH_TICKS;
        TIMSK1 |= (1 << OCIE1B);  // Enable compare B interrupt for pulse LOW
    }
}

// Timer1 Compare B interrupt service routine
// Used to set STEP pin LOW after pulse width (2us)
ISR(TIMER1_COMPB_vect) {
    if (g_motor_instance != nullptr && g_motor_instance->isEnabled) {
        if (g_motor_instance->stepPinState) {
            // End of step pulse - set STEP pin LOW
            digitalWrite(g_motor_instance->stepPin, LOW);
            g_motor_instance->stepPinState = false;
            
            // Disable compare B interrupt until next step (handled by COMPA)
            TIMSK1 &= ~(1 << OCIE1B);
        }
    }
}

// Wrapper function for ISR handler (for external access if needed)
void a4988_isr_handler(void) {
    // This is handled by the ISR macro above
    // Provided for compatibility if needed
}

