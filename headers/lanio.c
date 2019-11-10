#include "lanio.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void openSocket
(
    GSocket **Socket,
    GSocketFamily SocketFamily,
    GSocketType SocketType,
    GSocketProtocol SocketProtocol
)
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

void joinMulticastGroup
(
    GSocket *Socket,
    gchar *MulticastAddressString,
    gchar *InterfaceName
)
{
    GInetAddress *MulticastAddress =
        g_inet_address_new_from_string(MulticastAddressString);
    GError *MulticastJoinError = NULL;

    g_socket_join_multicast_group(Socket, MulticastAddress,
                                    FALSE, InterfaceName, &MulticastJoinError);
        // FALSE : No need for source-specific multicast
        // NULL : Listen on all Ethernet interfaces

    g_clear_object(&MulticastAddress);

    processGError("Error joining SAP Multicast group", MulticastJoinError);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gssize receivePacket
(
    GSocket *Socket,
    gchar *StringBuffer,
    gssize StringBufferSize
)
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
        g_warning("SQLite open error : %s\n", SQLiteOpenErrorString);

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
        "CREATE TABLE IF NOT EXISTS " SAP_TABLE_NAME " "
        "("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "timestamp INTEGER DEFAULT " SQLITE_UNIX_CURRENT_TS ", "
            "sap_hash INTEGER UNIQUE, "
            "sap_sourceaddress VARCHAR, "
            "sdp VARCHAR, "
            "sdp_sourcetype VARCHAR, "
            "sdp_sourcename VARCHAR, "
            "sdp_sourceinfo VARCHAR, "
            "sdp_streamaddress VARCHAR, "
            "sdp_udpport INTEGER, "
            "sdp_payloadtype INTEGER, "
            "sdp_bitdepth INTEGER, "
            "sdp_samplerate INTEGER, "
            "sdp_channelcount INTEGER, "
            "sdp_packettime INTEGER, "
            "sdp_ptpdomain INTEGER, "
            "sdp_ptpgmid VARCHAR, "
            "sdp_ptpoffset INTEGER "
        ") ; "
        "CREATE TRIGGER IF NOT EXISTS AFTER UPDATE ON " SAP_TABLE_NAME " "
        "WHEN OLD.timestamp < " SQLITE_UNIX_CURRENT_TS " - (1 * " MINUTE ") "
        "BEGIN DELETE FROM " SAP_TABLE_NAME " "
        "WHERE id = OLD.id ;"
        "END";

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

void processSQLiteExecError
(
    gint SQLiteExecErrorCode,
    gchar *SQLiteExecErrorString,
    gchar *SQLQuery
)
{
    if(SQLiteExecErrorCode != 0)
    {
        g_warning
        (
            "SQLite exec error : %s\nSQLite query : %s\n",
            SQLiteExecErrorString,
            SQLQuery
        );

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
    g_debug("=== BEGIN RECEIVED SAP PACKET ===");

    g_debug("SAP Version : \t\t\t%d", PacketToPrint->SAPVersion);
    g_debug
    (
        "Address Type : \t\t\t%s",
        PacketToPrint->AddressType ==
            SAP_SOURCE_IS_IPV4 ? "IPv4" : "IPv6"
    );
    g_debug
    (
        "Message Type : \t\t\t%s",
        PacketToPrint->MessageType ==
            SAP_ANNOUNCEMENT_PACKET ? "Announcement" : "Deletion"
    );
    g_debug("Encryption : \t\t\t\t%s",
                PacketToPrint->Encryption ? "Encrypted" : "Not encrypted");
    g_debug("Compression : \t\t\t%s",
                PacketToPrint->Compression ? "Compressed" : "Not compressed");
    g_debug("Authentication header length : \t%d",
                PacketToPrint->AuthenticationLength);
    g_debug("Identifier Hash : \t\t\t0x%04X", PacketToPrint->MessageIdentifierHash);
    g_debug("Sender IP : \t\t\t\t%s", PacketToPrint->OriginatingSourceAddress);
    g_debug("Payload type : \t\t\t%s", PacketToPrint->PayloadType);
    g_debug("SDP description :\n%s", PacketToPrint->SDPDescription);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void insertSAPPacketInSAPTable(sqlite3 *SDPDatabase, SAPPacket *PacketToInsert)
{
    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;

    // printSAPPacket(PacketToInsert);

    SDPParameters *SDPParametersToInsert = NULL;
    SDPParametersToInsert =
        convertSDPStringToStruct(PacketToInsert->SDPDescription);

    if(!SDPParametersToInsert)
    {
        g_info
        (
            "Invalid SDP description. Ignoring SAP ID = 0x%04X",
            PacketToInsert->MessageIdentifierHash
        );
        goto error;
    }

    // printSDPStruct(SDPParametersToInsert);

    gchar *SQLQuery =
        g_strdup_printf
        (
            "INSERT INTO " SAP_TABLE_NAME " "
            // "(timestamp, sap_hash, sap_sourceaddress, sdp) "
            "VALUES "
            "( "
                "NULL, "
                SQLITE_UNIX_CURRENT_TS_ESCAPED ", "
                "'%" G_GUINT16_FORMAT "', " // Hash
                "'%s', " // Source address
                "'%s', " // SDP
                "'%s', " // Source type
                "'%s', " // Source name
                "'%s', " // Source info
                "'%s', " // Stream address
                "'%" G_GUINT16_FORMAT "', " // UDP port
                "'%u', " // Payload type
                "'%u', " // Bit depth
                "'%" G_GUINT32_FORMAT "', " // Sample rate
                "'%u', " // Channel count
                "'%" G_GUINT16_FORMAT "', " // Packet time
                "'%u', " // PTP domain
                "'%s', " // PTP GMID
                "'%" G_GUINT64_FORMAT "' " // Offset from GM
            ") "
            "ON CONFLICT (sap_hash) "
                "DO UPDATE SET timestamp = " SQLITE_UNIX_CURRENT_TS_ESCAPED,
            PacketToInsert->MessageIdentifierHash,
            PacketToInsert->OriginatingSourceAddress,
            PacketToInsert->SDPDescription,
            SDPParametersToInsert->SourceType,
            SDPParametersToInsert->SourceName,
            SDPParametersToInsert->SourceInfo,
            SDPParametersToInsert->StreamAddress,
            SDPParametersToInsert->UDPPort,
            SDPParametersToInsert->PayloadType,
            SDPParametersToInsert->BitDepth,
            SDPParametersToInsert->SampleRate,
            SDPParametersToInsert->ChannelCount,
            SDPParametersToInsert->PacketTime,
            SDPParametersToInsert->PTPGrandMasterClockDomain,
            SDPParametersToInsert->PTPGrandMasterClockID,
            SDPParametersToInsert->OffsetFromMasterClock
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
        g_debug
        (
            "Inserted or updated SAP entry.\n\tID = 0x%04X",
            PacketToInsert->MessageIdentifierHash
        );

    freeSDPStruct(SDPParametersToInsert);
    g_free(SQLQuery);

    return;

    error:
        g_free(SDPParametersToInsert);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void removeSAPPacketFromSAPTable
(
    sqlite3 *SDPDatabase,
    SAPPacket* PacketToRemove
)
{
    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;

    gchar *SQLQuery =
        g_strdup_printf
        (
            "DELETE FROM " SAP_TABLE_NAME " "
            "WHERE sap_hash = %" G_GUINT16_FORMAT,
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
        g_debug
        (
            "Removed SAP entry.\n\tID = 0x%04X",
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
        g_debug
        (
            "Invalid SAP Packet. Ignoring...\n\tID = 0x%04X",
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
        "DELETE FROM " SAP_TABLE_NAME " "
        "WHERE timestamp < " SQLITE_UNIX_CURRENT_TS " - (1 * " MINUTE ")";

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
        g_info("Removed outdated SAP entries.");

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean callback_insertIncomingSAPPackets
(
    GSocket *Socket,
    GIOCondition condition,
    gpointer Data
)
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
        g_message("Connection reset. Terminating...");
        return FALSE;
    }

    ReceivedSAPPAcket = convertSAPStringToStruct(SAPPacketBuffer);

    // printSAPPacket(ReceivedSAPPAcket);

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void setUpSAPPacketLoop(GMainLoop *Loop, GSocket *Socket, sqlite3 *Database)
{
    // Every 3 seconds, delete outdated database entries
    static data_deleteOldSDPEntries TimeoutDeleteData;

    TimeoutDeleteData.DiscoveryLoop = Loop;
    TimeoutDeleteData.Database = Database;

    g_timeout_add
    (
        3 * MSEC_IN_SEC,
        (GSourceFunc) callback_deleteOldSDPEntries,
        &TimeoutDeleteData
    );

    // Insert or delete database entries based on incoming SAP packets
    GSource *SAPSocketMonitoring =
        g_socket_create_source(Socket, G_IO_IN | G_IO_PRI, NULL);
            // NULL : No GCancellable needed
    g_source_attach(SAPSocketMonitoring, NULL);
        // NULL : Use default context

    static data_insertIncomingSAPPackets SAPSocketMonitoringData;

    SAPSocketMonitoringData.DiscoveryLoop = Loop;
    SAPSocketMonitoringData.Database = Database;

    g_source_set_callback
    (
        SAPSocketMonitoring,
        (GSourceFunc) callback_insertIncomingSAPPackets,
        &SAPSocketMonitoringData,
        NULL
    );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void discoverSAPAnnouncements(sqlite3 *SDPDatabase, gchar *InterfaceName)
{
    // Set up an UDP/IPv4 socket for SAP discovery
    GSocket *SAPSocket = NULL;

    openSocket(&SAPSocket, G_SOCKET_FAMILY_IPV4,
                G_SOCKET_TYPE_DATAGRAM,	G_SOCKET_PROTOCOL_DEFAULT);

    // Bind the SAP socket to listen to all local interfaces
    bindSocket(SAPSocket, SAP_LOCAL_ADDRESS, SAP_MULTICAST_PORT);

    // Subscribe to the SAP Multicast group
    joinMulticastGroup(SAPSocket, SAP_MULTICAST_ADDRESS, InterfaceName);

    // Create the SAP table if it does not exist in the database
    createSAPTable(SDPDatabase);

    // Set up the SAP packet receiving loop
    GMainLoop *SAPDiscoveryLoop = g_main_loop_new(NULL, FALSE);
        // NULL : Use default context
        // FALSE : Don't run the loop right away
    setUpSAPPacketLoop(SAPDiscoveryLoop, SAPSocket, SDPDatabase);

    // Start the loop
    g_main_loop_run(SAPDiscoveryLoop);

    // Cleanup section
    g_main_loop_unref(SAPDiscoveryLoop);
    g_clear_object(&SAPSocket);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void parseDiscoveryCLIOptions
(
    DiscoveryCLIParameters *Parameters,
    gint argc,
    gchar *argv[]
)
{
    GOptionEntry CommandLineOptionEntries[] =
    {
        { "interface", 'i', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
            &Parameters->Interface,
            "Network interface to listen to (mandatory)",
            NULL },
        { "terminal", 't', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
            &Parameters->DiscoverTerminal,
            "Start stream discovery in the terminal instead of as a daemon",
            NULL },
        { "debug", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
            &Parameters->Debug,
            "Enable debugging output (verbose)",
            NULL },
        { NULL }
    };

    GOptionContext *CommandLineOptionContext =
        g_option_context_new("- " PROG_LONG_NAME);

    g_option_context_set_summary
    (
        CommandLineOptionContext,
        "Discover AES67 streams available on the network"
    );
    g_option_context_set_description
    (
        CommandLineOptionContext,
        "Version " PROG_VERSION "\nReport bugs to dev@lanio.com"
    );
    g_option_context_add_main_entries
    (
        CommandLineOptionContext,
        CommandLineOptionEntries,
        NULL
    );

    parseCLIContext(CommandLineOptionContext, argc, argv);

    gboolean CheckParameters =
        Parameters->Interface != NULL &&
        checkNetworkInterfaceName(Parameters->Interface);

    checkCLIParameters(CheckParameters, CommandLineOptionContext);

    g_option_context_free(CommandLineOptionContext);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

guint getStringArraySize(gchar **StringArray)
{
    if(StringArray)
        return g_strv_length(StringArray);
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gchar* getSDPDatabasePath(void)
{
    gchar *ReturnPath = NULL;

    gchar *WorkingDirectory =
        g_strconcat
        (
            g_get_home_dir(),
            "/",
            WORKING_HOME_DIRECTORY_NAME,
            NULL
        );

    gint MkdirReturn =
        g_mkdir_with_parents(WorkingDirectory, WORKING_DIRECTORY_MASK);

    if(MkdirReturn == 0)
        ReturnPath =
            g_strconcat(WorkingDirectory, "/", SDP_DATABASE_FILENAME, NULL);
    else
    {
        g_debug
        (
            "Unable to use directory %s. Falling back to temporary directory.",
            WorkingDirectory
        );

        WorkingDirectory =
            g_strconcat
            (
                g_get_tmp_dir(),
                "/",
                WORKING_TEMP_DIRECTORY_NAME,
                NULL
            );

        MkdirReturn =
            g_mkdir_with_parents(WorkingDirectory, WORKING_DIRECTORY_MASK);

        if(MkdirReturn == 0)
            ReturnPath =
                g_strconcat
                (
                    WorkingDirectory,
                    "/",
                    SDP_DATABASE_FILENAME,
                    NULL
                );
    }

    if(!ReturnPath)
    {
        perror("Unable to access working directory : ");
        exit(EXIT_FAILURE);
    }

    g_debug("SDP database path : %s", ReturnPath);

    g_free(WorkingDirectory);

    return ReturnPath;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void initDiscoveryCLIParameters(DiscoveryCLIParameters *ParametersToInit)
{
    ParametersToInit->DiscoverTerminal = FALSE;
    ParametersToInit->Debug = FALSE;
    ParametersToInit->Interface = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void daemonizeDiscovery(void)
{
    pid_t ChildPID, ChildSID;

    ChildPID = fork();
    if(ChildPID < 0)
    {
        g_error("Error while forking.");
        exit(EXIT_FAILURE);
    }

    if (ChildPID > 0)
    {
        g_debug("Successfully forked. PID = %d", ChildPID);
        g_print(PROG_NAME " SAP discovery daemon started.\n");
        exit(EXIT_SUCCESS);
    }

    umask(0);

    // Create a new SID for the child process
    ChildSID = setsid();
    if (ChildSID < 0)
    {
        g_error("Error while creating SID.");
        exit(EXIT_FAILURE);
    }
    // Change the current working directory to a safe place
    if ((chdir("/")) < 0)
    {
        g_error("Error while changing directory.");
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void initListDiscoveredCLIParameters
    (ListDiscoveredCLIParameters *ParametersToInit)
{
    ParametersToInit->CSV = FALSE;
    ParametersToInit->StreamCategory = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void parseListDiscoveredCLIOptions
(
    ListDiscoveredCLIParameters *Parameters,
    gint argc,
    gchar *argv[]
)
{
    GOptionEntry CommandLineOptionEntries[] =
    {
        { "ugly", 'u', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
            &Parameters->CSV,
            "Display the available streams in ugly CSV values "
                "instead of nice ASCII tables",
            NULL },
        { "categories", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
            &Parameters->StreamCategory,
            "Display only the selected category of streams.\n"
            "Possible options : (dante|ravenna|files)",
            NULL },
        { NULL }
    };

    GOptionContext *CommandLineOptionContext =
        g_option_context_new("- " PROG_LONG_NAME);

    g_option_context_set_summary
    (
        CommandLineOptionContext,
        "List AES67 streams discovered by lanio-discovery"
    );
    g_option_context_set_description
    (
        CommandLineOptionContext,
        "Version " PROG_VERSION "\nReport bugs to dev@lanio.com"
    );
    g_option_context_add_main_entries
    (
        CommandLineOptionContext,
        CommandLineOptionEntries,
        NULL
    );

    parseCLIContext(CommandLineOptionContext, argc, argv);

    gboolean CheckParameters = TRUE;
    checkCLIParameters(CheckParameters, CommandLineOptionContext);

    g_option_context_free(CommandLineOptionContext);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void checkCLIParameters(gboolean Expression, GOptionContext *Context)
{
    if(!Expression)
    {
        PRINT_CLI_HELP(Context);
        exit(EXIT_FAILURE);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void parseCLIContext(GOptionContext *Context, gint argc, gchar *argv[])
{
    GError *Error = NULL;

    gboolean ParseReturn =
        g_option_context_parse
        (
            Context,
            &argc,
            &argv,
            &Error
        );

    if(!ParseReturn)
    {
        g_warning
        (
            "Error in command line : %s\n",
            Error->message
        );

        PRINT_CLI_HELP(Context);

        g_option_context_free(Context);
        exit(EXIT_FAILURE);
    }

    g_clear_error(&Error);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean checkValidSDPString(gchar *sdp)
{
    return
        g_regex_match_simple
            (
                REGEX_SDP,
                sdp,
                G_REGEX_CASELESS | G_REGEX_MULTILINE,
                G_REGEX_MATCH_NOTEMPTY
            );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void callback_printHashTable(gpointer Key, gpointer Value, gpointer Data)
{
    g_print("Key : %s\t Value : %s\n", (gchar*) Key, (gchar*) Value);

    if(g_strcmp0((gchar*) Key, "mediaclk") == 0)
    {
        GMatchInfo *RegexMediaclkMatchInfo = NULL;

        gboolean RegexCheck =
            checkRegex
            (
                REGEX_SDP_ATTRIBUTE_MEDIACLK,
                (gchar*) Value,
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexMediaclkMatchInfo
            );

        g_print("--%s--\n", RegexCheck ? "VALID" : "INVALID");

        gchar *OffsetString =
            g_match_info_fetch_named(RegexMediaclkMatchInfo, "offset");
        g_print("\tOffset = %s\n", OffsetString);

        g_free(OffsetString);
    }

    if(g_strcmp0((gchar*) Key, "recvonly") == 0)
    {
        g_print("--VALID--\n\tSender does not expect return\n");
    }

    if(g_strcmp0((gchar*) Key, "rtpmap") == 0)
    {
        GMatchInfo *RegexRtpmapMatchInfo = NULL;

        gboolean RegexCheck =
            checkRegex
            (
                REGEX_SDP_ATTRIBUTE_RTPMAP,
                (gchar*) Value,
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexRtpmapMatchInfo
            );

        g_print("--%s--\n", RegexCheck ? "VALID" : "INVALID");

        gchar *PayloadString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "payload");
        gchar *BitrateString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "bitrate");
        gchar *SamplerateString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "samplerate");
        gchar *ChannelsString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "channels");

        g_print("\tPayload = %s\n", PayloadString);
        g_print("\tBitrate = %s\n", BitrateString);
        g_print("\tSample Rate = %s\n", SamplerateString);
        g_print("\tChannels = %s\n", ChannelsString);

        g_free(PayloadString);
        g_free(BitrateString);
        g_free(SamplerateString);
        g_free(ChannelsString);
    }

    if(g_strcmp0((gchar*) Key, "ts-refclk") == 0)
    {
        GMatchInfo *RegexTsrefclkMatchInfo = NULL;

        gboolean RegexCheck =
            checkRegex
            (
                REGEX_SDP_ATTRIBUTE_TSREFCLK,
                (gchar*) Value,
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexTsrefclkMatchInfo
            );

        g_print("--%s--\n", RegexCheck ? "VALID" : "INVALID");

        gchar *GMIDString =
            g_match_info_fetch_named(RegexTsrefclkMatchInfo, "gmid");
        gchar *DomainString =
            g_match_info_fetch_named(RegexTsrefclkMatchInfo, "domain");

        g_print("\tGrandMaster ID = %s\n", GMIDString);
        g_print("\tPTP Domain = %s\n", DomainString);

        g_free(GMIDString);
        g_free(DomainString);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean checkRegex
(
    gchar *Pattern,
    gchar *String,
    GRegexCompileFlags CompileFlags,
    GRegexMatchFlags MatchFlags,
    GMatchInfo **MatchInfo
)
{
    GError *RegexError = NULL;
    GRegex *Regex =
        g_regex_new
        (
            Pattern,
            CompileFlags,
            MatchFlags,
            &RegexError
        );
    processGError("Error creating regex", RegexError);

    gboolean ReturnValue = g_regex_match(Regex, String, MatchFlags, MatchInfo);

    g_regex_unref(Regex);

    return ReturnValue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SDPParameters* convertSDPStringToStruct(gchar *SDPStringToProcess)
{
    if(!checkValidSDPString(SDPStringToProcess))
        return NULL;

    SDPParameters *ReturnStruct = g_malloc0(sizeof(SDPParameters));

    // Convert each line as a new string in an array
    gchar **StringArray = g_strsplit(SDPStringToProcess, "\r\n", 0);

    gchar **ParameterArray = NULL;
    gboolean RegexCheck = FALSE;

    // Hash table for storing the "a="" attributes
    GHashTable *SDPAttributes =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    for // Iterate on each splitted non-empty string
    (
        gsize i = 0;
        StringArray[i] != NULL && StringArray[i][0] != '\0';
        i++
    )
    {
        // Separate SDP keys and values
        ParameterArray = g_strsplit(StringArray[i], "=", 2);

        // SDP Connection value "c="
        if(g_strcmp0(ParameterArray[0],"c") == 0)
        {
            GMatchInfo *RegexConnectionMatchInfo = NULL;

            RegexCheck =
                checkRegex
                (
                    REGEX_SDP_VALUE_CONNECTION,
                    ParameterArray[1],
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexConnectionMatchInfo
                );

            if(!RegexCheck)
            {
                g_info("Invalid SDP : Wrong connection section.");
                g_debug("\t%s", StringArray[i]);

                g_match_info_free(RegexConnectionMatchInfo);
                goto checkerror;
            }

            gchar *AddressString =
                g_match_info_fetch_named(RegexConnectionMatchInfo, "address");
            gchar *TTLString =
                g_match_info_fetch_named(RegexConnectionMatchInfo, "ttl");

            ReturnStruct->StreamAddress = g_strdup(AddressString);

            g_free(AddressString);
            g_free(TTLString);
            g_match_info_free(RegexConnectionMatchInfo);
        }

        // SDP Timing value "t="
        if(g_strcmp0(ParameterArray[0],"t") == 0)
        {
            RegexCheck = g_strcmp0(ParameterArray[1],"0 0") == 0;

            if(!RegexCheck)
            {
                g_info("Invalid SDP : Session is not permanent.");
                g_debug("\t%s", StringArray[i]);

                goto checkerror;
            }
        }

        // SDP Media section "m="
        if(g_strcmp0(ParameterArray[0],"m") == 0)
        {
            GMatchInfo *RegexMediaMatchInfo = NULL;

            RegexCheck =
                checkRegex
                (
                    REGEX_SDP_VALUE_MEDIA,
                    ParameterArray[1],
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexMediaMatchInfo
                );

            if(!RegexCheck)
            {
                g_info("Invalid SDP : Wrong media section.");
                g_debug("\t%s", StringArray[i]);

                g_match_info_free(RegexMediaMatchInfo);
                goto checkerror;
            }

            gchar *PortString =
                g_match_info_fetch_named(RegexMediaMatchInfo, "port");
            gchar *PayloadString =
                g_match_info_fetch_named(RegexMediaMatchInfo, "payload");

            ReturnStruct->UDPPort = atoi(PortString);

            g_free(PortString);
            g_free(PayloadString);
            g_match_info_free(RegexMediaMatchInfo);
        }

        // SDP Attributes "a=""
        if(g_strcmp0(ParameterArray[0],"a") == 0)
        {
            // Split attribute line "a=key:value"
            GMatchInfo *RegexAttributeMatchInfo = NULL;
            checkRegex
            (
                REGEX_SDP_VALUE_ATTRIBUTE,
                ParameterArray[1],
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexAttributeMatchInfo
            );

            if(!RegexCheck)
            {
                g_info("Invalid SDP : Wrong attribute section.");
                g_debug("\t%s", StringArray[i]);

                g_match_info_free(RegexAttributeMatchInfo);
                goto checkerror;
            }

            gchar *KeyString =
                g_match_info_fetch_named(RegexAttributeMatchInfo, "key");
            gchar *ValueString =
                g_match_info_fetch_named(RegexAttributeMatchInfo, "value");

            g_hash_table_insert(SDPAttributes, KeyString, ValueString);

            // Don't g_free KeyString and ValueString because of the hash table
            g_match_info_free(RegexAttributeMatchInfo);
        }

        if(g_strcmp0(ParameterArray[0],"s") == 0)
            ReturnStruct->SourceName = g_strdup(ParameterArray[1]);

        if(g_strcmp0(ParameterArray[0],"i") == 0)
            ReturnStruct->SourceInfo = g_strdup(ParameterArray[1]);

        if(g_strcmp0(ParameterArray[0],"v") == 0)
        {
            if(!(g_strcmp0(ParameterArray[1],"0") == 0))
            {
                g_info("Invalid SDP : Wrong SDP version.");
                g_debug("\t%s", StringArray[i]);

                goto checkerror;
            }
        }

        g_strfreev(ParameterArray);
    }

    if
    (
        !g_hash_table_contains(SDPAttributes, "recvonly") ||
        g_hash_table_contains(SDPAttributes, "sendrecv") ||
        g_hash_table_contains(SDPAttributes, "sendonly"))
    {
        g_info("Invalid SDP : Wrong send/receive attribute.");

        goto check_sendrecv_error;
    }

    g_hash_table_foreach
    (
        SDPAttributes,
        callback_insertAttributeTableinSDPStruct,
        ReturnStruct
    );

    g_hash_table_destroy(SDPAttributes);
    g_strfreev(StringArray);

    return ReturnStruct;

    checkerror:
        g_strfreev(ParameterArray);
    check_sendrecv_error:
        g_hash_table_destroy(SDPAttributes);
        g_strfreev(StringArray);
        g_free(ReturnStruct);

        return NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void callback_insertAttributeTableinSDPStruct
(
    gpointer Key,
    gpointer Value,
    gpointer SDPStruct
)
{
    if(g_strcmp0((gchar*) Key, "mediaclk") == 0)
    {
        GMatchInfo *RegexMediaclkMatchInfo = NULL;

        gboolean RegexCheck =
            checkRegex
            (
                REGEX_SDP_ATTRIBUTE_MEDIACLK,
                (gchar*) Value,
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexMediaclkMatchInfo
            );

        if(!RegexCheck)
        {
            g_info("Invalid SDP : Wrong mediaclk attribute.");
            g_debug("\t%s=%s", (gchar*) Key, (gchar*) Value);

            g_match_info_free(RegexMediaclkMatchInfo);
            return;
        }

        gchar *OffsetString =
            g_match_info_fetch_named(RegexMediaclkMatchInfo, "offset");

        ((SDPParameters*) SDPStruct)->OffsetFromMasterClock =
            g_ascii_strtoull(OffsetString, NULL, 10);

        g_free(OffsetString);
        g_match_info_free(RegexMediaclkMatchInfo);
    }

    if(g_strcmp0((gchar*) Key, "rtpmap") == 0)
    {
        GMatchInfo *RegexRtpmapMatchInfo = NULL;

        gboolean RegexCheck =
            checkRegex
            (
                REGEX_SDP_ATTRIBUTE_RTPMAP,
                (gchar*) Value,
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexRtpmapMatchInfo
            );

        if(!RegexCheck)
        {
            g_info("Invalid SDP : Wrong rtpmap attribute.");
            g_debug("\t%s=%s", (gchar*) Key, (gchar*) Value);

            g_match_info_free(RegexRtpmapMatchInfo);
            return;
        }

        gchar *PayloadString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "payload");
        gchar *BitdepthString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "bitdepth");
        gchar *SamplerateString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "samplerate");
        gchar *ChannelsString =
            g_match_info_fetch_named(RegexRtpmapMatchInfo, "channels");

        ((SDPParameters*) SDPStruct)->PayloadType = atoi(PayloadString);
        ((SDPParameters*) SDPStruct)->BitDepth = atoi(BitdepthString);
        ((SDPParameters*) SDPStruct)->SampleRate =
            g_ascii_strtoull(SamplerateString, NULL, 10);
        ((SDPParameters*) SDPStruct)->ChannelCount = atoi(ChannelsString);

        g_free(PayloadString);
        g_free(BitdepthString);
        g_free(SamplerateString);
        g_free(ChannelsString);
        g_match_info_free(RegexRtpmapMatchInfo);
    }

    if(g_strcmp0((gchar*) Key, "ts-refclk") == 0)
    {
        GMatchInfo *RegexTsrefclkMatchInfo = NULL;

        gboolean RegexCheck =
            checkRegex
            (
                REGEX_SDP_ATTRIBUTE_TSREFCLK,
                (gchar*) Value,
                G_REGEX_CASELESS,
                G_REGEX_MATCH_NOTEMPTY,
                &RegexTsrefclkMatchInfo
            );

        if(!RegexCheck)
        {
            g_info("Invalid SDP : Wrong ts-refclk attribute.");
            g_debug("\t%s=%s", (gchar*) Key, (gchar*) Value);

            g_match_info_free(RegexTsrefclkMatchInfo);
            return;
        }

        gchar *GMIDString =
            g_match_info_fetch_named(RegexTsrefclkMatchInfo, "gmid");
        gchar *DomainString =
            g_match_info_fetch_named(RegexTsrefclkMatchInfo, "domain");

        ((SDPParameters*) SDPStruct)->PTPGrandMasterClockID =
            g_strdup(GMIDString);
        ((SDPParameters*) SDPStruct)->PTPGrandMasterClockDomain =
            atoi(DomainString);

        g_free(GMIDString);
        g_free(DomainString);
        g_match_info_free(RegexTsrefclkMatchInfo);
    }

    if(g_strcmp0((gchar*) Key, "ptime") == 0)
    {
        ((SDPParameters*) SDPStruct)->PacketTime = atoi((gchar*) Value);
    }

    if(g_strcmp0((gchar*) Key, "keywds") == 0)
    {
        ((SDPParameters*) SDPStruct)->SourceType = g_strdup((gchar*) Value);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void printSDPStruct(SDPParameters *StructToPrint)
{
    g_debug("=== BEGIN SDP STRUCT ===");
    g_debug("Source Type :\t\t\t%s", StructToPrint->SourceType);
    g_debug("Source Name :\t\t\t%s", StructToPrint->SourceName);
    g_debug("Source Info :\t\t\t%s", StructToPrint->SourceInfo);;
    g_debug("Stream Address :\t\t%s", StructToPrint->StreamAddress);
    g_debug("UDP Port :\t\t\t%" G_GUINT16_FORMAT, StructToPrint->UDPPort);
    g_debug("Payload Type :\t\t%u", StructToPrint->PayloadType);
    g_debug("Bit Depth :\t\t\t%u", StructToPrint->BitDepth);
    g_debug("Sample Rate :\t\t\t%" G_GUINT32_FORMAT, StructToPrint->SampleRate);
    g_debug("Channel Count :\t\t%u", StructToPrint->ChannelCount);
    g_debug("Packet Time :\t\t\t%" G_GUINT16_FORMAT, StructToPrint->PacketTime);
    g_debug("PTP GM Clock Domain :\t\t%u",
        StructToPrint->PTPGrandMasterClockDomain);
    g_debug("PTP GM Clock ID :\t\t%s", StructToPrint->PTPGrandMasterClockID);
    g_debug("Offset from GM Clock :\t%" G_GUINT64_FORMAT,
        StructToPrint->OffsetFromMasterClock);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void freeSDPStruct(SDPParameters *StructToFree)
{
    g_free(StructToFree->SourceType);
    g_free(StructToFree->SourceName);
    g_free(StructToFree->SourceInfo);
    g_free(StructToFree->StreamAddress);
    g_free(StructToFree->PTPGrandMasterClockID);

    g_free(StructToFree);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void printSAPEntries(sqlite3 *SDPDatabase, gboolean PrintMode)
{
    gchar *SQLCountQuery =
        g_strdup_printf
        (
            "SELECT COUNT(id) FROM " SAP_TABLE_NAME " ;"
        );

    gint SQLiteExecErrorCode = 0;
    gchar *SQLiteExecErrorString = NULL;
    gint SQLCount = 0;

    SQLiteExecErrorCode =
        sqlite3_exec
        (
            SDPDatabase,
            SQLCountQuery,
            callback_returnSQLCount,
            &SQLCount,
            &SQLiteExecErrorString
        );

    processSQLiteExecError
    (
        SQLiteExecErrorCode,
        SQLiteExecErrorString,
        SQLCountQuery
    );

    g_free(SQLCountQuery);

    if(SQLCount <= 0 && PrintMode == SDP_DATABASE_PRINT_MODE_NICE)
    {
        g_printerr(SAP_TABLE_DISPLAY_NAME " : No current streams.\n");
        return;
    }

    gchar *SQLSelectQuery =
        g_strdup_printf
        (
            "SELECT "
                "sap_hash, sdp_sourcetype, sdp_sourcename, "
                "sdp_streamaddress, sdp_channelcount, sdp_bitdepth, "
                "sdp_samplerate, sdp_ptpgmid, sdp_ptpdomain "
            "FROM " SAP_TABLE_NAME " "
            "ORDER BY sdp_sourcename ASC ;"
        );

    if(PrintMode == SDP_DATABASE_PRINT_MODE_CSV)
    {
        g_print
        (
            "Hash,Stream type,Source name,Stream address,Channels,Bit depth,"
            "Sample rate,PTP GMID,PTP domain\n"
        );

        SQLiteExecErrorCode =
            sqlite3_exec
            (
                SDPDatabase,
                SQLSelectQuery,
                callback_printSDPInCSV,
                NULL,
                &SQLiteExecErrorString
            );
    }
    else if(PrintMode == SDP_DATABASE_PRINT_MODE_NICE)
    {
        ft_table_t *ASCIITable = ft_create_table();

        ft_set_border_style(ASCIITable, FT_DOUBLE2_STYLE);
        ft_set_cell_prop
        (
            ASCIITable,
            0,
            FT_ANY_COLUMN,
            FT_CPROP_ROW_TYPE,
            FT_ROW_HEADER
        );

        ft_write_ln
        (
            ASCIITable,
            "Hash",
            "Stream type",
            "Source name",
            "Stream address",
            "Channels",
            "Bit depth",
            "Sample rate",
            "PTP GMID",
            "PTP domain"
        );

        SQLiteExecErrorCode =
            sqlite3_exec
            (
                SDPDatabase,
                SQLSelectQuery,
                callback_insertSDPEntriesInFormattedTable,
                ASCIITable,
                &SQLiteExecErrorString
            );

        printf("\n%s\n", ft_to_string(ASCIITable));

        ft_destroy_table(ASCIITable);
    }

    processSQLiteExecError
    (
        SQLiteExecErrorCode,
        SQLiteExecErrorString,
        SQLSelectQuery
    );

    g_free(SQLSelectQuery);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gint callback_returnSQLCount
(
    gpointer ReturnCount,
    gint ColumnCount,
    gchar **DataRow,
    gchar **ColumnRow
)
{
    gint *Temp = (gint*) ReturnCount;
    *Temp = atoi(DataRow[0]);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gint callback_printSDPInCSV
(
    gpointer Useless,
    gint ColumnCount,
    gchar **DataRow,
    gchar **ColumnRow
)
{
    gsize i;
    for(i = 0 ; i < ColumnCount - 1 ; i++)
    {
        g_print("%s,", DataRow[i]);
    }
    g_print("%s\n", DataRow[i]);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void printMDNSEntries(gboolean PrintMode)
{
    if(PrintMode == SDP_DATABASE_PRINT_MODE_NICE)
        g_printerr(MDNS_TABLE_DISPLAY_NAME " : No current streams.\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void printSDPFilesEntries(gboolean PrintMode)
{
    if(PrintMode == SDP_DATABASE_PRINT_MODE_NICE)
        g_printerr(SDPFILES_TABLE_DISPLAY_NAME " : No current streams.\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gint callback_insertSDPEntriesInFormattedTable
(
    gpointer ASCIITable,
    gint ColumnCount,
    gchar **DataRow,
    gchar **ColumnRow
)
{
    ft_table_t *Table = (ft_table_t*) ASCIITable;
    const gchar **Row = (const gchar**) DataRow;

    ft_row_write_ln(Table, ColumnCount, Row);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

GLogWriterOutput lanioLogWriter
(
    GLogLevelFlags LogLevel,
    const GLogField *Fields,
    gsize NumberOfFields,
    gpointer Data
)
{
    GLogLevelFlags WantedLogLevel;

    if(((DiscoveryCLIParameters*) Data)->Debug)
        WantedLogLevel = G_LOG_LEVEL_DEBUG;
    else
        WantedLogLevel = G_LOG_LEVEL_INFO;


    if(LogLevel > WantedLogLevel)
        return G_LOG_WRITER_HANDLED;

    if(((DiscoveryCLIParameters*) Data)->DiscoverTerminal)
        g_log_writer_standard_streams(LogLevel, Fields, NumberOfFields, NULL);
    else
        g_log_writer_journald(LogLevel, Fields, NumberOfFields, NULL);

    return G_LOG_WRITER_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gboolean checkNetworkInterfaceName(gchar *InterfaceName)
{
    gboolean ReturnValue = FALSE;

    GError *DirectoryError = NULL;
    GDir *InterfaceDirectory =
        g_dir_open(NETWORK_INTERFACES_PATH, 0, &DirectoryError);
        // 0 : Flags, currently unused. Reserved for future GLib versions.

    processGError
    (
        "Unable to open network interfaces directory",
        DirectoryError
    );

    const gchar *FileName = NULL;
    while( (FileName = g_dir_read_name(InterfaceDirectory)) )
    {
        if(g_strcmp0(FileName, InterfaceName) == 0)
        {
            ReturnValue = TRUE;
            break;
        }
    }

    g_dir_close(InterfaceDirectory);

    if(!ReturnValue)
    {
        g_warning("%s : Invalid network interface name.\n", InterfaceName);
    }

    return ReturnValue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

FILE* checkLockFile(gchar *LockFilePath, gchar *ErrorMessage)
{
    FILE *LockFile = g_fopen(LockFilePath, "w+");

    if(!LockFile)
    {
        g_warning("Can't open lock file %s\n", LockFilePath);
        exit(EXIT_FAILURE);
    }

    struct flock Lock;
    memset(&Lock, 0, sizeof(Lock));

    Lock.l_type = F_WRLCK;
    Lock.l_whence = SEEK_SET;
    Lock.l_start = 0;
    Lock.l_len = 0;

    if(fcntl(fileno(LockFile), F_SETLK, &Lock) == -1)
    {
        g_printerr("%s\n", ErrorMessage);
        exit(EXIT_FAILURE);
    }

    g_free(LockFilePath);

    return LockFile;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
