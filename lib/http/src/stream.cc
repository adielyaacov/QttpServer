#include "native/stream.h"

#ifdef SSL_TLS_UV
  #include <uv_tls.h>
#endif

using namespace native;
using namespace base;

// Consulted with libuv team and they claim that this is intentional since they
// rather cast to struct iovec (linux) and WSABUF (windows).
// Note that normally we should use uv_buf_init (uv.h) to initialize.
#ifndef CREATE_UVBUF
    #ifdef _WIN32
        #define CREATE_UVBUF(LEN, BUF) { uv_buf_t { static_cast<size_t>(LEN), const_cast<char*>(BUF) } }
    #else
        #define CREATE_UVBUF(LEN, BUF) { uv_buf_t { const_cast<char*>(BUF), static_cast<size_t>(LEN) } }
    #endif
#endif

bool stream::listen(std::function<void(native::error)> callback, int backlog)
{
  callbacks::store(get()->data, native::internal::uv_cid_listen, callback);

  return uv_listen(get<uv_stream_t>(), backlog, [](uv_stream_t* s, int status) {
    callbacks::invoke<decltype(callback)>(s->data, native::internal::uv_cid_listen, ((status != 0) ? error(status) : error()));
  }) == 0;
}

bool stream::accept(stream* client)
{
#ifdef SSL_TLS_UV
  uv_tls_t* sclient = new uv_tls_t;
  memset(sclient, 0, sizeof(uv_tls_t));
  evt_ctx_t* ctx = (evt_ctx_t*) client->get<uv_stream_t>()->data;
  evt_tls_t* t = evt_ctx_get_tls(ctx);
  if(t != nullptr) {
    // FIXME later when this starts working.
    // assert( t != NULL );
    t->data = sclient;
    sclient->tls = t;
    sclient->tls_rd_cb = NULL;
    sclient->tls_cls_cb = NULL;
    sclient->tls_hsk_cb = NULL;
    sclient->tls_wr_cb = NULL;
  }
#endif

  bool result = uv_accept(get<uv_stream_t>(), client->get<uv_stream_t>()) == 0;

#ifdef SSL_TLS_UV
  // FIXME won't need (t != nullptr) later when this starts working.
  if(result && t != nullptr) {
    assert( sclient != nullptr );
    int rv = evt_tls_accept(sclient->tls, []( evt_tls_t *t, int status) {
      uv_tls_t *ut = (uv_tls_t*)t->data;
      assert( ut != NULL && ut->tls_hsk_cb != NULL);
      ut->tls_hsk_cb(ut, status - 1);
    });
  }
#endif

  return result;
}

bool stream::read_start(std::function<void(const char* buf, ssize_t len)> callback)
{
  return read_start<0>(callback);
}

bool stream::read_stop()
{
  return uv_read_stop(get<uv_stream_t>()) == 0;
}

// TODO: implement read2_start()

bool stream::write(const char* buf, int len, std::function<void(error)> callback)
{
  uv_buf_t bufs[] = CREATE_UVBUF(len, buf);
  callbacks::store(get()->data, native::internal::uv_cid_write, callback);
  return uv_write(new uv_write_t, get<uv_stream_t>(), bufs, 1, [](uv_write_t* req, int status) {
    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_write, ((status != 0) ? error(status) : error()));
    delete req;
  }) == 0;
}

bool stream::write(const std::string& buf, std::function<void(error)> callback)
{
  uv_buf_t bufs[] = CREATE_UVBUF((unsigned long)buf.length(), buf.c_str());
  callbacks::store(get()->data, native::internal::uv_cid_write, callback);
  return uv_write(new uv_write_t, get<uv_stream_t>(), bufs, 1, [](uv_write_t* req, int status) {
    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_write, ((status != 0) ? error(status) : error()));
    delete req;
  }) == 0;
}

bool stream::write(const std::vector<char>& buf, std::function<void(error)> callback)
{
  uv_buf_t bufs[] = CREATE_UVBUF((unsigned long)buf.size(), &buf[0]);
  callbacks::store(get()->data, native::internal::uv_cid_write, callback);
  return uv_write(new uv_write_t, get<uv_stream_t>(), bufs, 1, [](uv_write_t* req, int status) {
    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_write, ((status != 0) ? error(status) : error()));
    delete req;
  }) == 0;
}

// TODO: implement write2()

bool stream::shutdown(std::function<void(error)> callback)
{
  callbacks::store(get()->data, native::internal::uv_cid_shutdown, callback);

  return uv_shutdown(new uv_shutdown_t, get<uv_stream_t>(), [](uv_shutdown_t* req, int status) {
    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_shutdown, ((status != 0) ? error(status) : error()));
    delete req;
  }) == 0;
}
