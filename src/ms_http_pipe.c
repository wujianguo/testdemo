//
//  ms_http_pipe.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_http_pipe.h"
#include "ms_memory_pool.h"

#define MS_F_ON_CONTENT_CALLED MG_F_USER_1

//static void check_buf(struct ms_http_pipe *pipe, const char *buf, int64_t pos, size_t len)
//{
//    char *fbuf = MS_MALLOC(len);
//    size_t read = pread(pipe->fp, fbuf, len, pos);
//    MS_ASSERT(read == len);
//    for (int i = 0; i<len; ++i) {
//        if (fbuf[i] != buf[i]) {
//            abort();
//        }
//    }
//    MS_FREE(fbuf);
//}

static int http_parse_content_range(const struct mg_str *header, int64_t *a, int64_t *b, int64_t *filesize) {
  /*
   * There is no snscanf. Headers are not guaranteed to be NUL-terminated,
   * so we have this. Ugh.
   */
  int result;
  char *p = (char *) MS_MALLOC(header->len + 1);
  memcpy(p, header->p, header->len);
  p[header->len] = '\0';
  result = sscanf(p, "bytes %" INT64_FMT "-%" INT64_FMT "/%" INT64_FMT, a, b, filesize);
  MS_FREE(p);
  return result;
}


static struct ms_http_pipe *cast_to_pipe(void *data) {
  return (struct ms_http_pipe *)data;
}

static void free_pipe(struct ms_http_pipe *pipe) {
  MS_DBG("pipe:%p", pipe);
  mbuf_free(&pipe->buf);
  MS_FREE(pipe);
}

static void pipe_handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (ev != MG_EV_POLL && ev != MG_EV_RECV && ev != MG_EV_HTTP_CHUNK) {
    MS_DBG("%s", ms_str_of_ev(ev));
  }
  //    return;
  struct ms_http_pipe *http_pipe = cast_to_pipe(nc->user_data);
  if (http_pipe->closing) {
    MS_ASSERT(nc->flags & MG_F_CLOSE_IMMEDIATELY);
    MS_DBG("pipe:%p closing... %s", http_pipe, ms_str_of_ev(ev));
    if (ev == MG_EV_CLOSE) {
      free_pipe(http_pipe);
    }
    return;
  }
  
  if (ev == MG_EV_RECV) {
    
  } else if (ev == MG_EV_HTTP_CHUNK) {
    struct http_message *hm = (struct http_message *)ev_data;
    if (!(nc->flags & MS_F_ON_CONTENT_CALLED)) {
      http_pipe->pipe.callback.on_header(&http_pipe->pipe, hm);
      if (http_pipe->pipe.callback.get_filesize(&http_pipe->pipe) == 0 || http_pipe->len == 0) {
        struct mg_str *range_hdr = mg_get_http_header(hm, "Content-Range");
        int64_t r1 = 0, r2 = 0, filesize = 0;
        http_parse_content_range(range_hdr, &r1, &r2, &filesize);

        nc->flags |= MS_F_ON_CONTENT_CALLED;
        if (filesize > 0) {
          http_pipe->pipe.callback.on_filesize(&http_pipe->pipe, filesize);
        }
        // TODO: fix no content-length header
        struct mg_str *v = mg_get_http_header(hm, "Content-Length");
        int64_t size = to64(v->p);
        http_pipe->len = size;
        MS_DBG("pipe:%p Content-Length: %lld, from:%lld", http_pipe, size, http_pipe->req_pos);
        http_pipe->pipe.callback.on_content_size(&http_pipe->pipe, http_pipe->req_pos, size);
      }
    }
    if (hm->body.len == 0) {
      return;
    }
    //        MS_ASSERT(pipe->get_filesize(pipe) > 0); // TODO: fix if filesize == 0
    MS_ASSERT(http_pipe->pos % MS_PIECE_UNIT_SIZE == 0);
    size_t off = 0;
    if (http_pipe->buf.len > 0 || hm->body.len < MS_PIECE_UNIT_SIZE) {
      /*
       *
       *                             |  hm->body  |
       *                             |            |
       *000000011111111111111111111112222222222222200000000000
       *       |                    |
       *       |  http_pipe->buf    |00000000000000000000
       *
       */
      size_t data_size = http_pipe->buf.size - http_pipe->buf.len;
      if (data_size > hm->body.len) {
        data_size = hm->body.len;
        /*
         *                             | hm->body.len |
         *                             |              |
         *00000001111111111111111111111222222222222222200000000000
         *       |                    |               |     |
         *       |  http_pipe->buf    |000000000000000000000|
         *       | http_pipe->buf.len |  data_size    |     |
         *       |          http_pipe->buf.size             |
         */
      } else {
        /*
         *                             | hm->body.len                       |
         *                             |                                    |
         *000000011111111111111111111112222222222222222222222222222222222222200000000000
         *       |                    |                     |
         *       |  http_pipe->buf    |000000000000000000000|
         *       | http_pipe->buf.len |        data_size    |
         *       |          http_pipe->buf.size             |
         */
      }
      mbuf_append(&http_pipe->buf, hm->body.p, data_size);
      off = data_size;
      if (http_pipe->buf.size == http_pipe->buf.len) {
        //                check_buf(pipe, pipe->buf.buf, pipe->pos, pipe->buf.len);
        // TODO: first pos += len, then callback recv
        http_pipe->pos += http_pipe->buf.len;
        http_pipe->pipe.callback.on_recv(&http_pipe->pipe, http_pipe->buf.buf, http_pipe->pos - http_pipe->buf.len, http_pipe->buf.len);
        mbuf_remove(&http_pipe->buf, http_pipe->buf.len);
        /*
         *                                        hm->body.len - off
         *                           hm->body               ^
         *                             ^                    |
         *                             |                    |
         *000000011111111111111111111112222222222222222222222222222222222222200000000000
         *       |                                          |
         *                                                  V
         *                                            http_pipe->pos
         */
      }

    }
    if (hm->body.len - off > 0) {
      size_t data_size = (hm->body.len - off) % MS_PIECE_UNIT_SIZE;
      size_t recv = hm->body.len - off - data_size;
      if (recv > 0) {
        //                check_buf(pipe, hm->body.p + off, pipe->pos, recv);
        // TODO: first pos += len, then callback recv
        http_pipe->pos += recv;
        http_pipe->pipe.callback.on_recv(&http_pipe->pipe, hm->body.p + off, http_pipe->pos - recv, recv);
      }
      mbuf_append(&http_pipe->buf, hm->body.p + off + recv, data_size);
    }
    
    if (http_pipe->pos + http_pipe->buf.len == http_pipe->req_pos + http_pipe->len) {
      // TODO: first pos += len, then callback recv
      http_pipe->pos += http_pipe->buf.len;
      http_pipe->pipe.callback.on_recv(&http_pipe->pipe, http_pipe->buf.buf, http_pipe->pos - http_pipe->buf.len, http_pipe->buf.len);
      mbuf_remove(&http_pipe->buf, http_pipe->buf.len);
    }
    
    nc->flags |= MG_F_DELETE_CHUNK;
    
  } else if (ev == MG_EV_HTTP_REPLY) {
    http_pipe->pipe.callback.on_complete(&http_pipe->pipe);
  } else if (ev == MG_EV_CLOSE) {
    http_pipe->pipe.callback.on_close(&http_pipe->pipe, 0);
    if (http_pipe->closing) {
      free_pipe(http_pipe);
    }
  }
}

