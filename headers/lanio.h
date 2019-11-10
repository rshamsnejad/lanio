#ifndef __LANIO_H__

#define __LANIO_H__

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Standard C headers ===

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

// === GLib headers ===

#define G_LOG_USE_STRUCTURED                TRUE
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

// === SQLite headers ===

#include <sqlite3.h>

// === libfort headers ===

#include "../lib/fort.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Constants ===

#define PROG_NAME                           "LANIO"
#define PROG_LONG_NAME                      "LANIO Audio Network I/O"
#define PROG_VERSION                        "<under development>"

#define SAP_MULTICAST_ADDRESS               "239.255.255.255"
#define SAP_MULTICAST_PORT                  9875

// SAP Socket local address is set to 0.0.0.0/<port>
// in order to listen to all local interfaces
#define SAP_LOCAL_ADDRESS                   "0.0.0.0"

#define NETWORK_INTERFACES_PATH             "/sys/class/net"

#define SDP_MAX_LENGTH                      2048
#define MIME_TYPE_MAX_LENGTH                256
#define SAP_IDENTIFIER_HASH_LENGTH          16
#define SAP_PACKET_BUFFER_SIZE              SDP_MAX_LENGTH + \
                                            MIME_TYPE_MAX_LENGTH + 256

#define SAP_SOURCE_IS_IPV4                  0
#define SAP_SOURCE_IS_IPV6                  1

#define SAP_ANNOUNCEMENT_PACKET             0
#define SAP_DELETION_PACKET                 1

#define WORKING_HOME_DIRECTORY_NAME         ".lanio"
#define WORKING_TEMP_DIRECTORY_NAME         "lanio"
#define WORKING_DIRECTORY_MASK              0744

#define SDP_DATABASE_FILENAME               "lanio-SDP.db"
#define SDP_DATABASE_PRINT_MODE_NICE        0
#define SDP_DATABASE_PRINT_MODE_CSV         1

#define SAP_TABLE_NAME                      "SAPDiscovery"
#define SAP_TABLE_DISPLAY_NAME              "Dante streams"
#define SAP_TABLE_CLI_PARAMETER             "dante"
#define MDNS_TABLE_NAME                     "MDNSDiscovery"
#define MDNS_TABLE_DISPLAY_NAME             "Ravenna streams"
#define MDNS_TABLE_CLI_PARAMETER            "ravenna"
#define SDPFILES_TABLE_NAME                 "SDPFiles"
#define SDPFILES_TABLE_DISPLAY_NAME         "User SDP files"
#define SDPFILES_TABLE_CLI_PARAMETER        "files"

#define MINUTE                              "60"
#define SQLITE_UNIX_CURRENT_TS              "(CAST(strftime('%s','now') as INT))"
#define SQLITE_UNIX_CURRENT_TS_ESCAPED      "(CAST(strftime('%%s','now') as INT))"

#define MSEC_IN_SEC                         1000

#define REGEX_SDP \
    "^(?:v|o|s|i|u|e|p|c|b|z|k|a|t|r|m)=([[:alnum:][:blank:][:punct:]]+)$"

#define REGEX_SDP_VALUE_ORIGIN \

#define REGEX_SDP_VALUE_CONNECTION \
    "^IN IP4 (?P<address>(?:[[:digit:]]{1,3}\\.){3}(?:[[:digit:]]{1,3}){1})\\/(?P<ttl>[[:digit:]]{1,2})$"

#define REGEX_SDP_VALUE_MEDIA \
    "^audio (?P<port>[[:digit:]]{1,5}) RTP\\/AVP (?P<payload>[[:digit:]]{1,3})$"

#define REGEX_SDP_VALUE_ATTRIBUTE \
    "^(?P<key>[[:alnum:]-]+)(?::(?P<value>[[:alnum:][:digit:][:punct:] ]+))?$"

#define REGEX_SDP_ATTRIBUTE_MEDIACLK \
    "^direct=(?P<offset>[[:digit:]]{1,20})$"

#define REGEX_SDP_ATTRIBUTE_RTPMAP \
    "^(?P<payload>[[:digit:]]{1,3}) L(?P<bitdepth>24|16)\\/(?P<samplerate>44100|48000|88200|96000|176400|192000)\\/(?P<channels>[[:digit:]]{1,3})$"

#define REGEX_SDP_ATTRIBUTE_TSREFCLK \
    "^ptp=IEEE1588-2008:(?P<gmid>(?:(?:[[:xdigit:]]{2}-){7})(?:[[:xdigit:]]{2})):(?P<domain>[[:digit:]]{1,3})$"

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

#define PRINT_CLI_HELP(ctx) \
    g_printerr\
    (\
        "%s\n",\
        g_option_context_get_help(ctx, TRUE, NULL)\
    )

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Structs ===

typedef struct _SAPPacket
{
    guint8      SAPVersion;
    gboolean    AddressType;
    gboolean    MessageType;
    gboolean    Encryption;
    gboolean    Compression;
    guint8      AuthenticationLength;
    guint16     MessageIdentifierHash;
    gchar      *OriginatingSourceAddress;
    gchar      *PayloadType;
    gchar      *SDPDescription;
}
    SAPPacket;

typedef struct _data_deleteOldSDPEntries
{
    GMainLoop  *DiscoveryLoop;
    sqlite3    *Database;
}
    data_deleteOldSDPEntries;

