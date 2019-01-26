//
//  ms_server.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms.h"
#include "ms_server.h"
#include "ms_session.h"
#include "ms_mem_storage.h"
#include "ms_http_pipe.h"
#include "ms_task.h"

#define MS_STRINGIFY(v) MS_STRINGIFY_HELPER(v)
#define MS_STRINGIFY_HELPER(v) #v

#define MS_VERSION_STRING_BASE  MS_STRINGIFY(MS_VERSION_MAJOR) "." \
MS_STRINGIFY(MS_VERSION_MINOR) "." \
MS_STRINGIFY(MS_VERSION_PATCH)

#if MS_VERSION_IS_RELEASE
# define MS_VERSION_STRING  MS_VERSION_STRING_BASE
#else
# define MS_VERSION_STRING  MS_VERSION_STRING_BASE "-" MS_VERSION_SUFFIX
#endif


unsigned int ms_version(void) {
  return MS_VERSION_HEX;
}


const char* ms_version_string(void) {
  return MS_VERSION_STRING;
}

char *ms_str_of_ev(int ev) {
  if (ev == MG_EV_POLL) {
    return "MG_EV_POLL";
  } else if (ev == MG_EV_ACCEPT) {
    return "MG_EV_ACCEPT";
  } else if (ev == MG_EV_CONNECT) {
    return "MG_EV_CONNECT";
  } else if (ev == MG_EV_RECV) {
    return "MG_EV_RECV";
  } else if (ev == MG_EV_SEND) {
    return "MG_EV_SEND";
  } else if (ev == MG_EV_CLOSE) {
    return "MG_EV_CLOSE";
  } else if (ev == MG_EV_TIMER) {
    return "MG_EV_TIMER";
  } else if (ev == MG_EV_HTTP_REQUEST) {
    return "MG_EV_HTTP_REQUEST";
  } else if (ev == MG_EV_HTTP_REPLY) {
    return "MG_EV_HTTP_REPLY";
  } else if (ev == MG_EV_HTTP_CHUNK) {
    return "MG_EV_HTTP_CHUNK";
  } else {
    return "unknown";
  }
}


static int s_exit_flag = 0;
static struct ms_server s_server;

struct ms_server *ms_default_server(void) {
  return &s_server;
}


static void init_media_server(struct ms_server *server) {
  QUEUE_INIT(&server->sessions);
  QUEUE_INIT(&server->tasks);
  mg_mgr_init(&server->mgr, NULL);
  
#ifdef __APPLE__
  signal(SIGPIPE, SIG_IGN);
#endif
}

static void uninit_media_server(struct ms_server *server) {
  // TODO:
}

static struct ms_ipipe *open_pipe(const struct mg_str url,
                           int64_t pos,
                           int64_t len,
                           struct ms_ipipe_callback callback) {
  return (struct ms_ipipe *)ms_http_pipe_create(url, pos, len, callback);
}

static struct ms_istorage *open_storage(void) {
  return (struct ms_istorage *)ms_mem_storage_open();
}

static struct ms_session *find_session(struct mg_connection *nc, struct ms_server *server) {
  QUEUE *q;
  struct ms_session *session = NULL;
  QUEUE_FOREACH(q, &server->sessions) {
    session = QUEUE_DATA(q, struct ms_session, node);
    if (session->connection == nc) {
      return session;
    }
  }
  return NULL;
}

static void remove_task_if_need(struct ms_server *server, double ts) {
  QUEUE *q;
  struct ms_task *task = NULL;
  QUEUE_FOREACH(q, &server->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    if (QUEUE_EMPTY(&task->readers)) {
      if (mg_time() - task->close_ts >= ts) {
        task->task.close(&task->task);
      }
    }
  }
}

static struct ms_task *find_or_create_task(const char *url, struct ms_server *server) {
  QUEUE *q;
  struct ms_task *task = NULL;
  QUEUE_FOREACH(q, &server->tasks) {
    task = QUEUE_DATA(q, struct ms_task, node);
    if (mg_strcmp(task->url, mg_mk_str(url)) == 0) {
      return task;
    }
  }
  remove_task_if_need(server, 5);
  struct ms_factory factory = {
    open_storage,
    open_pipe
  };
  task = ms_task_open(mg_mk_str(url), factory);
  QUEUE_INSERT_TAIL(&server->tasks, &task->node);
  return task;
}

