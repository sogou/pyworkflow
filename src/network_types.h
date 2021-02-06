#ifndef PYWF_NETWORK_TYPES_H
#define PYWF_NETWORK_TYPES_H
#include "common_types.h"
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFHttpServer.h"

class HttpAttachment final : public protocol::ProtocolMessage::Attachment {
public:
    HttpAttachment() : total_size(0) {}
    HttpAttachment(const HttpAttachment&) = delete;
    ~HttpAttachment() {
        {
            py::gil_scoped_acquire acquire;
            pybytes.clear();
        }
        nocopy_body.clear();
    }
    // I suppose the caller has GIL
    // for append, get_body, clear
    void append(py::bytes b, const char *p, size_t sz) {
        if(sz > 0) {
            pybytes.emplace_back(b);
            nocopy_body.emplace_back(p, sz);
            total_size += sz;
        }
    }
    void append(const char *p, size_t sz) noexcept {
        nocopy_body.emplace_back(p, sz);
        total_size += sz;
    }
    std::string get_body() const {
        std::string body;
        body.reserve(total_size);
        for(const auto &b : nocopy_body) {
            body.append(b.first, b.second);
        }
        return body;
    }
    void clear() {
        pybytes.clear();
        nocopy_body.clear();
        total_size = 0;
    }
private:
    std::vector<py::bytes> pybytes;
    std::vector<std::pair<const char*, size_t>> nocopy_body;
    size_t total_size;
};

