#include <ao/ao.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <signal.h>
#include <math.h>
#include "mongoose/mongoose.h"

static struct mg_serve_http_opts s_http_server_opts;

mpg123_handle *mh = NULL;
ao_device *device = NULL;
int stop = 0;
struct mg_mgr mgr;

size_t write_callback(char *delivered_data, size_t size, size_t nmemb, void *user_data) {
    int err;
    off_t frame_offset;
    unsigned char *audio;
    size_t done;
    ao_sample_format format;
    int channels, encoding;
    long rate;

    mpg123_feed(mh, delivered_data, size * nmemb);
    do {
        err = mpg123_decode_frame(mh, &frame_offset, &audio, &done);
        switch(err) {
            case MPG123_NEW_FORMAT:
                mpg123_getformat(mh, &rate, &channels, &encoding);
                format.bits = mpg123_encsize(encoding) * 8;
                format.rate = rate;
                format.channels = channels;
                format.byte_format = AO_FMT_NATIVE;
                format.matrix = 0;
                device = ao_open_live(ao_default_driver_id(), &format, NULL);
                break;
            case MPG123_OK:
                ao_play(device, audio, done);
                break;
            case MPG123_NEED_MORE:
                break;
            default:
                break;
        }
    } while(done > 0);

    return size * nmemb;
}

int progress_callback(void *client, double down_total, double down_now, double up_total, double up_now) {
    if(stop) {
        return 1;
    }

    return 0;
}

void play_stream(char *stream_uri) {
    CURL *ch;

    ch = curl_easy_init();
    if(ch) {
        curl_easy_setopt(ch, CURLOPT_URL, stream_uri);
        curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ch, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(ch, CURLOPT_NOPROGRESS, 0L); // make curl use the progress_callback


        mh = mpg123_new(NULL, NULL);
        if(mh) {

            mpg123_open_feed(mh);

            curl_easy_perform(ch);
            stop = 0;
            curl_easy_cleanup(ch);

            mpg123_close(mh);
            mpg123_delete(mh);

            ao_close(device);
        }

    }
}

static void handle_play(struct mg_connection *con, struct http_message *msg) {
    char stream_uri[200];
    mg_get_http_var(&msg->body, "streamURI", stream_uri, sizeof(stream_uri));
    mg_printf(con, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_send_http_chunk(con, "", 0);
    play_stream(stream_uri);
}

static void handle_stop(struct mg_connection *con, struct http_message *msg) {
    mg_printf(con, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_send_http_chunk(con, "", 0);
    stop = 1;
}

static void ev_handler(struct mg_connection *con, int ev, void *ev_data) {
    struct http_message *msg = (struct http_message *) ev_data;

    switch(ev) {
        case MG_EV_HTTP_REQUEST:
            if(mg_vcmp(&msg->uri, "/play") == 0) {
                handle_play(con, msg);
            }
            else if(mg_vcmp(&msg->uri, "/stop") == 0) {
                handle_stop(con, msg);
            }
            else {
                mg_serve_http(con, msg, s_http_server_opts);
            }
            break;
        default:
            break;
    }
}


void cleanup() {
    //if(mh) {
    //    ao_close(device);
    //}
    ao_shutdown();
    mpg123_exit();
}

void signal_handler(int s) {
    if(s == SIGINT) {
        stop = 1;
        //mg_mgr_free(&mgr);
        cleanup();
        exit(130);
    }
}



int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);

    mpg123_init();
    ao_initialize();

    if(strcmp("-s", argv[1]) == 0) {
        //struct mg_mgr mgr;
        struct mg_connection *con;

        mg_mgr_init(&mgr, NULL);
        con = mg_bind(&mgr, "8080", ev_handler);
        mg_set_protocol_http_websocket(con);
        mg_enable_multithreading(con);
        s_http_server_opts.document_root = "./static";

        for(;;) {
            mg_mgr_poll(&mgr, 1000);
        }

        mg_mgr_free(&mgr);
        cleanup();
    } else {
        play_stream(argv[1]);
        cleanup();
    }

    return 0;
}
