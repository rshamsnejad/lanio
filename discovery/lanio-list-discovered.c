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

    if(CommandLineParameters.StreamCategory)
    {
        if
        (
            g_strcmp0
            (
                CommandLineParameters.StreamCategory,
                SAP_TABLE_CLI_PARAMETER
            ) == 0
        )
        {
            printSAPEntries
            (
                SDPDatabase,
                CommandLineParameters.CSV
            );
        }
        else if
        (
            g_strcmp0
            (
                CommandLineParameters.StreamCategory,
                MDNS_TABLE_CLI_PARAMETER
            ) == 0
        )
        {
            printMDNSEntries(CommandLineParameters.CSV);
        }
        else if
        (
            g_strcmp0
            (
                CommandLineParameters.StreamCategory,
                SDPFILES_TABLE_CLI_PARAMETER
            ) == 0
        )
        {
            printSDPFilesEntries(CommandLineParameters.CSV);
        }
        else
        {
            g_warning("Unknown stream category");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printSAPEntries
        (
            SDPDatabase,
            CommandLineParameters.CSV
        );
        printMDNSEntries(CommandLineParameters.CSV);
        printSDPFilesEntries(CommandLineParameters.CSV);
    }

    sqlite3_close(SDPDatabase);
    freeWorkingDirectoryList(&WorkingDirectories);

    g_debug("Reached end of main() in lanio-list-discovered.c");
    return EXIT_SUCCESS;
}
