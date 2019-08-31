#ifndef LINUXAES67

#define LINUXAES67

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Standard C headers ===

#include <stdlib.h>
#include <string.h>

// === GLib headers ===

#include <glib.h>
#include <gio/gio.h>

// === SQLite headers ===

#include <sqlite3.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Constants ===

#define SAP_MULTICAST_ADDRESS							"239.255.255.255"
#define SAP_MULTICAST_PORT								9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS									"0.0.0.0"

#define SAP_PACKET_BUFFER_SIZE						1024

#define SAP_PACKET_PREHEADER_SIZE					8
#define SAP_PACKET_HEADER_SIZE						24

#define IPV4_ADDRESS_LENGTH								16

#define SDP_DATABASE_FILENAME							"./SDP.db"
#define SDP_TABLE_NAME										"SDPDescriptions"

#define MINUTE														"60"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Macros ===

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Function prototypes ===

void openSocket(GSocket **Socket, GSocketFamily SocketFamily,
								GSocketType SocketType,	GSocketProtocol SocketProtocol);

void bindSocket(GSocket **Socket, gchar *Address, gint Port);

void joinMulticastGroup(GSocket **Socket, gchar *MulticastAddressString);

gssize receivePacket(GSocket **Socket, gchar *SourceAddress,
											gssize SourceAddressSize, gchar *StringBuffer,
												gssize StringBufferSize);

void printPacket(gchar *PacketString, gssize PacketStringBytesRead,
									gchar *PacketSourceAddress);

void processGError(gchar *ErrorMessage, GError *ErrorStruct);

void processSQLiteOpenError(int SQLiteErrorCode);

gchar* getAddressStringFromSocket(GSocketAddress *SocketAddress);

void createSQLiteTable(sqlite3 **SDPDatabase, gchar *TableName);

void processSQLiteExecError(gint SQLiteExecErrorCode,
															gchar *SQLiteExecErrorString);

void insertStringInSQLiteTable(sqlite3 **SDPDatabase, char *TableName,
																gchar *ColumnName, gchar *DataString);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // LINUXAES67
