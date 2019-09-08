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

    gboolean    ShowParameter = FALSE;
    gboolean    DiscoverParameter = FALSE;
    // gchar     **CommandLineRemainingOptions = NULL;

    parseCommandLineOptions(&ShowParameter, &DiscoverParameter, argc, argv);

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

    if(ShowParameter)
        g_print("Teeny weeny streamy\n");
    else if(DiscoverParameter)
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
        if(ChildPID == 0)
            g_debug("CHILDEUH");
        /* The Big Loop */
        discoverSAPAnnouncements(SDPDatabase);
    }

    // g_strfreev(CommandLineRemainingOptions);
    sqlite3_close(SDPDatabase);

    return EXIT_SUCCESS;
}
