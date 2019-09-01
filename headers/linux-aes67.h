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

#define SAP_MULTICAST_ADDRESS						"239.255.255.255"
#define SAP_MULTICAST_PORT							9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS								"0.0.0.0"

#define SDP_MAX_LENGTH									2048
#define MIME_TYPE_MAX_LENGTH						256
#define SAP_PACKET_BUFFER_SIZE					SDP_MAX_LENGTH+MIME_TYPE_MAX_LENGTH+256


#define SAP_PACKET_PREHEADER_SIZE				8
#define SAP_PACKET_HEADER_SIZE					24

#define SAP_SOURCE_IS_IPV4							0
#define SAP_SOURCE_IS_IPV6							1

#define SAP_ANNOUNCEMENT_PACKET					0
#define SAP_DELETION_PACKET							1

#define IPV4_ADDRESS_LENGTH							16

#define SDP_DATABASE_FILENAME						"./SDP.db"
#define SDP_TABLE_NAME									"SDPDescriptions"

#define MINUTE													"60"
#define SQLITE_UNIX_CURRENT_TS					"(CAST(strftime('%s','now') as INT))"
#define SQLITE_UNIX_CURRENT_TS_ESCAPED	"(CAST(strftime('%%s','now') as INT))"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Structs ===

typedef struct _SAPPacket
{
	guint8 SAPVersion = 0;
	gboolean AddressType = 0;
	gboolean MessageType = 0;
	gboolean Encryption = 0;
	gboolean Compression = 0;
	guint8 AuthenticationLength = 0;
	gint16 MessageIdentifierHash = 0;
	gint32 OriginatingSourceAddress = 0;
	gchar PayloadType[MIME_TYPE_MAX_LENGTH] = {'\0'};
	gchar SDPDescription[SDP_MAX_LENGTH] = {'\0'};
} SAPPacket;

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

void createSDPTable(sqlite3 **SDPDatabase);

void processSQLiteExecError(gint SQLiteExecErrorCode,
															gchar *SQLiteExecErrorString);

void insertStringInSQLiteTable(sqlite3 **SDPDatabase, char *TableName,
																gchar *ColumnName, gchar *DataString);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // LINUXAES67
