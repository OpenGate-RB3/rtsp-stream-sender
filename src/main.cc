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
    program.add_argument("ip").help("ip address of the rtsp server, required and not provided\n");
    program.add_argument("-port","-p").help("<port1> <port2> <port3> <port4> port 1 is video track, port 2 is audio track, port 3 video RTCP, port 4 audio RTCP: default is <5000> <5001> <5002> <5003> ").nargs(4).default_value(std::vector<std::string>{"5000","5001","5002","5003"}).scan<'d',int>();
    program.add_argument("-w","-width").help("width of capture, default: "+std::to_string(DEFAULT_WIDTH)).default_value(std::to_string(DEFAULT_WIDTH)).scan<'d',int>();
    program.add_argument("-h","-height").help("height of capture, default: "+std::to_string(DEFAULT_HEIGHT)).default_value(std::to_string(DEFAULT_HEIGHT)).scan<'d',int>();
    program.add_argument("-raw","-raw_format").help("encoding of stream, default: NV12").default_value(DEFAULT_RAW_FORMAT);
    program.add_argument("-ai_raw").help("AI inference stream format, default: RGB").default_value(DEFAULT_AI_RAW_FORMAT);
    program.add_argument("-ai_port").help("-ai_port <port> a port needs to be provided").required();
    program.add_argument("-ai_ip").help("ipaddress of the udp socket to send over").required();
    program.add_epilog("The program ships with defaults for most of these parameter and their providing is only need if configuration of the system is non-default");
    try {
        program.parse_args(argc, argv);
        // load app context here
        cxt.ip_address = program.get<std::string>("ip"); // this is required
        const auto ports = program.get<std::vector<int>>("-port");
        cxt.video_port = ports[0];
        cxt.audio_port = ports[1];
        cxt.video_rtcp_port = ports[2];
        cxt.audio_rtcp_port = ports[3];
        cxt.width = program.get<int>("-width");
        cxt.height = program.get<int>("-height");
        cxt.format = program.get<std::string>("-raw_format");
        cxt.ai_format = program.get<std::string>("-ai_raw");
        cxt.ai_ip_address = program.get<std::string>("-ai_ip");
        cxt.ai_port = program.get<int>("-ai_port");
    }catch(const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        std::cerr << program << std::endl;
        exit(1);
    }
}

