#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t train_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef enum {
    TRAIN_NOT_PRESENT,
    TRAIN_PRESENT,
    TRAIN_ERROR
} TrainState;
typedef struct {
    int BoomGate;       // Requested gate position: 0 = down, 1 = up.
    int RedLight;       // TRAIN stop signal: 1 = STOP, 0 = stop signal off.
    int RedLightFlash;  // ROADSIDE flashing-warning enable: 0 = off, 1 = on.
} Hardware;
typedef struct {
    int train_detected;
    int hardware_error;
    Hardware hardware;
    TrainState train_state;
    int gate_down_confirmed; // INPUT: 1 only when gate-down feedback is valid.
} TrainData;

// Caller holds train_mutex while accessing shared TrainData through these functions.
// The functions do not lock again internally.
void TrainStateLogic(TrainData *trainData);
void TrainLogicNode(void *state_ptr, void *inputs);
void *TrainLogicThread(void *state_ptr);

int main(void) {
    // Initialize the train data
    TrainData trainData = {
        .train_detected = 0,
        .hardware_error = 0,
        .hardware = {.BoomGate = 1, .RedLight = 1, .RedLightFlash = 0},
        .train_state = TRAIN_NOT_PRESENT,
        .gate_down_confirmed = 0
    };
    while (1) {
        pthread_mutex_lock(&train_mutex);

        // MANUAL TEST INPUTS: uncomment/change values here.
        // 1 = train detected, a hardware fault, or confirmed gate down respectively.
        // trainData.train_detected = 1;
        // trainData.hardware_error = 0;
        // trainData.gate_down_confirmed = 0;

        // Select the state, then store the resulting output commands.
        TrainStateLogic(&trainData);
        TrainLogicNode(&trainData, NULL);
        // Copy results under the lock; printing below uses this local snapshot.
        TrainData snapshot = trainData;
        pthread_mutex_unlock(&train_mutex);
        static int previous_state = -1;
        static int previous_stop = -1;
        if ((int)snapshot.train_state != previous_state ||
            snapshot.hardware.RedLight != previous_stop) {
            printf("Train state=%d | gate command=%s | train STOP=%d | warning enable=%d\n",
                   (int)snapshot.train_state,
                   snapshot.hardware.BoomGate ? "UP" : "DOWN",
                   snapshot.hardware.RedLight, snapshot.hardware.RedLightFlash);
            previous_state = (int)snapshot.train_state;
            previous_stop = snapshot.hardware.RedLight;
        }
        // Sleep for a while before checking again
        usleep(500000); // 500 ms
    }
}

//------------------------------------------------------------------------------------------------
// ***********************************************************************************************
//    Code below is for the train logic node, which is separate from the controller and traffic light nodes.
// ***********************************************************************************************
//------------------------------------------------------------------------------------------------


void TrainStateLogic(TrainData *trainData) {
    // Implement the logic to update the train state based on detection and hardware status
    if (trainData->hardware_error) {
        trainData->train_state = TRAIN_ERROR;
    } else if (trainData->train_detected) {
        trainData->train_state = TRAIN_PRESENT;
    } else {
        trainData->train_state = TRAIN_NOT_PRESENT;
    }
}

void TrainLogicNode(void *state_ptr, void *inputs) {
    (void)inputs;
    TrainData *trainData = state_ptr;
    Hardware *hardware = &trainData->hardware;
    // Check the train state and update hardware accordingly
    switch (trainData->train_state) {
        case TRAIN_NOT_PRESENT:
            hardware->BoomGate = 1; // No train: command gate UP.
            hardware->RedLight = 1; // Keep train STOP on while road is open.
            hardware->RedLightFlash = 0; // Red light flash off
            break;
        case TRAIN_PRESENT:
            hardware->BoomGate = 0; // Train present: command gate DOWN.
            hardware->RedLight = trainData->gate_down_confirmed == 1 ? 0 : 1;
            hardware->RedLightFlash = 1; // Red light flash on
            break;
        case TRAIN_ERROR:
            hardware->BoomGate = 0; // Request closure; a fault may prevent it.
            hardware->RedLight = 1; // Stop the train.
            hardware->RedLightFlash = 1; // Keep road users warned.
            break;
        default:
            // Handle unexpected state
            hardware->BoomGate = 0; // Boom gate down
            hardware->RedLight = 1; // Red light on
            hardware->RedLightFlash = 1; // Warn on an unknown state too.
            break;
    }
}
