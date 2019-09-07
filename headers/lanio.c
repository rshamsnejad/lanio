#include "lanio.h"

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

void bindSocket(GSocket *Socket, gchar *Address, gint Port)
{
    GInetAddress *LocalAddress = g_inet_address_new_from_string(Address);
    GSocketAddress *SocketAddress =
        g_inet_socket_address_new(LocalAddress, Port);
    GError *SocketBindError = NULL;

    g_socket_bind(Socket, SocketAddress, TRUE, &SocketBindError);
        // TRUE : Allow other UDP sockets to be bound to the same address

    g_clear_object(&LocalAddress);
    g_clear_object(&SocketAddress);

    processGError("Error binding SAP socket", SocketBindError);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void joinMulticastGroup(GSocket *Socket, gchar *MulticastAddressString)
{
    GInetAddress *MulticastAddress =
        g_inet_address_new_from_string(MulticastAddressString);
    GError *MulticastJoinError = NULL;

    g_socket_join_multicast_group(Socket, MulticastAddress,
                                    FALSE, NULL, &MulticastJoinError);
        // FALSE : No need for source-specific multicast
        // NULL : Listen on all Ethernet interfaces

    g_clear_object(&MulticastAddress);

    processGError("Error joining SAP Multicast group", MulticastJoinError);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gssize receivePacket(GSocket *Socket, gchar *StringBuffer,
                            gssize StringBufferSize)
{
    // Reinitialize the packet string buffer
    memset(StringBuffer, '\0', StringBufferSize);

    GError *PacketError = NULL;

    gint PacketStringBytesRead =
        g_socket_receive_from
        (
            Socket,
            NULL,
            StringBuffer,
            StringBufferSize,
            NULL,
            &PacketError
        );

    if(PacketStringBytesRead < 0)
        processGError("Error receiving packet", PacketError);
    else if(PacketStringBytesRead == 0) // The connection has been closed
        return 0;

    // > StringBuffer points to the packet data at this point

    return PacketStringBytesRead;
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

void createSAPTable(sqlite3 *SDPDatabase)
{
    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;

    gchar *SQLQuery =
        "CREATE TABLE IF NOT EXISTS \
        " SAP_TABLE_NAME " \
        ( \
        id INTEGER PRIMARY KEY AUTOINCREMENT, \
        timestamp INTEGER DEFAULT " SQLITE_UNIX_CURRENT_TS ", \
        hash INTEGER UNIQUE, \
        source VARCHAR, \
        sdp VARCHAR \
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
            SDPDatabase,
            SQLQuery,
            NULL, NULL, // No callback function needed
            &SQLiteExecErrorString
        );

    processSQLiteExecError
    (
        SQLiteExecErrorCode,
        SQLiteExecErrorString,
        SQLQuery
    );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void processSQLiteExecError(gint SQLiteExecErrorCode,
                                gchar *SQLiteExecErrorString,
                                    gchar *SQLQuery)
{
    if(SQLiteExecErrorCode != 0)
    {
        g_printerr("SQLite exec error : %s\n", SQLiteExecErrorString);
        g_printerr("SQLite query : %s\n", SQLQuery);

        exit(EXIT_FAILURE);
    }

    sqlite3_free(SQLiteExecErrorString);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SAPPacket* convertSAPStringToStruct(gchar *SAPString)
{
    SAPPacket *ReturnPacket = g_malloc0(sizeof(SAPPacket));

    // *** Refer to RFC 2974 for details on the header parts extracted below ***

    // 8 first bits are the flags
    guint8 PacketFlags = SAPString[0];

    // First two bits are useless...                   7
    // Because reasons.                                6
    ReturnPacket->SAPVersion    = GET_BIT(PacketFlags, 5);
    ReturnPacket->AddressType   = GET_BIT(PacketFlags, 4);
    // Fifth bit is reserved, therefore useless to us. 3
    ReturnPacket->MessageType   = GET_BIT(PacketFlags, 2);
    ReturnPacket->Encryption    = GET_BIT(PacketFlags, 1);
    ReturnPacket->Compression   = GET_BIT(PacketFlags, 0);

    // 8 following bits give an unsigned integer
    // for the authentication header length
    ReturnPacket->AuthenticationLength = (guint8) SAPString[1];

    // 16 following bits are a unique hash attached to the stream
    ReturnPacket->MessageIdentifierHash =
        (guint16) concatenateBytes((guint8 *) SAPString, 2, 3);

    // If AddressType is IPv4, the following 32 bits give the IPv4 address.
    // Otherwise if it's IPv6, the following 128 bits give the IPv6 address.
    // But since only IPv4 is supported for now, we will not consider IPv6.
    // An error will be yielded when checking this struct if it comes with IPv6.
    gsize AddressEndingByte = 0;
    GSocketFamily AddressFamily;

    if(ReturnPacket->AddressType == SAP_SOURCE_IS_IPV4)
    {
        AddressEndingByte = 7;
        AddressFamily = G_SOCKET_FAMILY_IPV4;
    }
    else if(ReturnPacket->AddressType == SAP_SOURCE_IS_IPV6)
    {
        AddressEndingByte = 19;
        AddressFamily = G_SOCKET_FAMILY_IPV6;
    }

    GInetAddress *SourceAddress =
        g_inet_address_new_from_bytes
        (
            (guint8*) &SAPString[4],
            AddressFamily
        );

    ReturnPacket->OriginatingSourceAddress = g_inet_address_to_string
                                                (SourceAddress);

    g_clear_object(&SourceAddress);

    // Authentication header is skipped, because it does not look like it's
    // used in AES67 SAP announcements.

    // Payload type is a MIME type, "application/sdp" in our case
    gsize PayloadTypeStartIndex =
        AddressEndingByte + ReturnPacket->AuthenticationLength + 1;

    ReturnPacket->PayloadType = g_strdup(&SAPString[PayloadTypeStartIndex]);

    // Now we can gather the actual SDP description
    ReturnPacket->SDPDescription =
        g_strdup
        (
            &SAPString
            [PayloadTypeStartIndex + strlen(ReturnPacket->PayloadType) + 1]
        );

    return ReturnPacket;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

guint32 concatenateBytes(guint8 *IntArray, gsize Start, gsize End)
{
    guint32 ReturnValue = 0;

    for
    (
        gsize index = End, shift = 0; // Start by shifting the LSBs by 0 bits
        index >= Start;
        index--, shift += 8
    )
    {
        // Shift the current byte, then append
        ReturnValue |= (IntArray[index] << shift);
    }

    return ReturnValue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void freeSAPPacket(SAPPacket *SAPStructToFree)
{
    g_free(SAPStructToFree->OriginatingSourceAddress);
    g_free(SAPStructToFree->PayloadType);
    g_free(SAPStructToFree->SDPDescription);

    g_free(SAPStructToFree);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void printSAPPacket(SAPPacket *PacketToPrint)
{
    g_print("=== BEGIN RECEIVED SAP PACKET ===\n");

    g_print("SAP Version : \t\t\t%d\n", PacketToPrint->SAPVersion);
    g_print
    (
        "Address Type : \t\t\t%s\n",
        PacketToPrint->AddressType ==
            SAP_SOURCE_IS_IPV4 ? "IPv4" : "IPv6"
    );
    g_print
    (
        "Message Type : \t\t\t%s\n",
        PacketToPrint->MessageType ==
            SAP_ANNOUNCEMENT_PACKET ? "Announcement" : "Deletion"
    );
    g_print("Encryption : \t\t\t%s\n",
                PacketToPrint->Encryption ? "Encrypted" : "Not encrypted");
    g_print("Compression : \t\t\t%s\n",
                PacketToPrint->Compression ? "Compressed" : "Not compressed");
    g_print("Authentication header length : \t%d\n",
                PacketToPrint->AuthenticationLength);
    g_print("Identifier Hash : \t\t0x%04X\n", PacketToPrint->MessageIdentifierHash);
    g_print("Sender IP : \t\t\t%s\n", PacketToPrint->OriginatingSourceAddress);
    g_print("Payload type : \t\t\t%s\n", PacketToPrint->PayloadType);
    g_print("SDP description :\n%s\n", PacketToPrint->SDPDescription);

    g_print("\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void insertSAPPacketInSAPTable(sqlite3 *SDPDatabase, SAPPacket* PacketToInsert)
{
    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;

    gchar *SQLQuery =
        g_strdup_printf
        (
            "INSERT INTO " SAP_TABLE_NAME " (timestamp, hash, source, sdp) \
            VALUES \
            ( \
                " SQLITE_UNIX_CURRENT_TS_ESCAPED ", \
                '%d', \
                '%s', \
                '%s' \
            ) \
            ON CONFLICT (hash) \
                DO UPDATE SET timestamp = " SQLITE_UNIX_CURRENT_TS_ESCAPED,
            PacketToInsert->MessageIdentifierHash,
            PacketToInsert->OriginatingSourceAddress,
            PacketToInsert->SDPDescription
        );

    SQLiteExecErrorCode =
        sqlite3_exec
        (
            SDPDatabase,
            SQLQuery,
            NULL, NULL, // No callback function needed
            &SQLiteExecErrorString
        );

    processSQLiteExecError
    (
        SQLiteExecErrorCode,
        SQLiteExecErrorString,
        SQLQuery
    );

    if(sqlite3_changes(SDPDatabase) > 0)
        g_print
        (
            "Inserted or updated SAP entry.\n\tID = 0x%04X\n",
            PacketToInsert->MessageIdentifierHash
        );

    g_free(SQLQuery);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void removeSAPPacketFromSAPTable(sqlite3 *SDPDatabase,
                                    SAPPacket* PacketToRemove)
{
    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;

    gchar *SQLQuery =
        g_strdup_printf
        (
            "DELETE FROM " SAP_TABLE_NAME " \
            WHERE hash = %d",
            PacketToRemove->MessageIdentifierHash
        );

    SQLiteExecErrorCode =
        sqlite3_exec
        (
            SDPDatabase,
            SQLQuery,
            NULL, NULL, // No callback function needed
            &SQLiteExecErrorString
        );

    processSQLiteExecError
    (
        SQLiteExecErrorCode,
        SQLiteExecErrorString,
        SQLQuery
    );

    if(sqlite3_changes(SDPDatabase) > 0)
        g_print
        (
            "Removed SAP entry.\n\tID = 0x%04X\n",
            PacketToRemove->MessageIdentifierHash
        );

    g_free(SQLQuery);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void updateSAPTable(sqlite3 *SDPDatabase, SAPPacket *PacketToProcess)
{
    if(checkSAPPacket(PacketToProcess))
    {
        if(PacketToProcess->MessageType == SAP_ANNOUNCEMENT_PACKET)
            insertSAPPacketInSAPTable(SDPDatabase, PacketToProcess);
        else if(PacketToProcess->MessageType == SAP_DELETION_PACKET)
            removeSAPPacketFromSAPTable(SDPDatabase, PacketToProcess);
    }
    else
        g_print
        (
            "Invalid SAP Packet. Ignoring...\n\tID = 0x%04X\n",
            PacketToProcess->MessageIdentifierHash
        );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean callback_deleteOldSDPEntries(gpointer Data)
{
    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;

    gchar *SQLQuery =
        "DELETE FROM " SAP_TABLE_NAME " \
        WHERE timestamp < " SQLITE_UNIX_CURRENT_TS " - (1 * " MINUTE ")";

    SQLiteExecErrorCode =
        sqlite3_exec
        (
            ((data_deleteOldSDPEntries*) Data)->Database,
            SQLQuery,
            NULL, NULL, // No callback function needed
            &SQLiteExecErrorString
        );

    processSQLiteExecError
    (
        SQLiteExecErrorCode,
        SQLiteExecErrorString,
        SQLQuery
    );

    if(sqlite3_changes(((data_deleteOldSDPEntries*) Data)->Database) > 0)
        g_print("Removed outdated SAP entries.\n");

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean callback_insertIncomingSAPPackets(GSocket *Socket,
                                             GIOCondition condition,
                                                gpointer Data)
{
    gchar SAPPacketBuffer[SAP_PACKET_BUFFER_SIZE] = {'\0'};
    gssize SAPPacketBufferBytesRead = 0;

    SAPPacket *ReceivedSAPPAcket = NULL;

    SAPPacketBufferBytesRead =
        receivePacket
        (
            Socket,
            SAPPacketBuffer,
            ARRAY_SIZE(SAPPacketBuffer)
        );

    if(SAPPacketBufferBytesRead <= 0) // The connection has been reset
    {
        g_print("Connection reset. Terminating...\n");
        return FALSE;
    }

    ReceivedSAPPAcket = convertSAPStringToStruct(SAPPacketBuffer);

    //printSAPPacket(ReceivedSAPPAcket);

    updateSAPTable
    (
        ((data_insertIncomingSAPPackets*) Data)->Database,
        ReceivedSAPPAcket
    );

    freeSAPPacket(ReceivedSAPPAcket);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean checkSAPPacket(SAPPacket *PacketToCheck)
{
    if
    (
        PacketToCheck->SAPVersion != 1 ||
        PacketToCheck->AddressType != SAP_SOURCE_IS_IPV4 ||
        PacketToCheck->Encryption != 0 ||
        PacketToCheck->Compression != 0 ||
        g_strcmp0(PacketToCheck->PayloadType, "application/sdp")
    )
        return FALSE;
    else
        return TRUE;
}
