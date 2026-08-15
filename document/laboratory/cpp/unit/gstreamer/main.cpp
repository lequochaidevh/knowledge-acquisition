#include <gst/gst.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. Initialize GStreamer
    gst_init(&argc, &argv);

    // 2. Create the pipeline string targeting videotestsrc and waylandsink
    // We add a 'capsfilter' to force a standard 60 FPS 720p stream
    const gchar* pipeline_str =
        "videotestsrc pattern=smpte ! "
        "video/x-raw, width=1280, height=720, framerate=60/1 ! "
        "autovideosink";

    GError*     error    = nullptr;
    GstElement* pipeline = gst_parse_launch(pipeline_str, &error);

    if (!pipeline) {
        std::cerr << "Failed to parse pipeline string: " << error->message << std::endl;
        g_clear_error(&error);
        return -1;
    }

    // 3. Create a GLib main loop to handle window events and scaling
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

    // 4. Listen for errors or End-Of-Stream messages on the bus
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(
        bus,
        [](GstBus* b, GstMessage* msg, gpointer data) -> gboolean {
            GMainLoop* l = static_cast<GMainLoop*>(data);
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR: {
                    GError* err   = nullptr;
                    gchar*  debug = nullptr;
                    gst_message_parse_error(msg, &err, &debug);
                    std::cerr << "Pipeline Error: " << err->message << std::endl;
                    g_clear_error(&err);
                    g_free(debug);
                    g_main_loop_quit(l);
                    break;
                }
                case GST_MESSAGE_EOS:
                    std::cout << "End of stream reached." << std::endl;
                    g_main_loop_quit(l);
                    break;
                default:
                    break;
            }
            return TRUE;
        },
        loop);
    gst_object_unref(bus);

    // 5. Start playback
    std::cout << "Starting Wayland video test pattern window..." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Run the window loop
    g_main_loop_run(loop);

    // 6. Clean up resources on exit
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}