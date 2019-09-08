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

    if(ShowParameter)
        g_print("Teeny weeny streamy\n");
    else if(DiscoverParameter)
        discoverSAPAnnouncements();

    // g_strfreev(CommandLineRemainingOptions);

    return EXIT_SUCCESS;
}
