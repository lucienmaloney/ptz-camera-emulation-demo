static gboolean do_seek (Context * ctx, ulong nanooffset);

struct PTZ {
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

    auto timeend = std::chrono::high_resolution_clock::now();
    ulong elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(timeend - timestart).count();
    begun = TRUE;

    return do_seek(ctx, elapsed % 10000000000);
  }

  static int ptzToSeconds(float p, float t, float z) {
    int panSecs = ((int)round(p / 2.5) + 6) * 10;
    int tiltSecs = ((int)round(t / 2.5) + 6) * 13 * 10;
    int zoomSecs = (int)round((z - 1) * 4) * 13 * 13 * 10;
    return panSecs + tiltSecs + zoomSecs;
  }
  
  static bool seekToPtz(Context* ctx, float p, float t, float z) {
    int start = ptzToSeconds(p, t, z);
    ptz.pan = p;
    ptz.tilt = t;
    ptz.zoom = z;
    return seekToRange(ctx, start, start + 10);
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

  static gboolean updatePan(Context* ctx, gchar* arg, gboolean* async) {
    *async = seekToPtz(ctx, atof(arg), ptz.tilt, ptz.zoom);
    return *async;
  }

  static gboolean updateTilt(Context* ctx, gchar* arg, gboolean* async) {
    *async = seekToPtz(ctx, ptz.pan, atof(arg), ptz.zoom);
    return *async;
  }

  static gboolean updateZoom(Context* ctx, gchar* arg, gboolean* async) {
    *async = seekToPtz(ctx, ptz.pan, ptz.tilt, atof(arg));
    return *async;
  }


  static gboolean absoluteMove(Context * ctx, gchar * arg, gboolean * async) {
    auto ptz = parsePTZ(arg);
    *async = seekToPtz(ctx, ptz[0], ptz[1], ptz[2]);
    return *async;
  };

  static gboolean relativeMove(Context * ctx, gchar * arg, gboolean * async) {;
    auto newptz = parsePTZ(arg);
    ptz.pan += newptz[0];
    ptz.tilt += newptz[1];
    ptz.zoom += newptz[2];
    *async = seekToPtz(ctx, ptz.pan, ptz.tilt, ptz.zoom);
    return *async;
  };

  static gboolean goToHome(Context * ctx, gchar * arg, gboolean * async) {;
    *async = seekToPtz(ctx, ptz.panHome, ptz.tiltHome, ptz.zoomHome);
    return *async;
  };

  static gboolean setHome(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    auto newptz = parsePTZ(arg);
    ptz.panHome = newptz[0];
    ptz.tiltHome = newptz[1];
    ptz.zoomHome = newptz[2];
    return true;
  };
};
