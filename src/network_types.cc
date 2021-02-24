#include "network_types.h"

void init_http_types(py::module_&);

void init_network_types(py::module_ &wf) {
    py::class_<WFServerParams>(wf, "ServerParams")
        .def(py::init([](){ return SERVER_PARAMS_DEFAULT; }))
        .def_readwrite("max_connections",       &WFServerParams::max_connections)
        .def_readwrite("peer_response_timeout", &WFServerParams::peer_response_timeout)
        .def_readwrite("receive_timeout",       &WFServerParams::receive_timeout)
        .def_readwrite("keep_alive_timeout",    &WFServerParams::keep_alive_timeout)
        .def_readwrite("request_size_limit",    &WFServerParams::request_size_limit)
        .def_readwrite("ssl_accept_timeout",    &WFServerParams::ssl_accept_timeout)
    ;

    init_http_types(wf);
}
