/*

lanio-discovery.c

Discover SAP announcement of Dante streams.

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "../../headers/lanio.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

gint main(gint argc, gchar *argv[])
{
    g_debug
    (
        "GLib version %u.%u.%u",
        glib_major_version,
        glib_minor_version,
        glib_micro_version
    );

    // Parse the command-line options
    DiscoveryCLIParameters CommandLineParameters;
    initDiscoveryCLIParameters(&CommandLineParameters);
    parseDiscoveryCommandLineOptions
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
        sqlite3_open(SDPDatabasePath, &SDPDatabase)
    );
    g_free(SDPDatabasePath);

    // Start the main loop in the terminal or as a daemon
    if(!CommandLineParameters.DiscoverDaemon)
    {
        g_debug(PROG_NAME " Discovery : mode terminal");

        g_log_set_writer_func(g_log_writer_standard_streams, NULL, NULL);

        discoverSAPAnnouncements(SDPDatabase);
    }
    else // if(CommandLineParameters.DiscoverDaemon)
    {
        g_debug(PROG_NAME " Discovery : mode daemon");

        daemonizeDiscovery();

        g_info(PROG_LONG_NAME "\n-- Started network discovery");

        discoverSAPAnnouncements(SDPDatabase);
    }


    sqlite3_close(SDPDatabase);

    g_debug("Reached end of main() in lanio-discovery.c");
    return EXIT_SUCCESS;
}
