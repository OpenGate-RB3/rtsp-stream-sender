#pragma once
#include<string>
namespace appContext{
    struct context{
        std::string ip_address;
        int video_port;
        int audio_port;
        int video_rtcp_port;
        int audio_rtcp_port;
        int video_rtcp_receive_port;
        int audio_rtcp_receive_port;
        int width;
        int height;
        std::string format;
        std::string ai_format;
        int ai_port; // if udp --> port to send over, will assume its over linklocal
        std::string ai_ip_address; // if udp the ip to send over
        // in future maybe add encoding option
        GstElement *pipeline;
        std::vector<GstElement *> elements;
    };
}