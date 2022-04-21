static gboolean cmd_help (Context * ctx, gchar * arg, gboolean * async);
static gboolean cmd_pause (Context * ctx, gchar * arg, gboolean * async);
static gboolean cmd_play (Context * ctx, gchar * arg, gboolean * async);
static gboolean cmd_range (Context * ctx, gchar * arg, gboolean * async);

struct Command {
  const gchar *name;
  gboolean has_argument;
  const gchar *help;
  gboolean (*func) (Context * ctx, gchar * arg, gboolean * async);
};

static Command commands[] = {
  {"help", FALSE, "Display list of valid commands", cmd_help},
  {"pause", FALSE, "Pause playback", cmd_pause},
  {"play", FALSE, "Resume playback", cmd_play},
  {"absoluteMove", TRUE, "Set the absolute PTZ position", PTZ::absoluteMove},
  {"relativeMove", TRUE, "Set the relative PTZ position", PTZ::relativeMove},
  {"goToHome", FALSE, "Move to the PTZ home position", PTZ::goToHome},
  {"setHome", TRUE, "Update the PTZ home position", PTZ::setHome},
  {"p", TRUE, "Update the PTZ pan position", PTZ::updatePan},
  {"t", TRUE, "Update the PTZ tilt position", PTZ::updateTilt},
  {"z", TRUE, "Update the PTZ zoom position", PTZ::updateZoom},
  {NULL},
};

#define MAKE_AND_ADD(var, pipe, name, label, elem_name) \
G_STMT_START { \
  if (G_UNLIKELY (!(var = (gst_element_factory_make (name, elem_name))))) { \
    GST_ERROR ("Could not create element %s", name); \
    goto label; \
  } \
  if (G_UNLIKELY (!gst_bin_add (GST_BIN_CAST (pipe), var))) { \
    GST_ERROR ("Could not add element %s", name); \
    goto label; \
  } \
} G_STMT_END

#define DEFAULT_RANGE "19000101T000000Z-19000101T000200Z"
#define DEFAULT_SPEED 1.0
#define DEFAULT_FRAMES "none"
#define DEFAULT_RATE_CONTROL "yes"
#define DEFAULT_REVERSE FALSE

static void
pad_added_cb (GstElement * src, GstPad * srcpad, GstElement * peer)
{
  GstPad *sinkpad = gst_element_get_static_pad (peer, "sink");

  gst_pad_link (srcpad, sinkpad);

  gst_object_unref (sinkpad);
}

static gboolean
setup (Context * ctx)
{
  GstElement *onvifparse, *queue, *vdepay, *vdec, *vconv, *toverlay, *tee,
      *vqueue;
  gboolean ret = FALSE;

  MAKE_AND_ADD (ctx->src, ctx->pipe, "rtspsrc", done, NULL);
  MAKE_AND_ADD (queue, ctx->pipe, "queue", done, NULL);
  MAKE_AND_ADD (onvifparse, ctx->pipe, "rtponvifparse", done, NULL);
  MAKE_AND_ADD (vdepay, ctx->pipe, "rtph264depay", done, NULL);
  MAKE_AND_ADD (vdec, ctx->pipe, "avdec_h264", done, NULL);
  MAKE_AND_ADD (vconv, ctx->pipe, "videoconvert", done, NULL);
  MAKE_AND_ADD (toverlay, ctx->pipe, "timeoverlay", done, NULL);
  MAKE_AND_ADD (tee, ctx->pipe, "tee", done, NULL);
  MAKE_AND_ADD (vqueue, ctx->pipe, "queue", done, NULL);
  MAKE_AND_ADD (ctx->sink, ctx->pipe, "xvimagesink", done, NULL);

  g_object_set (ctx->src, "location", rtspAddress.c_str(), NULL);
  g_object_set (ctx->src, "onvif-mode", TRUE, NULL);
  g_object_set (ctx->src, "tcp-timeout", 0, NULL);
  g_object_set (toverlay, "show-times-as-dates", TRUE, NULL);

  g_object_set (toverlay, "datetime-format", "%a %d, %b %Y - %T", NULL);

  g_signal_connect (ctx->src, "pad-added", G_CALLBACK (pad_added_cb), queue);

  if (!gst_element_link_many (queue, onvifparse, vdepay, vdec, vconv, toverlay,
          tee, vqueue, ctx->sink, NULL)) {
    goto done;
  }

  g_object_set (ctx->src, "onvif-rate-control", FALSE, "is-live", FALSE, NULL);

  if (!g_strcmp0 (ctx->seek_params->rate_control, "no")) {
    g_object_set (ctx->sink, "sync", FALSE, NULL);
  }

  ret = TRUE;

done:
  return ret;
}

