/*

SAP_discovery.c

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

    DiscoveryCLIParameters CommandLineParameters;
    initDiscoveryCLIParameters(&CommandLineParameters);

    parseDiscoveryCommandLineOptions
    (
        &CommandLineParameters,
        argc,
        argv);

    /* guint CommandLineRemainingOptionsSize =
        getStringArraySize(CommandLineRemainingOptions); */

    // Set up the SDP Database file
    gchar *SDPDatabasePath = getSDPDatabasePath();
    sqlite3 *SDPDatabase = NULL;

    processSQLiteOpenError
    (
        sqlite3_open(SDPDatabasePath, &SDPDatabase)
    );

    g_free(SDPDatabasePath);

    if(!CommandLineParameters.DiscoverDaemon)
    {
        g_debug(PROG_NAME " Discovery : mode terminal");

        g_log_set_writer_func(g_log_writer_standard_streams, NULL, NULL);

        discoverSAPAnnouncements(SDPDatabase);
    }
    else // if(CommandLineParameters.DiscoverDaemon)
    {
        g_debug(PROG_NAME " Discovery : mode terminal");

        daemonizeDiscovery();

        g_info(PROG_LONG_NAME "\n-- Started network discovery");

        discoverSAPAnnouncements(SDPDatabase);
    }

    // g_strfreev(CommandLineRemainingOptions);
    sqlite3_close(SDPDatabase);

    g_debug("Reached end of main() in SAP discovery.");
    return EXIT_SUCCESS;
}
