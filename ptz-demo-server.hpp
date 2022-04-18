#include <iostream>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <tclap/CmdLine.h>

std::string filename;

#include "src/server-helpers.hpp"

void handleArgs(TCLAP::CmdLine& cmd, int argc, char* argv[]);
GMainLoop* initializeLoop();