template<class Req, class Resp>
class PyWFNetworkTask : public PySubTask {
public:
    using ReqType = Req;
    using RespType = Resp;
    using OriginType = WFNetworkTask<typename Req::OriginType, typename Resp::OriginType>;
    using _py_callback_t = std::function<void(PyWFNetworkTask<Req, Resp>)>;
    PyWFNetworkTask()                         : PySubTask()  {}
    PyWFNetworkTask(OriginType *p)            : PySubTask(p) {}
    PyWFNetworkTask(const PyWFNetworkTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss() { this->get()->dismiss(); }
    void noreply() { this->get()->noreply(); }
    ReqType get_req()              { return ReqType(this->get()->get_req());   }
    RespType get_resp()            { return RespType(this->get()->get_resp()); }
    int get_state() const          { return this->get()->get_state();          }
    int get_error() const          { return this->get()->get_error();          }
    int get_timeout_reason() const { return this->get()->get_timeout_reason(); }
    long long get_task_seq() const { return this->get()->get_task_seq();       }
    // TODO get_peer_addr, get_connection
    void set_send_timeout(int t)   { this->get()->set_send_timeout(t); }
    void set_receive_timeout(int t){ this->get()->set_receive_timeout(t); }
    void set_keep_alive(int t)     { this->get()->set_keep_alive(t); }
    void set_callback(_py_callback_t cb) {
        auto deleter = std::make_shared<TaskDeleterWrapper<_py_callback_t, OriginType>>(
            std::move(cb), this->get());
        this->get()->set_callback([deleter](OriginType *p) {
            auto resp = p->get_resp();
            const void *data = nullptr;
            size_t size = 0;
            if(resp->get_parsed_body(&data, &size) && size > 0) {
                resp->append_output_body_nocopy(data, size);
            }
            py_callback_wrapper(deleter->get_func(), PyWFNetworkTask<Req, Resp>(p));
        });
    }
    void set_user_data(py::object obj) {
        void *old = this->get()->user_data;
        if(old != nullptr) {
            delete static_cast<py::object*>(old);
        }
        py::object *p = nullptr;
        if(obj.is_none() == false) p = new py::object(obj);
        this->get()->user_data = static_cast<void*>(p);
    }
    py::object get_user_data() const {
        void *context = this->get()->user_data;
        if(context == nullptr) return py::none();
        return *static_cast<py::object*>(context);
    }
};
class CopyableHttpRequest {
    using header_type = std::vector<std::pair<std::string, std::string>>;
public:
    CopyableHttpRequest() {}
    CopyableHttpRequest(
        bool chunked, bool keep_alive, std::string method, std::string request_uri,
        std::string http_version, header_type headers, std::string body
    ) :
        chunked(chunked), keep_alive(keep_alive), method(std::move(method)),
        request_uri(std::move(request_uri)), http_version(std::move(http_version)),
        headers(std::move(headers)), body(std::move(body))
    {}
    bool is_chunked()              { return chunked; }
    bool is_keep_alive()           { return keep_alive; }
    std::string get_method()       { return method; }
    std::string get_request_uri()  { return request_uri; }
    std::string get_http_version() { return http_version; }
    header_type get_headers()      { return headers; }
    py::bytes get_parsed_body()    { return py::bytes(body.c_str(), body.size()); }
private:
    bool chunked{false};
    bool keep_alive{false};
    std::string method;
    std::string request_uri;
    std::string http_version;
    header_type headers;
    std::string body;
};
class CopyableHttpResponse {
    using header_type = std::vector<std::pair<std::string, std::string>>;
public:
    CopyableHttpResponse() {}
    CopyableHttpResponse(
        bool chunked, bool keep_alive, std::string status_code, std::string reason_phrase,
        std::string http_version, header_type headers, std::string body
    ) :
        chunked(chunked), keep_alive(keep_alive), status_code(std::move(status_code)),
        reason_phrase(std::move(reason_phrase)), http_version(std::move(http_version)),
        headers(std::move(headers)), body(std::move(body))
    {}
    bool is_chunked()               { return chunked; }
    bool is_keep_alive()            { return keep_alive; }
    std::string get_status_code()   { return status_code; }
    std::string get_reason_phrase() { return reason_phrase; }
    std::string get_http_version()  { return http_version; }
    header_type get_headers()       { return headers; }
    py::bytes get_parsed_body()     { return py::bytes(body.c_str(), body.size()); }
private:
    bool chunked{false};
    bool keep_alive{false};
    std::string status_code;
    std::string reason_phrase;
    std::string http_version;
    header_type headers;
    std::string body;
};

/**
 * This is a common supper class for PyHttpRequest and PyHttpResponse.
 * There is no need to export this class to python.
 */
class PyHttpMessage : public PyWFBase {
public:
    using OriginType = protocol::HttpMessage;
    PyHttpMessage()                       : PyWFBase()  {}
    PyHttpMessage(OriginType *p)          : PyWFBase(p) {}
    PyHttpMessage(const PyHttpMessage &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    bool is_chunked()    const { return this->get()->is_chunked(); }
    bool is_keep_alive() const { return this->get()->is_keep_alive(); }
    bool add_header_pair(const std::string &k, const std::string &v) {
        return this->get()->add_header_pair(k.c_str(), v.c_str());
    }
    bool set_header_pair(const std::string &k, const std::string &v) {
        return this->get()->set_header_pair(k.c_str(), v.c_str());
    }
    std::string get_http_version() const {
        return this->get()->get_http_version();
    }
    std::vector<std::pair<std::string, std::string>> get_headers() const {
        std::vector<std::pair<std::string, std::string>> headers;
        protocol::HttpHeaderCursor resp_cursor(this->get());
        std::string name, value;
        while(resp_cursor.next(name, value)) {
            headers.emplace_back(name, value);
        }
        return headers;
    }
    py::bytes get_body() const {
        auto attach = static_cast<HttpAttachment*>(this->get()->get_attachment());
        if(attach) {
            return attach->get_body();
        }
        return protocol::HttpUtil::decode_chunked_body(this->get());
    }
    bool set_http_version(const std::string &s) { return this->get()->set_http_version(s); }
    bool append_body(py::bytes b) {
        auto attach = static_cast<HttpAttachment*>(this->get()->get_attachment());
        if(attach == nullptr) { // Which means it is the first time to append body
            this->get()->clear_output_body();
            attach = new HttpAttachment();
            this->get()->set_attachment(attach);
        }
        char *buffer = nullptr;
        ssize_t length = 0;
        if(PYBIND11_BYTES_AS_STRING_AND_SIZE(b.ptr(), &buffer, &length)) {
            // TODO there is an error
            return false;
        }
        if(length <= 0) return true; // Nothing todo
        if(this->get()->append_output_body((const void*)buffer, (size_t)length)) {
            attach->append(b, buffer, length);
            return true;
        }
        return false;
    }
    bool append_body(py::str s) {
        return append_body((py::bytes)s);
    }
    void clear_output_body() {
        auto attach = static_cast<HttpAttachment*>(this->get()->get_attachment());
        if(attach) attach->clear();
        this->get()->clear_output_body();
    }
    void end_parsing()                          { this->get()->end_parsing(); }
    size_t get_output_body_size() const { return this->get()->get_output_body_size(); }
    void set_size_limit(size_t size)    { this->get()->set_size_limit(size); }
    size_t get_size_limit()       const { return this->get()->get_size_limit(); }
protected:
    std::string _get_parsed_body() const {
        auto attach = static_cast<HttpAttachment*>(this->get()->get_attachment());
        if(attach) {
            return attach->get_body();
        }
        return protocol::HttpUtil::decode_chunked_body(this->get());
    }
};

class PyHttpRequest : public PyHttpMessage {
public:
    using OriginType = protocol::HttpRequest;
    PyHttpRequest()                       : PyHttpMessage()  {}
    PyHttpRequest(OriginType *p)          : PyHttpMessage(p) {}
    PyHttpRequest(const PyHttpRequest &o) : PyHttpMessage(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    CopyableHttpRequest copy() const {
        return CopyableHttpRequest(
            is_chunked(), is_keep_alive(), get_method(), get_request_uri(),
            get_http_version(), get_headers(), _get_parsed_body());
    }
    void move_to(PyHttpRequest &o) {
        *(o.get()) = std::move(*(this->get()));
    }
    std::string get_method() const {
        return this->get()->get_method();
    }
    std::string get_request_uri() const {
        return this->get()->get_request_uri();
    }
    bool set_method(const std::string &s)       { return this->get()->set_method(s); }
    bool set_request_uri(const std::string &s)  { return this->get()->set_request_uri(s); }
private:
};

class PyHttpResponse : public PyHttpMessage {
public:
    using OriginType = protocol::HttpResponse;
    PyHttpResponse()                        : PyHttpMessage()  {}
    PyHttpResponse(OriginType *p)           : PyHttpMessage(p) {}
    PyHttpResponse(const PyHttpResponse &o) : PyHttpMessage(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    CopyableHttpResponse copy() const {
        return CopyableHttpResponse(
            is_chunked(), is_keep_alive(), get_status_code(), get_reason_phrase(),
            get_http_version(), get_headers(), _get_parsed_body());
    }
    void move_to(PyHttpResponse &o) {
        *(o.get()) = std::move(*(this->get()));
    }
    std::string get_status_code() const {
        return this->get()->get_status_code();
    }
    std::string get_reason_phrase() const {
        return this->get()->get_reason_phrase();
    }
    bool set_status_code(const std::string &status_code) {
        return this->get()->set_status_code(status_code);
    }
    bool set_reason_phrase(const std::string &phrase) {
        return this->get()->set_reason_phrase(phrase);
    }
private:
};

template<typename Req, typename Resp>
class PyWFServer {
public:
    using ReqType = Req;
    using RespType = Resp;
    using OriginType = WFServer<typename Req::OriginType, typename Resp::OriginType>;
    using _py_process_t = std::function<void(PyWFNetworkTask<Req, Resp>)>;
    PyWFServer(_py_process_t proc)
        : process(std::move(proc)),
        server([this](WFNetworkTask<typename Req::OriginType, typename Resp::OriginType> *p) {
            auto req = p->get_req();
            const void *data = nullptr;
            size_t size = 0;
            if(req->get_parsed_body(&data, &size) && size > 0) {
                req->append_output_body_nocopy(data, size);
            }
            py_callback_wrapper(this->process, PyWFNetworkTask<Req, Resp>(p));
        }) {}
    PyWFServer(WFServerParams params, _py_process_t proc)
        : process(std::move(proc)),
        server(&params, [this](WFNetworkTask<typename Req::OriginType, typename Resp::OriginType> *p) {
            auto req = p->get_req();
            const void *data = nullptr;
            size_t size = 0;
            if(req->get_parsed_body(&data, &size) && size > 0) {
                req->append_output_body_nocopy(data, size);
            }
            py_callback_wrapper(this->process, PyWFNetworkTask<Req, Resp>(p));
        }) {}
    int start_0(unsigned short port) {
        return server.start(port);
    }
    int start_1(unsigned short port, const std::string &cert_file, const std::string &key_file) {
        if(cert_file.empty() || key_file.empty()) {
            return server.start(port);
        }
        return server.start(port, cert_file.c_str(), key_file.c_str());
    }
    int start_2(int family, const std::string &host, unsigned short port,
        const std::string &cert_file, const std::string &key_file) {
        if(cert_file.empty() || key_file.empty()) {
            return server.start(family, host.c_str(), port, nullptr, nullptr);
        }
        return server.start(family, host.c_str(), port, cert_file.c_str(), key_file.c_str());
    }
    void shutdown() {
        server.shutdown();
    }
    void wait_finish() {
        server.wait_finish();
    }
    void stop() {
        server.stop();
    }
    ~PyWFServer() {
        release_wrapped_function(this->process);
    }

    _py_process_t process;
private:
    OriginType server;
};

using PyWFHttpTask       = PyWFNetworkTask<PyHttpRequest, PyHttpResponse>;
using PyWFHttpServer     = PyWFServer<PyHttpRequest, PyHttpResponse>;
using py_http_callback_t = std::function<void(PyWFHttpTask)>;
using py_http_process_t  = std::function<void(PyWFHttpTask)>;

#endif // PYWF_NETWORK_TYPES_H
