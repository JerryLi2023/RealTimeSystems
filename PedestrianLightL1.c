#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>
#include <sys/iomsg.h>
#include <string.h>
#include <time.h>

#ifndef BUF_SIZE
#define BUF_SIZE 100
#endif


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
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)vv
    int LNS;            // LeftNorthSouth
    int RNS;            // RightNorthSouth
    int TEW;            // TopEastWest
    int BEW;            // BottomEastWest
} Pedstrian_client_data;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Pedstrian_client_reply;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // our data (unique id from client)vv
    int LNS;            // LeftNorthSouth
    int RNS;            // RightNorthSouth
    int TEW;            // TopEastWest
    int BEW;            // BottomEastWest
    int time;
    int peroid;
    int stateChange;    // See if states have changed
} Pedstrian_server_data;
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Pedstrian_server_reply;

// prototypes
void PedestrianStates(void *state_ptr1, void *state_ptr2);
void *button_checker (void *state_ptr);
void *client_StartL1(void *state_ptr);
int client_PedestrianL1(void *state_ptr);
void *server_StartL1(void *state_ptr);
int server_PedestrianL1(void *state_ptr);
void DisplayPedestrianLights(const PedestrianLightStates *state, int flash_on);

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("Client running\n");

    pthread_t  th1, th2, th3;
    PedstrianLight light = {0};
    PedestrianLightStates state = {.states = TRAFFIC_RED};
    DisplayPedestrianLights(&state, 1);

    // Create and start the thread
    int error = pthread_create(&th1, NULL, button_checker, &light);
    if (error == 0) error = pthread_create(&th2, NULL, client_StartL1, &light);
    if (error == 0) error = pthread_create(&th3, NULL, server_StartL1, &light);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    int red_flash;
    int time;
    int peroid;

    while (1) {
        pthread_mutex_lock(&light_mutex);

        if (light.stateChange == 0) {
            pthread_mutex_unlock(&light_mutex);
            usleep(10000);
            continue;
        }

        red_flash = light.timer - (light.timer + 9) / 10; // Round clearance UP. 
        time = light.timer;
        peroid = light.peroid;
        PedestrianStates(&light, &state);
        light.stateChange = 0;
        light.phaseActive = 1;

        // Consume only the requests being served, under the same mutex.
        // New button edges arriving after this remain pending for next time.
        if (state.LeftNorthSouthLight) light.LeftNorthSouth = 0;
        if (state.RightNorthSouthLight) light.RightNorthSouth = 0;
        if (state.TopEastWestLight) light.TopEastWest = 0;
        if (state.BottomEastWestLight) light.BottomEastWest = 0;
        pthread_mutex_unlock(&light_mutex);

        for (int i = 0; i < time; i++) {
            if (i >= red_flash) {
                state.states = TRAFFIC_RED_Flash;
                // Two half-second output steps per second of clearance.
                for (int j = 0; j < peroid * 2; j++) {
                    DisplayPedestrianLights(&state, (j % 2) == 0);
                    struct timespec delay = {0, 500000000L};
                    while (nanosleep(&delay, &delay) == -1 && errno == EINTR) { }
                }
            } else {
                state.states = TRAFFIC_GREEN;
                DisplayPedestrianLights(&state, 1);
                // Set the output BEFORE waiting. Sleep may be interrupted.
                unsigned remaining = (unsigned)peroid;
                while (remaining != 0) remaining = sleep(remaining);
            }
        }

        // Always finish the complete cycle at steady red.
        state.states = TRAFFIC_RED;
        DisplayPedestrianLights(&state, 1);
        pthread_mutex_lock(&light_mutex);
        light.phaseActive = 0;
        pthread_mutex_unlock(&light_mutex);
    }
}

