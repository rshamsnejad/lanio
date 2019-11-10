/*

lanio-receive.c

Receive AES67 streams discovered by lanio-discovery.

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
    ReceiveCLIParameters CommandLineParameters;
    initReceiveCLIParameters(&CommandLineParameters);
    parseReceiveCLIOptions
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
            SQLITE_OPEN_READONLY,
            NULL
        )
    );
    g_free(SDPDatabasePath);

    g_info("Receiving Stream ID %u", CommandLineParameters.StreamID);

    if(CommandLineParameters.JACK)
    {
        g_info("Outputting to JACK server");
    }
    else if(CommandLineParameters.ALSADevice)
    {
        g_info
            ("Outputting to ALSA Device %s", CommandLineParameters.ALSADevice);
    }

    sqlite3_close(SDPDatabase);
    freeWorkingDirectoryList(&WorkingDirectories);

    g_debug("Reached end of main() in lanio-receive.c");
    return EXIT_SUCCESS;
}
