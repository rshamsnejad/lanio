/*

SAP_discovery.c

Discover SAP announcement of Dante streams.

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "../headers/linux-aes67.h"

#define SAP_MULTICAST_ADDRESS "239.255.255.255"
#define SAP_MULTICAST_PORT 9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS "0.0.0.0"

#define SAP_PACKET_BUFFER_SIZE 1024

#define SAP_PACKET_PREHEADER_SIZE 8
#define SAP_PACKET_HEADER_SIZE 24

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void openSocket(GSocket **Socket, GSocketFamily SocketFamily,
								GSocketType SocketType,	GSocketProtocol SocketProtocol)
{
	GError *SocketOpenError = NULL;

	*Socket = g_socket_new(SocketFamily, SocketType,
													SocketProtocol, &SocketOpenError);

	processGError("Error opening SAP socket", SocketOpenError);
}

////////////////////////////////////////////////////////////////////////////////

void bindSocket(GSocket **Socket, gchar *Address, gint Port)
{
	GInetAddress *LocalAddress = g_inet_address_new_from_string(Address);
	GSocketAddress *SocketAddress = g_inet_socket_address_new
																	(LocalAddress, Port);
	GError *SocketBindError = NULL;

	g_socket_bind(*Socket, SocketAddress, TRUE, &SocketBindError);
	// TRUE : Allow other UDP sockets to be bound to the same address

	g_object_unref(LocalAddress);
	g_object_unref(SocketAddress);

	processGError("Error binding SAP socket", SocketBindError);
}

////////////////////////////////////////////////////////////////////////////////

void joinMulticastGroup(GSocket **Socket, gchar *MulticastAddressString)
{
	GInetAddress *MulticastAddress = g_inet_address_new_from_string
																		(MulticastAddressString);
	GError *MulticastJoinError = NULL;

	g_socket_join_multicast_group(*Socket, MulticastAddress,
																	FALSE, NULL, &MulticastJoinError);
	// FALSE : No need for source-specific multicast
	// NULL : Listen on all Ethernet interfaces

	g_object_unref(MulticastAddress);

	processGError("Error joining SAP Multicast group", MulticastJoinError);
}

void callback_SAPLoopIdle(GMainLoop *Loop)
{
	puts("-----------------------");
	puts("Waiting for a packet...");
	puts("-----------------------");

	g_usleep(2 * G_USEC_PER_SEC);

	g_main_loop_quit(Loop);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


gint main(gint argc, gchar *argv[])
{

	// Set up an UDP/IPv4 socket for SAP discovery

	GSocket *SAPSocket = NULL;

	openSocket(&SAPSocket, G_SOCKET_FAMILY_IPV4,
							G_SOCKET_TYPE_DATAGRAM,	G_SOCKET_PROTOCOL_DEFAULT);

	// Bind the SAP socket to listen to all local interfaces

	bindSocket(&SAPSocket, SAP_LOCAL_ADDRESS, SAP_MULTICAST_PORT);

	// Subscribe to the SAP Multicast group

	joinMulticastGroup(&SAPSocket, SAP_MULTICAST_ADDRESS);

	// Begin the SAP packet receiving loop

	GMainLoop *SAPPacketReceivingLoop = g_main_loop_new(NULL, FALSE);
	// NULL : Use default GMainContext
	// FALSE : Don't run the loop right away

	g_idle_add((GSourceFunc) callback_SAPLoopIdle, SAPPacketReceivingLoop);

	g_main_loop_run(SAPPacketReceivingLoop);

	g_main_loop_unref(SAPPacketReceivingLoop);


	puts("Bye");



	// gchar SAPPacketString[SAP_PACKET_BUFFER_SIZE];
	// gssize SAPPacketStringBytesRead = 0;
	// GSocketAddress *SAPPacketSourceSocket = NULL;
	// GInetAddress *SAPPacketSourceAddress = NULL;
	// GError *SAPPacketError = NULL;
	//
	// while(TRUE)
	// {
	// 	puts("Begin loop");
	// 	// Reinitialize the packet string buffer
	// 	memset(SAPPacketString, '\0', sizeof(SAPPacketString));
	//
	// 	SAPPacketStringBytesRead =
	// 		g_socket_receive_from
	// 		(
	// 			SAPSocket,
	// 			&SAPPacketSourceSocket,
	// 			SAPPacketString,
	// 			sizeof(SAPPacketString),
	// 			NULL,
	// 			&SAPPacketError
	// 		);
	//
	// 	if(SAPPacketStringBytesRead < 0)
	// 		processGError("Error receiving SAP packet", SAPPacketError);
	// 	else if(SAPPacketStringBytesRead == 0) // The connection has been closed
	// 		break;
	//
	// 	SAPPacketSourceAddress = g_inet_socket_address_get_address
	// 													 ((GInetSocketAddress *) SAPPacketSourceSocket);
	//
	// 	printf
	// 	(
	// 		"=== %s: %ld byte SAP packet with %ld byte SDP description :\n",
	// 		g_inet_address_to_string(SAPPacketSourceAddress),
	// 		SAPPacketStringBytesRead,
	// 		SAPPacketStringBytesRead-SAP_PACKET_HEADER_SIZE
	// 	);
	// 	printf("=== TYPE : %s\n\n", &SAPPacketString[SAP_PACKET_PREHEADER_SIZE]);
	// 	printf("%s\n\n\n\n", &SAPPacketString[SAP_PACKET_HEADER_SIZE]);
	//
	// }
}
