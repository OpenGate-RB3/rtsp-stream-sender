#include <stdio.h>
#include <defaults.hpp>
#include <gst/gst.h>
#include <iostream>
#include <argparse.hpp>
#include <info.hpp>

int main() {
    printf("hello world!\n");
    argparse::ArgumentParser program("gst-open-gate-rtsp-sender",version);
    program.add_description("utility for sending video and audio data over udp to an rtsp endpoint\n");
    program.add_argument("-w","-width").help("width of capture, default: 1280").scan<'d',int>();
    program.add_argument("-h","-height").help("height of capture, default: 960").scan<'d',int>();
    program.add_argument("-enc","-encode").help("encoding of stream, default: NV12");
    program.add_argument("-ai_enc").help("AI inference stream encoding, default: rgb");
    program.add_argument("-port","-p").help("two ports one being the video track port and the other being the audio track").nargs(2).scan<'d',int>();
    return 0;
}
