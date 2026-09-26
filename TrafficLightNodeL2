#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <unistd.h>


typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
	int ClientID;       // our data (unique id from client)vv
    int LNS; 			// LeftNorthSouth
    int RNS;			// RightNorthSouth
    int TEW;			// TopEastWest
    int BEW;			// BottomEastWest
} Pedstrian_data;

typedef struct {
	struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} Pedstrian_reply;

// prototypes
int server_PedestrianL1();

int main(int argc, char *argv[]) {
	printf("Control node running\n");

	pthread_t  th1;
	void *retval;

	// Create and start the thread
	pthread_create (&th1, NULL, button_checker, NULL);


	pthread_join (th1, &retval);

	printf("Main Controller Terminated....\n");
	return ret;
}

/*** Server code ***/
int server_PedestrianL1()
{
	int serverPID=0, chid=0; 	// Server PID and channel ID

	serverPID = getpid(); 		// get server process ID

	// Create Channel
	chid = ChannelCreate(_NTO_CHF_DISCONNECT);
	if (chid == -1)  // _NTO_CHF_DISCONNECT flag used to allow detach
	{
	    printf("\nFailed to create communication channel on server\n");
		return EXIT_FAILURE;
	}

	FILE *serverFile;

	serverFile = fopen("/tmp/myServer.info", "w");

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

		   SingleStep_TrafficLight_SM(&CurrentState, &msg.data);  // pass address

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
