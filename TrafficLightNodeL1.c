#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

// State Machine Structure
typedef struct {
    int time;
    int peroid;
} Settings;
typedef enum {
    TRAFFIC_GREEN,
    TRAFFIC_YELLOW,
    TRAFFIC_RED
} TrafficLightState;
typedef enum {
    NS, // North-South
    EW, // East-West
    NW, // North-West
    WS, // West-South
    EN, // East-North
    SE, // South-East
} TrafficState;
typedef struct {
    int NE, NS, NW;
    int EN, ES, EW;
    int SN, SE, SW;
    int WN, WE, WS;
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} Movements;
typedef struct {
    int Left_NS, Right_NS;
    int Top_EW, Bottom_EW;
} ButtonPresses;
typedef struct {
    TrafficLightState trafficState;
    TrafficState trafficDirection;
    TrafficState controllerDirction;
    TrafficState trafficData;
    Movements outputL1;
    Movements outputL2;
    ButtonPresses buttons;
    int train_detected; // Flag to indicate if a train is detected
} TrafficLight;

// Pedestrian Structure
typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
	int ClientID;       // our data (unique id from client)vv
    int LNS; 			// LeftNorthSouth
    int RNS;			// RightNorthSouth
    int TEW;			// TopEastWest
    int BEW;			// BottomEastWest
} Pedstrian_server_data;
typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Pedstrian_server_reply;


// Global variables
Settings settings = {0};
int train_detected = 0; // Flag to indicate if a train is detected

// Function prototypes
void TrafficLogicNode(void *state_ptr, void *inputs);
void TrafficLogicNodeL2(void *state_ptr, void *inputs);
void TrainLogicNode(void *state_ptr, void *inputs);
void ControllerStateMachine(void *state_ptr, void *inputs);
void CrossCommunicationStateMachine(void *state_ptr, void *inputs);
void NoControllerStateMachine(void *state_ptr, void *inputs);
int server_PedestrianL1();

int main() {
    // Initialize the traffic light state
    TrafficLight light = {TRAFFIC_GREEN, NS, {0}, {0}, 0};

    pthread_t  th1;
	void *retval;

	// Create and start the thread
	pthread_create (&th1, NULL, server_PedestrianL1, NULL);


	pthread_join (th1, &retval);

    return 0;
}

void ControllerStateMachine(void *state_ptr, void *inputs) {
    // Implement the controller state machine logic here
    // This function will manage the traffic light states based on inputs and timing
    enum TrafficLight light = *(TrafficLight *)state_ptr;
    while (1) {
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (train_detected) {
                break; // Exit the loop if a train is detected
            }
        }
        if (train_detected) {
            TrainLogicNode(state_ptr, inputs);
        } else {
            light.trafficDirection = light.controllerDirction;
        }
    }
}

void CrossCommunicationStateMachine(void *state_ptr, void *inputs) {
    // Implement the cross-communication state machine logic here
    // This function will handle communication between different traffic light nodes
    enum TrafficLight light = *(TrafficLight *)state_ptr;
    while (1) {
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (train_detected) {
                break; // Exit the loop if a train is detected
            }
        }
        if (train_detected) {
            TrainLogicNode(state_ptr, inputs);
        } else {
            TrafficLogicNode(state_ptr, inputs);
            TrafficLogicNodeL2(state_ptr, inputs);
        }
    }
}

void NoControllerStateMachine(void *state_ptr, void *inputs) {
    // Implement the no-controller state machine logic here
    // This function will handle the traffic light behavior when there is no controller
    enum TrafficLight light = *(TrafficLight *)state_ptr;
    while (1) {
        for (int i = 0; i < settings.time; i++) {
            sleep(settings.peroid);
            if (train_detected) {
                break; // Exit the loop if a train is detected
            }
        }
        if (train_detected) {
            TrainLogicNode(state_ptr, inputs);
        } else {
            TrafficLogicNode(state_ptr, inputs);
        }
    }
}

