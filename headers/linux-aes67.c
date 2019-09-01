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

void processSQLiteOpenError(gint SQLiteOpenErrorCode)
{
	if(SQLiteOpenErrorCode != SQLITE_OK)
	{
		const gchar *SQLiteOpenErrorString = sqlite3_errstr(SQLiteOpenErrorCode);
		g_printerr("SQLite open error : %s\n", SQLiteOpenErrorString);

		g_free((gpointer) SQLiteOpenErrorString);
		exit(EXIT_FAILURE);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void createSAPTable(sqlite3 **SDPDatabase)
{
	gint SQLiteExecErrorCode = 0;
	gchar *SQLiteExecErrorString = NULL;

	gchar *SQLQuery =
		"CREATE TABLE IF NOT EXISTS \
		" SAP_TABLE_NAME " \
		( \
			id INTEGER PRIMARY KEY AUTOINCREMENT, \
			timestamp INTEGER \
				DEFAULT " SQLITE_UNIX_CURRENT_TS ", \
			sdp VARCHAR UNIQUE \
		) ; "
		"CREATE TRIGGER IF NOT EXISTS AFTER UPDATE ON " SAP_TABLE_NAME " \
		WHEN OLD.timestamp < " SQLITE_UNIX_CURRENT_TS " - (1 * " MINUTE ") \
		BEGIN \
		DELETE FROM " SAP_TABLE_NAME " \
		WHERE id = OLD.id ;\
		END";

	SQLiteExecErrorCode =
		sqlite3_exec
		(
			*SDPDatabase,
			SQLQuery,
			NULL, NULL, // No callback function needed
			&SQLiteExecErrorString
		);

	processSQLiteExecError(SQLiteExecErrorCode, SQLiteExecErrorString);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void processSQLiteExecError(gint SQLiteExecErrorCode,
															gchar *SQLiteExecErrorString)
{
	if(SQLiteExecErrorCode != 0)
	{
		g_print("SQLite exec error : %s\n", SQLiteExecErrorString);

		sqlite3_free(SQLiteExecErrorString);
		exit(EXIT_FAILURE);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void insertStringInSQLiteTable(sqlite3 **SDPDatabase, char *TableName,
																gchar *ColumnName, gchar *DataString)
{
	gint SQLiteExecErrorCode = 0;
	gchar *SQLiteExecErrorString = NULL;

	gchar *SQLQuery =
	g_strdup_printf
	(
		"INSERT INTO %s (%s) \
		VALUES ('%s') \
		ON CONFLICT (%s) DO UPDATE SET timestamp = " SQLITE_UNIX_CURRENT_TS_ESCAPED,
		TableName, ColumnName, DataString, ColumnName
	);

	SQLiteExecErrorCode =
		sqlite3_exec
		(
			*SDPDatabase,
			SQLQuery,
			NULL, NULL, // No callback function needed
			&SQLiteExecErrorString
		);

	g_free(SQLQuery);

	processSQLiteExecError(SQLiteExecErrorCode, SQLiteExecErrorString);

	g_print("Inserted or updated\n\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
