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

    if(CommandLineParameters.JACK)
    {
        g_info("Outputting to JACK server not yet supported. Aborting");
        exit(EXIT_FAILURE);
    }
    else if(CommandLineParameters.ALSADevice)
        g_info
            ("Outputting to ALSA Device %s", CommandLineParameters.ALSADevice);

    gchar *ReturnedSDP =
        getSDPFromHash(SDPDatabase, CommandLineParameters.StreamID);
    if(!checkValidSDPString(ReturnedSDP))
        g_info("Stream ID %u not found.", CommandLineParameters.StreamID);
    else
        g_info("Stream ID %u selected", CommandLineParameters.StreamID);


    GMainLoop *LanioReceiveLoop = g_main_loop_new(NULL, FALSE);

        // Connecting UNIX signal handlers
    static data_callback_terminateOnUNIXSignals SignalInterruptData;
    SignalInterruptData.LoopToQuit = LanioReceiveLoop;
    SignalInterruptData.Signal = SIGINT;
    g_unix_signal_add
    (
        SignalInterruptData.Signal,
        (GSourceFunc) callback_terminateOnUNIXSignals,
        &SignalInterruptData
    );
    static data_callback_terminateOnUNIXSignals SignalTerminateData;
    SignalTerminateData.LoopToQuit = LanioReceiveLoop;
    SignalTerminateData.Signal = SIGTERM;
    g_unix_signal_add
    (
        SignalTerminateData.Signal,
        (GSourceFunc) callback_terminateOnUNIXSignals,
        &SignalTerminateData
    );

    GstElement
        *Pipeline, *SDPDecoder, *DecodeBin, *Converter, *Resampler, *ALSASink;

    Pipeline    = gst_pipeline_new("AES67 receiver");
    SDPDecoder  = gst_element_factory_make("sdpsrc", "SDP Decoder");
    DecodeBin   = gst_element_factory_make("decodebin", "Decode Bin");
    Converter   = gst_element_factory_make("audioconvert", "Audio Converter");
    Resampler   = gst_element_factory_make("audioresample", "Audio Resampler");
    ALSASink    = gst_element_factory_make("alsasink", "ALSA Sink");

    if
    (
        !Pipeline || !SDPDecoder || !DecodeBin ||
        !Converter || !Resampler || !ALSASink
    )
    {
        g_printerr("One GStreamer element could not be created. Aborting.");
        exit(EXIT_FAILURE);
    }

    g_object_set(G_OBJECT(SDPDecoder), "sdp", ReturnedSDP, NULL);
    // g_object_set(G_OBJECT(SDPDecoder), "location", "sdp://test.sdp", NULL);
    g_object_set
    (
        G_OBJECT(ALSASink),
        "device",
        CommandLineParameters.ALSADevice,
        NULL
    );

    GstBus *PipelineMessageBus = gst_pipeline_get_bus(GST_PIPELINE(Pipeline));
    guint PipelineMessageBusWatchID =
        gst_bus_add_watch
        (
            PipelineMessageBus,
            callback_processBusMessages,
            LanioReceiveLoop
        );
    gst_object_unref(PipelineMessageBus);

    gst_bin_add_many
    (
        GST_BIN(Pipeline),
        SDPDecoder,
        DecodeBin,
        Converter,
        Resampler,
        ALSASink,
        NULL
    );

    gst_element_link_many
    (
        Converter,
        Resampler,
        ALSASink,
        NULL
    );

        // SDPDecoder and DecodeBin have dynamic pads
    g_signal_connect
    (
        SDPDecoder,
        "pad-added",
        G_CALLBACK(callback_onPadAdded),
        DecodeBin
    );
    g_signal_connect
    (
        DecodeBin,
        "pad-added",
        G_CALLBACK(callback_onPadAdded),
        Converter
    );

    g_info("Playing Stream ID %u", CommandLineParameters.StreamID);
    gst_element_set_state(Pipeline, GST_STATE_PLAYING);
    g_main_loop_run(LanioReceiveLoop);

    gst_element_set_state(Pipeline, GST_STATE_NULL);
    // gst_object_unref(GST_OBJECT(Pipeline));
    g_source_remove(PipelineMessageBusWatchID);
    g_main_loop_unref(LanioReceiveLoop);

////////////////////////////////////////////////////////////////////////////////


/////////////////////////////// CLEANUP ////////////////////////////////////////

    sqlite3_close(SDPDatabase);
    freeWorkingDirectoryList(&WorkingDirectories);
    freeReceiveCLIParameters(&CommandLineParameters);

////////////////////////////////////////////////////////////////////////////////


    g_debug("Reached end of main() in lanio-receive.c");
    return EXIT_SUCCESS;
}