void TrafficLogicNode(void *state_ptr, void *inputs) {

    enum TrafficLight light = *(TrafficLight *)state_ptr;

    while (1) {
        // Simulate traffic light state changes
        switch (light.trafficState) {
            case NS:
                if (light.button.Right_NS) {
                    light.outputL1.NE = 0;
                    light.outputL1.Right_NS = 1;
                } else {
                    light.outputL1.NE = 1;
                    light.outputL1.Right_NS = 0;
                }
                light.outputL1.NS = 1;
                light.outputL1.NW = 0;
                light.outputL1.EN = 0;
                light.outputL1.ES = 0;
                light.outputL1.EW = 0;
                light.outputL1.SN = 1;
                light.outputL1.SE = 0;
                if (light.button.Left_NS) {
                    light.outputL1.SW = 0;
                    light.outputL1.Left_NS = 1;
                } else {
                    light.outputL1.SW = 1;
                    light.outputL1.Left_NS = 0;
                }
                light.outputL1.WN = 0;
                light.outputL1.WE = 0;
                light.outputL1.WS = 0;
                light.outputL1.Top_EW = 0;
                light.outputL1.Bottom_EW = 0;
                break;
            case EW:
                light.outputL1.NE = 0;
                light.outputL1.NS = 0;
                light.outputL1.NW = 0;
                light.outputL1.EN = 0;
                if (light.button.Bottom_EW) {
                    light.outputL1.ES = 0;
                    light.outputL1.Bottom_EW = 1;
                } else {
                    light.outputL1.ES = 1;
                    light.outputL1.Bottom_EW = 0;
                }
                light.outputL1.EW = 1;
                light.outputL1.SN = 0;
                light.outputL1.SE = 0;
                light.outputL1.SW = 0;
                if (light.button.Top_EW) {
                    light.outputL1.WN = 0;
                    light.outputL1.Top_EW = 1;
                } else {
                    light.outputL1.WN = 1;
                    light.outputL1.Top_EW = 0;
                }
                light.outputL1.WE = 1;
                light.outputL1.WS = 0;
                light.outputL1.Left_NS = 0;
                light.outputL1.Right_NS = 0;
                break;
            case NW:
                if (light.button.Right_NS) {
                    light.outputL1.NE = 0;
                    light.outputL1.Right_NS = 1;
                } else {
                    light.outputL1.NE = 1;
                    light.outputL1.Right_NS = 0;
                }
                if (light.button.Bottom_NS) {
                    light.outputL1.NS = 0;
                    light.outputL1.Bottom_NS = 1;
                } else {
                    light.outputL1.NS = 1;
                    light.outputL1.Bottom_NS = 0;
                }
                light.outputL1.NW = 1;
                light.outputL1.EN = 0;
                light.outputL1.ES = 0;
                light.outputL1.EW = 0;
                light.outputL1.SN = 0;
                light.outputL1.SE = 0;
                light.outputL1.SW = 0;
                light.outputL1.WN = 1;
                light.outputL1.WE = 0;
                light.outputL1.WS = 0;
                light.outputL1.Left_NS = 0;
                light.outputL1.Top_EW = 0;
                break;
            case WS:
                light.outputL1.NE = 0;
                light.outputL1.NS = 0;
                light.outputL1.NW = 0;
                light.outputL1.EN = 0;
                light.outputL1.ES = 0;
                light.outputL1.EW = 0;
                light.outputL1.SN = 0;
                light.outputL1.SE = 0;
                light.outputL1.SW = 1;
                if (light.button.Top_EW) {
                    light.outputL1.WN = 0;
                    light.outputL1.Top_EW = 1;
                } else {
                    light.outputL1.WN = 1;
                    light.outputL1.Top_EW = 0;
                }
                if (light.button.Right_NS) {
                    light.outputL1.WE = 0;
                    light.outputL1.Right_NS = 1;
                } else {
                    light.outputL1.WE = 1;
                    light.outputL1.Right_NS = 0;
                }
                light.outputL1.WS = 1;
                light.outputL1.Left_NS = 0;
                light.outputL1.Bottom_EW = 0;
                break;
            case EN:
                light.outputL1.NE = 1;
                light.outputL1.NS = 0;
                light.outputL1.NW = 0;
                light.outputL1.EN = 1;
                if (light.button.Bottom_EW) {
                    light.outputL1.ES = 0;
                    light.outputL1.Bottom_EW = 1;
                } else {
                    light.outputL1.ES = 1;
                    light.outputL1.Bottom_EW = 0;
                }
                if (light.button.Left_NS) {
                    light.outputL1.EW = 0;
                    light.outputL1.Left_NS = 1;
                } else {
                    light.outputL1.EW = 1;
                    light.outputL1.Left_NS = 0;
                }
                light.outputL1.SN = 0;
                light.outputL1.SE = 0;
                light.outputL1.SW = 0;
                light.outputL1.WN = 0;
                light.outputL1.WE = 0;
                light.outputL1.WS = 0;
                light.outputL1.Top_EW = 0;
                light.outputL1.Right_NS = 0;
                break;
            case SE:
                light.outputL1.NE = 0;
                light.outputL1.NS = 0;
                light.outputL1.NW = 0;
                light.outputL1.EN = 0;
                light.outputL1.ES = 1;
                light.outputL1.EW = 0;
                if (light.button.Top_EW) {
                    light.outputL1.SN = 0;
                    light.outputL1.Top_EW = 1;
                } else {
                    light.outputL1.SN = 1;
                    light.outputL1.Top_EW = 0;
                }
                light.outputL1.SE = 1;
                if (light.button.Left_NS) {
                    light.outputL1.SW = 0;
                    light.outputL1.Left_NS = 1;
                } else {
                    light.outputL1.SW = 1;
                    light.outputL1.Left_NS = 0;
                }
                light.outputL1.WN = 0;
                light.outputL1.WE = 0;
                light.outputL1.WS = 0;
                light.outputL1.Bottom_EW = 0;
                light.outputL1.Right_NS = 0;
                break;
            default:
                light.outputL1.NE = 0;
                light.outputL1.NS = 0;
                light.outputL1.NW = 0;
                light.outputL1.EN = 0;
                light.outputL1.ES = 0;
                light.outputL1.EW = 0;
                light.outputL1.SN = 0;
                light.outputL1.SE = 0;
                light.outputL1.SW = 0;
                light.outputL1.WN = 0;
                light.outputL1.WE = 0;
                light.outputL1.WS = 0;
                light.outputL1.Left_NS = 0;
                light.outputL1.Right_NS = 0;
                light.outputL1.Top_EW = 0;
                light.outputL1.Bottom_EW = 0;
                break;
        }
    }
}

