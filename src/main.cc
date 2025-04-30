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
    program.add_argument("-port","-p").help("[port1] [port2] port 1 is video track, port 2 is audio track").nargs(2).default_value(std::vector<std::string>{"5000","5001"}).scan<'d',int>();
    program.add_argument("-w","-width").help("width of capture, default: "+std::to_string(DEFAULT_WIDTH)).default_value(std::to_string(DEFAULT_WIDTH)).scan<'d',int>();
    program.add_argument("-h","-height").help("height of capture, default: "+std::to_string(DEFAULT_HEIGHT)).default_value(std::to_string(DEFAULT_HEIGHT)).scan<'d',int>();
    program.add_argument("-raw","-raw_format").help("encoding of stream, default: NV12").default_value(DEFAULT_RAW_FORMAT);
    program.add_argument("-ai_raw").help("AI inference stream format, default: RGB").default_value(DEFAULT_AI_RAW_FORMAT);
    program.add_argument("-ai_udp").help("specify if you want gstreamer to dump video data over udp instead of appsink").default_value(false).implicit_value(true);
    program.add_argument("-ai_port").help("-ai_port [port] if -ai_udp is present a port needs to be provided");
    program.add_argument("-ai_ip").help("ipaddress of the udp socket to send over");
    program.add_epilog("The program ships with defaults for most of these paramater and their providing is only need if configuration of the system is non-default");
    try {
        program.parse_args(argc, argv);
        // load app context here
    }catch(const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        std::cerr << program << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    printf("hello world!\n");

    return 0;
}
