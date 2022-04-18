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

  rtspAddress = rtspUrl.getValue();

  GOptionContext *optctx;
  Context ctx = { 0, };
  GstBus *bus;
  GError *error = NULL;
  const gchar *range = NULL;
  const gchar *frames = NULL;
  const gchar *rate_control = NULL;
  /* gchar *default_speed = */
  /*     g_strdup_printf ("Speed to request (default: %.1f)", DEFAULT_SPEED); */
  SeekParameters seek_params =
      { NULL, DEFAULT_SPEED, NULL, NULL, DEFAULT_REVERSE };
  GOptionEntry entries[] = {
    /* {"range", 0, 0, G_OPTION_ARG_STRING, &range, */
    /*     "Range to seek (default: " DEFAULT_RANGE ")", "RANGE"}, */
    /* {"speed", 0, 0, G_OPTION_ARG_DOUBLE, &seek_params.speed, */
    /*     default_speed, "SPEED"}, */
    /* {"frames", 0, 0, G_OPTION_ARG_STRING, &frames, */
    /*     "Frames to request (default: " DEFAULT_FRAMES ")", "FRAMES"}, */
    /* {"rate-control", 0, 0, G_OPTION_ARG_STRING, &rate_control, */
    /*     "Apply rate control on the client side (default: " */
    /*       DEFAULT_RATE_CONTROL ")", "RATE_CONTROL"}, */
    /* {"reverse", 0, 0, G_OPTION_ARG_NONE, &seek_params.reverse, */
    /*     "Playback direction", ""}, */
    {NULL}
  };

  optctx = g_option_context_new ("<rtsp-url> - ONVIF RTSP Client");
  g_option_context_add_main_entries (optctx, entries, NULL);
  g_option_context_add_group (optctx, gst_init_get_option_group ());
  g_option_context_parse (optctx, &argc, &argv, &error);
  g_option_context_free (optctx);

  seek_params.range = g_strdup (range ? range : DEFAULT_RANGE);
  seek_params.frames = g_strdup (frames ? frames : DEFAULT_FRAMES);
  seek_params.rate_control =
      g_strdup (rate_control ? rate_control : DEFAULT_RATE_CONTROL);

  ctx.seek_params = &seek_params;
  ctx.new_range = TRUE;
  ctx.reset_sync = FALSE;

  ctx.pipe = gst_pipeline_new (NULL);
  setup(&ctx);
  do_seek (&ctx);

  ctx.loop = g_main_loop_new (NULL, FALSE);

  bus = gst_pipeline_get_bus (GST_PIPELINE (ctx.pipe));
  gst_bus_add_watch (bus, (GstBusFunc) bus_message_cb, &ctx);

  gst_element_set_state (ctx.pipe, GST_STATE_PLAYING);

  g_main_loop_run (ctx.loop);

  return 0;
}
