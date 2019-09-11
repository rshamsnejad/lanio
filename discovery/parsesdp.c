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
    gchar sdp[] =
        "v=0\n"
        "o=- 3287835752 3287835752 IN IP4 192.168.0.102\n"
        "s=DIGISUPPORT-AudiowayBridge : 32\n"
        "c=IN IP4 239.69.207.194/32\n"
        "t=0 0\n"
        "a=keywds:Dante\n"
        "m=audio 5004 RTP/AVP 97\n"
        "i=2 channels: 03, 04\n"
        "a=recvonly\n"
        "a=rtpmap:97 L24/48000/2\n"
        "a=ptime:1\n"
        "a=ts-refclk:ptp=IEEE1588-2008:00-1D-C1-FF-FE-0D-A9-88:0\n"
        "a=mediaclk:direct=1057709823\n";

    g_print("\t==== SDP :\n%s", sdp);

    gchar **StringArray = g_strsplit(sdp, "\n", 0);

    g_print("\t==== String array :\n");

    for
    (
        gsize i = 0;
        StringArray[i] != NULL && StringArray[i][0] != '\0';
        i++
    )
    {
        g_print("%s\n", StringArray[i]);
    }

    gchar **ParameterArray = NULL;

    g_print("\t==== String array of string arrays :\n");

    for
    (
        gsize i = 0;
        StringArray[i] != NULL && StringArray[i][0] != '\0';
        i++
    )
    {
        ParameterArray = g_strsplit(StringArray[i], "=", 2);

        g_print("%s", ParameterArray[0]);
        g_print("\t%s\n", ParameterArray[1]);

        // TODO : g_regex for parsing ParameterArray[1]

        g_strfreev(ParameterArray);
    }

    g_strfreev(StringArray);
    return EXIT_SUCCESS;
}
