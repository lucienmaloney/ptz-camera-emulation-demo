struct PTZ {
  float pan;
  float tilt;
  float zoom;
  float panHome;
  float tiltHome;
  float zoomHome;

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
    *async = false;
    return true;
  };
  static gboolean setHome(Context * ctx, gchar * arg, gboolean * async) {;
    *async = false;
    return true;
  };
};

