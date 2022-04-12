#include "ptz-demo-server.hpp"

int main(int argc, char* argv[]) {
  GMainLoop* loop = g_main_loop_new(NULL, false);
  std::cout << "hello world\n";
  GstRTSPServer* server = gst_rtsp_onvif_server_new();
  std::cout << server << std::endl;
  return 0;
}
