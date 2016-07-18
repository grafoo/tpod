#include "../mongoose/mongoose.h"
#include <ao/ao.h>
#include <curl/curl.h>
#include <math.h>
#include <mpg123.h>
#include <mrss.h>
#include <signal.h>

#define TPOD_MODE_SRV 0
#define TPOD_MODE_CLI 1

static struct mg_serve_http_opts s_http_server_opts;

mpg123_handle *mh = NULL;
ao_device *device = NULL;

int playback_stop = 0;
int srv = 1; // keep mongoose event loop running
int mode = TPOD_MODE_SRV;

size_t write_callback(char *delivered_data, size_t size, size_t nmemb, void *user_data) {
    int err;
    off_t frame_offset;
    unsigned char *audio;
    size_t done = 1;
    ao_sample_format format;
    int channels, encoding;
    long rate;

    mpg123_feed(mh, delivered_data, size * nmemb);
    while(done > 0) {
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
            default:
                break;
        }
    }

    return size * nmemb;
}

int progress_callback(void *client, double down_total, double down_now, double up_total, double up_now) {
    if(playback_stop) {
        return 1;
    }

    return 0;
}

void play_stream(char *stream_uri) {
    CURL *ch = NULL;

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
            curl_easy_cleanup(ch);

            mpg123_close(mh);
            mpg123_delete(mh);

            ao_close(device);

            playback_stop = 0;
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
    playback_stop = 1;
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
    playback_stop = 1;
    ao_shutdown();
    mpg123_exit();
}

void signal_handler(int s) {
    if(s == SIGINT) {
        switch(mode) {
            case TPOD_MODE_SRV:
                srv = 0;
                break;
            case TPOD_MODE_CLI:
                playback_stop = 1;
                cleanup();
                exit(130);
                break;
            default:
                break;
        }
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);

    mpg123_init();
    ao_initialize();

    if(strcmp("-s", argv[1]) == 0) {
        struct mg_mgr mgr;
        struct mg_connection *con;

        mg_mgr_init(&mgr, NULL);
        con = mg_bind(&mgr, "8080", ev_handler);
        mg_set_protocol_http_websocket(con);
        mg_enable_multithreading(con);
        s_http_server_opts.document_root = "./static";

        while(srv) {
            mg_mgr_poll(&mgr, 1000);
        }

        mg_mgr_free(&mgr);
        cleanup();

        exit(130);
    }
    else if(strcmp("-p", argv[1]) == 0) {
        mrss_t *feed;
        mrss_item_t *episode;
        mrss_tag_t *other_tags;
        mrss_parse_url_with_options_and_error(argv[2], &feed, NULL, NULL);
        printf("%s\n", feed->title);
        printf("%s\n", feed->link);
        printf("%s\n", feed->description);
        printf("%s\n", feed->pubDate);
        printf("%s\n", feed->lastBuildDate);
        episode = feed->item;
        while(episode) {
            printf("%s\n", episode->title);
            printf("%s\n", episode->link);
            printf("%s\n", episode->description);
            printf("%s\n", episode->enclosure_url);
            if(episode->other_tags){
                other_tags = episode->other_tags;
                while(other_tags){
                    if(strcmp(other_tags->name, "duration") == 0) {
                        printf("%s\n", other_tags->value);
                    }
                    other_tags = other_tags->next;
                }
            }
            episode = episode->next;
        }
        mrss_free(feed);
    }
    else {
        mode = 1;
        play_stream(argv[1]);
        cleanup();
    }

    return 0;
}
