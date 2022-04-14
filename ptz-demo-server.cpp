#include "ptz-demo-server.hpp"

int main(int argc, char* argv[]) {
  TCLAP::CmdLine cmd("PTZ Emulation Demo w/ RTSP+ONVIF", ' ', "0.1");

  TCLAP::UnlabeledValueArg<std::string> segmentedFile("filename", "The segmented video file to stream from", false, "segmented.mp4", "filename");
  cmd.add(segmentedFile);

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
  filename = segmentedFile.getValue();

  GMainLoop* loop = g_main_loop_new(NULL, false);
  GstRTSPServer* server = gst_rtsp_onvif_server_new();
  GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points (server);
  GstRTSPMediaFactory* factory = onvif_factory_new();
  gst_rtsp_media_factory_set_media_gtype (factory, GST_TYPE_RTSP_ONVIF_MEDIA);
  gst_rtsp_mount_points_add_factory (mounts, "/", factory);
  gst_rtsp_server_attach (server, NULL);

  std::cout << "Demo server running at rtsp://127.0.0.1:" << gst_rtsp_server_get_service (server) << std::endl;
  g_main_loop_run(loop);

  return 0;
}
