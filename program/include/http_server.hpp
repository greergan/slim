#ifndef __SLIM__HTTP__SERVER__HPP
#define __SLIM__HTTP__SERVER__HPP
#include <uv.h>
#include <stdlib.h>
#include <string>
#include <logger.hpp>
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>
std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8_value(isolate, value);
    return std::string(*utf8_value);
}
namespace slim::http {
    uv_loop_t* server_loop;
    void init(uv_loop_t *loop) { server_loop = loop; }
    struct Server {
        private:
            uv_tcp_t server;
            int error;
        public:
        Server(const int port, const char* host) {
            sockaddr_in addr;
            slim::log::system::handle_libuv_error("TCP initilization failed: ", uv_tcp_init(server_loop, &server));
            slim::log::system::handle_libuv_error("Address error: ", uv_ip4_addr(host, port, &addr));
            slim::log::system::handle_libuv_error("Bind error: ", uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0));
            slim::log::system::handle_libuv_error("Listen error: ", uv_listen((uv_stream_t*) &server, 512, on_connection));
            slim::log::system::notice("Slim HTTP server listening on ", host, " port ", port);
        }
        ~Server() {
            std::cout << "destructing\n";
        }
        static void allocate(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
            buf->base = (char *)malloc(suggested_size);
            buf->len = suggested_size;
        }
        static void on_close(uv_handle_t* peer) { free(peer); }
        static void on_connection(uv_stream_t* server, const int status) {
            slim::log::system::info("New HTTP connection");
            if(status) {
                slim::log::system::warn("Connect error: ", uv_err_name(status), ": ", uv_strerror(status));
                return;
            }
        }
        static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
            if(nread <= 0) { if(buf) free(buf->base); return; }
            free(buf->base);
        }
    };
    Server* http_server;
    void start(const v8::FunctionCallbackInfo<v8::Value> &args) {
        if(args.Length() == 0) {
            //throw error
        }
        else {
            v8::Isolate* isolate = args.GetIsolate();
            v8::HandleScope scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> properties = args[0]->ToObject(context).ToLocalChecked();
            if(!properties.IsEmpty()) {
                v8::Local<v8::Name> port_name = v8::String::NewFromUtf8(isolate, "port").ToLocalChecked();
                v8::Local<v8::Name> host_name = v8::String::NewFromUtf8(isolate, "host").ToLocalChecked();
                v8::Local<v8::Value> port_value = properties->Get(context, port_name).ToLocalChecked();
                std::string host = ValueToString(isolate, properties->Get(context, host_name).ToLocalChecked());
                int port = (port_value->IsInt32()) ? port_value->Int32Value(context).FromJust() : 0;
                if(port > 0 && host != "undefined") {
                    http_server = new Server(port, host.c_str());
                }
                else {
                    //throw an error;
                }
            }

        }
    }
    void stop() { if(http_server != NULL) delete http_server; }
    void expose(v8::Isolate* isolate, v8::Local<v8::Context> context) {
        v8pp::module server_module(isolate);
        server_module.set("http", &start);
        v8::Maybe result = context->Global()->Set(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "slim"), server_module.new_instance());
    }
};
#endif
/*
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if(nread < 0) {
        if(buf->base)
            free(buf->base);
        return;
    }
    if(nread == 0) {
        free(buf->base);
        return;
    }
    http_request* request = calloc(1, sizeof(http_request));
    if(request == NULL) {
        free(buf->base);
        fprintf(stderr, "Allocate error: %s\n", strerror(errno));
        uv_close((uv_handle_t*) stream, on_close);
        return;
    }
    stream->data = request;
    request->handle = (uv_handle_t*) stream;
    request->num_headers = sizeof(request->headers) / sizeof(request->headers[0]);
    int nparsed = phr_parse_request(
        buf->base,
        buf->len,
        &request->method,
        &request->method_len,
        &request->path,
        &request->path_len,
        &request->minor_version,
        request->headers,
        &request->num_headers,
        0);
    if(nparsed < 0) {
        free(request);
        free(buf->base);
        fprintf(stderr, "Invalid request\n");
        uv_close((uv_handle_t*) stream, on_close);
        return;
    }
  
    // TODO: handle reading whole payload
    request->payload = buf->base + nparsed;
    request->payload_len = buf->len - nparsed;
    request_complete(request);
    free(buf->base);
}*/
/* static void on_connection(uv_stream_t* server, int status) {
    uv_stream_t* stream;
    int r;
    if(status != 0) {
        fprintf(stderr, "Connect error: %s: %s\n", uv_err_name(status), uv_strerror(status));
        return;
    }
    stream = (uv_stream_t *)malloc(sizeof(uv_tcp_t));
    if(stream == NULL) {
        fprintf(stderr, "Allocate error: %s\n", strerror(errno));
        return;
    }
    r = uv_tcp_init(server_loop, (uv_tcp_t*) stream);
    if(r) {
        fprintf(stderr, "Socket creation error: %s: %s\n", uv_err_name(r), uv_strerror(r));
    return;
    }
    r = uv_tcp_simultaneous_accepts((uv_tcp_t*) stream, 1);
    if(r) {
        fprintf(stderr, "Flag error: %s: %s\n", uv_err_name(r), uv_strerror(r));
        return;
    }
    r = uv_accept(server, stream);
    if(r) {
        fprintf(stderr, "Accept error: %s: %s\n", uv_err_name(r), uv_strerror(r));
        return;
    }
    r = uv_tcp_nodelay((uv_tcp_t*) stream, 1);
    if(r) {
        fprintf(stderr, "Flag error: %s: %s\n", uv_err_name(r), uv_strerror(r));
        return;
    }
    r = uv_read_start(stream, on_alloc, on_read);
    if(r) {
        fprintf(stderr, "Read error: %s: %s\n", uv_err_name(r), uv_strerror(r));
        uv_close((uv_handle_t*) stream, on_close);
    }
} */