void TrafficLogicNodeL2(void *state_ptr, void *inputs) {

    enum TrafficLight light = *(TrafficLight *)state_ptr;

    while (1) {
        // Simulate traffic light state changes
        switch (light.trafficState) {
            case NS:
                if (light.button.Right_NS) {
                    light.outputL2.NE = 0;
                    light.outputL2.Right_NS = 1;
                } else {
                    light.outputL2.NE = 1;
                    light.outputL2.Right_NS = 0;
                }
                light.outputL2.NS = 1;
                light.outputL2.NW = 0;
                light.outputL2.EN = 0;
                light.outputL2.ES = 0;
                light.outputL2.EW = 0;
                light.outputL2.SN = 1;
                light.outputL2.SE = 0;
                if (light.button.Left_NS) {
                    light.outputL2.SW = 0;
                    light.outputL2.Left_NS = 1;
                } else {
                    light.outputL2.SW = 1;
                    light.outputL2.Left_NS = 0;
                }
                light.outputL2.WN = 0;
                light.outputL2.WE = 0;
                light.outputL2.WS = 0;
                light.outputL2.Top_EW = 0;
                light.outputL2.Bottom_EW = 0;
                break;
            case EW:
                light.outputL2.NE = 0;
                light.outputL2.NS = 0;
                light.outputL2.NW = 0;
                light.outputL2.EN = 0;
                if (light.button.Bottom_EW) {
                    light.outputL2.ES = 0;
                    light.outputL2.Bottom_EW = 1;
                } else {
                    light.outputL2.ES = 1;
                    light.outputL2.Bottom_EW = 0;
                }
                light.outputL2.EW = 1;
                light.outputL2.SN = 0;
                light.outputL2.SE = 0;
                light.outputL2.SW = 0;
                if (light.button.Top_EW) {
                    light.outputL2.WN = 0;
                    light.outputL2.Top_EW = 1;
                } else {
                    light.outputL2.WN = 1;
                    light.outputL2.Top_EW = 0;
                }
                light.outputL2.WE = 1;
                light.outputL2.WS = 0;
                light.outputL2.Left_NS = 0;
                light.outputL2.Right_NS = 0;
                break;
            case NW:
                if (light.button.Right_NS) {
                    light.outputL2.NE = 0;
                    light.outputL2.Right_NS = 1;
                } else {
                    light.outputL2.NE = 1;
                    light.outputL2.Right_NS = 0;
                }
                if (light.button.Bottom_NS) {
                    light.outputL2.NS = 0;
                    light.outputL2.Bottom_NS = 1;
                } else {
                    light.outputL2.NS = 1;
                    light.outputL2.Bottom_NS = 0;
                }
                light.outputL2.NW = 1;
                light.outputL2.EN = 0;
                light.outputL2.ES = 0;
                light.outputL2.EW = 0;
                light.outputL2.SN = 0;
                light.outputL2.SE = 0;
                light.outputL2.SW = 0;
                light.outputL2.WN = 1;
                light.outputL2.WE = 0;
                light.outputL2.WS = 0;
                light.outputL2.Left_NS = 0;
                light.outputL2.Top_EW = 0;
                break;
            case WS:
                light.outputL2.NE = 0;
                light.outputL2.NS = 0;
                light.outputL2.NW = 0;
                light.outputL2.EN = 0;
                light.outputL2.ES = 0;
                light.outputL2.EW = 0;
                light.outputL2.SN = 0;
                light.outputL2.SE = 0;
                light.outputL2.SW = 1;
                if (light.button.Top_EW) {
                    light.outputL2.WN = 0;
                    light.outputL2.Top_EW = 1;
                } else {
                    light.outputL2.WN = 1;
                    light.outputL2.Top_EW = 0;
                }
                if (light.button.Right_NS) {
                    light.outputL2.WE = 0;
                    light.outputL2.Right_NS = 1;
                } else {
                    light.outputL2.WE = 1;
                    light.outputL2.Right_NS = 0;
                }
                light.outputL2.WS = 1;
                light.outputL2.Left_NS = 0;
                light.outputL2.Bottom_EW = 0;
                break;
            case EN:
                light.outputL2.NE = 1;
                light.outputL2.NS = 0;
                light.outputL2.NW = 0;
                light.outputL2.EN = 1;
                if (light.button.Bottom_EW) {
                    light.outputL2.ES = 0;
                    light.outputL2.Bottom_EW = 1;
                } else {
                    light.outputL2.ES = 1;
                    light.outputL2.Bottom_EW = 0;
                }
                if (light.button.Left_NS) {
                    light.outputL2.EW = 0;
                    light.outputL2.Left_NS = 1;
                } else {
                    light.outputL2.EW = 1;
                    light.outputL2.Left_NS = 0;
                }
                light.outputL2.SN = 0;
                light.outputL2.SE = 0;
                light.outputL2.SW = 0;
                light.outputL2.WN = 0;
                light.outputL2.WE = 0;
                light.outputL2.WS = 0;
                light.outputL2.Top_EW = 0;
                light.outputL2.Right_NS = 0;
                break;
            case SE:
                light.outputL2.NE = 0;
                light.outputL2.NS = 0;
                light.outputL2.NW = 0;
                light.outputL2.EN = 0;
                light.outputL2.ES = 1;
                light.outputL2.EW = 0;
                if (light.button.Top_EW) {
                    light.outputL2.SN = 0;
                    light.outputL2.Top_EW = 1;
                } else {
                    light.outputL2.SN = 1;
                    light.outputL2.Top_EW = 0;
                }
                light.outputL2.SE = 1;
                if (light.button.Left_NS) {
                    light.outputL2.SW = 0;
                    light.outputL2.Left_NS = 1;
                } else {
                    light.outputL2.SW = 1;
                    light.outputL2.Left_NS = 0;
                }
                light.outputL2.WN = 0;
                light.outputL2.WE = 0;
                light.outputL2.WS = 0;
                light.outputL2.Bottom_EW = 0;
                light.outputL2.Right_NS = 0;
                break;
            default:
                light.outputL2.NE = 0;
                light.outputL2.NS = 0;
                light.outputL2.NW = 0;
                light.outputL2.EN = 0;
                light.outputL2.ES = 0;
                light.outputL2.EW = 0;
                light.outputL2.SN = 0;
                light.outputL2.SE = 0;
                light.outputL2.SW = 0;
                light.outputL2.WN = 0;
                light.outputL2.WE = 0;
                light.outputL2.WS = 0;
                light.outputL2.Left_NS = 0;
                light.outputL2.Right_NS = 0;
                light.outputL2.Top_EW = 0;
                light.outputL2.Bottom_EW = 0;
                break;
        }
    }
}

