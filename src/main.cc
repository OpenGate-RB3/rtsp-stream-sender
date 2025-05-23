#include <cstdio>
#include <defaults.hpp>
#include <gst/gst.h>
#include <iostream>
#include <argparse.hpp>
#include <info.hpp>
#include <appContext.hpp>


appContext::context cxt;

void parse_args(const int argc, char **argv) {
    argparse::ArgumentParser program("gst-open-gate-rtsp-sender",version);
    program.add_description("utility for sending video and audio data over udp to an rtsp endpoint\n");
    program.add_argument("-location").help("rtsp address where to send the stream, for example rtsp://example:5000").required();
    program.add_argument("-w","-width").help("width of capture, default: "+std::to_string(DEFAULT_WIDTH)).default_value(DEFAULT_WIDTH).scan<'d',int>();
    program.add_argument("-h","-height").help("height of capture, default: "+std::to_string(DEFAULT_HEIGHT)).default_value(DEFAULT_HEIGHT).scan<'d',int>();
    program.add_argument("-raw","-raw_format").help("encoding of stream, default: NV12").default_value(DEFAULT_RAW_FORMAT);
    program.add_argument("-ai_raw").help("AI inference stream format, default: RGB").default_value(DEFAULT_AI_RAW_FORMAT);
    program.add_argument("-ai_location").help("location of the rtsp server to send too").required();
    program.add_argument("-fps").help("frame rate of the camera source").default_value(DEFAULT_FRAMES).scan<'d',int>();
    program.add_argument("-jpeg_amount","-jfps").help("frame rate of the jpeg stream for ai inference").default_value(DEFAULT_JPEG_RATE).scan<'d',int>();
    program.add_epilog("The program ships with defaults for most of these parameter and their providing is only need if configuration of the system is non-default");
    try {
        program.parse_args(argc, argv);
        // load app context here
        cxt.url_location = program.get<std::string>("location"); // this is required
        cxt.width = program.get<int>("-width");
        cxt.height = program.get<int>("-height");
        cxt.format = program.get<std::string>("-raw_format");
        cxt.ai_format = program.get<std::string>("-ai_raw");
        cxt.ai_location = program.get<std::string>("-ai_location");
        cxt.fps = program.get<int>("-fps");
        cxt.jfps = program.get<int>("-jfps");
    }catch(const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        std::cerr << program << std::endl;
        exit(1);
    }
}

