#pragma once
#include <string>

// start with low quality image for now
const int DEFAULT_WIDTH = 1280;
const int DEFAULT_HEIGHT = 960;

// assume standard 30 frames a second for stream
const int DEFAULT_FRAMES = 30;

const int DEFAULT_JPEG_RATE = 10;

// assume stream default encoding to NV12 and assume AI is feed raw RBG frames no encoding
const std::string DEFAULT_RAW_FORMAT = "NV12";
const std::string DEFAULT_AI_RAW_FORMAT = "RGB";
const std::string DEFAULT_ENCODING = "v4l2h264enc";