void TrainLogicNode(void *state_ptr, void *inputs) {

    enum TrafficLight light = *(TrafficLight *)state_ptr

    switch (light.trafficState) {
        case NS:
            if (light.button.Right_NS) {
                light.output.NE = 0;
                light.output.Right_NS = 1;
            } else {
                light.output.NE = 1;
                light.output.Right_NS = 0;
            }
            light.output.NS = 1;
            light.output.NW = 0;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 1;
            light.output.SE = 0;
            if (light.button.Left_NS) {
                light.output.SW = 0;
                light.output.Left_NS = 1;
            } else {
                light.output.SW = 1;
                light.output.Left_NS = 0;
            }
            light.output.WN = 0;
            light.output.WE = 0;
            light.output.WS = 0;
            light.output.Top_EW = 0;
            light.output.Bottom_EW = 0;
            break;
        case NW:
            if (light.button.Right_NS) {
                light.output.NE = 0;
                light.output.Right_NS = 1;
            } else {
                light.output.NE = 1;
                light.output.Right_NS = 0;
            }
            if (light.button.Bottom_NS) {
                light.output.NS = 0;
                light.output.Bottom_NS = 1;
            } else {
                light.output.NS = 1;
                light.output.Bottom_NS = 0;
            }
            light.output.NW = 1;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 0;
            light.output.SE = 0;
            light.output.SW = 0;
            light.output.WN = 1;
            light.output.WE = 0;
            light.output.WS = 0;
            light.output.Left_NS = 0;
            light.output.Top_EW = 0;
        case WS:
            light.output.NE = 0;
            light.output.NS = 0;
            light.output.NW = 0;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 0;
            light.output.SE = 0;
            light.output.SW = 1;
            if (light.button.Top_EW) {
                light.output.WN = 0;
                light.output.Top_EW = 1;
            } else {
            light.output.WN = 1;
                light.output.Top_EW = 0;
            }
            if (light.button.Right_NS) {
                light.output.WE = 0;
                light.output.Right_NS = 1;
            } else {
                light.output.WE = 1;
                light.output.Right_NS = 0;
            }
            light.output.WS = 1;
            light.output.Left_NS = 0;
            light.output.Bottom_EW = 0;
            break;
        default:
            light.output.NE = 0;
            light.output.NS = 0;
            light.output.NW = 0;
            light.output.EN = 0;
            light.output.ES = 0;
            light.output.EW = 0;
            light.output.SN = 0;
            light.output.SE = 0;
            light.output.SW = 0;
            light.output.WN = 0;
            light.output.WE = 0;
            light.output.WS = 0;
            light.output.Left_NS = 0;
            light.output.Right_NS = 0;
            light.output.Top_EW = 0;
            light.output.Bottom_EW = 0;
            break;
    }
}

