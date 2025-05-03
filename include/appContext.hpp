#pragma once
#include<string>
namespace appContext{
    struct context{
        std::string url_location;
        int width;
        int height;
        std::string format;
        std::string ai_format;
        std::string ai_location; // rtsp server location for sending the jpeg images over
        // in future maybe add encoding option
        GstElement *pipeline;
        std::vector<GstElement *> elements;
    };
}