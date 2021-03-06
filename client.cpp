// name: Will Storey and Brandon Henry
// netid: wms176 and bdh354
// assignment: PA2

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include "packet.cpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

using namespace std;

#define WINDOW_SIZE 7
#define SEQUENCENUM 8

int main (int argc, char *argv[]){

	// initialize variables
	int numSent = 0;
	int numReceived = 0;
	int base = 0;
	int init = 0;
	int myinitial = 0;

	int sending, receiving;
	int nextSequenceNum;

	char fileBuffer[1024][31];
	char sendData[37];
    char receiveData[12];
	char receiverBuffer[1024];
	char chunkBuffer[30];

	struct sockaddr_in emulatorSent, emulatorReceived;
	struct hostent *sendAddress = gethostbyname(argv[1]);
    struct hostent *receiveAddress = gethostbyname("localhost");
	struct timeval timeout;

	socklen_t sentLength, receivedLength;

	// setup timeout
	timeout.tv_sec = 0;
	timeout.tv_usec = 2000000;

	// create packet to store received data
	packet *packetReceived = new packet(-1, -1, -1, receiveData);

	// create send and receive sockets
	sending = socket(AF_INET, SOCK_DGRAM, 0);
	receiving = socket(AF_INET, SOCK_DGRAM, 0);

	// setup sent socket address
	emulatorSent.sin_family = AF_INET;
    emulatorSent.sin_port = htons(atoi(argv[2]));

	memcpy(&emulatorSent.sin_addr, sendAddress->h_addr_list[0], sendAddress->h_length);

	// setup received socket address
	emulatorReceived.sin_family = AF_INET;
    emulatorReceived.sin_port = htons(atoi(argv[3]));

	memcpy(&emulatorReceived.sin_addr, receiveAddress->h_addr_list[0], receiveAddress->h_length);

	// bind each piece for receiving
	bind(receiving,(struct sockaddr *) &emulatorReceived, sizeof(emulatorReceived));
	setsockopt(receiving, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	// obtain length of sent and receive addresses
	sentLength = sizeof(emulatorSent);
	receivedLength = sizeof(emulatorReceived);

	ifstream in_file(argv[4], ifstream::binary);
	ofstream seqnum_file("seqnum.log"), ack_file("ack.log");

	while(1){

        int fullSize = base + WINDOW_SIZE;

		    while ((numSent < (base + WINDOW_SIZE)) && (in_file.read(chunkBuffer, sizeof(chunkBuffer)).gcount() >= 1)){
			       bzero(sendData, sizeof(sendData));
			       nextSequenceNum = numSent % SEQUENCENUM;
			       strcpy(fileBuffer[numSent], chunkBuffer);
			       packet *packetSent = new packet(1, nextSequenceNum, strlen(fileBuffer[numSent]), fileBuffer[numSent]);
			       packetSent->serialize(sendData);
			       delete packetSent;
			       sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, sentLength);
			       printf("type: %d %s\n", nextSequenceNum, sendData);
			       seqnum_file << nextSequenceNum << endl;
			       numSent += 1;
			       bzero(chunkBuffer, sizeof(chunkBuffer));
		     }

		    bzero(receiverBuffer, sizeof(receiverBuffer));
		    bzero(receiveData, sizeof(receiveData));

		    if ((recvfrom(receiving, receiverBuffer, sizeof(receiverBuffer), 0, (struct sockaddr *) &emulatorReceived, &receivedLength) == -1) && (errno == 11)){
            int bottom = base - 1;
			      for (int n = bottom; n <numSent; ++n){
				         if(init == myinitial){
				               nextSequenceNum = n % SEQUENCENUM;
				               bzero(sendData, sizeof(sendData));
				               packet *packetSent = new packet(1, nextSequenceNum, strlen(fileBuffer[n]), fileBuffer[n]);
				               packetSent->serialize(sendData);
				               delete packetSent;
				               printf("Resent packet %d: %s\n", nextSequenceNum, sendData);
				               sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, sentLength);
				               seqnum_file << nextSequenceNum << endl;
				         }
			       }
			   continue;
		  }

		else{
			packetReceived->deserialize(receiverBuffer);

			if (packetReceived->getSeqNum() == (base % SEQUENCENUM)){
				ack_file << packetReceived->getSeqNum() << endl;
				base += 1;
				numReceived += 1;
			}
		}

		if (numSent == numReceived){
			nextSequenceNum = base % SEQUENCENUM;
			bzero(sendData, sizeof(sendData));
			packet *packetSent = new packet(3, nextSequenceNum, 0, NULL);
			packetSent->serialize(sendData);
			delete packetSent;
			sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, sentLength);
			cout << "Sending EOT packet to server." << endl;
			seqnum_file << nextSequenceNum << endl;

			bzero(receiverBuffer, sizeof(receiverBuffer));
			recvfrom(receiving, receiverBuffer, sizeof(receiverBuffer), 0, (struct sockaddr *) &emulatorReceived, &receivedLength);
			packetReceived->deserialize(receiverBuffer);
			if (packetReceived->getType() == 2){
				cout << "EOT packet received. Exiting.-----------------------------------" << endl;
				ack_file << packetReceived->getSeqNum() << endl;
				break;
			}

		}
		
	}

    close(sending);
	close(receiving);

	seqnum_file.close();
	ack_file.close();
	delete packetReceived;
    return 0;
}