/*** Server code ***/
int server_PedestrianL1(void *state_ptr) {
	int serverPID=0, chid=0; 	// Server PID and channel ID

    enum TrafficLight light = *(TrafficLight *)state_ptr

	serverPID = getpid(); 		// get server process ID

	// Create Channel
	chid = ChannelCreate(_NTO_CHF_DISCONNECT);
	if (chid == -1)  // _NTO_CHF_DISCONNECT flag used to allow detach
	{
	    printf("\nFailed to create communication channel on server\n");
		return EXIT_FAILURE;
	}

	FILE *serverFile;

	serverFile = fopen("/tmp/PedestrianL1.info", "w");

	if (serverFile == NULL)
	{
	    perror("Failed to open /tmp/myServer.info");
	    ChannelDestroy(chid);
	    return EXIT_FAILURE;
	}

	fprintf(serverFile, "%d\n%d\n", serverPID, chid);

	fclose(serverFile);

	printf("Server information written to /tmp/myServer.info\n");

	printf("Server Listening for Clients on:\n");
	printf("These printf statements can be removed when the myServer.info file is implemented\n");
	printf("  --> Process ID   : %d \n", serverPID);
	printf("  --> Channel ID   : %d \n\n", chid);

	/*
	 *   Your code here to write this information to a file at a known location so a client can grab it...
	 *
	 *   The data should be written to a file like:
	 *   /tmp/myServer.info
	 *	 serverPID  (first line of file)
 	 *	 Channel ID (second line of file)
	 */


	Pedstrian_server_data msg;
	int rcvid=0, msgnum=0;  	// no message received yet
	int Stay_alive=0, living=0;	// server stays running (ignores _PULSE_CODE_DISCONNECT request)

	Pedstrian_server_reply replymsg; 			// replymsg structure for sending back to client
	replymsg.hdr.type = 0x01;
	replymsg.hdr.subtype = 0x00;
	enum states CurrentState = State0;

	living =1;
	while (living)
	{
	   // Do your MsgReceive's here now with the chid
	   rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);

	   if (rcvid == -1)  // Error condition, exit
	   {
		   printf("\nFailed to MsgReceive\n");
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
					   printf("\nServer was told to Detach from ClientID:%d ...\n", msg.ClientID);
					   continue;
				   }
				   else
				   {
					   printf("\nServer received Detach pulse from ClientID:%d but rejected it ...\n", msg.ClientID);
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
	   if(rcvid > 0) // if true then A message was received
	   {
		   msgnum++;

		   // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
		   if (msg.hdr.type == _IO_CONNECT )
		   {
			   MsgReply( rcvid, EOK, NULL, 0 );
			   printf("\n gns service is running....");
			   continue;	// go back to top of while loop
		   }

		   // Some other I/O message was received; reject it
		   if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
		   {
			   MsgError( rcvid, ENOSYS );
			   printf("\n Server received and IO message and rejected it....");
			   continue;	// go back to top of while loop
		   }

		   // A message (presumably ours) received

		   // put your message handling code here and assemble a reply message
		   light.trafficData.Left_NS = msg.LNS;
           light.trafficData.Right_NS = msg.RNS;
           light.trafficData.Top_EW = msg.TEW;
           light.trafficData.Bottom_EW = msg.BEW;

		   MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
	   } else {
		   printf("\nERROR: Server received something, but could not handle it correctly\n");
	   }

	}

	printf("\nServer received Destroy command\n");
	// destroyed channel before exiting
	ChannelDestroy(chid);


	return EXIT_SUCCESS;
}
