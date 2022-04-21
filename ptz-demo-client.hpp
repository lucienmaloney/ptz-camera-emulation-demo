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

struct PTZData {
  float pan;
  float tilt;
  float zoom;
  float panHome;
  float tiltHome;
  float zoomHome;

  PTZData(float p, float t, float z): pan{p}, panHome{p}, tilt{t}, tiltHome{t}, zoom{z}, zoomHome{z} {}
};

PTZData ptz {0, 0, 1};

#include "src/ptzTranslation.hpp"
#include "src/client-helpers.hpp"
