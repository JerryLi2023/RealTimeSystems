# Run make inside a QNX SDP environment for your chosen target architecture.
CC = qcc
CFLAGS = -std=gnu99 -Wall -Wextra
SUPPORT = SampleCode_DisplayTransport.c
HEADERS = SampleCode_DisplayProtocol.h SampleCode_DisplayTransport.h
all: controller l1 l2 ped1 ped2 train display
controller: ControllerNode.c
	$(CC) $(CFLAGS) ControllerNode.c -o $@
l1: TrafficLightNodeL1.c $(SUPPORT) $(HEADERS) SampleCode_DisplayTrafficAdapter.inc
	$(CC) $(CFLAGS) TrafficLightNodeL1.c $(SUPPORT) -o $@
l2: TrafficLightNodeL2.c $(SUPPORT) $(HEADERS) SampleCode_DisplayTrafficAdapter.inc
	$(CC) $(CFLAGS) TrafficLightNodeL2.c $(SUPPORT) -o $@
ped1: PedestrianLightL1.c $(SUPPORT) $(HEADERS) SampleCode_DisplayPedestrianAdapter.inc
	$(CC) $(CFLAGS) PedestrianLightL1.c $(SUPPORT) -o $@
ped2: PedestrianLightL2.c $(SUPPORT) $(HEADERS) SampleCode_DisplayPedestrianAdapter.inc
	$(CC) $(CFLAGS) PedestrianLightL2.c $(SUPPORT) -o $@
train: TrainNode.c $(SUPPORT) $(HEADERS)
	$(CC) $(CFLAGS) TrainNode.c $(SUPPORT) -o $@
display: DisplayNode_2D.c
	$(CC) $(CFLAGS) DisplayNode_2D.c -lsocket -o $@
.PHONY: all
