static gboolean do_seek (Context * ctx);

struct PTZ {
  static float pan;
  static float tilt;
  static float zoom;
  static float panHome;
  static float tiltHome;
  static float zoomHome;

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

  static int ptzToSeconds(float p, float t, float z) {
    int panSecs = ((int)round(p / 2.5) + 6) * 10;
    int tiltSecs = ((int)round(t / 2.5) + 6) * 13 * 10;
    int zoomSecs = (int)round((z - 1) * 4) * 13 * 13 * 10;
    return panSecs + tiltSecs + zoomSecs;
  }
  
  static bool seekToPtz(Context* ctx, float p, float t, float z) {
    int start = ptzToSeconds(p, t, z);
    /* pan = p; */
    /* tilt = t; */
    /* zoom = z; */
    return seekToRange(ctx, start + 1, start + 10);
  }

  static std::vector<float> parsePTZ(gchar* arg) {
    std::vector<float> v {0, 0, 1};
    v[0] = atof(arg);
    int i = 1;
    while (*arg != '\0' && i < 3) {
      if (*arg == ',') {
        v[i] = atof(arg + 1);
        i++;
      }
      arg++;
    }
    return v;
  }

  static gboolean absoluteMove(Context * ctx, gchar * arg, gboolean * async) {
    auto ptz = parsePTZ(arg);
    *async = seekToPtz(ctx, ptz[0], ptz[1], ptz[2]);
    return *async;
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
    *async = seekToPtz(ctx, 0, 0, 1);
    return *async;
  };
  static gboolean setHome(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    return true;
  };
};

