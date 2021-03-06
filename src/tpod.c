#define _XOPEN_SOURCE /* needed for time functions */

#include "mongoose/mongoose.h"
#include <ao/ao.h>
#include <curl/curl.h>
#include <jansson.h>
#include <math.h>
#include <mpg123.h>
#include <mrss.h>
#include <signal.h>
#include <sqlite3.h>
#include <string.h>
#include <time.h>

#define TPOD_MODE_SRV 0
#define TPOD_MODE_CLI 1

static struct mg_serve_http_opts s_http_server_opts;
static const struct mg_str msg_http_method_get = MG_MK_STR("GET");
static const struct mg_str msg_http_method_post = MG_MK_STR("POST");

sqlite3 *db;
mpg123_handle *mh;
ao_device *device;

int playback_stop = 0;
int playback_pause = 0;
int srv = 1; /* keep mongoose event loop running */
int mode = TPOD_MODE_SRV;

CURL *ch = NULL;

char **tokenize_string(const char *string, const char *delimiter);
char **select_podcasts(int *);
char *load_episodes();
static size_t header_callback(char *buffer, size_t size, size_t nitems,
                              void *userdata);

/* todo: save podcast episodes to db rather than loading them every time */
char *load_episodes() {
  int i;
  int podcast_uris_num;
  char **podcast_uris = select_podcasts(&podcast_uris_num);

  mrss_t *feed;
  mrss_item_t *episode;
  mrss_tag_t *other_tags;

  json_t *jsn_podcasts_obj = json_object();
  json_t *jsn_podcasts_arr = json_array();

  for (i = 0; i < podcast_uris_num; i++) {
    mrss_parse_url_with_options_and_error(podcast_uris[i], &feed, NULL, NULL);
    json_t *jsn_podcast_obj = json_object();
    json_object_set_new(jsn_podcast_obj, "title", json_string(feed->title));
    json_t *jsn_episodes_arr = json_array();
    episode = feed->item;
    while (episode) {
      struct tm tm_publish;
      time_t time_now, time_publish;

      memset(&tm_publish, 0, sizeof(struct tm));
      time(&time_now);

      /*
       episode->pubDate is formated like e.g. Sat, 30 Jul 2016 00:00:00 +0200
       when parsing the datetime string only year, month and day will be used
       for windows something like e.g.
         char month[4];
         int day, year;
         sscanf(episode->pubDate, "%*s %d %s %d %*d:%*d:%*d %*s", &day, month,
       &year);
       could be used instead of relying on strptime
      */

      /* time string format like "Sat, 06 Aug 2016 08:14:24 +0200" */
      if (strptime(episode->pubDate, "%a, %0d %b %Y %T %z", &tm_publish) !=
          NULL) {
        double time_diff = difftime(time_now, mktime(&tm_publish));
        if (time_diff / 86400.0 > 30.0) {
          break;
        }
      }
      /* time string format like "Sat, 30 Jul 2016 01:00:00 GMT" */
      else if (strptime(episode->pubDate, "%a, %0d %b %Y %T %Z", &tm_publish) !=
               NULL) {
        double time_diff = difftime(time_now, mktime(&tm_publish));
        if (time_diff / 86400.0 > 30.0) {
          break;
        }
      } else {
        printf("unsopported time format: %s\n", episode->pubDate);
        break;
      }

      json_t *jsn_episode_obj = json_object();
      json_object_set_new(jsn_episode_obj, "title",
                          json_string(episode->title));
      json_object_set_new(jsn_episode_obj, "description",
                          json_string(episode->description));
      json_object_set_new(jsn_episode_obj, "stream_uri",
                          json_string(episode->enclosure_url));
      if (episode->other_tags) {
        other_tags = episode->other_tags;
        while (other_tags) {
          if (strcmp(other_tags->name, "duration") == 0) {
            json_object_set_new(jsn_episode_obj, "duration",
                                json_string(other_tags->value));
          }
          other_tags = other_tags->next;
        }
      }
      episode = episode->next;
      json_array_append_new(jsn_episodes_arr, jsn_episode_obj);
    }
    mrss_free(feed);
    json_object_set_new(jsn_podcast_obj, "episodes", jsn_episodes_arr);
    json_array_append_new(jsn_podcasts_arr, jsn_podcast_obj);
    free(podcast_uris[i]);
  }

  free(podcast_uris);

  json_object_set_new(jsn_podcasts_obj, "podcasts", jsn_podcasts_arr);

  return json_dumps(jsn_podcasts_obj, JSON_COMPACT);
}

