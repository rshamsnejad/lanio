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

    // Convert each line as a new string in an array
    gchar **StringArray = g_strsplit(SDPString, "\n", 0);

    gchar **ParameterArray = NULL;
    gboolean RegexCheck = FALSE;

    // Hash table for the "a="" attributes
    GHashTable *SDPAttributes =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    g_print("\t==== String array of string arrays :\n");

    for
    (
        gsize i = 0;
        StringArray[i] != NULL && StringArray[i][0] != '\0';
        i++
    )
    {
        // Separate SDP keys and values
        ParameterArray = g_strsplit(StringArray[i], "=", 2);

        g_print("%s", ParameterArray[0]);
        g_print("\t%s\n", ParameterArray[1]);

        // SDP Connection value "c="
        if(g_strcmp0(ParameterArray[0],"c") == 0)
        {
            GError *RegexConnectionError = NULL;
            GRegex *RegexConnection =
                g_regex_new
                (
                    REGEX_SDP_VALUE_CONNECTION,
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexConnectionError
                );
            processGError("Error with regex for c=", RegexConnectionError);

            GMatchInfo *RegexConnectionMatchInfo = NULL;

            RegexCheck =
                g_regex_match
                (
                    RegexConnection,
                    ParameterArray[1],
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexConnectionMatchInfo
                );

            g_print
            (
                "--------%s--------\n",
                RegexCheck ? "VALID" : "INVALID"
            );

            gchar *AddressString =
                g_match_info_fetch_named(RegexConnectionMatchInfo, "address");
            gchar *TTLString =
                g_match_info_fetch_named(RegexConnectionMatchInfo, "ttl");
            g_print("\t\tAddress = %s\n", AddressString);
            g_print("\t\tTTL = %s\n", TTLString);

            g_free(AddressString);
            g_free(TTLString);
            g_match_info_free(RegexConnectionMatchInfo);
            g_regex_unref(RegexConnection);
        }

        // SDP Timing value "t="
        if(g_strcmp0(ParameterArray[0],"t") == 0)
        {
            RegexCheck = g_strcmp0(ParameterArray[1],"0 0") == 0;

            g_print
            (
                "--------%s--------\n",
                RegexCheck ? "VALID ; Permanent." : "INVALID"
            );
        }

        // SDP Media section "m="
        if(g_strcmp0(ParameterArray[0],"m") == 0)
        {
            GError *RegexMediaError = NULL;
            GRegex *RegexMedia =
                g_regex_new
                (
                    REGEX_SDP_VALUE_MEDIA,
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexMediaError
                );
            processGError("Error with regex for m=", RegexMediaError);

            GMatchInfo *RegexMediaMatchInfo = NULL;

            RegexCheck =
                g_regex_match
                (
                    RegexMedia,
                    ParameterArray[1],
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexMediaMatchInfo
                );

            g_print
            (
                "--------%s--------\n",
                RegexCheck ? "VALID" : "INVALID"
            );

            gchar *PortString =
                g_match_info_fetch_named(RegexMediaMatchInfo, "port");
            gchar *PayloadString =
                g_match_info_fetch_named(RegexMediaMatchInfo, "payload");
            g_print("\t\tPort = %s\n", PortString);
            g_print("\t\tPayload = %s\n", PayloadString);

            g_free(PortString);
            g_free(PayloadString);
            g_match_info_free(RegexMediaMatchInfo);
            g_regex_unref(RegexMedia);
        }

        // SDP Attributes "a=""
        if(g_strcmp0(ParameterArray[0],"a") == 0)
        {
            GError *RegexAttributeError = NULL;
            GRegex *RegexAttribute =
                g_regex_new
                (
                    REGEX_SDP_VALUE_ATTRIBUTE,
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexAttributeError
                );
            processGError("Error with regex for a=", RegexAttributeError);

            // Split attribute line "a=key:value"
            GMatchInfo *RegexAttributeMatchInfo = NULL;
            RegexCheck =
                g_regex_match
                (
                    RegexAttribute,
                    ParameterArray[1],
                    G_REGEX_MATCH_NOTEMPTY,
                    &RegexAttributeMatchInfo
                );

            g_print
            (
                "--------%s--------\n",
                RegexCheck ? "VALID" : "INVALID"
            );

            gchar *KeyString =
                g_match_info_fetch_named(RegexAttributeMatchInfo, "key");
            gchar *ValueString =
                g_match_info_fetch_named(RegexAttributeMatchInfo, "value");

            g_hash_table_insert(SDPAttributes, KeyString, ValueString);

            g_match_info_free(RegexAttributeMatchInfo);
            g_regex_unref(RegexAttribute);
        }

        g_strfreev(ParameterArray);
    }

    g_hash_table_foreach(SDPAttributes, callback_printHashTable, NULL);

    g_hash_table_destroy(SDPAttributes);
    g_strfreev(StringArray);

    return EXIT_SUCCESS;
}
