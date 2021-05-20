#ifndef PYWF_HTTP_TYPES_H
#define PYWF_HTTP_TYPES_H

#include "network_types.h"

static inline std::string __as_string(const char *p) {
    std::string s;
    if(p) s.assign(p);
    return s;
}

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

    // I suppose the caller has GIL for append, get_body, clear
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
        return __as_string(this->get()->get_http_version());
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
    bool append_bytes_body(py::bytes b) {
        auto attach = static_cast<HttpAttachment*>(this->get()->get_attachment());
        if(attach == nullptr) { // Which means it is the first time to append body
            this->get()->clear_output_body();
            attach = new HttpAttachment();
            this->get()->set_attachment(attach);
        }
        char *buffer = nullptr;
        ssize_t length = 0;
        if(PYBIND11_BYTES_AS_STRING_AND_SIZE(b.ptr(), &buffer, &length)) {
            // there is an error
            return false;
        }
        if(length <= 0) return true; // Nothing todo
        if(this->get()->append_output_body((const void*)buffer, (size_t)length)) {
            attach->append(b, buffer, length);
            return true;
        }
        return false;
    }

    bool append_str_body(py::str s) {
        return append_bytes_body((py::bytes)s);
    }

    void clear_output_body() {
        auto attach = static_cast<HttpAttachment*>(this->get()->get_attachment());
        if(attach) attach->clear();
        this->get()->clear_output_body();
    }

    void end_parsing()                  { this->get()->end_parsing(); }
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

    void move_to(PyHttpRequest &o) {
        *(o.get()) = std::move(*(this->get()));
    }

    std::string get_method() const {
        return __as_string(this->get()->get_method());
    }

    std::string get_request_uri() const {
        return __as_string(this->get()->get_request_uri());
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

    void move_to(PyHttpResponse &o) {
        *(o.get()) = std::move(*(this->get()));
    }

    std::string get_status_code() const {
        return __as_string(this->get()->get_status_code());
    }

    std::string get_reason_phrase() const {
        return __as_string(this->get()->get_reason_phrase());
    }

    bool set_status_code(const std::string &status_code) {
        return this->get()->set_status_code(status_code);
    }

    bool set_reason_phrase(const std::string &phrase) {
        return this->get()->set_reason_phrase(phrase);
    }
};

using PyWFHttpTask       = PyWFNetworkTask<PyHttpRequest, PyHttpResponse>;
using PyWFHttpServer     = PyWFServer<PyHttpRequest, PyHttpResponse>;
using py_http_callback_t = std::function<void(PyWFHttpTask)>;
using py_http_process_t  = std::function<void(PyWFHttpTask)>;

template<>
struct pytype<WFHttpTask> {
    using type = PyWFHttpTask;
};
template<>
struct pytype<WFHttpServer> {
    using type = PyWFHttpServer;
};

#endif // PYWF_HTTP_TYPES_H
