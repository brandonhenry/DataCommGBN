// name: Will Storey and Brandon Henry
// netid: wms176 and bdh354
// assignment: PA2

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

using namespace std;

int main(int argc, char *argv[]) {
	int sending;
	int receiving;
	char receiverBuffer[1024];
	int nextSequencenum = 0;
	char sendData[12];
	char receiveData[37];

	socklen_t sentLength, receivedLength;

	struct hostent *sendAddress = gethostbyname(argv[1]);
    struct hostent *receiveAddress = gethostbyname("localhost");
    struct sockaddr_in emulatorSent;
	struct sockaddr_in emulatorReceived;

	packet *packetReceived = new packet(-1, -1, -1, receiveData);

	int init = 0;
	int initial = 0;

	sending = socket(AF_INET, SOCK_DGRAM, 0);
	receiving = socket(AF_INET, SOCK_DGRAM, 0);

	emulatorReceived.sin_family = AF_INET;
    emulatorReceived.sin_port = htons(atoi(argv[2]));

	memcpy(&emulatorReceived.sin_addr, receiveAddress->h_addr_list[0], receiveAddress->h_length);
	bind(receiving,(struct sockaddr *) &emulatorReceived, sizeof(emulatorReceived));

    emulatorSent.sin_family = AF_INET;
    emulatorSent.sin_port = htons(atoi(argv[3]));

	memcpy(&emulatorSent.sin_addr, sendAddress->h_addr_list[0], sendAddress->h_length);

	receivedLength = sizeof(emulatorReceived);
	sentLength = sizeof(emulatorSent);

	ofstream arrival_file("arrival.log"), out_file(argv[4]);

    while(1){

		bzero(receiverBuffer, sizeof(receiverBuffer));
		bzero(sendData, sizeof(sendData));
		bzero(receiveData, sizeof(receiveData));

		recvfrom(receiving, receiverBuffer, sizeof(receiverBuffer), 0, (struct sockaddr *) &emulatorReceived, &receivedLength);
		packetReceived->deserialize(receiverBuffer);

		arrival_file << packetReceived->getSeqNum() << endl;

		if (packetReceived->getType() == 3){
			if(init == initial){
				packet *packetSent = new packet(2, packetReceived->getSeqNum(), 0, NULL);
				packetSent->serialize(sendData);
				delete packetSent;
				sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, sentLength);
				break;
			}
		}

		if (packetReceived->getSeqNum() == (nextSequencenum % SEQUENCENUM)){
			if(init == initial){
				packet *packetSent = new packet(0, packetReceived->getSeqNum(), 0, NULL);
				packetSent->serialize(sendData);
				delete packetSent;
				sendto(sending, sendData, strlen(sendData), 0, (struct sockaddr *) &emulatorSent, sentLength);
				out_file << packetReceived->getData();
				nextSequencenum++;
			}
		}
	}

	close(receiving);
    close(sending);

	delete packetReceived;
    return 0;
}