bool setup_pipline(appContext::context & cxt) {
    // top level
    GstElement *qtiqmfsrc, *capsfilter, *tee, *capsfilter_rgb;
    // video pipeline
    GstElement *queue_stream, *v4l2h264enc, *h264parse, *rtph264pay, *udpsinkVideo; // video pipeline --> encode into h264 --> send to rtph264pay --> rtpbin --> udpsink
    // audio pipeline
    GstElement *pulsesrc, *audioconvert, *voaacenc, *rtpmp4apay, *udpsinkAudio; // audio pipeline --> audio src --> convert to mpeg4 --> rtp packetize --> rtpbin --> udpsink
    // ai pipeline
    GstElement *queue_ai, *videoconvert_ai, *ai_udpSink; // video src --> convert to rgb raw --> appsink || udpsink
    // RTSP  specif stuff
    GstElement *rtpbin;
    // RTCP protocol stuff
    GstElement *udpsink_rtcp_video, *udpsink_rtcp_audio;
    GstElement *udpsrc_rtcp_video, *udpsrc_rtcp_audio;
    //
    GstCaps *filterscap, *aiCap; // needed to define stream conversions during differnt steps

    // top level components
    qtiqmfsrc = gst_element_factory_make("qtiqmfsrc", "qtiqmmfsrc");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    tee = gst_element_factory_make("tee", "tee"); // similar to command line utility splits stream
    capsfilter_rgb = gst_element_factory_make("capsfilter", "capsfilterrgb");
    //queues
    queue_stream = gst_element_factory_make("queue", "queue");
    queue_ai = gst_element_factory_make("queue", "queueai");
    //video components
    v4l2h264enc = gst_element_factory_make("v4l2h264enc", "v4l2h264enc");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    rtph264pay = gst_element_factory_make("rtph264pay", "rtph264pay");
    // audio components
    pulsesrc = gst_element_factory_make("pulsesrc", "pulsesrc");
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    voaacenc = gst_element_factory_make("voaacenc", "voaacenc");
    rtpmp4apay = gst_element_factory_make("rtpmp4apay", "rtpmp4apay");

    // rtpbin
    rtpbin = gst_element_factory_make("rtpbin", "rtpbin");
    // udpsink rtsp
    udpsinkVideo = gst_element_factory_make("udpsink", "udpsink_video");
    udpsinkAudio = gst_element_factory_make("udpsink", "udpsink_audio");
    udpsink_rtcp_video = gst_element_factory_make("udpsink", "udpsink_rtcp_video");
    udpsink_rtcp_audio = gst_element_factory_make("udpsink", "udpsink_rtcp_audio");

    udpsrc_rtcp_video = gst_element_factory_make("udpsrc", "udpsrc_rtcp_video");
    udpsrc_rtcp_audio = gst_element_factory_make("udpsrc", "udpsrc_rtcp_audio");
    // ai sinks
    ai_udpSink = gst_element_factory_make("ai_udp", "ai_udp");
    videoconvert_ai = gst_element_factory_make("videoconvert", "videoconvert_ai");
    // check if components were correctly made
    if (!qtiqmfsrc
        || !capsfilter
        || !tee
        || !capsfilter_rgb
        ||!queue_stream
        || !queue_ai
        || !v4l2h264enc
        || !h264parse
        || !rtph264pay
        || !pulsesrc
        || !audioconvert
        || !videoconvert_ai
        || !voaacenc
        || !rtpmp4apay
        || !rtpbin
        || !udpsinkVideo
        || !udpsinkAudio
        || !udpsink_rtcp_video
        || !udpsink_rtcp_audio
        || !udpsrc_rtcp_video
        || !udpsrc_rtcp_audio
        || !ai_udpSink
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
        "framerate", GST_TYPE_FRACTION, 30, 1,
        "compression", G_TYPE_STRING, "ubwc",
        "interlace-mode", G_TYPE_STRING, "progressive",
        "colorimetry", G_TYPE_STRING, "bt601",
        NULL
        );
    aiCap = gst_caps_new_simple("video/x-raw","format", G_TYPE_STRING, cxt.ai_format.c_str(),NULL);
    // set caps filters for base stream
    g_object_set(G_OBJECT(capsfilter), "caps", filterscap, NULL);
    gst_caps_unref(filterscap);
    // set caps filter for ai stream, ie nv12 to rgb native
    g_object_set(G_OBJECT(capsfilter_rgb), "caps", aiCap, NULL);
    gst_caps_unref(aiCap);

    // Encoder configuration
    g_object_set(G_OBJECT(v4l2h264enc), "capture-io-mode",5,"output-io-mode",5, NULL);
    g_object_set(G_OBJECT(h264parse),"config-interval",-1,NULL);
    g_object_set(G_OBJECT(rtph264pay),"pt",96,NULL);
    g_object_set(G_OBJECT(rtpmp4apay),"pt",97,NULL);

    // udp set up
    g_object_set(G_OBJECT(udpsinkVideo),"host",cxt.ip_address.c_str(),"port",cxt.video_port,NULL);
    g_object_set(G_OBJECT(udpsinkAudio),"host",cxt.ip_address,"port",cxt.audio_port,NULL);

    // RTCP sinks
    g_object_set(G_OBJECT(udpsink_rtcp_video),"host",cxt.ip_address,"port",cxt.video_rtcp_port,NULL);
    g_object_set(G_OBJECT(udpsink_rtcp_audio),"host", cxt.ip_address,"port",cxt.audio_rtcp_port,NULL);

    // RTCP sources
    g_object_set(G_OBJECT(udpsrc_rtcp_video), "port", 5005, NULL);
    g_object_set(G_OBJECT(udpsink_rtcp_audio), "port", 5007, NULL);

    // Add to pipeline, this is huge block of items
    // this really does not matter, linking is what actually dictates how data flows
    gst_bin_add_many(GST_BIN(cxt.pipeline),
        qtiqmfsrc,capsfilter,tee,queue_stream,
        queue_ai, videoconvert_ai,voaacenc,
        capsfilter_rgb,v4l2h264enc,h264parse,
        rtph264pay,pulsesrc,audioconvert,
        rtpmp4apay,rtpbin,udpsinkVideo,udpsinkAudio,
        ai_udpSink, udpsink_rtcp_audio, udpsink_rtcp_video,
        udpsrc_rtcp_video, udpsrc_rtcp_audio, NULL
    );
    // start linking Elements together
    return true;
}

int main(int argc, char *argv[]) {
    parse_args(argc,argv);
    // now have to set up gstreamer stuff here
    cxt.pipeline = gst_pipeline_new("pipeline");
    if (!cxt.pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return 1;
    }
    return 0;
}