static GstClockTime
get_current_position (Context * ctx, gboolean reverse)
{
  GstSample *sample;
  GstBuffer *buffer;
  GstClockTime ret;

  g_object_get (ctx->sink, "last-sample", &sample, NULL);

  buffer = gst_sample_get_buffer (sample);

  ret = GST_BUFFER_PTS (buffer);

  if (reverse && GST_CLOCK_TIME_IS_VALID (GST_BUFFER_DURATION (buffer)))
    ret += GST_BUFFER_DURATION (buffer);

  gst_sample_unref (sample);

  return ret;
}

static GstEvent *
translate_seek_parameters (Context * ctx, SeekParameters * seek_params, ulong nanooffset = 0)
{
  GstEvent *ret = NULL;
  gchar *range_str = NULL;
  GstRTSPTimeRange *rtsp_range;
  GstSeekType start_type, stop_type;
  GstClockTime start, stop;
  gdouble rate;
  GstSeekFlags flags;
  gchar **split = NULL;
  GstClockTime trickmode_interval = 0;

  range_str = g_strdup_printf ("clock=%s", seek_params->range);

  if (gst_rtsp_range_parse (range_str, &rtsp_range) != GST_RTSP_OK) {
    GST_ERROR ("Failed to parse range %s", range_str);
    goto done;
  }

  gst_rtsp_range_get_times (rtsp_range, &start, &stop);

  if (start > stop) {
    GST_ERROR ("Invalid range, start > stop: %s", seek_params->range);
    goto done;
  }

  start = (GstClockTime)(start + nanooffset);
  stop = (GstClockTime)(stop + 50000000);

  start_type = GST_SEEK_TYPE_SET;
  stop_type = GST_SEEK_TYPE_SET;

  if (!ctx->new_range) {
    GstClockTime current_position =
        get_current_position (ctx, seek_params->reverse);

    if (seek_params->reverse) {
      stop_type = GST_SEEK_TYPE_SET;
      stop = current_position;
    } else {
      start_type = GST_SEEK_TYPE_SET;
      start = current_position;
    }
  }

  ctx->new_range = FALSE;

  flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

  split = g_strsplit (seek_params->frames, "/", 2);

  if (!g_strcmp0 (split[0], "intra")) {
    if (split[1]) {
      guint64 interval;
      gchar *end;

      interval = g_ascii_strtoull (split[1], &end, 10);

      if (!end || *end != '\0') {
        GST_ERROR ("Unexpected interval value %s", split[1]);
        goto done;
      }

      trickmode_interval = interval * GST_MSECOND;
    }
    flags = (GstSeekFlags)(flags | GST_SEEK_FLAG_TRICKMODE_KEY_UNITS);
  } else if (!g_strcmp0 (split[0], "predicted")) {
    if (split[1]) {
      GST_ERROR ("Predicted frames mode does not allow an interval (%s)",
          seek_params->frames);
      goto done;
    }
    flags = (GstSeekFlags)(flags | GST_SEEK_FLAG_TRICKMODE_FORWARD_PREDICTED);
  } else if (g_strcmp0 (split[0], "none")) {
    GST_ERROR ("Invalid frames mode (%s)", seek_params->frames);
    goto done;
  }

  if (seek_params->reverse) {
    rate = -1.0 * seek_params->speed;
  } else {
    rate = 1.0 * seek_params->speed;
  }

  ret = gst_event_new_seek (rate, GST_FORMAT_TIME, flags,
      start_type, start, stop_type, stop);

  if (trickmode_interval)
    gst_event_set_seek_trickmode_interval (ret, trickmode_interval);

done:
  if (split)
    g_strfreev (split);
  g_free (range_str);
  return ret;
}

