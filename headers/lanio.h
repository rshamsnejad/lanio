#ifndef __LANIO_H__

#define __LANIO_H__

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

#define SAP_MULTICAST_ADDRESS               "239.255.255.255"
#define SAP_MULTICAST_PORT                  9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS                   "0.0.0.0"

#define SDP_MAX_LENGTH                      2048
#define MIME_TYPE_MAX_LENGTH                256
#define SAP_IDENTIFIER_HASH_LENGTH          16
#define SAP_PACKET_BUFFER_SIZE              SDP_MAX_LENGTH + \
                                            MIME_TYPE_MAX_LENGTH + 256


#define SAP_PACKET_PREHEADER_SIZE           8
#define SAP_PACKET_HEADER_SIZE              24

#define SAP_SOURCE_IS_IPV4                  0
#define SAP_SOURCE_IS_IPV6                  1

#define SAP_ANNOUNCEMENT_PACKET             0
#define SAP_DELETION_PACKET                 1

#define IPV4_ADDRESS_LENGTH                 16

#define SDP_DATABASE_FILENAME               "../SDP.db"
#define SAP_TABLE_NAME                      "SAPDiscovery"

#define MINUTE                              "60"
#define SQLITE_UNIX_CURRENT_TS              "(CAST(strftime('%s','now') as INT))"
#define SQLITE_UNIX_CURRENT_TS_ESCAPED      "(CAST(strftime('%%s','now') as INT))"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Macros ===

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define BYTE_TO_BINARY_STRING_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY_STRING(byte)  \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

#define GET_BIT(number, n) ((number >> n) & 1 ? 1 : 0)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Structs ===

typedef struct _SAPPacket
{
    guint8 SAPVersion;
    gboolean AddressType;
    gboolean MessageType;
    gboolean Encryption;
    gboolean Compression;
    guint8 AuthenticationLength;
    guint16 MessageIdentifierHash;
    gchar *OriginatingSourceAddress;
    gchar *PayloadType; //[MIME_TYPE_MAX_LENGTH];
    gchar *SDPDescription; //[SDP_MAX_LENGTH];
} SAPPacket;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Function prototypes ===

void openSocket(GSocket **Socket, GSocketFamily SocketFamily,
                    GSocketType SocketType,    GSocketProtocol SocketProtocol);

void bindSocket(GSocket *Socket, gchar *Address, gint Port);

void joinMulticastGroup(GSocket *Socket, gchar *MulticastAddressString);

gssize receivePacket(GSocket *Socket, gchar *StringBuffer,
                            gssize StringBufferSize);

void printPacketUgly(gchar *PacketString, gssize PacketStringBytesRead,
                    gchar *PacketSourceAddress);

void processGError(gchar *ErrorMessage, GError *ErrorStruct);

void processSQLiteOpenError(int SQLiteErrorCode);

gchar* getAddressStringFromSocket(GSocketAddress *SocketAddress);

void createSAPTable(sqlite3 *SDPDatabase);

void processSQLiteExecError(gint SQLiteExecErrorCode,
                                gchar *SQLiteExecErrorString,
                                    gchar *SQLQuery);

void insertStringInSQLiteTable(sqlite3 *SDPDatabase, char *TableName,
                                    gchar *ColumnName, gchar *DataString);

SAPPacket* convertSAPStringToStruct(gchar *SAPString);

guint32 concatenateBytes(guint8 *IntArray, gsize Start, gsize End);

void freeSAPPacket(SAPPacket *SAPStructToFree);

void printSAPPacket(SAPPacket *PacketToPrint);

void insertSAPPacketInSAPTable(sqlite3 *SDPDatabase, SAPPacket* PacketToInsert);

void removeSAPPacketFromSAPTable(sqlite3 *SDPDatabase,
                                    SAPPacket* PacketToRemove);

void updateSAPTable(sqlite3 *SDPDatabase, SAPPacket *PacketToProcess);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // __LANIO_H__
