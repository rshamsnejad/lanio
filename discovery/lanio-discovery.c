/*

lanio-discovery.c

Discover SAP announcement of Dante streams.

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "../headers/lanio.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gint main(gint argc, gchar *argv[])
{
    // Parse the command-line options
    DiscoveryCLIParameters CommandLineParameters;
    initDiscoveryCLIParameters(&CommandLineParameters);
    parseDiscoveryCLIOptions
    (
        &CommandLineParameters,
        argc,
        argv
    );

    // Set up the SDP Database file
    gchar *SDPDatabasePath = getSDPDatabasePath();
    sqlite3 *SDPDatabase = NULL;
    processSQLiteOpenError
    (
        sqlite3_open_v2
        (
            SDPDatabasePath,
            &SDPDatabase,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            NULL
        )
    );
    g_free(SDPDatabasePath);

    // Redirect the logs to stdout/stderr or journald
    // depending on the CLI parameters
    g_log_set_writer_func
    (
        lanioLogWriter,
        &CommandLineParameters,
        NULL
    );

    g_debug
    (
        "GLib version %u.%u.%u",
        glib_major_version,
        glib_minor_version,
        glib_micro_version
    );

    // Start the main loop in the terminal or as a daemon
    if(CommandLineParameters.DiscoverTerminal)
    {
        g_debug(PROG_NAME " Discovery : mode terminal");
    }
    else // if(!CommandLineParameters.DiscoverTerminal)
    {
        g_debug(PROG_NAME " Discovery : mode daemon");

        daemonizeDiscovery();
    }

/////////////// LOCK ////////////////

    FILE *DiscoveryLockFile = g_fopen("/tmp/lanio-discovery.lock", "w+");

    if(!DiscoveryLockFile)
    {
        g_error("File open error\n");
        exit(EXIT_FAILURE);
    }

    struct flock DiscoveryLock;
    memset(&DiscoveryLock, 0, sizeof(DiscoveryLock));

    DiscoveryLock.l_type = F_WRLCK;
    DiscoveryLock.l_whence = SEEK_SET;
    DiscoveryLock.l_start = 0;
    DiscoveryLock.l_len = 0;

    if(fcntl(fileno(DiscoveryLockFile), F_SETLK, &DiscoveryLock) == -1)
    {
        g_error("File lock error\n");
        exit(EXIT_FAILURE);
    }

/////////////// LOCK ////////////////

    g_info
    (
        PROG_LONG_NAME "\n"
        "-- Starting network discovery on interface %s",
        CommandLineParameters.Interface
    );

    discoverSAPAnnouncements(SDPDatabase, CommandLineParameters.Interface);

    sqlite3_close(SDPDatabase);
    fclose(DiscoveryLockFile);

    g_debug("Reached end of main() in lanio-discovery.c");
    return EXIT_SUCCESS;
}
