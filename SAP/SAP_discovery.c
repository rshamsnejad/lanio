/*

SAP_discovery.c

Discover SAP announcement of Dante streams.

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SAP_MULTICAST_PORT 9875
#define SAP_MULTICAST_IP "239.255.255.255"

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int addrlen, sap_socket, cnt;
	struct ip_mreq mreq;
	char message[1024];

	/* set up socket */
	sap_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(sap_socket < 0)
	{
		perror("Error opening SAP socket");
		exit(1);
	}

	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(SAP_MULTICAST_PORT);
	addrlen = sizeof(addr);

	/* receive */
	if(bind(sap_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		perror("Error binding SAP socket");
		exit(1);
	}

	mreq.imr_multiaddr.s_addr = inet_addr(SAP_MULTICAST_IP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if(setsockopt(sap_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		perror("Error subscribing to SAP multicast group");
		exit(1);
	}

	while (1)
	{
		memset(message, 0, sizeof(message));

		cnt = recvfrom(sap_socket, message, sizeof(message), 0,	(struct sockaddr *) &addr, &addrlen);
		if(cnt < 0)
		{
			perror("Error receiving SAP data");
			exit(1);
		}
		else if(cnt == 0)
			break;

			printf("=== %s: %d byte SAP packet with %d byte SDP description :\n", inet_ntoa(addr.sin_addr), cnt, cnt-24);
			printf("=== TYPE : %s\n\n", &message[8]);
			printf("%s\n\n\n\n", &message[24]);

			// for(int i = 0 ; i < cnt ; i++)
			// {
			// 	printf("message[%d] : %X\t%c\n", i, message[i], message[i]);
			// }
		}
}
