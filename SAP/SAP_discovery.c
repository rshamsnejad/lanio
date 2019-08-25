/*

SAP_discovery.c

Discover SAP announcement of Dante streams.

*/

#include "../headers/linux-aes67.h"

#define SAP_MULTICAST_ADDRESS "239.255.255.255"
#define SAP_MULTICAST_PORT 9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS "0.0.0.0"

#define SAP_PACKET_BUFFER_SIZE 1024

#define SAP_PACKET_PREHEADER_SIZE 8
#define SAP_PACKET_HEADER_SIZE 24


gint main(gint argc, gchar *argv[])
{

	// Set up an UDP/IPv4 socket for SAP discovery

	GSocket *SAPSocket = NULL;
	GError *SAPSocketOpenError = NULL;

	SAPSocket = g_socket_new
							(
								G_SOCKET_FAMILY_IPV4,
								G_SOCKET_TYPE_DATAGRAM,
								G_SOCKET_PROTOCOL_DEFAULT,
								&SAPSocketOpenError
							);

	ProcessGError("Error opening SAP socket", SAPSocketOpenError);


	// Bind the SAP socket to listen to all local interfaces

	GInetAddress *SAPLocalAddress = g_inet_address_new_from_string
																	(SAP_LOCAL_ADDRESS);
	GSocketAddress *SAPSocketAddress = g_inet_socket_address_new
																		 (SAPLocalAddress, SAP_MULTICAST_PORT);
	GError *SAPSocketBindError = NULL;

	g_socket_bind(SAPSocket, SAPSocketAddress, TRUE, &SAPSocketBindError);

	ProcessGError("Error binding SAP socket", SAPSocketBindError);


	// Subscribe to the SAP Multicast group

	GInetAddress *SAPMulticastAddress = g_inet_address_new_from_string
																			(SAP_MULTICAST_ADDRESS);
	GError *SAPMulticastJoinError = NULL;

	g_socket_join_multicast_group
	(
		SAPSocket,
		SAPMulticastAddress,
		FALSE,
		NULL,
		&SAPMulticastJoinError
	);

	ProcessGError("Error joining SAP Multicast group", SAPMulticastJoinError);


	// Begin the SAP packet receiving loop

	gchar SAPPacketString[SAP_PACKET_BUFFER_SIZE];
	gssize SAPPacketStringBytesRead = 0;
	GSocketAddress *SAPPacketSourceSocket = NULL;
	GInetAddress *SAPPacketSourceAddress = NULL;
	GError *SAPPacketError = NULL;

	while(TRUE)
	{
		// Reinitialize the packet string buffer
		memset(SAPPacketString, '\0', sizeof(SAPPacketString));

		SAPPacketStringBytesRead = g_socket_receive_from
															 (
																 SAPSocket,
																 &SAPPacketSourceSocket,
																 SAPPacketString,
																 sizeof(SAPPacketString),
																 NULL,
																 &SAPPacketError
															 );
		if(SAPPacketStringBytesRead < 0)
			ProcessGError("Error receiving SAP packet", SAPPacketError);
		else if(SAPPacketStringBytesRead == 0) // The connection has been closed
			break;

		SAPPacketSourceAddress = g_inet_socket_address_get_address
														 ((GInetSocketAddress *) SAPPacketSourceSocket);

		printf
		(
			"=== %s: %d byte SAP packet with %d byte SDP description :\n",
			g_inet_address_to_string(SAPPacketSourceAddress),
			SAPPacketStringBytesRead,
			SAPPacketStringBytesRead-SAP_PACKET_HEADER_SIZE
		);
		printf("=== TYPE : %s\n\n", &SAPPacketString[SAP_PACKET_PREHEADER_SIZE]);
		printf("%s\n\n\n\n", &SAPPacketString[SAP_PACKET_HEADER_SIZE]);

	}
}
