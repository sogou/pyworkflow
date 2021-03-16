#include "common_types.h"
#include "workflow/EndpointParams.h"
#include "workflow/WFGlobal.h"
#include "workflow/WFTask.h"
#include <sstream>

static constexpr struct WFGlobalSettings PYWF_GLOBAL_SETTINGS_DEFAULT =
{
    .endpoint_params = ENDPOINT_PARAMS_DEFAULT,
    .dns_ttl_default = 12 * 3600,
    .dns_ttl_min     = 180,
    .dns_threads     = 4,
    .poller_threads  = 4,
    .handler_threads = 4,
    .compute_threads = 4,
};

std::mutex CountableSeriesWork::series_mtx;
size_t CountableSeriesWork::series_counter = 0;
std::condition_variable CountableSeriesWork::series_cv;

PySeriesWork create_series_work(PySubTask &first, py_series_callback_t cb) {
    auto ptr = CountableSeriesWork::create_series_work(
        first.get(), [cb](const SeriesWork *p) {
        py_callback_wrapper(cb, PyConstSeriesWork(const_cast<SeriesWork*>(p)));
    });
    return PySeriesWork(ptr);
}
PySeriesWork create_series_work(PySubTask &first, PySubTask &last, py_series_callback_t cb) {
    auto ptr = CountableSeriesWork::create_series_work(
        first.get(), last.get(), [cb](const SeriesWork *p) {
        py_callback_wrapper(cb, PyConstSeriesWork(const_cast<SeriesWork*>(p)));
    });
    return PySeriesWork(ptr);
}
void start_series_work(PySubTask &first, py_series_callback_t cb) {
    CountableSeriesWork::start_series_work(
        first.get(), [cb](const SeriesWork *p) {
        py_callback_wrapper(cb, PyConstSeriesWork(const_cast<SeriesWork*>(p)));
    });
}
void start_series_work(PySubTask &first, PySubTask &last, py_series_callback_t cb) {
    CountableSeriesWork::start_series_work(
        first.get(), last.get(), [cb](const SeriesWork *p) {
        py_callback_wrapper(cb, PyConstSeriesWork(const_cast<SeriesWork*>(p)));
    });
}

PyParallelWork create_parallel_work(py_parallel_callback_t cb) {
    auto ptr = CountableParallelWork::create_parallel_work([cb](const ParallelWork *p) {
        py_callback_wrapper(cb, PyConstParallelWork(const_cast<ParallelWork*>(p)));
    });
    return PyParallelWork(ptr);
}
PyParallelWork create_parallel_work(std::vector<PySeriesWork> &v, py_parallel_callback_t cb) {
    std::vector<SeriesWork*> works(v.size());
    for(size_t i = 0; i < v.size(); i++) works[i] = v[i].get();
    auto ptr = CountableParallelWork::create_parallel_work(works.data(), works.size(),
        [cb](const ParallelWork *p) {
        py_callback_wrapper(cb, PyConstParallelWork(const_cast<ParallelWork*>(p)));
    });
    return PyParallelWork(ptr);
}
void start_parallel_work(std::vector<PySeriesWork> &v, py_parallel_callback_t cb) {
    std::vector<SeriesWork*> works(v.size());
    for(size_t i = 0; i < v.size(); i++) works[i] = v[i].get();
    CountableParallelWork::start_parallel_work(works.data(), works.size(),
        [cb](const ParallelWork *p) {
        py_callback_wrapper(cb, PyConstParallelWork(const_cast<ParallelWork*>(p)));
    });
}

void PyWorkflow_library_init(const struct WFGlobalSettings s) {
    WORKFLOW_library_init(&s);
}

WFGlobalSettings get_global_settings() {
    return *WFGlobal::get_global_settings();
}

PySeriesWork py_series_of(const PySubTask &t) {
    SeriesWork *s = series_of(t.get());
    return PySeriesWork(s);
}

std::string get_error_string(int state, int error) {
    std::string s;
    const char *p = WFGlobal::get_error_string(state, error);
    if(p) s.assign(p);
    return s;
}

/**
 * Do some initialize before start.
 * The user should not call this function.
 */
