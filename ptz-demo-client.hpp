#include <iostream>
#include <cmath>
#include <gst/gst.h>
#include <gst/rtsp/rtsp.h>
#include <tclap/CmdLine.h>

std::string rtspAddress;

struct SeekParameters {
  gchar *range;
  gdouble speed;
  gchar *frames;
  gchar *rate_control;
  gboolean reverse;
};


struct Context {
  GstElement *src;
  GstElement *sink;
  GstElement *pipe;
  SeekParameters *seek_params;
  GMainLoop *loop;
  GIOChannel *io;
  gboolean new_range;
  guint io_watch_id;
  gboolean reset_sync;
};

#include "src/ptzTranslation.hpp"
#include "src/client-helpers.hpp"
