/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2012, 2013 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "nghttp2_submit.h"

#include <string.h>
#include <assert.h>

#include "nghttp2_session.h"
#include "nghttp2_frame.h"
#include "nghttp2_helper.h"
#include "nghttp2_priority_spec.h"

/* This function takes ownership of |nva_copy|. Regardless of the
   return value, the caller must not free |nva_copy| after this
   function returns. */
static int32_t submit_headers_shared(nghttp2_session *session, uint8_t flags,
                                     int32_t stream_id,
                                     const nghttp2_priority_spec *pri_spec,
                                     nghttp2_nv *nva_copy, size_t nvlen,
                                     const nghttp2_data_provider *data_prd,
                                     void *stream_user_data,
                                     uint8_t attach_stream) {
  int rv;
  uint8_t flags_copy;
  nghttp2_outbound_item *item = NULL;
  nghttp2_frame *frame = NULL;
  nghttp2_headers_category hcat;
  nghttp2_mem *mem;

  mem = &session->mem;

  if (stream_id == 0) {
    rv = NGHTTP2_ERR_INVALID_ARGUMENT;
    goto fail;
  }

  item = nghttp2_mem_malloc(mem, sizeof(nghttp2_outbound_item));
  if (item == NULL) {
    rv = NGHTTP2_ERR_NOMEM;
    goto fail;
  }

  nghttp2_outbound_item_init(item);

  if (data_prd != NULL && data_prd->read_callback != NULL) {
    item->aux_data.headers.data_prd = *data_prd;
  }

  item->aux_data.headers.stream_user_data = stream_user_data;
  item->aux_data.headers.attach_stream = attach_stream;

  flags_copy = (flags & (NGHTTP2_FLAG_END_STREAM | NGHTTP2_FLAG_PRIORITY)) |
               NGHTTP2_FLAG_END_HEADERS;

  if (stream_id == -1) {
    if (session->next_stream_id > INT32_MAX) {
      rv = NGHTTP2_ERR_STREAM_ID_NOT_AVAILABLE;
      goto fail;
    }

    stream_id = session->next_stream_id;
    session->next_stream_id += 2;

    hcat = NGHTTP2_HCAT_REQUEST;
  } else {
    /* More specific categorization will be done later. */
    hcat = NGHTTP2_HCAT_HEADERS;
  }

  frame = &item->frame;

  nghttp2_frame_headers_init(&frame->headers, flags_copy, stream_id, hcat,
                             pri_spec, nva_copy, nvlen);

  rv = nghttp2_session_add_item(session, item);

  if (rv != 0) {
    nghttp2_frame_headers_free(&frame->headers, mem);
    goto fail2;
  }

  if (hcat == NGHTTP2_HCAT_REQUEST) {
    return stream_id;
  }

  return 0;

fail:
  /* nghttp2_frame_headers_init() takes ownership of nva_copy. */
  nghttp2_nv_array_del(nva_copy, mem);
fail2:
  nghttp2_mem_free(mem, item);

  return rv;
}

static void adjust_priority_spec_weight(nghttp2_priority_spec *pri_spec) {
  if (pri_spec->weight < NGHTTP2_MIN_WEIGHT) {
    pri_spec->weight = NGHTTP2_MIN_WEIGHT;
  } else if (pri_spec->weight > NGHTTP2_MAX_WEIGHT) {
    pri_spec->weight = NGHTTP2_MAX_WEIGHT;
  }
}

static int32_t submit_headers_shared_nva(nghttp2_session *session,
                                         uint8_t flags, int32_t stream_id,
                                         const nghttp2_priority_spec *pri_spec,
                                         const nghttp2_nv *nva, size_t nvlen,
                                         const nghttp2_data_provider *data_prd,
                                         void *stream_user_data,
                                         uint8_t attach_stream) {
  int rv;
  nghttp2_nv *nva_copy;
  nghttp2_priority_spec copy_pri_spec;
  nghttp2_mem *mem;

  mem = &session->mem;

  if (pri_spec) {
    copy_pri_spec = *pri_spec;
    adjust_priority_spec_weight(&copy_pri_spec);
  } else {
    nghttp2_priority_spec_default_init(&copy_pri_spec);
  }

  rv = nghttp2_nv_array_copy(&nva_copy, nva, nvlen, mem);
  if (rv < 0) {
    return rv;
  }

  return submit_headers_shared(session, flags, stream_id, &copy_pri_spec,
                               nva_copy, nvlen, data_prd, stream_user_data,
                               attach_stream);
}

int32_t nghttp2_submit_trailer(nghttp2_session *session, int32_t stream_id,
                               const nghttp2_nv *nva, size_t nvlen) {
  return submit_headers_shared_nva(session, NGHTTP2_FLAG_END_STREAM, stream_id,
                                   NULL, nva, nvlen, NULL, NULL, 0);
}

