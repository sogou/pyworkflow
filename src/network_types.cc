#include "network_types.h"

PyWFHttpTask create_http_task(const std::string &url, int redirect_max, int retry_max, py_http_callback_t cb) {
    WFHttpTask *ptr = WFTaskFactory::create_http_task(url, redirect_max, retry_max, nullptr);
    PyWFHttpTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}
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
    py::class_<PyWFHttpTask, PySubTask>(wf, "HttpTask")
        .def("start",               &PyWFHttpTask::start)
        .def("dismiss",             &PyWFHttpTask::dismiss)
        .def("noreply",             &PyWFHttpTask::noreply)
        .def("get_req",             &PyWFHttpTask::get_req)
        .def("get_resp",            &PyWFHttpTask::get_resp)
        .def("get_state",           &PyWFHttpTask::get_state)
        .def("get_error",           &PyWFHttpTask::get_error)
        .def("get_timeout_reason",  &PyWFHttpTask::get_timeout_reason)
        .def("get_task_seq",        &PyWFHttpTask::get_task_seq)
        .def("set_send_timeout",    &PyWFHttpTask::set_send_timeout)
        .def("set_receive_timeout", &PyWFHttpTask::set_receive_timeout)
        .def("set_keep_alive",      &PyWFHttpTask::set_keep_alive)
        .def("set_callback",        &PyWFHttpTask::set_callback)
        .def("set_user_data",       &PyWFHttpTask::set_user_data)
        .def("get_user_data",       &PyWFHttpTask::get_user_data)
    ;
    py::class_<CopyableHttpRequest>(wf, "CopyableHttpRequest")
        .def(py::init())
        .def("is_chunked",       &CopyableHttpRequest::is_chunked)
        .def("is_keep_alive",    &CopyableHttpRequest::is_keep_alive)
        .def("get_method",       &CopyableHttpRequest::get_method)
        .def("get_request_uri",  &CopyableHttpRequest::get_request_uri)
        .def("get_http_version", &CopyableHttpRequest::get_http_version)
        .def("get_headers",      &CopyableHttpRequest::get_headers)
        .def("get_parsed_body",  &CopyableHttpRequest::get_parsed_body)
    ;
    py::class_<CopyableHttpResponse>(wf, "CopyableHttpResponse")
        .def(py::init())
        .def("is_chunked",        &CopyableHttpResponse::is_chunked)
        .def("is_keep_alive",     &CopyableHttpResponse::is_keep_alive)
        .def("get_status_code",   &CopyableHttpResponse::get_status_code)
        .def("get_reason_phrase", &CopyableHttpResponse::get_reason_phrase)
        .def("get_http_version",  &CopyableHttpResponse::get_http_version)
        .def("get_headers",       &CopyableHttpResponse::get_headers)
        .def("get_parsed_body",   &CopyableHttpResponse::get_parsed_body)
    ;
    py::class_<PyHttpMessage, PyWFBase>(wf, "HttpMessage"); // Just export a class name
    py::class_<PyHttpRequest, PyHttpMessage>(wf, "HttpRequest")
        //.def("copy",                 &PyHttpRequest::copy)
        .def("move_to",              &PyHttpRequest::move_to)
        .def("is_chunked",           &PyHttpRequest::is_chunked)
        .def("is_keep_alive",        &PyHttpRequest::is_keep_alive)
        .def("get_method",           &PyHttpRequest::get_method)
        .def("get_request_uri",      &PyHttpRequest::get_request_uri)
        .def("get_http_version",     &PyHttpRequest::get_http_version)
        .def("get_headers",          &PyHttpRequest::get_headers)
        .def("get_body",             &PyHttpRequest::get_body)
        .def("end_parsing",          &PyHttpRequest::end_parsing)
        .def("set_method",           &PyHttpRequest::set_method)
        .def("set_request_uri",      &PyHttpRequest::set_request_uri)
        .def("set_http_version",     &PyHttpRequest::set_http_version)
        .def("add_header_pair",      &PyHttpRequest::add_header_pair)
        .def("set_header_pair",      &PyHttpRequest::set_header_pair)
        .def("append_body",          static_cast<bool(PyHttpRequest::*)(py::bytes)>(&PyHttpRequest::append_body))
        .def("append_body",          static_cast<bool(PyHttpRequest::*)(py::str)>(&PyHttpRequest::append_body))
        .def("clear_body",           &PyHttpRequest::clear_output_body)
        .def("get_body_size",        &PyHttpRequest::get_output_body_size)
        .def("set_size_limit",       &PyHttpRequest::set_size_limit)
        .def("get_size_limit",       &PyHttpRequest::get_size_limit)
    ;
    py::class_<PyHttpResponse, PyHttpMessage>(wf, "HttpResponse")
        //.def("copy",                 &PyHttpResponse::copy)
        .def("move_to",              &PyHttpResponse::move_to)
        .def("is_chunked",           &PyHttpResponse::is_chunked)
        .def("is_keep_alive",        &PyHttpResponse::is_keep_alive)
        .def("get_status_code",      &PyHttpResponse::get_status_code)
        .def("get_reason_phrase",    &PyHttpResponse::get_reason_phrase)
        .def("get_http_version",     &PyHttpResponse::get_http_version)
        .def("get_headers",          &PyHttpResponse::get_headers)
        .def("get_body",             &PyHttpResponse::get_body)
        .def("end_parsing",          &PyHttpResponse::end_parsing)
        .def("set_status_code",      &PyHttpResponse::set_status_code)
        .def("set_reason_phrase",    &PyHttpResponse::set_reason_phrase)
        .def("set_http_version",     &PyHttpResponse::set_http_version)
        .def("add_header_pair",      &PyHttpResponse::add_header_pair)
        .def("set_header_pair",      &PyHttpResponse::set_header_pair)
        .def("append_body",          static_cast<bool(PyHttpResponse::*)(py::bytes)>(&PyHttpResponse::append_body))
        .def("append_body",          static_cast<bool(PyHttpResponse::*)(py::str)>(&PyHttpResponse::append_body))
        .def("clear_body",           &PyHttpResponse::clear_output_body)
        .def("get_body_size",        &PyHttpResponse::get_output_body_size)
        .def("set_size_limit",       &PyHttpResponse::set_size_limit)
        .def("get_size_limit",       &PyHttpResponse::get_size_limit)
    ;
    py::class_<PyWFHttpServer>(wf, "HttpServer")
        .def(py::init<py_http_process_t>())
        .def(py::init<WFServerParams, py_http_process_t>())
        .def("start", &PyWFHttpServer::start_1, py::arg("port"), py::arg("cert_file") = std::string(),
            py::arg("key_file") = std::string())
        .def("start", &PyWFHttpServer::start_2, py::arg("family"), py::arg("host"), py::arg("port"),
            py::arg("cert_file") = std::string(), py::arg("key_file") = std::string())
        .def("stop",  &PyWFHttpServer::stop)
    ;
    wf.def("create_http_task", &create_http_task, py::arg("url"), py::arg("redirect_max"),
        py::arg("retry_max"), py::arg("callback"));
}