bool setup_pipline(appContext::context & cxt) {
    // todo likely add error checking on gstreamer functions beyond line 140
    // top level
    GstElement *qtiqmfsrc, *capsfilter, *tee, *capsfilter_rgb, *capsfilter_rate_limit;
    // video pipeline
    GstElement *queue_stream, *v4l2h264enc, *h264parse, *rtph264pay; // video pipeline --> encode into h264 --> send to rtph264pay --> rtpclientsink
    // audio pipeline
    GstElement *pulsesrc, *audioCapsFilter , *audioconvert, *mp3enc, *rtpmpapay; // audio pipeline --> audio src --> convert to mpeg4 --> rtp packetize --> rtpclientsink
    // ai pipeline
    GstElement *queue_ai, *videoconvert_ai, *jpegenc, *rtpjpeg; // video src --> convert to rgb raw --> jpeg --> rtpclientsink
    // RTSP  specif stuff
    GstElement *rtpsinkClient, *rtpsinkClient_ai;
    // ratelimit
    GstElement *videorate_ai;
    GstCaps *filterscap, *aiCap, *fps_caps, *audio_caps; // needed to define stream conversions during differnt steps

    // top level components
    qtiqmfsrc = gst_element_factory_make("qtiqmfsrc", "qtiqmmfsrc");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    tee = gst_element_factory_make("tee", "tee"); // similar to command line utility splits stream
    capsfilter_rgb = gst_element_factory_make("capsfilter", "capsfilterrgb");
    capsfilter_rate_limit = gst_element_factory_make("capsfilter", "capsfilterratelimit");
    //queues
    queue_stream = gst_element_factory_make("queue", "queue_stream");
    queue_ai = gst_element_factory_make("queue", "queue_ai");
    //video components
    v4l2h264enc = gst_element_factory_make("v4l2h264enc", "v4l2h264enc");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    rtph264pay = gst_element_factory_make("rtph264pay", "rtph264pay");
    // audio components
    pulsesrc = gst_element_factory_make("pulsesrc", "pulsesrc");
    audioCapsFilter = gst_element_factory_make("capsfilter", "audioCapsFilter");
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    mp3enc = gst_element_factory_make("lamemp3enc", "lamemp3enc");
    rtpmpapay = gst_element_factory_make("rtpmpapay", "rtpmpapay");

    // rtpsink client for standar video stream
    rtpsinkClient = gst_element_factory_make("rtspclientsink", "rtpsink");
    // ai sinks
    videorate_ai = gst_element_factory_make("videorate", "videorate_ai");
    videoconvert_ai = gst_element_factory_make("videoconvert", "videoconvert_ai");
    jpegenc = gst_element_factory_make("jpegenc", "jpegenc");
    rtpjpeg = gst_element_factory_make("rtpjpegpay", "rtpjpeg");
    rtpsinkClient_ai = gst_element_factory_make("rtspclientsink", "rtpsinkclient_ai");

    // check if components were correctly made
    if (   !qtiqmfsrc
        || !capsfilter
        || !tee
        || !capsfilter_rgb
        || !capsfilter_rate_limit
        ||!queue_stream
        || !queue_ai
        || !v4l2h264enc
        || !h264parse
        || !rtph264pay
        || !pulsesrc
        || !audioCapsFilter
        || !audioconvert
        || !videoconvert_ai
        || !mp3enc
        || !rtpmpapay
        || !rtpsinkClient
        || !rtpsinkClient_ai
        || !jpegenc
        || !rtpjpeg
        || !videorate_ai
        )
    {
        std::cerr << "Failed to create GstElements" << std::endl;
        return false;
    }
    // set up caps
    filterscap = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, cxt.format.c_str(),
        "width", G_TYPE_INT, cxt.width,
        "height", G_TYPE_INT, cxt.height,
        "framerate", GST_TYPE_FRACTION, cxt.fps, 1,
        "compression", G_TYPE_STRING, "ubwc",
        "interlace-mode", G_TYPE_STRING, "progressive",
        "colorimetry", G_TYPE_STRING, "bt601",
        NULL
        );
    aiCap = gst_caps_new_simple("video/x-raw","format", G_TYPE_STRING, cxt.ai_format.c_str(),NULL);
    fps_caps = gst_caps_new_simple("video/x-raw","framerate", GST_TYPE_FRACTION, cxt.jfps, 1, NULL);
    audio_caps = gst_caps_new_simple("audio/x-raw",
        "format",G_TYPE_STRING,"S16LE",
        "width", G_TYPE_INT, 16,
        "channels", G_TYPE_INT, 1,
        "rate", G_TYPE_INT, 48000,
        NULL); // from qualcomm example, try this an explore later
    // set caps filters for base stream
    g_object_set(G_OBJECT(capsfilter), "caps", filterscap, NULL);
    gst_caps_unref(filterscap);
    // set caps filter for ai stream, ie nv12 to rgb native
    g_object_set(G_OBJECT(capsfilter_rgb), "caps", aiCap, NULL);
    gst_caps_unref(aiCap);
    g_object_set(G_OBJECT(capsfilter_rate_limit), "caps", fps_caps, NULL);
    gst_caps_unref(fps_caps);
    g_object_set(G_OBJECT(audioCapsFilter),"caps", audio_caps, NULL);
    gst_caps_unref(audio_caps);

    // ai rate limit setup
    g_object_set(videorate_ai, "drop-only", TRUE, NULL);

    // Encoder configuration
    g_object_set(G_OBJECT(v4l2h264enc), "capture-io-mode",5,"output-io-mode",5, NULL);
    g_object_set(G_OBJECT(h264parse),"config-interval",-1,NULL);
    g_object_set(G_OBJECT(rtph264pay),"pt",96,NULL); // h264
    g_object_set(G_OBJECT(rtpmpapay),"pt",14,NULL); // mpeg (mp3)
    g_object_set(G_OBJECT(rtpjpeg), "pt", 26, NULL); // jpeg (M-JPEG)

    // rtsp setup here
    g_object_set(G_OBJECT(rtpsinkClient), "location",cxt.url_location,"sync",FALSE,"async",FALSE,NULL);
    g_object_set(G_OBJECT(rtpsinkClient_ai),"location", cxt.ai_location,"sync",FALSE,"async",FALSE,NULL);


    // Add to pipeline, this is huge block of items
    // this really does not matter, linking is what actually dictates how data flows
    gst_bin_add_many(GST_BIN(cxt.pipeline),
            // top level elements
            qtiqmfsrc,
            capsfilter,
            tee,
            capsfilter_rgb,
            capsfilter_rate_limit,
            // Video pipeline
            queue_stream,
            v4l2h264enc,
            h264parse,
            rtph264pay,
            // Audio pipeline
            pulsesrc,
            audioCapsFilter,
            audioconvert,
            mp3enc,
            rtpmpapay,
            // AI pipeline
            queue_ai,
            videoconvert_ai,
            jpegenc,
            rtpjpeg,
            // RTSP
            rtpsinkClient,
            rtpsinkClient_ai,
            // Ratelimit
            videorate_ai,
            NULL
    );


    // link main elements together
    gst_element_link_many(qtiqmfsrc,capsfilter,tee,NULL);
    // link queues to tee
    GstPad * tee_src_pad = gst_element_request_pad_simple (tee,"src_%u");
    GstPad * queue_dest = gst_element_get_static_pad(queue_ai, "sink");
    gst_pad_link(tee_src_pad,queue_dest);
    gst_object_unref(queue_dest);
    gst_object_unref(tee_src_pad);
    tee_src_pad = gst_element_request_pad_simple (tee,"src_%u");
    queue_dest = gst_element_get_static_pad(queue_stream, "sink");
    gst_pad_link(tee_src_pad,queue_dest);
    gst_object_unref(queue_dest);
    gst_object_unref(tee_src_pad);

    // link AI branch to main elements
    gst_element_link_many(queue_ai,videorate_ai,capsfilter_rate_limit,
        videoconvert_ai,capsfilter_rgb, jpegenc, rtpjpeg,NULL);

    // link videostream pipeline
    gst_element_link_many(queue_stream,v4l2h264enc,h264parse,rtph264pay, NULL);

    // link audio to pipeline
    gst_element_link_many(pulsesrc,audioCapsFilter,audioconvert,mp3enc,rtpmpapay,NULL);

    // link video and audio streams to rtpcleintsink
    GstPad * video_rtp_sink = gst_element_request_pad_simple (rtpsinkClient, "sink_0");
    GstPad * video_src_pad = gst_element_get_static_pad(rtph264pay, "src");
    gst_pad_link(video_src_pad, video_rtp_sink);
    gst_object_unref(video_rtp_sink);
    gst_object_unref(video_src_pad);

    GstPad * audio_rtp_sink = gst_element_request_pad_simple (rtpsinkClient, "sink_1");
    GstPad * audio_src_pad = gst_element_get_static_pad (rtpmpapay, "src");
    gst_pad_link(audio_src_pad, audio_rtp_sink);
    gst_object_unref(audio_rtp_sink);
    gst_object_unref(audio_src_pad);

    GstPad * jpeg_rtp_sink = gst_element_request_pad_simple (rtpsinkClient_ai, "sink_0");
    GstPad * jpeg_rtp_src = gst_element_get_static_pad (rtpjpeg,"src");
    gst_pad_link(jpeg_rtp_src, jpeg_rtp_sink);
    gst_object_unref(jpeg_rtp_src);
    gst_object_unref(jpeg_rtp_sink);

    // add items into list for cleanup after application stops
    // Top level elements
    cxt.elements.push_back(qtiqmfsrc);
    cxt.elements.push_back(capsfilter);
    cxt.elements.push_back(tee);
    cxt.elements.push_back(capsfilter_rgb);
    cxt.elements.push_back(capsfilter_rate_limit);

    // Video pipeline elements
    cxt.elements.push_back(queue_stream);
    cxt.elements.push_back(v4l2h264enc);
    cxt.elements.push_back(h264parse);
    cxt.elements.push_back(rtph264pay);

    // Audio pipeline elements
    cxt.elements.push_back(pulsesrc);
    cxt.elements.push_back(audioconvert);
    cxt.elements.push_back(audioCapsFilter);
    cxt.elements.push_back(mp3enc);
    cxt.elements.push_back(rtpmpapay);

    // AI pipeline elements
    cxt.elements.push_back(queue_ai);
    cxt.elements.push_back(videoconvert_ai);
    cxt.elements.push_back(jpegenc);
    cxt.elements.push_back(rtpjpeg);

    // RTSP elements
    cxt.elements.push_back(rtpsinkClient);
    cxt.elements.push_back(rtpsinkClient_ai);

    // Ratelimit elements
    cxt.elements.push_back(videorate_ai);
    return true;
}