size_t write_callback(char *delivered_data, size_t size, size_t nmemb,
                      void *user_data) {
  if (!playback_pause) {
    int err;
    off_t frame_offset;
    unsigned char *audio;
    size_t done = 1;
    ao_sample_format format;
    int channels, encoding;
    long rate;

    mpg123_feed(mh, delivered_data, size * nmemb);
    while (done > 0) {
      err = mpg123_decode_frame(mh, &frame_offset, &audio, &done);
      switch (err) {
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
  return CURL_WRITEFUNC_PAUSE;
}

int progress_callback(void *client, double down_total, double down_now,
                      double up_total, double up_now) {
  if (playback_stop) {
    return 1;
  }

  if ((!playback_pause) && (!playback_stop)) {
    curl_easy_pause(ch, CURLPAUSE_CONT);
  }

  return 0;
}

void play_stream(char *stream_uri) {

  ch = curl_easy_init();
  if (ch) {
    curl_easy_setopt(ch, CURLOPT_URL, stream_uri);
    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(ch, CURLOPT_PROGRESSFUNCTION, progress_callback);
    curl_easy_setopt(ch, CURLOPT_NOPROGRESS,
                     0L); // make curl use the progress_callback

    mh = mpg123_new(NULL, NULL);
    if (mh) {
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
  mg_printf(con, "%s", "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n");
  play_stream(stream_uri);
}

static void handle_stop(struct mg_connection *con, struct http_message *msg) {
  mg_printf(con, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
  mg_send_http_chunk(con, "", 0);
  playback_stop = 1;
}

int tpod_mg_str_cmp(const struct mg_str *sample_str,
                    const struct mg_str *test_str) {
  return sample_str->len == test_str->len &&
         memcmp(sample_str->p, test_str->p, sample_str->len) == 0;
}

static void ev_handler(struct mg_connection *con, int ev, void *ev_data) {
  struct http_message *msg = (struct http_message *)ev_data;

  switch (ev) {
  case MG_EV_HTTP_REQUEST:
    if (mg_vcmp(&msg->uri, "/play") == 0) {
      handle_play(con, msg);
    } else if (mg_vcmp(&msg->uri, "/pause") == 0) {
      if (playback_pause) {
        playback_pause = 0;
      } else {
        playback_pause = 1;
      }
      mg_printf(con, "HTTP/1.1 200 OK\r\n\r\n%s");
    } else if (mg_vcmp(&msg->uri, "/stop") == 0) {
      handle_stop(con, msg);
    } else if (mg_vcmp(&msg->uri, "/init") == 0) {
      if (tpod_mg_str_cmp(&msg->method, &msg_http_method_get)) {
        /* char query_string[msg->query_string.len + 1]; */
        /* strncpy(query_string, msg->query_string.p, msg->query_string.len); */

        char *jsnstr_podcasts_obj = load_episodes();

        mg_printf(con, "HTTP/1.1 200 OK\r\nContent-Type: "
                       "application/json\r\nContent-Length: %d\r\n\r\n%s",
                  (int)strlen(jsnstr_podcasts_obj), jsnstr_podcasts_obj);
      }
    } else {
      mg_serve_http(con, msg, s_http_server_opts);
    }
    break;
  default:
    break;
  }
}

void cleanup() {
  sqlite3_close(db);
  playback_stop = 1;
  ao_shutdown();
  mpg123_exit();
}

void signal_handler(int s) {
  if (s == SIGINT) {
    switch (mode) {
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

char **select_podcasts(int *podcasts_num) {
  char **podcasts_tmp = NULL;
  *podcasts_num = 0;
  sqlite3_stmt *stmt;
  int i;

  if (sqlite3_prepare_v2(db, "select uri from podcasts", -1, &stmt, NULL) !=
      SQLITE_OK) {
    printf("failed to prepare statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    cleanup();
  }

  for (i = 0; sqlite3_step(stmt) == SQLITE_ROW; i++) {
    const char *podcast_str = sqlite3_column_text(stmt, 0);
    int podcast_len = strlen(sqlite3_column_text(stmt, 0));

    podcasts_tmp =
        realloc(podcasts_tmp, (sizeof(podcasts_tmp) + 1) * sizeof(char *));
    podcasts_tmp[i] = malloc(podcast_len * sizeof(char));
    strcpy(podcasts_tmp[i], podcast_str);
    *podcasts_num += 1;
  }

  sqlite3_finalize(stmt);
  return podcasts_tmp;
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_handler);

  if (sqlite3_open("tpod.db", &db) != SQLITE_OK) {
    printf("failed to open database: %s\n", sqlite3_errmsg(db));
    cleanup();
    return 1;
  }

  mpg123_init();

  ao_initialize();

  if (strcmp("-s", argv[1]) == 0) {
    struct mg_mgr mgr;
    struct mg_connection *con;

    mg_mgr_init(&mgr, NULL);
    con = mg_bind(&mgr, "8080", ev_handler);
    mg_set_protocol_http_websocket(con);
    mg_enable_multithreading(con);
    s_http_server_opts.document_root = "./static";

    while (srv) {
      mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    cleanup();

    exit(130);
  } else {
    mode = 1;
    play_stream(argv[1]);
    cleanup();
  }

  return 0;
}
