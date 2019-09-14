/*

lanio-list-discovered.c

List available streams discovered by lanio-discovery.

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
    g_debug
    (
        "GLib version %u.%u.%u",
        glib_major_version,
        glib_minor_version,
        glib_micro_version
    );

    // Parse the command-line options
    ListDiscoveredCLIParameters CommandLineParameters;
    initListDiscoveredCLIParameters(&CommandLineParameters);
    parseListDiscoveredCLIOptions
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
            SQLITE_OPEN_READONLY,
            NULL
        )
    );
    g_free(SDPDatabasePath);

    printSDPEntries
    (
        SDPDatabase,
        SAP_TABLE_NAME,
        SAP_TABLE_DISPLAY_NAME,
        CommandLineParameters.CSV
    );

    sqlite3_close(SDPDatabase);

    g_debug("Reached end of main() in lanio-list-discovered.c");
    return EXIT_SUCCESS;
}
