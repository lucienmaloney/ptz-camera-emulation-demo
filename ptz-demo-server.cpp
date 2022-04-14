#include "ptz-demo-server.hpp"

int main(int argc, char* argv[]) {
  TCLAP::CmdLine cmd("PTZ Emulation Demo w/ RTSP+ONVIF", ' ', "0.1");

  TCLAP::UnlabeledValueArg<std::string> filename("filename", "The segmented video file to stream from", false, "segmented.mp4", "filename");
  cmd.add(filename);

  TCLAP::ValueArg<float> segmentLength("l", "segment-length", "The length of one segment of the overall video in seconds", false, 10, "segment-length");
  cmd.add(segmentLength);

  TCLAP::ValueArg<int> panAngles("p", "pan-angles", "The number of angles that panning is allowed from", false, 13, "pan-angles");
  cmd.add(panAngles);

  TCLAP::ValueArg<float> panDelta("x", "pan-delta", "The delta in degrees between pan angles", false, 2.5, "pan-delta");
  cmd.add(panDelta);

  TCLAP::ValueArg<int> tiltAngles("t", "tilt-angles", "The number of angles that tilting is allowed from", false, 13, "tilt-angles");
  cmd.add(tiltAngles);

  TCLAP::ValueArg<float> tiltDelta("y", "tilt-delta", "The delta in degrees between tilt angles", false, 2.5, "tilt-delta");
  cmd.add(tiltDelta);

  TCLAP::ValueArg<int> zoomLevels("z", "zoom-angles", "The number of zoom levels", false, 5, "zoom-angles");
  cmd.add(zoomLevels);

  TCLAP::ValueArg<float> zoomDelta("d", "zoom-delta", "The delta between zoom levels", false, 0.25, "zoom-delta");
  cmd.add(zoomDelta);

  cmd.parse(argc, argv);

  GMainLoop* loop = g_main_loop_new(NULL, false);
  std::cout << "hello world\n";
  GstRTSPServer* server = gst_rtsp_onvif_server_new();
  std::cout << server << std::endl;
  return 0;
}