// Output adapter
void DisplayPedestrianLights(const PedestrianLightStates *state, int flash_on) {
    const int enabled[4] = {
        state->LeftNorthSouthLight, state->RightNorthSouthLight,
        state->TopEastWestLight, state->BottomEastWestLight
    };
    const char *name[4] = {"LNS", "RNS", "TEW", "BEW"};
    for (int i = 0; i < 4; i++) {
        const char *colour = "RED";
        if (enabled[i] && state->states == TRAFFIC_GREEN) colour = "GREEN";
        else if (enabled[i] && state->states == TRAFFIC_RED_Flash)
            colour = flash_on ? "RED (flashing)" : "OFF (flashing)";
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


void *button_checker (void *state_ptr) {

    PedstrianLight *light = state_ptr;
    PedstrianButton previous = {0};
    // INPUT TODO: a GPIO/keyboard reader must update light->button.
    // Use this same mutex when writing the fields; debounce GPIO first.

    while (1) {
        usleep(5000);
        pthread_mutex_lock(&light_mutex);
        if ((light->button.LeftNorthSouthButton == 1 && previous.LeftNorthSouthButton == 0) ||
            (light->button.LeftSouthNorthButton == 1 && previous.LeftSouthNorthButton == 0)) {
            light->LeftNorthSouth = 1;
        }
        if ((light->button.RightNorthSouthButton == 1 && previous.RightNorthSouthButton == 0) ||
            (light->button.RightSouthNorthButton == 1 && previous.RightSouthNorthButton == 0)) {
            light->RightNorthSouth = 1;
        }
        if ((light->button.TopEastWestButton == 1 && previous.TopEastWestButton == 0) ||
            (light->button.TopWestEastButton == 1 && previous.TopWestEastButton == 0)) {
            light->TopEastWest = 1;
        }
        if ((light->button.BottomEastWestButton == 1 && previous.BottomEastWestButton == 0) ||
            (light->button.BottomWestEastButton == 1 && previous.BottomWestEastButton == 0)) {
            light->BottomEastWest = 1;
        }
        previous = light->button;
        pthread_mutex_unlock(&light_mutex);
    }
    return NULL;
}

void *client_StartL1(void *state_ptr) {
    while (1) {
        sleep(2);
        (void)client_PedestrianL1(state_ptr);
    }

    printf("Main (client) Terminated....\n");
    return NULL;
}

/*** Client code ***/
int client_PedestrianL1(void *state_ptr) {

    PedstrianLight *light = state_ptr;

    int serverPID;
    int serverCHID;

    FILE *serverFile;

    serverFile = fopen("/tmp/PedestrianToTraffic_L1.info", "r");

    if (serverFile == NULL) {
        perror("Failed to open /tmp/PedestrianToTraffic_L1.info");
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

        printf("Server information loaded from file:\n");
    Pedstrian_client_data msg = {0};
    Pedstrian_client_reply reply = {0};

    msg.ClientID = 500;

    int server_coid;

    printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
    printf("   --> on channel: %d\n\n", serverCHID);

    // set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverCHID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1)
    {
        printf("\n    ERROR, could not connect to server!\n\n");
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
        pthread_mutex_lock(&light_mutex);
        msg.LNS = light->LeftNorthSouth;
        msg.RNS = light->RightNorthSouth;
        msg.TEW = light->TopEastWest;
        msg.BEW = light->BottomEastWest;
        pthread_mutex_unlock(&light_mutex);

        if (MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply)) == -1) {
            perror("MsgSend");
            break;
        } else { // now process the reply
            printf("   -->Reply is: '%.*s'\n", (int)sizeof(reply.buf), reply.buf);
        }
    }


    // Close the connection
    printf("\n Sending message to server to tell it to close the connection\n");
    ConnectDetach(server_coid);

    return EXIT_SUCCESS;
}

void *server_StartL1(void *state_ptr) {
    printf("Server running\n");

    (void)server_PedestrianL1(state_ptr);

    printf("Main (Server) Terminated....\n");
    return NULL;
}

/*** Server code ***/
int server_PedestrianL1(void *state_ptr) {
    int serverPID=0, chid=0;

    serverPID = getpid();

    PedstrianLight *light = state_ptr;

    // Create Channel
    chid = ChannelCreate(_NTO_CHF_DISCONNECT);
    if (chid == -1)  // _NTO_CHF_DISCONNECT flag used to allow detach
    {
        printf("\nFailed to create communication channel on server\n");
        return EXIT_FAILURE;
    }

    FILE *serverFile;

    serverFile = fopen("/tmp/TrafficToPedestrian_L1.info", "w");

    if (serverFile == NULL)
    {
        perror("Failed to open /tmp/TrafficToPedestrian_L1.info");
        ChannelDestroy(chid);
        return EXIT_FAILURE;
    }

    fprintf(serverFile, "%d\n%d\n", serverPID, chid);

    fclose(serverFile);

    printf("Server information written to /tmp/TrafficToPedestrian_L1.info\n");

    printf("Server Listening for Clients on:\n");
    printf("  --> Process ID   : %d \n", serverPID);
    printf("  --> Channel ID   : %d \n\n", chid);

    Pedstrian_server_data msg = {0};
    struct _msg_info info;
    int rcvid=0, msgnum=0;      // no message received yet
    int Stay_alive=0, living=0; // server stays running (ignores _PULSE_CODE_DISCONNECT request)

    Pedstrian_server_reply replymsg = {0};           // replymsg structure for sending back to client
    replymsg.hdr.type = 0x01;
    replymsg.hdr.subtype = 0x00;
    snprintf(replymsg.buf, sizeof(replymsg.buf), "OK");

    living =1;
    while (living)
    {
       // Do your MsgReceive's here now with the chid
       rcvid = MsgReceive(chid, &msg, sizeof(msg), &info);

       if (rcvid == -1)  // Error condition, exit
       {
           if (errno == EINTR) continue;
           perror("MsgReceive");
           break;
       }

       // did we receive a Pulse or message?
       // for Pulses:
       if (rcvid == 0)  //  Pulse received, work out what type
       {
           switch (msg.hdr.code)
           {
               case _PULSE_CODE_DISCONNECT:
                    // A client disconnected all its connections by running
                    // name_close() for each name_open()  or terminated
                   if( Stay_alive == 0)
                   {
                       ConnectDetach(msg.hdr.scoid);
                       printf("\nServer was told to Detach from connection:%d ...\n", msg.hdr.scoid);
                       continue;
                   }
                   else
                   {
                       printf("\nServer received Detach pulse from connection:%d but rejected it ...\n", msg.hdr.scoid);
                   }
                   break;

               case _PULSE_CODE_UNBLOCK:
                    // REPLY blocked client wants to unblock (was hit by a signal
                    // or timed out).  It's up to you if you reply now or later.
                   printf("\nServer got _PULSE_CODE_UNBLOCK after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_COIDDEATH:  // from the kernel
                   printf("\nServer got _PULSE_CODE_COIDDEATH after %d, msgnum\n", msgnum);
                   break;

               case _PULSE_CODE_THREADDEATH: // from the kernel
                   printf("\nServer got _PULSE_CODE_THREADDEATH after %d, msgnum\n", msgnum);
                   break;

               default:
                   // Some other pulse sent by one of your processes or the kernel
                   printf("\nServer got some other pulse after %d, msgnum\n", msgnum);
                   break;

           }
           continue;// go back to top of while loop
       }

       // for messages:
       if(rcvid > 0) {
           msgnum++;

            // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
            if (msg.hdr.type == _IO_CONNECT )
            {
                MsgReply( rcvid, EOK, NULL, 0 );
                printf("\n gns service is running....");
                continue;   // go back to top of while loop
            }

            // Some other I/O message was received; reject it
            if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
            {
                MsgError( rcvid, ENOSYS );
                printf("\n Server received and IO message and rejected it....");
                continue;   // go back to top of while loop
            }

            if ((size_t)info.msglen != sizeof(msg) ||
                (size_t)info.srcmsglen != sizeof(msg)) {
                MsgError(rcvid, EMSGSIZE);
                continue;
            }
            if (msg.hdr.type != 0x00 ||
                (msg.stateChange != 0 && msg.stateChange != 1)) {
                MsgError(rcvid, EINVAL);
                continue;
            }

            // An unchanged status must not overwrite a pending command.
            if (msg.stateChange == 1) {
                const int any_green = msg.LNS || msg.RNS || msg.TEW || msg.BEW;
                if ((msg.LNS != 0 && msg.LNS != 1) ||
                    (msg.RNS != 0 && msg.RNS != 1) ||
                    (msg.TEW != 0 && msg.TEW != 1) ||
                    (msg.BEW != 0 && msg.BEW != 1) ||
                    (any_green && (msg.time < 2 || msg.time > 3600 ||
                                   msg.peroid < 1 || msg.peroid > 3600 ||
                                   msg.time > 3600 / msg.peroid))) {
                    MsgError(rcvid, EINVAL);
                    continue;
                }
                pthread_mutex_lock(&light_mutex);
                if (light->phaseActive || light->stateChange) {
                    pthread_mutex_unlock(&light_mutex);
                    MsgError(rcvid, EBUSY);
                    continue;
                }
                light->LeftNorthSouthLight = msg.LNS;
                light->RightNorthSouthLight = msg.RNS;
                light->TopEastWestLight = msg.TEW;
                light->BottomEastWestLight = msg.BEW;
                light->timer = any_green ? msg.time : 0;
                light->peroid = any_green ? msg.peroid : 1;
                light->stateChange = 1;
                pthread_mutex_unlock(&light_mutex);
            }

           MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
       }
       else
       {
           printf("\nERROR: Server received something, but could not handle it correctly\n");
       }

    }

    printf("\nServer received Destroy command\n");
    // destroyed channel before exiting
    ChannelDestroy(chid);
    unlink("/tmp/TrafficToPedestrian_L1.info");

    return EXIT_FAILURE;
}
