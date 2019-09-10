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

    if(CommandLineParameters.LogStandard)
    {
        g_debug("Output to standard streams");

        g_log_set_writer_func(g_log_writer_standard_streams, NULL, NULL);
    }
    else if(CommandLineParameters.LogJournald)
    {
        g_debug("Output to journald");

        g_log_set_writer_func(g_log_writer_journald, NULL, NULL);
    }

    if(CommandLineParameters.Show)
    {
        g_debug("Show mode");

        g_print("Teeny weeny streamy\n");
    }
    else if(CommandLineParameters.DiscoverTerminal)
    {
        g_debug("Discover mode, terminal");

        discoverSAPAnnouncements(SDPDatabase);
    }
    else if(CommandLineParameters.DiscoverDaemon)
    {
        g_debug("Discover mode, daemon");

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

        if(!CommandLineParameters.LogStandard)
        {
            /* Close out the standard file descriptors */
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }

        /* Daemon-specific initialization goes here */
        g_info(PROG_LONG_NAME "\n-- Started network discovery");
        /* The Big Loop */
        discoverSAPAnnouncements(SDPDatabase);
    }

    // g_strfreev(CommandLineRemainingOptions);
    sqlite3_close(SDPDatabase);

    g_debug("Reached end of main() in SAP discovery.");
    return EXIT_SUCCESS;
}
