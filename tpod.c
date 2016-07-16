#include <ao/ao.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <signal.h>
#include <math.h>

mpg123_handle *mh = NULL;
ao_device *device = NULL;
int stop = 0;

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

void signal_handler(int s) {
    if(s == SIGINT) {
        stop = 1;
    }
}

int main(int argc, char **argv) {
    char *url = argv[1];

    signal(SIGINT, signal_handler);

    CURL *ch;

    ch = curl_easy_init();
    if(ch) {
        curl_easy_setopt(ch, CURLOPT_URL, url);
        curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ch, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(ch, CURLOPT_NOPROGRESS, 0L); // make curl use the progress_callback

        mpg123_init();

        mh = mpg123_new(NULL, NULL);
        if(mh) {
            ao_initialize();

            mpg123_open_feed(mh);

            curl_easy_perform(ch);
            curl_easy_cleanup(ch);

            mpg123_close(mh);
            mpg123_delete(mh);

            ao_close(device);
            ao_shutdown();
        }

        mpg123_exit();
    }

    return 0;
}
