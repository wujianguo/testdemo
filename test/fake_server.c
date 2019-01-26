//
//  fake_server.c
//  libms
//
//  Created by Jianguo Wu on 2019/1/4.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#include "fake_server.h"
#include "ms.h"

static struct ms_fake_server s_server = {0};

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *)p;
    
    struct ms_fake_server *server = &s_server;
    QUEUE *q;
    struct ms_fake_nc *nct;
    QUEUE_FOREACH(q, &server->nc) {
      nct = QUEUE_DATA(q, struct ms_fake_nc, node);
      if (mg_strcmp(hm->uri, mg_mk_str(nct->uri)) == 0) {
        mg_http_serve_file(nc, hm, nct->path, mg_mk_str("video/mp4"), mg_mk_str(""));
        return;
      }
    }
    mg_http_send_error(nc, 404, "Not Found");
  }
}

static void add_nc(struct ms_fake_server *server, char *path, char *uri, enum ms_fake_type type) {
  struct ms_fake_nc *nc = (struct ms_fake_nc *)MS_MALLOC(sizeof(struct ms_fake_nc));
  memset(nc, 0, sizeof(struct ms_fake_nc));
  QUEUE_INIT(&nc->node);
  memcpy(nc->path, path, MG_MAX_PATH - 1);
  memcpy(nc->uri, uri, MG_MAX_PATH - 1);
  nc->type = type;
  cs_stat_t st;
  if (mg_stat(path, &st) == 0) {
    nc->filesize = st.st_size;
  }
  MS_ASSERT(nc->filesize > 0);
  QUEUE_INSERT_TAIL(&server->nc, &nc->node);
  mg_register_http_endpoint(ms_default_server()->nc, nc->uri, ev_handler);

}

void start_fake_server(void) {
  QUEUE_INIT(&s_server.nc);
  add_nc(&s_server, "/Users/wujianguo/Documents/temp/wildo.mp4", "/fake/wildo.mp4", ms_fake_type_normal);
}

void fake_url(struct ms_fake_nc *nc, char *url, int url_len) {
    snprintf(url, url_len, "http://127.0.0.1:%s%s", ms_default_server()->port, nc->uri);
}

struct ms_fake_nc *nc_of(enum ms_fake_type type) {
  QUEUE *q;
  struct ms_fake_nc *nct;
  QUEUE_FOREACH(q, &s_server.nc) {
    nct = QUEUE_DATA(q, struct ms_fake_nc, node);
    if (nct->type == type) {
      return nct;
    }
  }
  abort();
  return NULL;
}