void inner_init() {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    OPENSSL_init_ssl(0, NULL);
#endif
    WORKFLOW_library_init(&PYWF_GLOBAL_SETTINGS_DEFAULT);
}

void init_common_types(py::module_ &wf) {
    wf.attr("WFT_STATE_UNDEFINED")   = (int)WFT_STATE_UNDEFINED;
    wf.attr("WFT_STATE_SUCCESS")     = (int)WFT_STATE_SUCCESS;
    wf.attr("WFT_STATE_TOREPLY")     = (int)WFT_STATE_TOREPLY;
    wf.attr("WFT_STATE_NOREPLY")     = (int)WFT_STATE_NOREPLY;
    wf.attr("WFT_STATE_SYS_ERROR")   = (int)WFT_STATE_SYS_ERROR;
    wf.attr("WFT_STATE_SSL_ERROR")   = (int)WFT_STATE_SSL_ERROR;
    wf.attr("WFT_STATE_DNS_ERROR")   = (int)WFT_STATE_DNS_ERROR;
    wf.attr("WFT_STATE_TASK_ERROR")  = (int)WFT_STATE_TASK_ERROR;
    wf.attr("WFT_STATE_ABORTED")     = (int)WFT_STATE_ABORTED;

    wf.attr("TOR_NOT_TIMEOUT")       = (int)TOR_NOT_TIMEOUT;
    wf.attr("TOR_WAIT_TIMEOUT")      = (int)TOR_WAIT_TIMEOUT;
    wf.attr("TOR_CONNECT_TIMEOUT")   = (int)TOR_CONNECT_TIMEOUT;
    wf.attr("TOR_TRANSMIT_TIMEOUT")  = (int)TOR_TRANSMIT_TIMEOUT;

    py::class_<EndpointParams>(wf, "EndpointParams")
        .def(py::init([]() { return ENDPOINT_PARAMS_DEFAULT; }))
        .def("__str__", [](const EndpointParams &self) -> std::string {
            std::ostringstream oss;
            oss << "EndpointParams {"
            << "max_connections: " << self.max_connections
            << ", connect_timeout: " << self.connect_timeout
            << ", response_timeout: " << self.response_timeout
            << ", ssl_connect_timeout: " << self.ssl_connect_timeout
            << "}";
            return oss.str();
        })
        .def_readwrite("max_connections",     &EndpointParams::max_connections)
        .def_readwrite("connect_timeout",     &EndpointParams::connect_timeout)
        .def_readwrite("response_timeout",    &EndpointParams::response_timeout)
        .def_readwrite("ssl_connect_timeout", &EndpointParams::ssl_connect_timeout)
    ;
    py::class_<WFGlobalSettings>(wf, "GlobalSettings")
        .def(py::init([]() { return PYWF_GLOBAL_SETTINGS_DEFAULT; }))
        .def("__str__", [](const WFGlobalSettings &self) -> std::string {
            (void)self;
            std::ostringstream oss;
            oss << "GlobalSettings {EndpointParams {"
            << "max_connections: " << self.endpoint_params.max_connections
            << ", connect_timeout: " << self.endpoint_params.connect_timeout
            << ", response_timeout: " << self.endpoint_params.response_timeout
            << ", ssl_connect_timeout: " << self.endpoint_params.ssl_connect_timeout
            << "}, dns_ttl_default: " << self.dns_ttl_default
            << ", dns_ttl_min: " << self.dns_ttl_min
            << ", poller_threads: " << self.poller_threads
            << ", handler_threads: " << self.handler_threads
            << ", compute_threads: " << self.compute_threads
            << "}";
            return oss.str();
        })
        .def_readwrite("endpoint_params", &WFGlobalSettings::endpoint_params)
        .def_readwrite("dns_ttl_default", &WFGlobalSettings::dns_ttl_default)
        .def_readwrite("dns_ttl_min",     &WFGlobalSettings::dns_ttl_min)
        .def_readwrite("dns_threads",     &WFGlobalSettings::dns_threads)
        .def_readwrite("poller_threads",  &WFGlobalSettings::poller_threads)
        .def_readwrite("handler_threads", &WFGlobalSettings::handler_threads)
        .def_readwrite("compute_threads", &WFGlobalSettings::compute_threads)
    ;
    py::class_<PyWFBase>(wf, "WFBase");
    py::class_<PySubTask, PyWFBase>(wf, "SubTask");

    py::class_<PyConstSeriesWork, PyWFBase>(wf, "ConstSeriesWork")
        .def("is_null",     &PyConstSeriesWork::is_null)
        .def("is_canceled", &PyConstSeriesWork::is_canceled)
        .def("get_context", &PyConstSeriesWork::get_context)
    ;
    py::class_<PyConstParallelWork, PySubTask>(wf, "ConstParallelWork")
        .def("is_null",     &PyConstParallelWork::is_null)
        .def("series_at",   &PyConstParallelWork::series_at)
        .def("get_context", &PyConstParallelWork::get_context)
        .def("size",        &PyConstParallelWork::size)
    ;

    py::class_<PySeriesWork, PyWFBase>(wf, "SeriesWork")
        .def("is_null",      &PySeriesWork::is_null)
        .def("__lshift__",   &PySeriesWork::operator<<)
        .def("start",        &PySeriesWork::start)
        .def("dismiss",      &PySeriesWork::dismiss)
        .def("push_back",    &PySeriesWork::push_back)
        .def("push_front",   &PySeriesWork::push_front)
        .def("cancel",       &PySeriesWork::cancel)
        .def("is_canceled",  &PySeriesWork::is_canceled)
        .def("set_callback", &PySeriesWork::set_callback)
        .def("set_context",  &PySeriesWork::set_context)
        .def("get_context",  &PySeriesWork::get_context)
    ;
    py::class_<PyParallelWork, PySubTask>(wf, "ParallelWork")
        .def("__mul__", [](PyParallelWork &self, PySeriesWork &t) -> PyParallelWork& {
            self.add_series(t);
            return self;
        })
        .def("is_null",      &PyParallelWork::is_null)
        .def("start",        &PyParallelWork::start)
        .def("dismiss",      &PyParallelWork::dismiss)
        .def("add_series",   &PyParallelWork::add_series)
        .def("series_at",    &PyParallelWork::series_at)
        .def("size",         &PyParallelWork::size)
        .def("set_callback", &PyParallelWork::set_callback)
        .def("set_context",  &PyParallelWork::set_context)
        .def("get_context",  &PyParallelWork::get_context)
    ;

    wf.def("WORKFLOW_library_init", &PyWorkflow_library_init);
    wf.def("get_global_settings",   &get_global_settings);
    wf.def("series_of",             &py_series_of);

    wf.def("create_series_work",
        static_cast<PySeriesWork(*)(PySubTask&, py_series_callback_t)>(&create_series_work),
        py::arg("first"), py::arg("callback")
    );
    wf.def("create_series_work",
        static_cast<PySeriesWork(*)(PySubTask&, PySubTask&, py_series_callback_t)>(&create_series_work),
        py::arg("first"), py::arg("last"), py::arg("callback")
    );
    wf.def("start_series_work",
        static_cast<void(*)(PySubTask&, py_series_callback_t)>(&start_series_work),
        py::arg("first"), py::arg("callback")
    );
    wf.def("start_series_work",
        static_cast<void(*)(PySubTask&, PySubTask&, py_series_callback_t)>(&start_series_work),
        py::arg("first"), py::arg("last"), py::arg("callback")
    );
    wf.def("create_parallel_work",
        static_cast<PyParallelWork(*)(py_parallel_callback_t)>(&create_parallel_work),
        py::arg("callback")
    );
    wf.def("create_parallel_work",
        static_cast<PyParallelWork(*)(std::vector<PySeriesWork>&, py_parallel_callback_t)>(&create_parallel_work),
        py::arg("all_series"), py::arg("callback")
    );
    wf.def("start_parallel_work", &start_parallel_work, py::arg("all_series"), py::arg("callback"));
    wf.def("wait_finish",         &CountableSeriesWork::wait_finish, py::call_guard<py::gil_scoped_release>());
    wf.def("wait_finish_timeout", &CountableSeriesWork::wait_finish_timeout, py::call_guard<py::gil_scoped_release>());
    wf.def("get_error_string",    &get_error_string, py::arg("state"), py::arg("error"));
    wf.def("inner_init",          &inner_init);
}
