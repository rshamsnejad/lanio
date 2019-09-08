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
    g_debug("GLib version %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);

    // Set up an UDP/IPv4 socket for SAP discovery
    GSocket *SAPSocket = NULL;

    openSocket(&SAPSocket, G_SOCKET_FAMILY_IPV4,
                G_SOCKET_TYPE_DATAGRAM,	G_SOCKET_PROTOCOL_DEFAULT);

    // Bind the SAP socket to listen to all local interfaces
    bindSocket(SAPSocket, SAP_LOCAL_ADDRESS, SAP_MULTICAST_PORT);

    // Subscribe to the SAP Multicast group
    joinMulticastGroup(SAPSocket, SAP_MULTICAST_ADDRESS);

    // Set up the SDP Database file
    sqlite3 *SDPDatabase = NULL;
    processSQLiteOpenError
    (
        sqlite3_open(SDP_DATABASE_FILENAME, &SDPDatabase)
    );
    createSAPTable(SDPDatabase);

    // Set up the SAP packet receiving loop
    GMainLoop *SAPDiscoveryLoop = g_main_loop_new(NULL, FALSE);
        // NULL : Use default context
        // FALSE : Don't run the loop right away
    setUpSAPPacketLoop(SAPDiscoveryLoop, SAPSocket, SDPDatabase);

    // Start the loop
    g_main_loop_run(SAPDiscoveryLoop);

    // Cleanup section
    g_main_loop_unref(SAPDiscoveryLoop);
    g_clear_object(&SAPSocket);
    sqlite3_close(SDPDatabase);

    return EXIT_SUCCESS;
}
