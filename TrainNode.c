#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/neutrino.h>

#define BUF_SIZE 100
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

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)
    int train_detected; // Flag to indicate if a train is detected
    int hardware_error; // Flag to indicate if there is a hardware error
} Train_client_data;

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Train_client_reply;

// Function prototypes
void TrainStateLogic(TrainData *trainData);
void TrainLogicNode(void *state_ptr, void *inputs);
void *client_Start_L1(void *state_ptr);
void *client_Start_L2(void *state_ptr);
void *client_Start_Controller(void *state_ptr);
int client_TrainL1(void *state_ptr);
int client_TrainL2(void *state_ptr);
int client_TrainController(void *state_ptr);


int main(void) {
    // Initialize the train data
    TrainData trainData = {
        .train_detected = 0,
        .hardware_error = 0,
        .hardware = {.BoomGate = 1, .RedLight = 1, .RedLightFlash = 0},
        .train_state = TRAIN_NOT_PRESENT,
        .gate_down_confirmed = 0
    };
    pthread_t  th1, th2, th3;
    int error = pthread_create(&th1, NULL, client_Start_L1, &trainData);
    if (error == 0) error = pthread_create(&th2, NULL, client_Start_L2, &trainData);
    if (error == 0) error = pthread_create(&th3, NULL, client_Start_Controller, &trainData);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read/simulate inputs here; protect shared updates with train_mutex.
        pthread_mutex_lock(&train_mutex);
        TrainStateLogic(&trainData);
        // Call the TrainLogicNode function to handle the train logic
        TrainLogicNode(&trainData, NULL);
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

void *client_Start_L1(void *state_ptr) {
    while (1) {
        sleep(2);
        (void)client_TrainL1(state_ptr);
    }

    return NULL;
}

void *client_Start_L2(void *state_ptr) {
    while (1) {
        sleep(2);
        (void)client_TrainL2(state_ptr);
    }

    return NULL;
}

void *client_Start_Controller(void *state_ptr) {
    while (1) {
        sleep(2);
        (void)client_TrainController(state_ptr);
    }

    return NULL;
}

/*** Client code ***/
int client_TrainL1(void *state_ptr) {

    TrainData *trainData = state_ptr;

    int serverPID;
    int serverCHID;

    FILE *serverFile;

    serverFile = fopen("/tmp/TrainToTraffic_L1.info", "r");

    if (serverFile == NULL) {
        perror("Failed to open /tmp/TrainToTraffic_L1.info");
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverPID) != 1) {
        printf("Failed to read server PID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverCHID) != 1) {
        printf("Failed to read server channel ID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    fclose(serverFile);
    if (serverPID <= 0 || serverCHID <= 0) {
        fprintf(stderr, "Invalid server PID or channel ID\n");
        return EXIT_FAILURE;
    }

    printf("Server information loaded from file:\n");
    Train_client_data msg = {0};
    Train_client_reply reply = {0};

    msg.ClientID = 500;

    int server_coid;

    printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
    printf("   --> on channel: %d\n\n", serverCHID);

    // set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1)
    {
        perror("ConnectAttach");
        return EXIT_FAILURE;
    }


    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverCHID);

    // We would have pre-defined data to stuff here
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;

    // Do whatever work you wanted with server connection
    while (1) {
        sleep(1);

        // Write your code
        pthread_mutex_lock(&train_mutex);
        msg.train_detected = trainData->train_detected;
        msg.hardware_error = trainData->hardware_error;
        pthread_mutex_unlock(&train_mutex);

        // Do not hold train_mutex while waiting for the remote reply.
        // Re-arm immediately before EACH send, covering both blocking states.
        uint64_t timeout_ns = 500000000ULL; // 500 ms (demo setting).
        memset(&reply, 0, sizeof(reply));
        if (TimerTimeout(CLOCK_MONOTONIC,
                         _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY,
                         NULL, &timeout_ns, NULL) == -1) {
            perror("TimerTimeout");
            break;
        }
        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            break;
        } else { // now process the reply
            printf("   -->Reply is: '%.*s'\n", (int)sizeof(reply.buf), reply.buf);
        }
    }


    // Close the connection
    printf("\n Detaching client connection; will retry\n");
    ConnectDetach(server_coid);

    return EXIT_FAILURE; // Inner loop ends only after an error.
}

/*** Client code ***/
int client_TrainL2(void *state_ptr) {

    TrainData *trainData = state_ptr;

    int serverPID;
    int serverCHID;

    FILE *serverFile;

    serverFile = fopen("/tmp/TrainToTraffic_L2.info", "r");

    if (serverFile == NULL) {
        perror("Failed to open /tmp/TrainToTraffic_L2.info");
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverPID) != 1) {
        printf("Failed to read server PID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverCHID) != 1) {
        printf("Failed to read server channel ID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    fclose(serverFile);
    if (serverPID <= 0 || serverCHID <= 0) {
        fprintf(stderr, "Invalid server PID or channel ID\n");
        return EXIT_FAILURE;
    }

    printf("Server information loaded from file:\n");
    Train_client_data msg = {0};
    Train_client_reply reply = {0};

    msg.ClientID = 500;

    int server_coid;

    printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
    printf("   --> on channel: %d\n\n", serverCHID);

    // set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1)
    {
        perror("ConnectAttach");
        return EXIT_FAILURE;
    }


    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverCHID);

    // We would have pre-defined data to stuff here
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;

    // Do whatever work you wanted with server connection
    while (1) {
        sleep(1);

        // Write your code
        pthread_mutex_lock(&train_mutex);
        msg.train_detected = trainData->train_detected;
        msg.hardware_error = trainData->hardware_error;
        pthread_mutex_unlock(&train_mutex);

        // Do not hold train_mutex while waiting for the remote reply.
        // Re-arm immediately before EACH send, covering both blocking states.
        uint64_t timeout_ns = 500000000ULL; // 500 ms (demo setting).
        memset(&reply, 0, sizeof(reply));
        if (TimerTimeout(CLOCK_MONOTONIC,
                         _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY,
                         NULL, &timeout_ns, NULL) == -1) {
            perror("TimerTimeout");
            break;
        }
        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            break;
        } else { // now process the reply
            printf("   -->Reply is: '%.*s'\n", (int)sizeof(reply.buf), reply.buf);
        }
    }


    // Close the connection
    printf("\n Detaching client connection; will retry\n");
    ConnectDetach(server_coid);

    return EXIT_FAILURE; // Inner loop ends only after an error.
}

/*** Client code ***/
int client_TrainController(void *state_ptr) {

    TrainData *trainData = state_ptr;

    int serverPID;
    int serverCHID;

    FILE *serverFile;

    serverFile = fopen("/tmp/TrainToController.info", "r");

    if (serverFile == NULL) {
        perror("Failed to open /tmp/TrainToController.info");
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverPID) != 1) {
        printf("Failed to read server PID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    if (fscanf(serverFile, "%d", &serverCHID) != 1) {
        printf("Failed to read server channel ID\n");
        fclose(serverFile);
        return EXIT_FAILURE;
    }

    fclose(serverFile);
    if (serverPID <= 0 || serverCHID <= 0) {
        fprintf(stderr, "Invalid server PID or channel ID\n");
        return EXIT_FAILURE;
    }

    printf("Server information loaded from file:\n");
    Train_client_data msg = {0};
    Train_client_reply reply = {0};

    msg.ClientID = 500;

    int server_coid;

    printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
    printf("   --> on channel: %d\n\n", serverCHID);

    // set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1)
    {
        perror("ConnectAttach");
        return EXIT_FAILURE;
    }


    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverCHID);

    // We would have pre-defined data to stuff here
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;

    // Do whatever work you wanted with server connection
    while (1) {
        sleep(1);

        // Write your code
        pthread_mutex_lock(&train_mutex);
        msg.train_detected = trainData->train_detected;
        msg.hardware_error = trainData->hardware_error;
        pthread_mutex_unlock(&train_mutex);

        // Do not hold train_mutex while waiting for the remote reply.
        // Re-arm immediately before EACH send, covering both blocking states.
        uint64_t timeout_ns = 500000000ULL; // 500 ms (demo setting).
        memset(&reply, 0, sizeof(reply));
        if (TimerTimeout(CLOCK_MONOTONIC,
                         _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY,
                         NULL, &timeout_ns, NULL) == -1) {
            perror("TimerTimeout");
            break;
        }
        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            break;
        } else { // now process the reply
            printf("   -->Reply is: '%.*s'\n", (int)sizeof(reply.buf), reply.buf);
        }
    }


    // Close the connection
    printf("\n Detaching client connection; will retry\n");
    ConnectDetach(server_coid);

    return EXIT_FAILURE; // Inner loop ends only after an error.
}