int32_t nghttp2_submit_headers(nghttp2_session *session, uint8_t flags,
                               int32_t stream_id,
                               const nghttp2_priority_spec *pri_spec,
                               const nghttp2_nv *nva, size_t nvlen,
                               void *stream_user_data) {
  flags &= NGHTTP2_FLAG_END_STREAM;

  if (pri_spec && !nghttp2_priority_spec_check_default(pri_spec)) {
    flags |= NGHTTP2_FLAG_PRIORITY;
  } else {
    pri_spec = NULL;
  }

  return submit_headers_shared_nva(session, flags, stream_id, pri_spec, nva,
                                   nvlen, NULL, stream_user_data, 0);
}

int nghttp2_submit_ping(nghttp2_session *session, uint8_t flags ,
                        const uint8_t *opaque_data) {
  return nghttp2_session_add_ping(session, NGHTTP2_FLAG_NONE, opaque_data);
}

int nghttp2_submit_priority(nghttp2_session *session, uint8_t flags ,
                            int32_t stream_id,
                            const nghttp2_priority_spec *pri_spec) {
  int rv;
  nghttp2_outbound_item *item;
  nghttp2_frame *frame;
  nghttp2_priority_spec copy_pri_spec;
  nghttp2_mem *mem;

  mem = &session->mem;

  if (stream_id == 0 || pri_spec == NULL) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  if (stream_id == pri_spec->stream_id) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  copy_pri_spec = *pri_spec;

  adjust_priority_spec_weight(&copy_pri_spec);

  item = nghttp2_mem_malloc(mem, sizeof(nghttp2_outbound_item));

  if (item == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }

  nghttp2_outbound_item_init(item);

  frame = &item->frame;

  nghttp2_frame_priority_init(&frame->priority, stream_id, &copy_pri_spec);

  rv = nghttp2_session_add_item(session, item);

  if (rv != 0) {
    nghttp2_frame_priority_free(&frame->priority);
    nghttp2_mem_free(mem, item);

    return rv;
  }

  return 0;
}

int nghttp2_submit_rst_stream(nghttp2_session *session, uint8_t flags ,
                              int32_t stream_id, uint32_t error_code) {
  if (stream_id == 0) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  return nghttp2_session_add_rst_stream(session, stream_id, error_code);
}

int nghttp2_submit_goaway(nghttp2_session *session, uint8_t flags ,
                          int32_t last_stream_id, uint32_t error_code,
                          const uint8_t *opaque_data, size_t opaque_data_len) {
  if (session->goaway_flags & NGHTTP2_GOAWAY_TERM_ON_SEND) {
    return 0;
  }
  return nghttp2_session_add_goaway(session, last_stream_id, error_code,
                                    opaque_data, opaque_data_len,
                                    NGHTTP2_GOAWAY_AUX_NONE);
}

int nghttp2_submit_shutdown_notice(nghttp2_session *session) {
  if (!session->server) {
    return NGHTTP2_ERR_INVALID_STATE;
  }
  if (session->goaway_flags) {
    return 0;
  }
  return nghttp2_session_add_goaway(session, (1u << 31) - 1, NGHTTP2_NO_ERROR,
                                    NULL, 0,
                                    NGHTTP2_GOAWAY_AUX_SHUTDOWN_NOTICE);
}

int nghttp2_submit_settings(nghttp2_session *session, uint8_t flags ,
                            const nghttp2_settings_entry *iv, size_t niv) {
  return nghttp2_session_add_settings(session, NGHTTP2_FLAG_NONE, iv, niv);
}

int32_t nghttp2_submit_push_promise(nghttp2_session *session, uint8_t flags ,
                                    int32_t stream_id, const nghttp2_nv *nva,
                                    size_t nvlen,
                                    void *promised_stream_user_data) {
  nghttp2_outbound_item *item;
  nghttp2_frame *frame;
  nghttp2_nv *nva_copy;
  uint8_t flags_copy;
  int32_t promised_stream_id;
  int rv;
  nghttp2_mem *mem;

  mem = &session->mem;

  if (stream_id == 0 || nghttp2_session_is_my_stream_id(session, stream_id)) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  if (!session->server) {
    return NGHTTP2_ERR_PROTO;
  }

  /* All 32bit signed stream IDs are spent. */
  if (session->next_stream_id > INT32_MAX) {
    return NGHTTP2_ERR_STREAM_ID_NOT_AVAILABLE;
  }

  item = nghttp2_mem_malloc(mem, sizeof(nghttp2_outbound_item));
  if (item == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }

  nghttp2_outbound_item_init(item);

  item->aux_data.headers.stream_user_data = promised_stream_user_data;

  frame = &item->frame;

  rv = nghttp2_nv_array_copy(&nva_copy, nva, nvlen, mem);
  if (rv < 0) {
    nghttp2_mem_free(mem, item);
    return rv;
  }

  flags_copy = NGHTTP2_FLAG_END_HEADERS;

  promised_stream_id = session->next_stream_id;
  session->next_stream_id += 2;

  nghttp2_frame_push_promise_init(&frame->push_promise, flags_copy, stream_id,
                                  promised_stream_id, nva_copy, nvlen);

  rv = nghttp2_session_add_item(session, item);

  if (rv != 0) {
    nghttp2_frame_push_promise_free(&frame->push_promise, mem);
    nghttp2_mem_free(mem, item);

    return rv;
  }

  return promised_stream_id;
}