static void stream_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_POLL && ev != MG_EV_SEND) {
    MS_DBG("handler:%p, %s", nc, ms_str_of_ev(ev));
  }
  struct ms_server *server = nc->user_data;
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *)p;
    MS_ASSERT(mg_strcmp(hm->method, mg_mk_str("GET")) == 0);
    char url[MG_MAX_HTTP_REQUEST_SIZE] = {0};
    mg_get_http_var(&hm->query_string, "url", url, MG_MAX_HTTP_REQUEST_SIZE);
    MS_DBG("%s", url);
    struct ms_task *task = find_or_create_task(url, server);
    if (task) {
      struct ms_session *session = ms_session_open(nc, hm, &task->task);
      QUEUE_INSERT_TAIL(&server->sessions, &session->node);
      task->task.add_reader(&task->task, (struct ms_ireader *)session);
//      ms_task_add_reader(task, (struct ms_ireader *)session);
      ms_session_try_transfer_data(session);
    }
  }
}

static void server_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev != MG_EV_POLL && ev != MG_EV_SEND) {
    MS_DBG("handler:%p, %s", nc, ms_str_of_ev(ev));
  }
  
  struct ms_server *server = nc->user_data;
  
  if (ev == MG_EV_HTTP_REQUEST) {
    mg_http_send_error(nc, 404, NULL);
  } else if (ev == MG_EV_SEND) {
    struct ms_session *session = find_session(nc, server);
    int *num_sent_bytes = (int *)p;
    if (session) {
      //            MS_ASSERT(*num_sent_bytes > 0);
      if (*num_sent_bytes < 0) {
        // TODO: handle error.
        MS_DBG("%d", errno);
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      } else {
        session->reader.on_send((struct ms_ireader *)session, *num_sent_bytes);
      }
    }
  } else if (ev == MG_EV_CLOSE) {
    struct ms_session *session = find_session(nc, server);
    if (session) {
//      ms_task_remove_reader(session->task, (struct ms_ireader *)session);
      session->task->remove_reader(session->task, (struct ms_ireader *)session);
      QUEUE_REMOVE(&session->node);
      ms_session_close(session);
    }
  } else if (ev == MG_EV_TIMER) {
    MS_DBG("%p timer", nc);
  }
}

void ms_start(const char *http_port, void (*callback)(void)) {
  init_media_server(&s_server);
  
  struct mg_connection *nc;
  
  MS_DBG("Starting web server on port %s", http_port);
  nc = mg_bind(&s_server.mgr, http_port, server_handler);
  if (nc == NULL) {
    MS_DBG("Failed to create listener");
    return;
  }
  strcpy(s_server.port, http_port);
  nc->user_data = &s_server;
  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
  s_server.nc = nc;
  
  mg_register_http_endpoint(nc, "/stream/?*", stream_handler);
  
  if (callback) {
    callback();
  }
  
  while (s_exit_flag == 0) {
    mg_mgr_poll(&s_server.mgr, 1000);
  }
  remove_task_if_need(&s_server, 0);
  mg_mgr_free(&s_server.mgr);
  uninit_media_server(&s_server);
  
}

void ms_stop() {
  s_exit_flag = 1;
}

static void *server_thread(void *argv) {
  ms_start("8090", NULL);
  return NULL;
}

void ms_asnyc_start() {
  pthread_t thread_id = (pthread_t) 0;
  pthread_attr_t attr;
  
  (void) pthread_attr_init(&attr);
  (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
  
  pthread_create(&thread_id, &attr, server_thread, NULL);
  pthread_attr_destroy(&attr);
}

int ms_generate_url(struct ms_url_param *input, char *out_url, size_t out_len) {
  struct mg_str encode = mg_url_encode_opt(mg_mk_str(input->url), mg_mk_str("._-$,;~()"), 0);
  int ret = snprintf(out_url, out_len - 1, "http://127.0.0.1:%s/stream/%s?url=%s", ms_default_server()->port, input->path, encode.p);
  free((void *) encode.p);
  return ret;
}

