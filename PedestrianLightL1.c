#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>


pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
	int LeftNorthSouthButton;
	int LeftSouthNorthButton;
	int RightNorthSouthButton;
	int RightSouthButton;
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
	struct PedstrianButton button;
} PedstrianLight;
typedef enum {
    TRAFFIC_GREEN,
    TRAFFIC_RED_Flash,
} PedestrianState;
typedef struct {
    int LeftNorthSouthLight;
	int RightNorthSouthLight;
	int TopEastWestLight;
	int BottomEastWestLight;
	enum PedestrianState states;
} PedestrianLightStates;
typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
	int ClientID;       // our data (unique id from client)vv
    int LNS; 			// LeftNorthSouth
    int RNS;			// RightNorthSouth
    int TEW;			// TopEastWest
    int BEW;			// BottomEastWest
} Pedstrian_client_data;
typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Pedstrian_client_reply;
typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
	int ClientID;       // our data (unique id from client)vv
    int LNS; 			// LeftNorthSouth
    int RNS;			// RightNorthSouth
    int TEW;			// TopEastWest
    int BEW;			// BottomEastWest
    int time
    int peroid;
    int stateChange;    // See if states have changed
} Pedstrian_server_data;
typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Pedstrian_server_reply;

// prototypes
void PedestrianStates(void *state_ptr1, void *state_ptr2);
void *button_checker (void *state_ptr1, void *state_ptr2);
void *client_StartL1(void *state_ptr);
int client_PedestrianL1(void *state_ptr);
void *server_StartL1(void *state_ptr);
int server_PedestrianL1(void *state_ptr);

int main(int argc, char *argv[]) {
	printf("Client running\n");

	pthread_t  th1, th2, th3;
	void *retval;
	PedstrianLight light = {0};
	PedestrianLightStates state = {.states = TRAFFIC_RED_Flash};

	// Create and start the thread
	pthread_create (&th1, NULL, button_checker, &light);
	pthread_create (&th2, NULL, client_StartL1, &light);
	pthread_create (&th3, NULL, server_StartL1, &light);

	int red_flash;
	int time;
	int peroid;

	while (1) {
		pthread_mutex_lock(&light_mutex);
		red_flash = light.time + (light.time/10); 
		time = light.time;
		peroid = light.peroid;
		PedestrianStates(&light, &state)
		pthread_mutex_unlock(&light_mutex);

        for (int i = 0; i < time; i++) {
            sleep(peroid);
			if (time >= red_flash) {
				state.states = TRAFFIC_RED_Flash;
			} else {
				state.states = TRAFFIC_GREEN;
			}
        }
    }

	pthread_join (th1, &retval);
	pthread_join (th2, &retval);

	printf("Main (client) Terminated....\n");
	return ret;
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

	PedstrianLight *light = state_ptr1;

	while (1) {
		usleep(5000);
		pthread_mutex_lock(&light_mutex);
		if (light->button.LeftNorthSouthButton == 1 || light->button.LeftSouthNorthButton == 1) {
			light->LeftNorthSouth = 1;
		}
		if (light->button.RightNorthSouthButton == 1 || light->button.RightSouthNorthButton == 1) {
			light->RightNorthSouth = 1;
		}
		if (light->button.TopEastWestButton == 1 || light->button.TopWestEastButton == 1) {
			light->TopEastWest = 1;
		}
		if (light->button.BottomEastWestButton == 1 || light->button.BottomWestEastButton == 1) {
			light->BottomEastWest = 1;
		}
		pthread_mutex_unlock(&light_mutex);
	}
}

void *client_StartL1(void *state_ptr) {
	int ret=0;
	while (1) {
		sleep(2);
		ret = client_PedestrianL1(state_ptr);
	}

	printf("Main (client) Terminated....\n");
	return ret;
}

/*** Client code ***/
int client_PedestrianL1(void *state_ptr) {

	PedstrianLight *light = state_ptr1;

	// connection data (you may need to edit this)
	int serverPID;	// CHANGE THIS Value to PID of the server process
	int	serverCHID;			// CHANGE THIS Value to Channel ID of the server process (typically 1)

	FILE *serverFile;

	serverFile = fopen("/tmp/PedestrianToTraffic_L1.info", "r");

	if (serverFile == NULL) {
		perror("Failed to open /tmp/PedestrianToTraffic.info");
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
    Pedstrian_client_data msg;
    Pedstrian_client_reply reply;

    msg.ClientID = 500;

    int server_coid;

	printf("   --> Trying to connect (server) process which has a PID: %d\n",   serverPID);
	printf("   --> on channel: %d\n\n", serverChID);

	// set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverChID, _NTO_SIDE_CHANNEL, 0);
	if (server_coid == -1)
	{
        printf("\n    ERROR, could not connect to server!\n\n");
        return EXIT_FAILURE;
	}


    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverChID);

    // We would have pre-defined data to stuff here
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;
    char message;

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
			printf(" Error data '%d' NOT sent to server\n", msg.data); // maybe we did not get a reply from the server
			break;
        } else { // now process the reply
			printf("   -->Reply is: '%s'\n", reply.buf);
        }
    }


    // Close the connection
    printf("\n Sending message to server to tell it to close the connection\n");
    ConnectDetach(server_coid);

    return EXIT_SUCCESS;
}

void *server_StartL1(void *state_ptr) {
	printf("Server running\n");

    int ret=0;
    ret = server_PedestrianL1(state_ptr);

	printf("Main (Server) Terminated....\n");
	return ret;
}

/*** Server code ***/
int server_PedestrianL1(void *state_ptr) {
	int serverPID=0, chid=0; 	// Server PID and channel ID

	serverPID = getpid(); 		// get server process ID

	PedstrianLight *light = state_ptr1;

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


	Pedstrian_data msg;
	int rcvid=0, msgnum=0;  	// no message received yet
	int Stay_alive=0, living=0;	// server stays running (ignores _PULSE_CODE_DISCONNECT request)

	Pedstrian_reply replymsg; 			// replymsg structure for sending back to client
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
	   if(rcvid > 0) {
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
			sleep(1);
			
			pthread_mutex_lock(&light_mutex);
			light->LeftNorthSouthLight = msg.LNS;
			light->RightNorthSouthLight = msg.RNS;
			light->TopEastWestLight = msg.TEW;
			light->BottomEastWestLight = msg.BEW;
			light->timer = msg.time;
			light->peroid = msg.peroid;
			light->stateChange = msg.stateChange;
			pthread_mutex_unlock(&light_mutex);

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


	return EXIT_SUCCESS;
}