int nghttp2_submit_window_update(nghttp2_session *session, uint8_t flags,
                                 int32_t stream_id,
                                 int32_t window_size_increment) {
  int rv;
  nghttp2_stream *stream = 0;
  if (window_size_increment == 0) {
    return 0;
  }
  flags = 0;
  if (stream_id == 0) {
    rv = nghttp2_adjust_local_window_size(
        &session->local_window_size, &session->recv_window_size,
        &session->recv_reduction, &window_size_increment);
    if (rv != 0) {
      return rv;
    }
  } else {
    stream = nghttp2_session_get_stream(session, stream_id);
    if (!stream) {
      return 0;
    }

    rv = nghttp2_adjust_local_window_size(
        &stream->local_window_size, &stream->recv_window_size,
        &stream->recv_reduction, &window_size_increment);
    if (rv != 0) {
      return rv;
    }
  }

  if (window_size_increment > 0) {
    if (stream_id == 0) {
      session->consumed_size =
          nghttp2_max(0, session->consumed_size - window_size_increment);
    } else {
      stream->consumed_size =
          nghttp2_max(0, stream->consumed_size - window_size_increment);
    }

    return nghttp2_session_add_window_update(session, flags, stream_id,
                                             window_size_increment);
  }
  return 0;
}

static uint8_t set_request_flags(const nghttp2_priority_spec *pri_spec,
                                 const nghttp2_data_provider *data_prd) {
  uint8_t flags = NGHTTP2_FLAG_NONE;
  if (data_prd == NULL || data_prd->read_callback == NULL) {
    flags |= NGHTTP2_FLAG_END_STREAM;
  }

  if (pri_spec) {
    flags |= NGHTTP2_FLAG_PRIORITY;
  }

  return flags;
}

int32_t nghttp2_submit_request(nghttp2_session *session,
                               const nghttp2_priority_spec *pri_spec,
                               const nghttp2_nv *nva, size_t nvlen,
                               const nghttp2_data_provider *data_prd,
                               void *stream_user_data) {
  uint8_t flags;

  if (pri_spec && nghttp2_priority_spec_check_default(pri_spec)) {
    pri_spec = NULL;
  }

  flags = set_request_flags(pri_spec, data_prd);

  return submit_headers_shared_nva(session, flags, -1, pri_spec, nva, nvlen,
                                   data_prd, stream_user_data, 0);
}

static uint8_t set_response_flags(const nghttp2_data_provider *data_prd) {
  uint8_t flags = NGHTTP2_FLAG_NONE;
  if (data_prd == NULL || data_prd->read_callback == NULL) {
    flags |= NGHTTP2_FLAG_END_STREAM;
  }
  return flags;
}

int nghttp2_submit_response(nghttp2_session *session, int32_t stream_id,
                            const nghttp2_nv *nva, size_t nvlen,
                            const nghttp2_data_provider *data_prd) {
  uint8_t flags = set_response_flags(data_prd);
  return submit_headers_shared_nva(session, flags, stream_id, NULL, nva, nvlen,
                                   data_prd, NULL, 1);
}

int nghttp2_submit_data(nghttp2_session *session, uint8_t flags,
                        int32_t stream_id,
                        const nghttp2_data_provider *data_prd) {
  int rv;
  nghttp2_outbound_item *item;
  nghttp2_frame *frame;
  nghttp2_data_aux_data *aux_data;
  uint8_t nflags = flags & NGHTTP2_FLAG_END_STREAM;
  nghttp2_mem *mem;

  mem = &session->mem;

  if (stream_id == 0) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  item = nghttp2_mem_malloc(mem, sizeof(nghttp2_outbound_item));
  if (item == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }

  nghttp2_outbound_item_init(item);

  frame = &item->frame;
  aux_data = &item->aux_data.data;
  aux_data->data_prd = *data_prd;
  aux_data->eof = 0;
  aux_data->flags = nflags;

  /* flags are sent on transmission */
  nghttp2_frame_data_init(&frame->data, NGHTTP2_FLAG_NONE, stream_id);

  rv = nghttp2_session_add_item(session, item);
  if (rv != 0) {
    nghttp2_frame_data_free(&frame->data);
    nghttp2_mem_free(mem, item);
    return rv;
  }
  return 0;
}

ssize_t nghttp2_pack_settings_payload(uint8_t *buf, size_t buflen,
                                      const nghttp2_settings_entry *iv,
                                      size_t niv) {
  if (!nghttp2_iv_check(iv, niv)) {
    return NGHTTP2_ERR_INVALID_ARGUMENT;
  }

  if (buflen < (niv * NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH)) {
    return NGHTTP2_ERR_INSUFF_BUFSIZE;
  }

  return nghttp2_frame_pack_settings_payload(buf, iv, niv);
}
