static gboolean do_seek (Context * ctx);

struct PTZ {
  float pan;
  float tilt;
  float zoom;
  float panHome;
  float tiltHome;
  float zoomHome;

  static std::string numToString(int x) {
    if (x == 0) {
      return "00";
    }
    std::string s = std::to_string(x);
    if (x < 10) {
      return std::string{'0'} + s;
    }
    return s;
  }

  static std::string timeToStringTime(int x) {
    int hours = x / 3600;
    int minutes = (x / 60) % 60;
    int seconds = x % 60;
    return numToString(hours) + numToString(minutes) + numToString(seconds) + "Z";
  }

  static bool seekToRange(Context* ctx, int start, int stop) {
    std::string base = "19000101T";
    std::string ptzRange = base + timeToStringTime(start) + '-' + base + timeToStringTime(stop);
    g_free (ctx->seek_params->range);
    ctx->seek_params->range = g_strdup(ptzRange.data());
    ctx->new_range = TRUE;

    return do_seek(ctx);
  }

  static gboolean absoluteMove(Context * ctx, gchar * arg, gboolean * async) {
    *async = false;
    return true;
  };
  static gboolean relativeMove(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    return true;
  };
  static gboolean continuousMove(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    return true;
  };
  static gboolean stop(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    return true;
  };
  static gboolean goToHome(Context * ctx, gchar * arg, gboolean * async) {;
    *async = seekToRange(ctx, 0, 10);
    return *async;
  };
  static gboolean setHome(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    return true;
  };
};

