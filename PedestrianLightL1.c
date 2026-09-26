#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

int LeftNorthSouth;
int RightNorthSouth;
int TopEastWest;
int BottomEastWest;
int LeftNorthSouthLight;
int RightNorthSouthLight;
int TopEastWestLight;
int BottomEastWestLight;
int timer;


typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
	int ClientID;       // our data (unique id from client)vv
    int LNS; 			// LeftNorthSouth
    int RNS;			// RightNorthSouth
    int TEW;			// TopEastWest
    int BEW;			// BottomEastWest
} my_data;

typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
	int LNSL; 			// LeftNorthSouth
	int RNSL;			// RightNorthSouth
	int TEWL;			// TopEastWest
	int BEWL;			// BottomEastWest
    int Time;
} my_reply;


// prototypes
int client(int serverPID, int serverCHID);
void *button_checker (void *data);
void *client_setup (void *data);

int main(int argc, char *argv[]) {
	printf("Client running\n");

	pthread_t  th1, th2;
	void *retval;

	// Create and start the thread
	pthread_create (&th1, NULL, button_checker, NULL);
	pthread_create (&th2, NULL, client_setup, NULL);


	pthread_join (th1, &retval);
	pthread_join (th2, &retval);

	printf("Main (client) Terminated....\n");
	return ret;
}

void *button_checker (void *data) {
	while (1) {
		usleep(5000);
		if (LeftNorthSouthButton == 1 || LeftSouthNorthButton == 1) {
			LeftNorthSouth = 1;
		}
		if (RightNorthSouthButton == 1 || RightSouthNorthButton == 1) {
			RightNorthSouth = 1;
		}
		if (TopEastWestButton == 1 || TopWestEastButton == 1) {
			TopEastWest = 1;
		}
		if (BottomEastWestButton == 1 || BottomWestEastButton == 1) {
			BottomEastWest = 1;
		}
	}
}

void *client_setup (void *data) {
	printf("Client running\n");
	// connection data (you may need to edit this)
	int serverPID;	// CHANGE THIS Value to PID of the server process
	int	serverCHID;			// CHANGE THIS Value to Channel ID of the server process (typically 1)

	FILE *serverFile;

	serverFile = fopen("/tmp/PedestrianL1.info", "r");

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
	int ret=0;
	ret = client(serverPID, serverCHID);
}

/*** Client code ***/
int client(int serverPID,  int serverChID) {
    my_data msg;
    my_reply reply;

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
    	msg.LNS = LeftNorthSouth;
    	msg.RNS = RightNorthSouth;
    	msg.TEW = TopEastWest;
    	msg.BEW = BottomEastWest;

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
