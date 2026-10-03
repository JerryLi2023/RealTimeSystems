/* PedestrianNode_LogicOnly.c
 * Based on your uploaded Pasted markdown(4).md.
 * Keeps the existing structs, mutex, button edge detection and timed light cycle.
 * Main runs the light cycle; one button thread polls while main is sleeping.
 * All client/server code, message structs, channels and PID/CHID files removed.
 *
 * GPIO hooks below are PLACEHOLDERS: no pins are accessed until you implement them.
 * No pin numbers, register addresses or electrical polarity are assumed.
 * Original spellings Pedstrian / peroid retained to minimise changes.
 * Build on QNX: qcc -std=gnu11 -Wall -Wextra PedestrianNode_LogicOnly.c -o PedestrianNode
 * Host check: gcc -std=gnu11 -Wall -Wextra -pthread PedestrianNode_LogicOnly.c -o PedestrianNode
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    int LeftNorthSouthButton;
    int LeftSouthNorthButton;
    int RightNorthSouthButton;
    int RightSouthNorthButton;
    int TopEastWestButton;
    int TopWestEastButton;
    int BottomEastWestButton;
    int BottomWestEastButton;
} PedstrianButton;
typedef struct {
    int LeftNorthSouth;
    int RightNorthSouth;
    int TopEastWest;
    int BottomEastWest;
    int LeftNorthSouthLight;
    int RightNorthSouthLight;
    int TopEastWestLight;
    int BottomEastWestLight;
    int timer;
    int peroid;
    int stateChange;
    int phaseActive;   // Local only: prevents overwriting a running cycle.
    PedstrianButton button;
} PedstrianLight;
typedef enum {
    TRAFFIC_GREEN,
    TRAFFIC_RED_Flash,
    TRAFFIC_RED,
} PedestrianState;
typedef struct {
    int LeftNorthSouthLight;
    int RightNorthSouthLight;
    int TopEastWestLight;
    int BottomEastWestLight;
    PedestrianState states;
} PedestrianLightStates;

// Function declarations.
int InitialiseExternalPins(void);
void ReadExternalButtons(PedstrianButton *buttons);
void WriteExternalLightPins(int crossing, int red_on, int green_on);
void DisplayPedestrianLights(const PedestrianLightStates *state, int flash_on);
void PedestrianStates(void *state_ptr1, void *state_ptr2);
void CheckPedestrianButtons(PedstrianLight *light, const PedstrianButton *buttons,
                            PedstrianButton *previous);
void *button_checker(void *state_ptr);
int SetPedestrianCommand(PedstrianLight *light, int LNS, int RNS, int TEW, int BEW,
                         int time, int peroid);
int RunPedestrianCycle(PedstrianLight *light, PedestrianLightStates *state);

// ============================================================================
// 1. EXTERNAL BUTTON / PIN CONNECTIONS — fill in this section for your board.
// ============================================================================
enum { PIN_LNS, PIN_RNS, PIN_TEW, PIN_BEW }; // Logical crossing IDs, NOT pin numbers.

int InitialiseExternalPins(void) {
    // TODO: initialise your GPIO driver/mapping, input pins and lamp output pins.
    // Return -1 (with errno set) on a real initialisation failure.
    // Stub succeeds only to let the logic run with console output and no hardware.
    return 0;
}

void ReadExternalButtons(PedstrianButton *buttons) {
    // TODO: replace these zeros with ALL eight debounced button readings.
    // Contract: 1 = pressed, 0 = released. Convert active-low GPIO readings here.
    // Debouncing is not implemented by this stub or by the edge detector.
    *buttons = (PedstrianButton){0};

    // buttons->LeftNorthSouthButton  = ...;
    // buttons->LeftSouthNorthButton  = ...;
    // buttons->RightNorthSouthButton = ...;
    // buttons->RightSouthNorthButton = ...;
    // buttons->TopEastWestButton     = ...;
    // buttons->TopWestEastButton     = ...;
    // buttons->BottomEastWestButton  = ...;
    // buttons->BottomWestEastButton  = ...;
    // For a temporary held-button test, replace ... with 1 on one assignment.
    // Release (0) then press (1) again to generate another edge.
}

void WriteExternalLightPins(int crossing, int red_on, int green_on) {
    // TODO: map PIN_LNS / PIN_RNS / PIN_TEW / PIN_BEW to your physical lamp pins.
    // red_on and green_on are logical 0/1 values; convert polarity here.
    // During a flashing-red OFF step both values are 0 for that crossing.
    // Called only by main. No GPIO is written by this placeholder.
    (void)crossing;
    (void)red_on;
    (void)green_on;
}

// ============================================================================
// 2. MAIN — light sequencing stays in main; only button polling is a worker.
// ============================================================================
int main(void) {
    PedstrianLight light = {0};
    PedestrianLightStates state = {.states = TRAFFIC_RED};
    pthread_t th1;

    printf("Pedestrian logic running (GPIO hooks are placeholders)\n");
    if (InitialiseExternalPins() == -1) {
        perror("InitialiseExternalPins");
        return EXIT_FAILURE;
    }
    DisplayPedestrianLights(&state, 1);

    int error = pthread_create(&th1, NULL, button_checker, &light);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        return EXIT_FAILURE;
    }

    // OPTIONAL LOGIC TEST: uncomment to allow LNS for ONE ten-second cycle.
    // These arguments are: light, LNS, RNS, TEW, BEW, time, peroid.
    // if (SetPedestrianCommand(&light, 1, 0, 0, 0, 10, 1) == -1)
    //     perror("SetPedestrianCommand");

    while (1) {
        // A button latches a request; it does not grant permission to cross.
        // Put local decision code here and call SetPedestrianCommand when needed.
        // Read light's shared request fields under light_mutex.
        RunPedestrianCycle(&light, &state);
        usleep(10000); // Short idle delay; no mutex is held while sleeping.
    }
    return EXIT_SUCCESS;
}

// ============================================================================
// 3. LOCAL COMMAND — replaces the old server's data update, without any IPC.
// ============================================================================
int SetPedestrianCommand(PedstrianLight *light, int LNS, int RNS, int TEW, int BEW,
                         int time, int peroid) {
    // Same value/timing checks and busy rule as your original server.
    // Returns 0 if queued, or -1 with errno = EINVAL / EBUSY.
    const int any_green = LNS || RNS || TEW || BEW;
    if (!light || (LNS != 0 && LNS != 1) || (RNS != 0 && RNS != 1) ||
        (TEW != 0 && TEW != 1) || (BEW != 0 && BEW != 1) ||
        (any_green && (time < 2 || time > 3600 || peroid < 1 || peroid > 3600 ||
                       time > 3600 / peroid))) {
        errno = EINVAL;
        return -1;
    }
    // This function locks internally: do not call it while holding light_mutex.
    pthread_mutex_lock(&light_mutex);
    if (light->phaseActive || light->stateChange) {
        pthread_mutex_unlock(&light_mutex);
        errno = EBUSY;
        return -1;
    }
    light->LeftNorthSouthLight = LNS;
    light->RightNorthSouthLight = RNS;
    light->TopEastWestLight = TEW;
    light->BottomEastWestLight = BEW;
    light->timer = any_green ? time : 0;
    light->peroid = any_green ? peroid : 1;
    light->stateChange = 1;
    pthread_mutex_unlock(&light_mutex);
    return 0;
}

// ============================================================================
//  Button Pressing Logics -> Code underneath is for the button checking thread. It checks for button presses and updates the light structure accordingly.
//  PEDESTRIAN CYCLE — original green -> flashing red -> steady red sequence.
// ============================================================================
int RunPedestrianCycle(PedstrianLight *light, PedestrianLightStates *state) {
    // Call only from main. Returns 0 if idle, 1 after completing a queued cycle.
    // state belongs to main; the button thread only shares light.
    pthread_mutex_lock(&light_mutex);
    if (light->stateChange == 0) {
        pthread_mutex_unlock(&light_mutex);
        return 0;
    }
    int time = light->timer;
    int peroid = light->peroid;
    int red_flash = time - (time + 9) / 10; // Last ceil(time/10) ticks flash.
    PedestrianStates(light, state);
    light->stateChange = 0;
    light->phaseActive = 1;

    if (state->LeftNorthSouthLight) light->LeftNorthSouth = 0;
    if (state->RightNorthSouthLight) light->RightNorthSouth = 0;
    if (state->TopEastWestLight) light->TopEastWest = 0;
    if (state->BottomEastWestLight) light->BottomEastWest = 0;
    pthread_mutex_unlock(&light_mutex);

    for (int i = 0; i < time; i++) {
        if (i >= red_flash) {
            state->states = TRAFFIC_RED_Flash;
            for (int j = 0; j < peroid * 2; j++) {
                DisplayPedestrianLights(state, (j % 2) == 0);
                struct timespec delay = {0, 500000000L};
                while (nanosleep(&delay, &delay) == -1 && errno == EINTR) { }
            }
        } else {
            state->states = TRAFFIC_GREEN;
            DisplayPedestrianLights(state, 1);
            unsigned remaining = (unsigned)peroid;
            while (remaining != 0) remaining = sleep(remaining);
        }
    }
    state->states = TRAFFIC_RED;
    DisplayPedestrianLights(state, 1);
    pthread_mutex_lock(&light_mutex);
    light->phaseActive = 0;
    pthread_mutex_unlock(&light_mutex);
    return 1;
}

// Same console display, now also forwarding logical lamp values to the pin hook.
void DisplayPedestrianLights(const PedestrianLightStates *state, int flash_on) {
    const int enabled[4] = {
        state->LeftNorthSouthLight, state->RightNorthSouthLight,
        state->TopEastWestLight, state->BottomEastWestLight
    };
    const char *name[4] = {"LNS", "RNS", "TEW", "BEW"};
    for (int i = 0; i < 4; i++) {
        const char *colour = "RED";
        int red_on = 1, green_on = 0;
        if (enabled[i] && state->states == TRAFFIC_GREEN) {
            colour = "GREEN";
            red_on = 0;
            green_on = 1;
        } else if (enabled[i] && state->states == TRAFFIC_RED_Flash) {
            colour = flash_on ? "RED (flashing)" : "OFF (flashing)";
            red_on = flash_on != 0;
        }
        WriteExternalLightPins(i, red_on, green_on);
        printf("%s: %s%s", name[i], colour, i == 3 ? "\n" : " | ");
    }
}

void PedestrianStates(void *state_ptr1, void *state_ptr2) {
    PedstrianLight *light = state_ptr1;
    PedestrianLightStates *StateMachine = state_ptr2;
    StateMachine->LeftNorthSouthLight = light->LeftNorthSouthLight;
    StateMachine->RightNorthSouthLight = light->RightNorthSouthLight;
    StateMachine->TopEastWestLight = light->TopEastWestLight;
    StateMachine->BottomEastWestLight = light->BottomEastWestLight;
}

void CheckPedestrianButtons(PedstrianLight *light, const PedstrianButton *buttons,
                            PedstrianButton *previous) {
    // previous belongs to the single input worker; keep it between calls.
    pthread_mutex_lock(&light_mutex);
    light->button = *buttons;
    if ((light->button.LeftNorthSouthButton == 1 && previous->LeftNorthSouthButton == 0) ||
        (light->button.LeftSouthNorthButton == 1 && previous->LeftSouthNorthButton == 0)) {
        light->LeftNorthSouth = 1;
    }
    if ((light->button.RightNorthSouthButton == 1 && previous->RightNorthSouthButton == 0) ||
        (light->button.RightSouthNorthButton == 1 && previous->RightSouthNorthButton == 0)) {
        light->RightNorthSouth = 1;
    }
    if ((light->button.TopEastWestButton == 1 && previous->TopEastWestButton == 0) ||
        (light->button.TopWestEastButton == 1 && previous->TopWestEastButton == 0)) {
        light->TopEastWest = 1;
    }
    if ((light->button.BottomEastWestButton == 1 && previous->BottomEastWestButton == 0) ||
        (light->button.BottomWestEastButton == 1 && previous->BottomWestEastButton == 0)) {
        light->BottomEastWest = 1;
    }
    *previous = light->button;
    pthread_mutex_unlock(&light_mutex);
}

void *button_checker(void *state_ptr) {
    PedstrianLight *light = state_ptr;
    PedstrianButton previous = {0};
    while (1) {
        PedstrianButton buttons;
        ReadExternalButtons(&buttons); // Hardware work is OUTSIDE the mutex.
        CheckPedestrianButtons(light, &buttons, &previous);
        usleep(5000); // Same 5 ms polling interval as original.
    }
    return NULL;
}
