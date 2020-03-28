// server.cpp
// authors: Antonio Barnes (adb680) and Jeremy Brown (jdb587)

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include "packet.cpp"
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define SEQUENCENUM 8
// Sequence Numbers
using namespace std;

int main(int argc, char *argv[]) {
	
	
	//Data that is received and sent 
	int sending;
	int receiving;
	//Buffering size for the receiver
	char receiverBuffer[1024];
	//Start the sequence numbers
	int nextSequencenum = 0;
	//Size of data sent
	char sendData[12];
	//Size of data received
	char receiveData[37];
	//Size of emulators
	socklen_t size1, size2;
	//Get the host address coverted sent
	struct hostent *sendAddress = gethostbyname(argv[1]);
	//Get the host address converted received
    	struct hostent *receiveAddress = gethostbyname("localhost");
	//Stuff sent from Emulator
    	struct sockaddr_in emulatorSent;
	//Stuff received from emulator
	struct sockaddr_in emulatorReceived;
	int loop = 0;
	
	
	packet *packetReceived = new packet(-1, -1, -1, receiveData); // Initialize the packet that contains the received data


	int initial = 0;	

	//Making the packet
	sending = socket(AF_INET, SOCK_DGRAM, 0);
	receiving = socket(AF_INET, SOCK_DGRAM, 0);
	
	// This is the server receiving
	emulatorReceived.sin_family = AF_INET;
    	emulatorReceived.sin_port = htons(atoi(argv[2]));
	memcpy(&emulatorReceived.sin_addr, receiveAddress->h_addr_list[0], receiveAddress->h_length);
	//Bind
	bind(receiving,(struct sockaddr *) &emulatorReceived, sizeof(emulatorReceived));
	
	// Sending phase
    	emulatorSent.sin_family = AF_INET;
    	emulatorSent.sin_port = htons(atoi(argv[3]));
	memcpy(&emulatorSent.sin_addr, sendAddress->h_addr_list[0], sendAddress->h_length);
	
	//Size of emulator 2
	size2 = sizeof(emulatorReceived);
	//Size of emulator 1
    	size1 = sizeof(emulatorSent);


	//Output files
	ofstream arrival_file("arrival.log"), out_file(argv[4]); 

    while(1) {
		
		// Clear buffers
		bzero(receiverBuffer, sizeof(receiverBuffer));
		bzero(sendData, sizeof(sendData));
		bzero(receiveData, sizeof(receiveData));
		
		// Receive data
		recvfrom(receiving, receiverBuffer, sizeof(receiverBuffer), 0, (struct sockaddr *) &emulatorReceived, &size2);
		packetReceived->deserialize(receiverBuffer);
		
		// Put sequence number to the arrival log
		arrival_file << packetReceived->getSeqNum() << endl;
		
		// When received, send
		if (packetReceived->getType() == 3) 
		{
			if(loop == initial) 
			{
			packet *packetSent = new packet(2, packetReceived->getSeqNum(), 0, NULL);
			packetSent->serialize(sendData);
			delete packetSent;
			sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, size1);
			break;
			}
		}
					
		//Give the ack
		if (packetReceived->getSeqNum() == (nextSequencenum % SEQUENCENUM)) 
		{
			if(loop == initial)
			{
			packet *packetSent = new packet(0, packetReceived->getSeqNum(), 0, NULL);
			//Serialize sent packet
			packetSent->serialize(sendData);
			//delete packet object
			delete packetSent;
			sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, size1);
			//Send to output
			out_file << packetReceived->getData(); 
			nextSequencenum++;
			}
		}
		
	}
	//Close port
	close(receiving);
	//Close port
    	close(sending);
	//Delete packet object
	delete packetReceived; 
    	return 0;
}