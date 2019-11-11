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

    // Redirect the logs to stdout/stderr or journald
    // depending on the CLI parameters
    data_lanioLogWriter LogParameters;
    LogParameters.Terminal = CommandLineParameters.Terminal;
    LogParameters.Debug = CommandLineParameters.Debug;
    g_log_set_writer_func
    (
        lanioLogWriter,
        &LogParameters,
        NULL
    );

    g_debug
    (
        "GLib version %u.%u.%u",
        glib_major_version,
        glib_minor_version,
        glib_micro_version
    );

    // Create and store the working directories' paths
    WorkingDirectoryList WorkingDirectories;
    initWorkingDirectoryList(&WorkingDirectories);

    // Check if an instance is already running based on lock file
    FILE *DiscoveryLockFile =
        checkOrCreateLockFile
        (
            LOCK_FILE_DISCOVERY,
            &WorkingDirectories,
            PROG_NAME " Discovery is already running. Aborting."
        );

    // Set up the SDP Database file
    gchar *SDPDatabasePath =
        getSDPDatabasePath(WorkingDirectories.DiscoveryWorkingDirectory);
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

    // Start the main loop in the terminal or as a daemon
    if(CommandLineParameters.Terminal)
    {
        g_debug(PROG_NAME " Discovery : mode terminal");
    }
    else // if(!CommandLineParameters.Terminal)
    {
        g_debug(PROG_NAME " Discovery : mode daemon");

        daemonizeDiscovery();
    }

    g_info
    (
        PROG_LONG_NAME "\n"
        "-- Starting network discovery on interface %s",
        CommandLineParameters.Interface
    );

    discoverSAPAnnouncements(SDPDatabase, CommandLineParameters.Interface);

    sqlite3_close(SDPDatabase);
    fclose(DiscoveryLockFile);
    freeWorkingDirectoryList(&WorkingDirectories);

    g_debug("Reached end of main() in lanio-discovery.c");
    return EXIT_SUCCESS;
}
