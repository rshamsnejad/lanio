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
    gchar SDPString[] =
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

    g_print("\t==== SDP :\n%s", SDPString);

    g_print
    (
        "\n---------------%s---------------\n\n",
        checkValidSDP(SDPString) ? "VALID" : "INVALID"
    );


    gchar **StringArray = g_strsplit(SDPString, "\n", 0);

    // g_print("\t==== String array :\n");
    //
    // for
    // (
    //     gsize i = 0;
    //     StringArray[i] != NULL && StringArray[i][0] != '\0';
    //     i++
    // )
    // {
    //     g_print("%s\n", StringArray[i]);
    // }

    gchar **ParameterArray = NULL;
    gboolean RegexCheck = FALSE;

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

        if(g_strcmp0(ParameterArray[0],"c") == 0)
        {
            RegexCheck =
                g_regex_match_simple
                (
                    REGEX_SDP_VALUE_CONNECTION,
                    ParameterArray[1],
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY
                );
            g_print
            (
                "--------%s--------\n",
                RegexCheck ? "VALID" : "INVALID"
            );

            gchar **ConnectionArray = g_strsplit(ParameterArray[1], " ", 3);

            RegexCheck =
                g_regex_match_simple
                (
                    REGEX_IP_WITH_CIDR,
                    ConnectionArray[2],
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY
                );
            gboolean CheckConnection =
                g_strcmp0(ConnectionArray[0], "IN") == 0 &&
                g_strcmp0(ConnectionArray[1], "IP4") == 0 &&
                RegexCheck; // REGEX TO FIX

            g_print
            (
                "\t\t%s IPv4 address\n",
                CheckConnection ? "Valid" : "Invalid"
            );

            g_strfreev(ConnectionArray);
        }

        g_strfreev(ParameterArray);
    }

    g_strfreev(StringArray);

    return EXIT_SUCCESS;
}
