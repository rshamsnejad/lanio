/*

SAP_discovery.c

Discover SAP announcement of Dante streams.

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "../headers/linux-aes67.h"

#define SAP_MULTICAST_ADDRESS							"239.255.255.255"
#define SAP_MULTICAST_PORT								9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS									"0.0.0.0"

#define SAP_PACKET_BUFFER_SIZE						1024

#define SAP_PACKET_PREHEADER_SIZE					8
#define SAP_PACKET_HEADER_SIZE						24

#define IPV4_ADDRESS_LENGTH								16

#define SAP_DATABASE_FILENAME							"./SDP.db"

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

	g_clear_object(&LocalAddress);
	g_clear_object(&SocketAddress);

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

	g_clear_object(&MulticastAddress);

	processGError("Error joining SAP Multicast group", MulticastJoinError);
}

////////////////////////////////////////////////////////////////////////////////

gssize receivePacket(GSocket **Socket, gchar *SourceAddress,
											gssize SourceAddressSize, gchar *StringBuffer,
												gssize StringBufferSize)
{
	// Reinitialize the packet string buffer
	memset(StringBuffer, '\0', StringBufferSize);

	GSocketAddress *PacketSourceSocket = NULL;
	GError *PacketError = NULL;

	gint PacketStringBytesRead =
		g_socket_receive_from
		(
			*Socket,
			&PacketSourceSocket,
			StringBuffer,
			StringBufferSize,
			NULL,
			&PacketError
		);

	if(PacketStringBytesRead < 0)
		processGError("Error receiving SAP packet", PacketError);
	else if(PacketStringBytesRead == 0) // The connection has been closed
		return 0;

	// > StringBuffer points to the packet data at this point

	gchar* AddressString = getAddressStringFromSocket(PacketSourceSocket);

	g_strlcpy
	(
		SourceAddress,
		AddressString,
		SourceAddressSize
	);

	// > SourceAddress points to the received packet's
	// source address at this point

	g_clear_object(&PacketSourceSocket);
	g_free(AddressString);

	return PacketStringBytesRead;
}

////////////////////////////////////////////////////////////////////////////////

void printPacket(gchar *PacketString, gssize PacketStringBytesRead,
									gchar *PacketSourceAddress)
{
	g_print
	(
		"=== %s: %ld byte SAP packet with %ld byte SDP description :\n",
		PacketSourceAddress,
		PacketStringBytesRead,
		PacketStringBytesRead-SAP_PACKET_HEADER_SIZE
	);
	g_print("=== TYPE : %s\n\n", &PacketString[SAP_PACKET_PREHEADER_SIZE]);
	g_print("%s\n\n\n\n", &PacketString[SAP_PACKET_HEADER_SIZE]);
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


	sqlite3 *SDPDatabase = NULL;

	processSQLiteError(sqlite3_open(SAP_DATABASE_FILENAME, &SDPDatabase));

	// Begin the SAP packet receiving loop

	gchar SAPPacketString[SAP_PACKET_BUFFER_SIZE] = {'\0'};
	gssize SAPPacketStringBytesRead = 0;
	gchar SAPPacketSourceAddress[IPV4_ADDRESS_LENGTH] = {'\0'};


	while(TRUE)
	{
		// puts("Begin loop");

		SAPPacketStringBytesRead =
			receivePacket
			(
				&SAPSocket,
				SAPPacketSourceAddress,
				ARRAY_SIZE(SAPPacketSourceAddress),
				SAPPacketString,
				ARRAY_SIZE(SAPPacketString)
			);

		if(SAPPacketStringBytesRead <= 0) // The connection has been reset
		{
			g_print("Terminated\n");
			break;
		}

		printPacket(SAPPacketString,
								SAPPacketStringBytesRead,
								SAPPacketSourceAddress);

	} // End of while()

	g_clear_object(&SAPSocket);
	sqlite3_close(SDPDatabase);
	return EXIT_SUCCESS;
}