static void prompt_on (Context * ctx);
static void prompt_off (Context * ctx);

static gboolean
cmd_help (Context * ctx, gchar * arg, gboolean * async)
{
  gboolean ret = TRUE;
  guint i;

  *async = FALSE;

  for (i = 0; commands[i].name; i++) {
    g_print ("%s: %s\n", commands[i].name, commands[i].help);
  }

  return ret;
}

static gboolean
cmd_pause (Context * ctx, gchar * arg, gboolean * async)
{
  gboolean ret;
  GstStateChangeReturn state_ret;

  g_print ("Pausing\n");

  state_ret = gst_element_set_state (ctx->pipe, GST_STATE_PAUSED);

  *async = state_ret == GST_STATE_CHANGE_ASYNC;
  ret = state_ret != GST_STATE_CHANGE_FAILURE;

  return ret;
}

static gboolean
cmd_play (Context * ctx, gchar * arg, gboolean * async)
{
  gboolean ret;
  GstStateChangeReturn state_ret;

  g_print ("Playing\n");

  state_ret = gst_element_set_state (ctx->pipe, GST_STATE_PLAYING);

  *async = state_ret == GST_STATE_CHANGE_ASYNC;
  ret = state_ret != GST_STATE_CHANGE_FAILURE;

  return ret;
}

static gboolean
do_seek (Context * ctx, ulong nanooffset = 0)
{
  gboolean ret = FALSE;
  GstEvent *event;

  if (!(event = translate_seek_parameters (ctx, ctx->seek_params, nanooffset))) {
    GST_ERROR ("Failed to create seek event");
    goto done;
  }

  if (ctx->seek_params->reverse)
    g_object_set (ctx->src, "onvif-rate-control", FALSE, NULL);

  if (ctx->reset_sync) {
    g_object_set (ctx->sink, "sync", TRUE, NULL);
    ctx->reset_sync = FALSE;
  }

  if (!gst_element_send_event (ctx->src, event)) {
    GST_ERROR ("Failed to seek rtspsrc");
    g_main_loop_quit (ctx->loop);
    goto done;
  }

  ret = TRUE;

done:
  return ret;
}

static gboolean
cmd_range (Context * ctx, gchar * arg, gboolean * async)
{
  gboolean ret = TRUE;

  g_print ("Switching to new range\n");

  g_free (ctx->seek_params->range);
  ctx->seek_params->range = g_strdup (arg);
  ctx->new_range = TRUE;

  ret = do_seek (ctx);

  *async = ret == TRUE;

  return ret;
}

static void
handle_command (Context * ctx, gchar * cmd)
{
  gchar **split;
  guint i;
  gboolean valid_command = FALSE;

  split = g_strsplit (cmd, ":", 0);

  ofs << cmd << std::endl;

  cmd = g_strstrip (split[0]);

  if (cmd == NULL || *cmd == '\0') {
    g_print ("> ");
    goto done;
  }

  for (i = 0; commands[i].name; i++) {
    if (!g_strcmp0 (commands[i].name, cmd)) {
      valid_command = TRUE;
      if (commands[i].has_argument && g_strv_length (split) != 2) {
        g_print ("Command %s expects exactly one argument:\n%s: %s\n", cmd,
            commands[i].name, commands[i].help);
      } else if (!commands[i].has_argument && g_strv_length (split) != 1) {
        g_print ("Command %s expects no argument:\n%s: %s\n", cmd,
            commands[i].name, commands[i].help);
      } else {
        gboolean async = FALSE;

        if (commands[i].func (ctx,
                commands[i].has_argument ? g_strstrip (split[1]) : NULL, &async)
            && async)
          prompt_off (ctx);
        else
          g_print ("> ");
      }
      break;
    }
  }

  if (!valid_command) {
    g_print ("Invalid command %s\n> ", cmd);
  }

done:
  g_strfreev (split);
}

