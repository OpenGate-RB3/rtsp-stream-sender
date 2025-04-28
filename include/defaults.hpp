#pragma once
#include <string>

// start with low quality image for now
const int DEFAULT_WIDTH = 1280;
const int DEFAULT_HEIGHT = 960;

// assume standard 30 frames a second for stream
const int DEFAULT_FRAMES = 30;

// assume stream default encoding to NV12
const std::string DEFAULT_ENCODING = "NV12";
const std::string DEFAULT_AI_ENCODING = "RGB";