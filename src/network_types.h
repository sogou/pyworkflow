#ifndef PYWF_NETWORK_TYPES_H
#define PYWF_NETWORK_TYPES_H
#include <arpa/inet.h>

#include "common_types.h"
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFHttpServer.h"

class __network_helper {
public:
    template<typename Task>
    static void client_prepare(Task*) {}
    static void client_prepare(WFHttpTask*);

    template<typename Task>
    static void server_prepare(Task*) {}
    static void server_prepare(WFHttpTask*);
};

template<class Req, class Resp>
class PyWFNetworkTask : public PySubTask {
public:
    using ReqType        = Req;
    using RespType       = Resp;
    using OriginType     = WFNetworkTask<typename Req::OriginType, typename Resp::OriginType>;
    using _py_callback_t = std::function<void(PyWFNetworkTask<Req, Resp>)>;
    using _deleter_t     = TaskDeleterWrapper<_py_callback_t, OriginType>;

    PyWFNetworkTask()                         : PySubTask()  {}
    PyWFNetworkTask(OriginType *p)            : PySubTask(p) {}
    PyWFNetworkTask(const PyWFNetworkTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }

    void dismiss()                  { this->get()->dismiss(); }
    void noreply()                  { this->get()->noreply(); }
    ReqType get_req()               { return ReqType(this->get()->get_req());   }
    RespType get_resp()             { return RespType(this->get()->get_resp()); }
    int get_state() const           { return this->get()->get_state();          }
    int get_error() const           { return this->get()->get_error();          }
    int get_timeout_reason() const  { return this->get()->get_timeout_reason(); }
    long long get_task_seq() const  { return this->get()->get_task_seq();       }
    void set_send_timeout(int t)    { this->get()->set_send_timeout(t); }
    void set_receive_timeout(int t) { this->get()->set_receive_timeout(t); }
    void set_keep_alive(int t)      { this->get()->set_keep_alive(t); }

    py::object get_peer_addr() const {
        char ip_str[INET6_ADDRSTRLEN + 1] = { 0 };
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof (addr);
        uint16_t port = 0;

        if (this->get()->get_peer_addr((struct sockaddr *)&addr, &addrlen) == 0) {
            if (addr.ss_family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)(&addr);
                inet_ntop(AF_INET, &sin->sin_addr, ip_str, addrlen);
                port = ntohs(sin->sin_port);
            }
            else if (addr.ss_family == AF_INET6) {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)(&addr);
                inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, addrlen);
                port = ntohs(sin6->sin6_port);
            }
        }
        return py::make_tuple(py::str(ip_str), py::int_(port));
    }

    void set_callback(_py_callback_t cb) {
        // The deleter will destruct both cb and user_data,
        // but now we just want to reset cb, so we must clear user_data first,
        // and set user_data at the end
        void *user_data = this->get()->user_data;
        this->get()->user_data = nullptr;
        auto deleter = std::make_shared<_deleter_t>(std::move(cb), this->get());
        this->get()->set_callback([deleter](OriginType *p) {
            __network_helper::client_prepare(p);
            py_callback_wrapper(deleter->get_func(), PyWFNetworkTask<Req, Resp>(p));
        });
        this->get()->user_data = user_data;
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

template<typename Req, typename Resp>
class PyWFServer {
public:
    using ReqType       = Req;
    using RespType      = Resp;
    using OriginType    = WFServer<typename Req::OriginType, typename Resp::OriginType>;
    using _py_process_t = std::function<void(PyWFNetworkTask<Req, Resp>)>;
    using _task_t       = WFNetworkTask<typename Req::OriginType, typename Resp::OriginType>;
    using _pytask_t     = PyWFNetworkTask<Req, Resp>;

    PyWFServer(_py_process_t proc)
        : process(std::move(proc)), server([this](_task_t *p) {
            __network_helper::server_prepare(p);
            py_callback_wrapper(this->process, _pytask_t(p));
        }) {}
    PyWFServer(WFServerParams params, _py_process_t proc)
        : process(std::move(proc)), server(&params, [this](_task_t *p) {
            __network_helper::server_prepare(p);
            py_callback_wrapper(this->process, _pytask_t(p));
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

    void shutdown()    { server.shutdown(); }
    void wait_finish() { server.wait_finish(); }
    void stop()        { server.stop(); }
    ~PyWFServer()      { release_wrapped_function(this->process); }

    _py_process_t process;
private:
    OriginType server;
};

#endif // PYWF_NETWORK_TYPES_H