static gboolean bus_call(GstBus * bus, GstMessage * msg, gpointer data) {
    GMainLoop * loop = (GMainLoop *) data;
    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR: {
            GError * error = nullptr;
            gchar * debug = nullptr;
            gst_message_parse_error(msg, &error, &debug);
            std::cerr << "err detected in gstreamer shutting down application: " << error->message << std::endl;
            g_error_free(error);
            g_free(debug);
            g_main_loop_quit (loop);
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError *err = nullptr;
            gchar *debug = nullptr;
            gst_message_parse_warning(msg, &err, &debug);
            std::cerr << "Warning: " << err->message << std::endl;
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS: {
            std::cout << "EOS received" << std::endl;
            g_main_loop_quit (loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}


int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    parse_args(argc,argv);
    // now have to set up gstreamer stuff here
    cxt.pipeline = gst_pipeline_new("pipeline");
    if (!cxt.pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return 1;
    }
    if (!setup_pipline(cxt)) {
        std::cerr << "Failed to setup pipeline" << std::endl;
        gst_object_unref(cxt.pipeline);
        return 1;
    }
    // todo set up gstreamer main loop and bus watching
    GMainLoop * loop = g_main_loop_new(NULL,FALSE);
    // Attach a bus to handle messages
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(cxt.pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);
    if (gst_element_set_state(cxt.pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING" << std::endl;
        gst_object_unref(cxt.pipeline);
        return 1;
    }
    std::cout << "Running pipeline..." << std::endl;
    g_main_loop_run(loop);
    // Clean shutdown
    std::cout << "Stopping pipeline..." << std::endl;
    gst_element_set_state(cxt.pipeline, GST_STATE_NULL);
    for (GstElement* elem : cxt.elements) {
        gst_object_unref(GST_OBJECT(elem));
    }
    cxt.elements.clear();
    gst_object_unref(cxt.pipeline);
    g_main_loop_unref(loop);
    return 0;
}
