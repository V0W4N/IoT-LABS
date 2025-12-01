#ifndef FSM_H
#define FSM_H

#include <Arduino.h>
#include <stdint.h>

// Maximum number of states and transitions per FSM
#define FSM_MAX_STATES 16
#define FSM_MAX_TRANSITIONS 32

// Forward declaration
struct FSM;

// Callback function types
typedef void (*StateCallback)(struct FSM* fsm);
typedef bool (*GuardCallback)(struct FSM* fsm);

// -----------------------------------------------------------------------------
// State definition
// -----------------------------------------------------------------------------
typedef struct FSMState {
    uint8_t id;                    // Unique state ID
    const char* name;              // State name for debugging
    StateCallback onEnter;         // Called when entering this state
    StateCallback onExit;          // Called when exiting this state
    StateCallback onUpdate;        // Called every FSM update while in this state
} FSMState;

// -----------------------------------------------------------------------------
// Transition definition
// -----------------------------------------------------------------------------
typedef struct FSMTransition {
    uint8_t fromState;             // Source state ID
    uint8_t toState;               // Target state ID
    uint8_t event;                 // Event that triggers this transition
    GuardCallback guard;           // Optional guard condition (can be NULL)
    StateCallback onTransition;    // Optional callback during transition
} FSMTransition;

// -----------------------------------------------------------------------------
// FSM instance structure
// -----------------------------------------------------------------------------
typedef struct FSM {
    const char* name;                              // FSM name for debugging
    FSMState states[FSM_MAX_STATES];               // Array of states
    FSMTransition transitions[FSM_MAX_TRANSITIONS]; // Array of transitions
    uint8_t stateCount;                            // Number of registered states
    uint8_t transitionCount;                       // Number of registered transitions
    uint8_t currentState;                          // Current state ID
    uint8_t previousState;                         // Previous state ID
    bool initialized;                              // FSM initialized flag
    void* userData;                                // User data pointer for callbacks
    unsigned long stateEntryTime;                  // Time when current state was entered
} FSM;

// -----------------------------------------------------------------------------
// FSM API Functions
// -----------------------------------------------------------------------------

/**
 * Initialize an FSM instance
 * @param fsm Pointer to FSM structure
 * @param name Name of the FSM (for debugging)
 */
void fsm_init(FSM* fsm, const char* name);

/**
 * Add a state to the FSM
 * @param fsm Pointer to FSM structure
 * @param id Unique state ID (0-255)
 * @param name State name (for debugging)
 * @param onEnter Callback when entering state (can be NULL)
 * @param onExit Callback when exiting state (can be NULL)
 * @param onUpdate Callback called every update while in state (can be NULL)
 * @return true if state added successfully
 */
bool fsm_add_state(FSM* fsm, uint8_t id, const char* name,
                   StateCallback onEnter, StateCallback onExit, StateCallback onUpdate);

/**
 * Add a transition to the FSM
 * @param fsm Pointer to FSM structure
 * @param fromState Source state ID
 * @param toState Target state ID
 * @param event Event ID that triggers this transition
 * @param guard Optional guard callback (can be NULL)
 * @param onTransition Optional callback during transition (can be NULL)
 * @return true if transition added successfully
 */
bool fsm_add_transition(FSM* fsm, uint8_t fromState, uint8_t toState,
                        uint8_t event, GuardCallback guard, StateCallback onTransition);

/**
 * Start the FSM with initial state
 * @param fsm Pointer to FSM structure
 * @param initialState Initial state ID
 * @return true if started successfully
 */
bool fsm_start(FSM* fsm, uint8_t initialState);

/**
 * Process an event
 * @param fsm Pointer to FSM structure
 * @param event Event ID to process
 * @return true if a transition occurred
 */
bool fsm_process_event(FSM* fsm, uint8_t event);

/**
 * Update the FSM (call periodically)
 * Calls the onUpdate callback of current state
 * @param fsm Pointer to FSM structure
 */
void fsm_update(FSM* fsm);

/**
 * Force transition to a specific state (bypasses normal transitions)
 * @param fsm Pointer to FSM structure
 * @param stateId Target state ID
 * @return true if transition occurred
 */
bool fsm_force_state(FSM* fsm, uint8_t stateId);

/**
 * Get current state ID
 * @param fsm Pointer to FSM structure
 * @return Current state ID
 */
uint8_t fsm_get_current_state(const FSM* fsm);

/**
 * Get current state name
 * @param fsm Pointer to FSM structure
 * @return Current state name or "UNKNOWN"
 */
const char* fsm_get_current_state_name(const FSM* fsm);

/**
 * Get time spent in current state (milliseconds)
 * @param fsm Pointer to FSM structure
 * @return Time in milliseconds since entering current state
 */
unsigned long fsm_get_time_in_state(const FSM* fsm);

/**
 * Set user data pointer
 * @param fsm Pointer to FSM structure
 * @param userData Pointer to user data
 */
void fsm_set_user_data(FSM* fsm, void* userData);

/**
 * Get user data pointer
 * @param fsm Pointer to FSM structure
 * @return User data pointer
 */
void* fsm_get_user_data(const FSM* fsm);

/**
 * Check if FSM is in a specific state
 * @param fsm Pointer to FSM structure
 * @param stateId State ID to check
 * @return true if currently in that state
 */
bool fsm_is_in_state(const FSM* fsm, uint8_t stateId);

/**
 * Print FSM status (for debugging via Serial)
 * @param fsm Pointer to FSM structure
 */
void fsm_print_status(const FSM* fsm);

#endif // FSM_H