static void pipe_connect(struct ms_ipipe *pipe) {
  struct ms_http_pipe *http_pipe = (struct ms_http_pipe *)pipe;
  char extra_headers[128] = {0};
  if (http_pipe->len > 0) {
    snprintf(extra_headers, 128, "Range: bytes=%lld-%lld\r\n", http_pipe->pos, http_pipe->pos + http_pipe->len);
  } else {
    snprintf(extra_headers, 128, "Range: bytes=%lld-\r\n", http_pipe->pos);
  }
  MS_DBG("pipe:%p connect %s", http_pipe, extra_headers);
  struct mg_connection *nc = mg_connect_http(&ms_default_server()->mgr, pipe_handler, http_pipe->url.p, extra_headers, NULL);
  nc->user_data = http_pipe;
  http_pipe->nc = nc;
}

static int64_t get_req_len(struct ms_ipipe *pipe) {
  struct ms_http_pipe *http_pipe = (struct ms_http_pipe *)pipe;
  return http_pipe->req_len;
}

static int64_t get_current_pos(struct ms_ipipe *pipe) {
  struct ms_http_pipe *http_pipe = (struct ms_http_pipe *)pipe;
  return http_pipe->pos;
}

static void pipe_close(struct ms_ipipe *pipe) {
  struct ms_http_pipe *http_pipe = (struct ms_http_pipe *)pipe;
  MS_DBG("http_pipe:%p", http_pipe);
  http_pipe->closing = 1;
  http_pipe->nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}


struct ms_http_pipe *ms_http_pipe_create(const struct mg_str url,
                                         int64_t pos,
                                         int64_t len,
                                         struct ms_ipipe_callback callback) {
  struct ms_http_pipe *http_pipe = (struct ms_http_pipe *)MS_MALLOC(sizeof(struct ms_http_pipe));
  memset(http_pipe, 0, sizeof(struct ms_http_pipe));
  MS_DBG("pipe:%p", http_pipe);
  QUEUE_INIT(&http_pipe->pipe.node);
  mbuf_init(&http_pipe->buf, MS_PIECE_UNIT_SIZE);
  http_pipe->url = url;
  http_pipe->pos = pos;
  if (len > 0) {
    http_pipe->len = len;
  }
  http_pipe->req_pos = http_pipe->pos;
  http_pipe->req_len = http_pipe->len;
  
  http_pipe->pipe.get_req_len = get_req_len;
  http_pipe->pipe.get_current_pos = get_current_pos;
  http_pipe->pipe.connect = pipe_connect;
  http_pipe->pipe.close = pipe_close;
  http_pipe->pipe.callback = callback;
  
  //    pipe->fp = open("/Users/wujianguo/Downloads/Friends.S01E01.1994.BluRay.720p.x264.AAC-iSCG.mp4", O_RDONLY);
  
  //    create_connection(pipe);
  return http_pipe;
}

//void ms_http_pipe_connect(struct ms_http_pipe *http_pipe) {
//  http_pipe->pipe.connect(&http_pipe->pipe);
//}


//void ms_http_pipe_forward(struct ms_http_pipe *pipe, size_t offset)
//{
//    pipe->pos += offset;
//    if (pipe->len > 0) {
//        pipe->len -= offset;
//    }
//}

//void ms_http_pipe_close(struct ms_http_pipe *http_pipe) {
//  http_pipe->pipe.close(&http_pipe->pipe);
//}

