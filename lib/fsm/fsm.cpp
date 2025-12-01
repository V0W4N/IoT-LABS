#include "fsm.h"
#include <string.h>

// -----------------------------------------------------------------------------
// Internal helper functions
// -----------------------------------------------------------------------------

/**
 * Find state by ID
 */
static FSMState* fsm_find_state(FSM* fsm, uint8_t stateId) {
    for (uint8_t i = 0; i < fsm->stateCount; i++) {
        if (fsm->states[i].id == stateId) {
            return &fsm->states[i];
        }
    }
    return NULL;
}

/**
 * Find matching transition for current state and event
 */
static FSMTransition* fsm_find_transition(FSM* fsm, uint8_t event) {
    for (uint8_t i = 0; i < fsm->transitionCount; i++) {
        FSMTransition* t = &fsm->transitions[i];
        if (t->fromState == fsm->currentState && t->event == event) {
            // Check guard condition if present
            if (t->guard == NULL || t->guard(fsm)) {
                return t;
            }
        }
    }
    return NULL;
}

/**
 * Execute state transition
 */
static void fsm_execute_transition(FSM* fsm, FSMTransition* transition) {
    FSMState* fromState = fsm_find_state(fsm, fsm->currentState);
    FSMState* toState = fsm_find_state(fsm, transition->toState);
    
    if (toState == NULL) {
        return; // Invalid target state
    }
    
    // Exit current state
    if (fromState != NULL && fromState->onExit != NULL) {
        fromState->onExit(fsm);
    }
    
    // Execute transition callback
    if (transition->onTransition != NULL) {
        transition->onTransition(fsm);
    }
    
    // Update state tracking
    fsm->previousState = fsm->currentState;
    fsm->currentState = transition->toState;
    fsm->stateEntryTime = millis();
    
    // Enter new state
    if (toState->onEnter != NULL) {
        toState->onEnter(fsm);
    }
}

// -----------------------------------------------------------------------------
// Public API Implementation
// -----------------------------------------------------------------------------

void fsm_init(FSM* fsm, const char* name) {
    memset(fsm, 0, sizeof(FSM));
    fsm->name = name;
    fsm->currentState = 0;
    fsm->previousState = 0;
    fsm->stateCount = 0;
    fsm->transitionCount = 0;
    fsm->initialized = false;
    fsm->userData = NULL;
    fsm->stateEntryTime = 0;
}

bool fsm_add_state(FSM* fsm, uint8_t id, const char* name,
                   StateCallback onEnter, StateCallback onExit, StateCallback onUpdate) {
    if (fsm->stateCount >= FSM_MAX_STATES) {
        return false;
    }
    
    // Check for duplicate state ID
    if (fsm_find_state(fsm, id) != NULL) {
        return false;
    }
    
    FSMState* state = &fsm->states[fsm->stateCount];
    state->id = id;
    state->name = name;
    state->onEnter = onEnter;
    state->onExit = onExit;
    state->onUpdate = onUpdate;
    
    fsm->stateCount++;
    return true;
}

bool fsm_add_transition(FSM* fsm, uint8_t fromState, uint8_t toState,
                        uint8_t event, GuardCallback guard, StateCallback onTransition) {
    if (fsm->transitionCount >= FSM_MAX_TRANSITIONS) {
        return false;
    }
    
    FSMTransition* t = &fsm->transitions[fsm->transitionCount];
    t->fromState = fromState;
    t->toState = toState;
    t->event = event;
    t->guard = guard;
    t->onTransition = onTransition;
    
    fsm->transitionCount++;
    return true;
}

bool fsm_start(FSM* fsm, uint8_t initialState) {
    FSMState* state = fsm_find_state(fsm, initialState);
    if (state == NULL) {
        return false;
    }
    
    fsm->currentState = initialState;
    fsm->previousState = initialState;
    fsm->stateEntryTime = millis();
    fsm->initialized = true;
    
    // Call onEnter for initial state
    if (state->onEnter != NULL) {
        state->onEnter(fsm);
    }
    
    return true;
}

bool fsm_process_event(FSM* fsm, uint8_t event) {
    if (!fsm->initialized) {
        return false;
    }
    
    FSMTransition* transition = fsm_find_transition(fsm, event);
    if (transition == NULL) {
        return false; // No matching transition
    }
    
    fsm_execute_transition(fsm, transition);
    return true;
}

void fsm_update(FSM* fsm) {
    if (!fsm->initialized) {
        return;
    }
    
    FSMState* state = fsm_find_state(fsm, fsm->currentState);
    if (state != NULL && state->onUpdate != NULL) {
        state->onUpdate(fsm);
    }
}

bool fsm_force_state(FSM* fsm, uint8_t stateId) {
    FSMState* fromState = fsm_find_state(fsm, fsm->currentState);
    FSMState* toState = fsm_find_state(fsm, stateId);
    
    if (toState == NULL) {
        return false;
    }
    
    // Exit current state
    if (fromState != NULL && fromState->onExit != NULL) {
        fromState->onExit(fsm);
    }
    
    // Update state tracking
    fsm->previousState = fsm->currentState;
    fsm->currentState = stateId;
    fsm->stateEntryTime = millis();
    
    // Enter new state
    if (toState->onEnter != NULL) {
        toState->onEnter(fsm);
    }
    
    return true;
}

uint8_t fsm_get_current_state(const FSM* fsm) {
    return fsm->currentState;
}

const char* fsm_get_current_state_name(const FSM* fsm) {
    for (uint8_t i = 0; i < fsm->stateCount; i++) {
        if (fsm->states[i].id == fsm->currentState) {
            return fsm->states[i].name;
        }
    }
    return "UNKNOWN";
}

unsigned long fsm_get_time_in_state(const FSM* fsm) {
    return millis() - fsm->stateEntryTime;
}

void fsm_set_user_data(FSM* fsm, void* userData) {
    fsm->userData = userData;
}

void* fsm_get_user_data(const FSM* fsm) {
    return fsm->userData;
}

bool fsm_is_in_state(const FSM* fsm, uint8_t stateId) {
    return fsm->currentState == stateId;
}

void fsm_print_status(const FSM* fsm) {
    Serial.print(F("[FSM:"));
    Serial.print(fsm->name);
    Serial.print(F("] State: "));
    Serial.print(fsm_get_current_state_name(fsm));
    Serial.print(F(" (ID: "));
    Serial.print(fsm->currentState);
    Serial.print(F(") | Time: "));
    Serial.print(fsm_get_time_in_state(fsm));
    Serial.println(F("ms"));
}