static gboolean
io_callback (GIOChannel * io, GIOCondition condition, Context * ctx)
{
  gboolean ret = TRUE;
  gchar *str;
  GError *error = NULL;

  switch (condition) {
    case G_IO_PRI:
    case G_IO_IN:
      switch (g_io_channel_read_line (io, &str, NULL, NULL, &error)) {
        case G_IO_STATUS_ERROR:
          GST_ERROR ("Failed to read commands from stdin: %s", error->message);
          g_clear_error (&error);
          g_main_loop_quit (ctx->loop);
          break;
        case G_IO_STATUS_EOF:
          g_print ("EOF received, bye\n");
          g_main_loop_quit (ctx->loop);
          break;
        case G_IO_STATUS_AGAIN:
          break;
        case G_IO_STATUS_NORMAL:
          handle_command (ctx, str);
          g_free (str);
          break;
      }
      break;
    case G_IO_ERR:
    case G_IO_HUP:
      GST_ERROR ("Failed to read commands from stdin");
      g_main_loop_quit (ctx->loop);
      break;
    case G_IO_OUT:
    default:
      break;
  }

  return ret;
}

#ifndef STDIN_FILENO
#ifdef G_OS_WIN32
#define STDIN_FILENO _fileno(stdin)
#else /* !G_OS_WIN32 */
#define STDIN_FILENO 0
#endif /* G_OS_WIN32 */
#endif /* STDIN_FILENO */

static void
prompt_on (Context * ctx)
{
  g_assert (!ctx->io);
  ctx->io = g_io_channel_unix_new (STDIN_FILENO);
  ctx->io_watch_id =
      g_io_add_watch (ctx->io, G_IO_IN, (GIOFunc) io_callback, ctx);
  g_print ("> ");
}

static void
prompt_off (Context * ctx)
{
  g_assert (ctx->io);
  g_source_remove (ctx->io_watch_id);
  g_io_channel_unref (ctx->io);
  ctx->io = NULL;
}

bool looped = false;

static gboolean
bus_message_cb (GstBus * bus, GstMessage * message, Context * ctx)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:{
      GstState olds, news, pendings;

      if (GST_MESSAGE_SRC (message) == GST_OBJECT (ctx->pipe)) {
        gst_message_parse_state_changed (message, &olds, &news, &pendings);
        GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (ctx->pipe),
            GST_DEBUG_GRAPH_SHOW_ALL, "playing");
      }
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_error (message, &error, &debug);

      gst_printerr ("Error: %s (%s)\n", error->message, debug);
      g_clear_error (&error);
      g_free (debug);
      g_main_loop_quit (ctx->loop);
      break;
    }
    case GST_MESSAGE_LATENCY:{
      gst_bin_recalculate_latency (GST_BIN (ctx->pipe));
      break;
    }
    case GST_MESSAGE_ASYNC_DONE:{
      if (!looped) prompt_on (ctx);
      if (looped) looped = false;
      break;
    }
    case 1:
      if (begun) {
        ctx->new_range = TRUE;
        do_seek(ctx, 65000000);
        looped = true;
      }
      break;
    default:
      break;
  }

  return TRUE;
}

void ptz_initialize(Context& ctx, SeekParameters& seek_params, int argc, char* argv[]) {
  GError *error = NULL;
  GOptionEntry entries[] = { {NULL} };
  GOptionContext *optctx;
  optctx = g_option_context_new ("");
  g_option_context_add_main_entries (optctx, entries, NULL);
  g_option_context_add_group (optctx, gst_init_get_option_group ());
  g_option_context_parse (optctx, 0, 0, &error);
  g_option_context_free (optctx);

  seek_params.range = g_strdup(DEFAULT_RANGE);
  seek_params.frames = g_strdup(DEFAULT_FRAMES);
  seek_params.rate_control = g_strdup(DEFAULT_RATE_CONTROL);

  ctx.seek_params = &seek_params;
  ctx.new_range = TRUE;
  ctx.reset_sync = FALSE;

  ctx.pipe = gst_pipeline_new (NULL);
  setup(&ctx);

  PTZ::seekToPtz(&ctx, ptz.panHome, ptz.tiltHome, ptz.zoomHome);

  ctx.loop = g_main_loop_new (NULL, FALSE);

}
