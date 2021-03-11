#ifndef PYWF_REDIS_TYPES_H
#define PYWF_REDIS_TYPES_H

#include "network_types.h"
#include "workflow/RedisMessage.h"

class PyRedisRequest : public PyWFBase {
public:
    using OriginType = protocol::RedisRequest;
    PyRedisRequest()                        : PyWFBase()  {}
    PyRedisRequest(OriginType *p)           : PyWFBase(p) {}
    PyRedisRequest(const PyRedisRequest &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    void move_to(PyRedisRequest &o) {
        *(o.get()) = std::move(*(this->get()));
    }

    void set_request(const std::string &cmd, const std::vector<std::string> &params) {
        this->get()->set_request(cmd, params);
    }

    py::str get_command() const {
        std::string cmd;
        this->get()->get_command(cmd);
        return cmd;
    }

    py::list get_params() const {
        py::list params;
        std::vector<std::string> params_vec;
        this->get()->get_params(params_vec);
        for(const auto &param : params_vec) {
            params.append(py::bytes(param));
        }
        return params;
    }

    void set_size_limit(size_t limit) { this->get()->set_size_limit(limit); }
    size_t get_size_limit() const     { return this->get()->get_size_limit(); }
};

class PyRedisResponse : public PyWFBase {
public:
    using OriginType = protocol::RedisResponse;
    PyRedisResponse()                         : PyWFBase()  {}
    PyRedisResponse(OriginType *p)            : PyWFBase(p) {}
    PyRedisResponse(const PyRedisResponse &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    void move_to(PyRedisResponse &o) {
        *(o.get()) = std::move(*(this->get()));
    }

    protocol::RedisValue get_result() const {
        protocol::RedisValue v;
        this->get()->get_result(v);
        return v;
    }

    bool set_result(const protocol::RedisValue &v) {
        return this->get()->set_result(v);
    }

    void set_size_limit(size_t limit) { this->get()->set_size_limit(limit); }
    size_t get_size_limit() const     { return this->get()->get_size_limit(); }
};

using PyWFRedisTask       = PyWFNetworkTask<PyRedisRequest, PyRedisResponse>;
using PyWFRedisServer     = PyWFServer<PyRedisRequest, PyRedisResponse>;
using py_redis_callback_t = std::function<void(PyWFRedisTask)>;
using py_redis_process_t  = std::function<void(PyWFRedisTask)>;

#endif // PYWF_REDIS_TYPES_H
