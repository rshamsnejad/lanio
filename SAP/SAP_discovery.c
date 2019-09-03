/*

SAP_discovery.c

Discover SAP announcement of Dante streams.

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "../headers/linux-aes67.h"

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

    bindSocket(&SAPSocket, SAP_LOCAL_ADDRESS, SAP_MULTICAST_PORT);

    // Subscribe to the SAP Multicast group

    joinMulticastGroup(&SAPSocket, SAP_MULTICAST_ADDRESS);

    // Open the SDP Database file
    sqlite3 *SDPDatabase = NULL;
    processSQLiteOpenError
    (
        sqlite3_open(SDP_DATABASE_FILENAME, &SDPDatabase)
    );

    createSAPTable(&SDPDatabase);

    // Begin the SAP packet receiving loop

    gchar SAPPacketBuffer[SAP_PACKET_BUFFER_SIZE] = {'\0'};
    gssize SAPPacketBufferBytesRead = 0;
    gchar SAPPacketSourceAddress[IPV4_ADDRESS_LENGTH] = {'\0'};

    SAPPacket *ReceivedSAPPAcket = NULL;

    while(TRUE)
    {
        // puts("Begin loop");

        SAPPacketBufferBytesRead =
            receivePacket
            (
                &SAPSocket,
                SAPPacketSourceAddress,
                ARRAY_SIZE(SAPPacketSourceAddress),
                SAPPacketBuffer,
                ARRAY_SIZE(SAPPacketBuffer)
            );

        if(SAPPacketBufferBytesRead <= 0) // The connection has been reset
        {
            g_print("Terminated\n");
            break;
        }

        ReceivedSAPPAcket = convertSAPStringToStruct(SAPPacketBuffer);

        printSAPPacket(ReceivedSAPPAcket);

        freeSAPPacket(ReceivedSAPPAcket);

        // printPacketUgly(SAPPacketBuffer,
        //                 SAPPacketBufferBytesRead,
        //                     SAPPacketSourceAddress);

        // insertStringInSQLiteTable(&SDPDatabase, SAP_TABLE_NAME, "sdp",
        //                             &SAPPacketBuffer[SAP_PACKET_HEADER_SIZE]);

    } // End of while()

    g_clear_object(&SAPSocket);
    sqlite3_close(SDPDatabase);
    return EXIT_SUCCESS;
}
