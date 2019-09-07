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

    // Setting up the SAP packet receiving loop
        GMainLoop *SAPDiscoveryLoop = g_main_loop_new(NULL, FALSE);
            // NULL : Use default context
            // FALSE : Don't run the loop right away

        // Every 3 seconds, delete outdated database entries
        data_deleteOldSDPEntries TimeoutDeleteData;

        TimeoutDeleteData.DiscoveryLoop = SAPDiscoveryLoop;
        TimeoutDeleteData.Database = SDPDatabase;

        g_timeout_add
        (
            3 * MSEC_IN_SEC,
            (GSourceFunc) callback_deleteOldSDPEntries,
            &TimeoutDeleteData
        );

        // Insert or delete database entries based on incoming SAP packets
        GSource *SAPSocketMonitoring =
            g_socket_create_source(SAPSocket, G_IO_IN | G_IO_PRI, NULL);
                // NULL : No GCancellable needed
        g_source_attach(SAPSocketMonitoring, NULL);
            // NULL : Use default context

        data_insertIncomingSAPPackets SAPSocketMonitoringData;

        SAPSocketMonitoringData.DiscoveryLoop = SAPDiscoveryLoop;
        SAPSocketMonitoringData.Database = SDPDatabase;

        g_source_set_callback
        (
            SAPSocketMonitoring,
            (GSourceFunc) callback_insertIncomingSAPPackets,
            &SAPSocketMonitoringData,
            NULL
        );

    // Start the loop
    g_main_loop_run(SAPDiscoveryLoop);

    // Cleanup section
    g_main_loop_unref(SAPDiscoveryLoop);
    g_clear_object(&SAPSocket);
    sqlite3_close(SDPDatabase);
    
    return EXIT_SUCCESS;
}