typedef struct _data_insertIncomingSAPPackets
{
    GMainLoop  *DiscoveryLoop;
    sqlite3    *Database;
}
    data_insertIncomingSAPPackets;

typedef struct _DiscoveryCLIParameters
{
    gboolean    DiscoverTerminal;
    gboolean    Debug;
    gchar      *Interface;
}
    DiscoveryCLIParameters;

typedef struct _SDPParameters
{
    gchar      *SourceType;
    gchar      *SourceName;
    gchar      *SourceInfo;
    gchar      *StreamAddress;
    guint16     UDPPort;
    guint8      PayloadType;
    guint8      BitDepth;
    guint32     SampleRate;
    guint8      ChannelCount;
    guint16     PacketTime;
    guint8      PTPGrandMasterClockDomain;
    gchar      *PTPGrandMasterClockID;
    guint64     OffsetFromMasterClock;
}
    SDPParameters;


typedef struct _ListDiscoveredCLIParameters
{
    gboolean    CSV;
    gchar      *StreamCategory;
}
    ListDiscoveredCLIParameters;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// === Function prototypes ===

void openSocket
(
    GSocket **Socket,
    GSocketFamily SocketFamily,
    GSocketType SocketType,
    GSocketProtocol SocketProtocol
);

void bindSocket(GSocket *Socket, gchar *Address, gint Port);

void joinMulticastGroup
(
    GSocket *Socket,
    gchar *MulticastAddressString,
    gchar *InterfaceName
);

gssize receivePacket
(
    GSocket *Socket,
    gchar *StringBuffer,
    gssize StringBufferSize
);

void processGError(gchar *ErrorMessage, GError *ErrorStruct);

void processSQLiteOpenError(int SQLiteErrorCode);

void createSAPTable(sqlite3 *SDPDatabase);

void processSQLiteExecError
(
    gint SQLiteExecErrorCode,
    gchar *SQLiteExecErrorString,
    gchar *SQLQuery
);

SAPPacket* convertSAPStringToStruct(gchar *SAPString);

guint32 concatenateBytes(guint8 *IntArray, gsize Start, gsize End);

void freeSAPPacket(SAPPacket *SAPStructToFree);

void printSAPPacket(SAPPacket *PacketToPrint);

void insertSAPPacketInSAPTable(sqlite3 *SDPDatabase, SAPPacket* PacketToInsert);

void removeSAPPacketFromSAPTable
(
    sqlite3 *SDPDatabase,
    SAPPacket* PacketToRemove
);

void updateSAPTable(sqlite3 *SDPDatabase, SAPPacket *PacketToProcess);

gboolean callback_deleteOldSDPEntries(gpointer Data);

gboolean callback_insertIncomingSAPPackets
(
    GSocket *Socket,
    GIOCondition condition,
    gpointer Data
);

gboolean checkSAPPacket(SAPPacket *PacketToCheck);

void setUpSAPPacketLoop(GMainLoop *Loop, GSocket *Socket, sqlite3 *Database);

void discoverSAPAnnouncements(sqlite3 *SDPDatabase, gchar *InterfaceName);

void parseDiscoveryCLIOptions
(
    DiscoveryCLIParameters *Parameters,
    gint argc,
    gchar *argv[]
);

guint getStringArraySize(gchar **StringArray);

gchar* getSDPDatabasePath(void);

void initDiscoveryCLIParameters(DiscoveryCLIParameters *ParametersToInit);

void daemonizeDiscovery(void);

void initListDiscoveredCLIParameters
    (ListDiscoveredCLIParameters *ParametersToInit);

void parseListDiscoveredCLIOptions
(
    ListDiscoveredCLIParameters *Parameters,
    gint argc,
    gchar *argv[]
);

void checkCLIParameters(gboolean Expression, GOptionContext *Context);

void parseCLIContext(GOptionContext *Context, gint argc, gchar *argv[]);

gboolean checkValidSDPString(gchar *sdp);

void callback_printHashTable(gpointer Key, gpointer Value, gpointer Data);

gboolean checkRegex
(
    gchar *Pattern,
    gchar *String,
    GRegexCompileFlags CompileFlags,
    GRegexMatchFlags MatchFlags,
    GMatchInfo **MatchInfo
);

SDPParameters* convertSDPStringToStruct(gchar *SDPStringToProcess);

void callback_insertAttributeTableinSDPStruct
(
    gpointer Key,
    gpointer Value,
    gpointer SDPStruct
);

void printSDPStruct(SDPParameters *StructToPrint);

void freeSDPStruct(SDPParameters *StructToFree);

void printSAPEntries(sqlite3 *SDPDatabase, gboolean PrintMode);

gint callback_returnSQLCount
(
    gpointer ReturnCount,
    gint ColumnCount,
    gchar **DataRow,
    gchar **ColumnRow
);

gint callback_printSDPInCSV
(
    gpointer Useless,
    gint ColumnCount,
    gchar **DataRow,
    gchar **ColumnRow
);

void printMDNSEntries(gboolean PrintMode);

void printSDPFilesEntries(gboolean PrintMode);

gint callback_insertSDPEntriesInFormattedTable
(
    gpointer ASCIITable,
    gint ColumnCount,
    gchar **DataRow,
    gchar **ColumnRow
);

GLogWriterOutput lanioLogWriter
(
    GLogLevelFlags LogLevel,
    const GLogField *Fields,
    gsize NumberOfFields,
    gpointer Data
);

gboolean checkNetworkInterfaceName(gchar *InterfaceName);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // __LANIO_H__
