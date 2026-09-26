#include <gst/gst.h>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    if (argc < 2) {
        g_printerr("Usage: %s <port1> [port2] [port3] ...\n", argv[0]);
        return -1;
    }

    GString *pipeline_str = g_string_new(
        "v4l2src device=/dev/video0 ! "
        "image/jpeg,width=640,height=480 ! "
        "jpegparse ! rtpjpegpay ! tee name=split_stream ");

    // bind udpsink with multi port
    for (int i = 1; i < argc; i++) {
        g_string_append_printf(
            pipeline_str,
            "split_stream. ! queue max-size-buffers=1 "
            "leaky=2 ! udpsink host=127.0.0.1 port=%s sync=false ",
            argv[i]);

        g_print("Streaming to %s at port %s ...\n", "127.0.0.1", argv[i]);
    }

    GError *    error    = NULL;
    GstElement *pipeline = gst_parse_launch(pipeline_str->str, &error);
    if (!pipeline) {
        g_printerr("Pipeline error: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
