#include "linux-aes67.h"

////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////

void processGError(gchar *ErrorMessage, GError *ErrorStruct)
{
	if(ErrorStruct)
	{
		g_printerr("%s : %s\n", ErrorMessage, ErrorStruct->message);
		g_error_free(ErrorStruct);
		exit(EXIT_FAILURE);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gchar* getAddressStringFromSocket(GSocketAddress *SocketAddress)
{
	return g_inet_address_to_string
				 (
					 g_inet_socket_address_get_address
					 ((GInetSocketAddress *) SocketAddress)
				 );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void processSQLiteError(int SQLiteErrorCode)
{
	if(SQLiteErrorCode != SQLITE_OK)
	{
		g_printerr("SQLite error : %s\n", sqlite3_errstr(SQLiteErrorCode));
		exit(EXIT_FAILURE);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
