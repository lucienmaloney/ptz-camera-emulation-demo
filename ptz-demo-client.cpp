#include "ptz-demo-client.hpp"

int main(int argc, char* argv[]) {
  TCLAP::CmdLine cmd("PTZ Emulation Demo Client w/ RTSP+ONVIF", ' ', "0.1");
  
  TCLAP::UnlabeledValueArg<std::string> rtspUrl("url", "The address of a currently running rtsp server to stream from", true, "", "url");
  cmd.add(rtspUrl);

  TCLAP::ValueArg<float> panHome("p", "pan-angle", "The pan angle to start from and use as the home position", false, 0, "pan-angle");
  cmd.add(panHome);

  TCLAP::ValueArg<float> tiltHome("t", "tilt-angle", "The tilt angle to start from and use as the home position", false, 0, "tilt-angle");
  cmd.add(tiltHome);

  TCLAP::ValueArg<float> zoomHome("z", "zoom-level", "The zoom level to start from and use as the home position", false, 1, "zoom-level");
  cmd.add(zoomHome);

  cmd.parse(argc, argv);

  ptz.panHome = panHome.getValue();
  ptz.tiltHome = tiltHome.getValue();
  ptz.zoomHome = zoomHome.getValue();

  rtspAddress = rtspUrl.getValue();

  Context ctx {0, 0, 0, 0, 0, 0, 0, 0, 0};
  SeekParameters seek_params = { NULL, DEFAULT_SPEED, NULL, NULL, DEFAULT_REVERSE };

  ptz_initialize(ctx, seek_params, argc, argv);

  GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (ctx.pipe));
  gst_bus_add_watch (bus, (GstBusFunc) bus_message_cb, &ctx);

  gst_element_set_state (ctx.pipe, GST_STATE_PLAYING);

  ofs << "\nNew Client Connection: " << ctime(&currenttime) << std::endl;
  g_main_loop_run (ctx.loop);

  return 0;
}
