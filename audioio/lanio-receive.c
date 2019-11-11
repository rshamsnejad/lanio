/*

lanio-receive.c

Receive AES67 streams discovered by lanio-discovery.

*/

#include "../headers/lanio.h"


gint main(gint argc, gchar *argv[])
{

////////////////////////////////// INIT ////////////////////////////////////////

        // Parse the command-line options
    ReceiveCLIParameters CommandLineParameters;
    initReceiveCLIParameters(&CommandLineParameters);

    parseReceiveCLIOptions(&CommandLineParameters, argc, argv);

        // Redirect the logs to stdout/stderr or journald
        // depending on the CLI parameters, and handle log levels
    setLogHandler(CommandLineParameters.Terminal, CommandLineParameters.Debug);

        // Print library versions to debug canal
    printLibraryVersions();

        // Create and store the working directories' paths
    WorkingDirectoryList WorkingDirectories;
    initWorkingDirectoryList(&WorkingDirectories);

        // Set up the SDP Database file
    sqlite3 *SDPDatabase = NULL;
    openSDPDatabase(&SDPDatabase, SQLITE_OPEN_READONLY, &WorkingDirectories);

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////// CORE ////////////////////////////////////////

    g_info("Receiving Stream ID %u", CommandLineParameters.StreamID);
    gchar *ReturnedSDP =
        getSDPFromHash(SDPDatabase, CommandLineParameters.StreamID);
    g_debug("%s", ReturnedSDP);

    if(CommandLineParameters.JACK)
        g_info("Outputting to JACK server");
    else if(CommandLineParameters.ALSADevice)
        g_info
            ("Outputting to ALSA Device %s", CommandLineParameters.ALSADevice);

////////////////////////////////////////////////////////////////////////////////


/////////////////////////////// CLEANUP ////////////////////////////////////////

    sqlite3_close(SDPDatabase);
    freeWorkingDirectoryList(&WorkingDirectories);

////////////////////////////////////////////////////////////////////////////////


    g_debug("Reached end of main() in lanio-receive.c");
    return EXIT_SUCCESS;
}
