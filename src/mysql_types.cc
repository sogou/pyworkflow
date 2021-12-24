#include "mysql_types.h"
using namespace std;

PyWFMySQLTask create_mysql_task(const std::string &url, int retry_max, py_mysql_callback_t cb) {
    WFMySQLTask *ptr = WFTaskFactory::create_mysql_task(url, retry_max, nullptr);
    PyWFMySQLTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

std::string mysql_datatype2str(int data_type) {
    std::string str;
    const char *p = datatype2str(data_type);
    if(p) str.assign(p);
    return p;
}

void init_mysql_types(py::module_ &wf) {
    if(!PyDateTimeAPI) { PyDateTime_IMPORT; }

    // MySQL status constant
    wf.attr("MYSQL_STATUS_NOT_INIT")   = (int)MYSQL_STATUS_NOT_INIT;
    wf.attr("MYSQL_STATUS_OK")         = (int)MYSQL_STATUS_OK;
    wf.attr("MYSQL_STATUS_GET_RESULT") = (int)MYSQL_STATUS_GET_RESULT;
    wf.attr("MYSQL_STATUS_ERROR")      = (int)MYSQL_STATUS_ERROR;
    wf.attr("MYSQL_STATUS_END")        = (int)MYSQL_STATUS_END;

    // MySQL type constant
    wf.attr("MYSQL_TYPE_DECIMAL")     = (int)MYSQL_TYPE_DECIMAL;
    wf.attr("MYSQL_TYPE_TINY")        = (int)MYSQL_TYPE_TINY;
    wf.attr("MYSQL_TYPE_SHORT")       = (int)MYSQL_TYPE_SHORT;
    wf.attr("MYSQL_TYPE_LONG")        = (int)MYSQL_TYPE_LONG;
    wf.attr("MYSQL_TYPE_FLOAT")       = (int)MYSQL_TYPE_FLOAT;
    wf.attr("MYSQL_TYPE_DOUBLE")      = (int)MYSQL_TYPE_DOUBLE;
    wf.attr("MYSQL_TYPE_NULL")        = (int)MYSQL_TYPE_NULL;
    wf.attr("MYSQL_TYPE_TIMESTAMP")   = (int)MYSQL_TYPE_TIMESTAMP;
    wf.attr("MYSQL_TYPE_LONGLONG")    = (int)MYSQL_TYPE_LONGLONG;
    wf.attr("MYSQL_TYPE_INT24")       = (int)MYSQL_TYPE_INT24;
    wf.attr("MYSQL_TYPE_DATE")        = (int)MYSQL_TYPE_DATE;
    wf.attr("MYSQL_TYPE_TIME")        = (int)MYSQL_TYPE_TIME;
    wf.attr("MYSQL_TYPE_DATETIME")    = (int)MYSQL_TYPE_DATETIME;
    wf.attr("MYSQL_TYPE_YEAR")        = (int)MYSQL_TYPE_YEAR;
    wf.attr("MYSQL_TYPE_NEWDATE")     = (int)MYSQL_TYPE_NEWDATE;
    wf.attr("MYSQL_TYPE_VARCHAR")     = (int)MYSQL_TYPE_VARCHAR;
    wf.attr("MYSQL_TYPE_BIT")         = (int)MYSQL_TYPE_BIT;
    wf.attr("MYSQL_TYPE_TIMESTAMP2")  = (int)MYSQL_TYPE_TIMESTAMP2;
    wf.attr("MYSQL_TYPE_DATETIME2")   = (int)MYSQL_TYPE_DATETIME2;
    wf.attr("MYSQL_TYPE_TIME2")       = (int)MYSQL_TYPE_TIME2;
    wf.attr("MYSQL_TYPE_TYPED_ARRAY") = (int)MYSQL_TYPE_TYPED_ARRAY;
    wf.attr("MYSQL_TYPE_JSON")        = (int)MYSQL_TYPE_JSON;
    wf.attr("MYSQL_TYPE_NEWDECIMAL")  = (int)MYSQL_TYPE_NEWDECIMAL;
    wf.attr("MYSQL_TYPE_ENUM")        = (int)MYSQL_TYPE_ENUM;
    wf.attr("MYSQL_TYPE_SET")         = (int)MYSQL_TYPE_SET;
    wf.attr("MYSQL_TYPE_TINY_BLOB")   = (int)MYSQL_TYPE_TINY_BLOB;
    wf.attr("MYSQL_TYPE_MEDIUM_BLOB") = (int)MYSQL_TYPE_MEDIUM_BLOB;
    wf.attr("MYSQL_TYPE_LONG_BLOB")   = (int)MYSQL_TYPE_LONG_BLOB;
    wf.attr("MYSQL_TYPE_BLOB")        = (int)MYSQL_TYPE_BLOB;
    wf.attr("MYSQL_TYPE_VAR_STRING")  = (int)MYSQL_TYPE_VAR_STRING;
    wf.attr("MYSQL_TYPE_STRING")      = (int)MYSQL_TYPE_STRING;
    wf.attr("MYSQL_TYPE_GEOMETRY")    = (int)MYSQL_TYPE_GEOMETRY;

    // MySQL packet constant
    wf.attr("MYSQL_PACKET_OTHER")        = (int)MYSQL_PACKET_OTHER;
    wf.attr("MYSQL_PACKET_OK")           = (int)MYSQL_PACKET_OK;
    wf.attr("MYSQL_PACKET_NULL")         = (int)MYSQL_PACKET_NULL;
    wf.attr("MYSQL_PACKET_EOF")          = (int)MYSQL_PACKET_EOF;
    wf.attr("MYSQL_PACKET_ERROR")        = (int)MYSQL_PACKET_ERROR;
    wf.attr("MYSQL_PACKET_GET_RESULT")   = (int)MYSQL_PACKET_GET_RESULT;
    wf.attr("MYSQL_PACKET_LOCAL_INLINE") = (int)MYSQL_PACKET_LOCAL_INLINE;

    py::class_<PyMySQLRequest, PyWFBase>(wf, "MySQLRequest")
        .def("is_null",        &PyMySQLRequest::is_null)
        .def("move_to",        &PyMySQLRequest::move_to)
        .def("set_query",      &PyMySQLRequest::set_query)
        .def("get_query",      &PyMySQLRequest::get_query)
        .def("query_is_unset", &PyMySQLRequest::query_is_unset)
        .def("get_seqid",      &PyMySQLRequest::get_seqid)
        .def("set_seqid",      &PyMySQLRequest::set_seqid)
        .def("get_command",    &PyMySQLRequest::get_command)
        .def("set_size_limit", &PyMySQLRequest::set_size_limit)
        .def("get_size_limit", &PyMySQLRequest::get_size_limit)
    ;

    py::class_<PyMySQLResponse, PyWFBase>(wf, "MySQLResponse")
        .def("is_null",            &PyMySQLResponse::is_null)
        .def("move_to",            &PyMySQLResponse::move_to)
        .def("is_ok_packet",       &PyMySQLResponse::is_ok_packet)
        .def("is_error_packet",    &PyMySQLResponse::is_error_packet)
        .def("get_packet_type",    &PyMySQLResponse::get_packet_type)
        .def("get_affected_rows",  &PyMySQLResponse::get_affected_rows)
        .def("get_last_insert_id", &PyMySQLResponse::get_last_insert_id)
        .def("get_warnings",       &PyMySQLResponse::get_warnings)
        .def("get_error_code",     &PyMySQLResponse::get_error_code)
        .def("get_error_msg",      &PyMySQLResponse::get_error_msg)
        .def("get_sql_state",      &PyMySQLResponse::get_sql_state)
        .def("get_info",           &PyMySQLResponse::get_info)
        .def("set_ok_packet",      &PyMySQLResponse::set_ok_packet)
        .def("get_seqid",          &PyMySQLResponse::get_seqid)
        .def("set_seqid",          &PyMySQLResponse::set_seqid)
        .def("get_command",        &PyMySQLResponse::get_command)
        .def("set_size_limit",     &PyMySQLResponse::set_size_limit)
        .def("get_size_limit",     &PyMySQLResponse::get_size_limit)
    ;

    py::class_<PyMySQLCell>(wf, "MySQLCell")
        .def("__str__",       &PyMySQLCell::__str__)
        .def("get_data_type", &PyMySQLCell::get_data_type)
        .def("is_null",       &PyMySQLCell::is_null)
        .def("is_int",        &PyMySQLCell::is_int)
        .def("is_string",     &PyMySQLCell::is_string)
        .def("is_float",      &PyMySQLCell::is_float)
        .def("is_double",     &PyMySQLCell::is_double)
        .def("is_ulonglong",  &PyMySQLCell::is_ulonglong)
        .def("is_date",       &PyMySQLCell::is_date)
        .def("is_time",       &PyMySQLCell::is_time)
        .def("is_datetime",   &PyMySQLCell::is_datetime)
        .def("as_int",        &PyMySQLCell::as_int)
        .def("as_float",      &PyMySQLCell::as_float)
        .def("as_double",     &PyMySQLCell::as_double)
        .def("as_ulonglong",  &PyMySQLCell::as_ulonglong)
        .def("as_string",     &PyMySQLCell::as_string)
        .def("as_date",       &PyMySQLCell::as_date)
        .def("as_time",       &PyMySQLCell::as_time)
        .def("as_datetime",   &PyMySQLCell::as_datetime)
        .def("as_bytes",      &PyMySQLCell::as_bytes)
        .def("as_object",     &PyMySQLCell::as_object)
    ;

    py::class_<PyMySQLField, PyWFBase>(wf, "MySQLField")
        .def("__str__",       &PyMySQLField::__str__)
        .def("__repr__",      &PyMySQLField::__repr__)
        .def("get_name",      &PyMySQLField::get_name)
        .def("get_org_name",  &PyMySQLField::get_org_name)
        .def("get_table",     &PyMySQLField::get_table)
        .def("get_org_table", &PyMySQLField::get_org_table)
        .def("get_db",        &PyMySQLField::get_db)
        .def("get_catalog",   &PyMySQLField::get_catalog)
        .def("get_def",       &PyMySQLField::get_def)
        .def("get_charsetnr", &PyMySQLField::get_charsetnr)
        .def("get_length",    &PyMySQLField::get_length)
        .def("get_flags",     &PyMySQLField::get_flags)
        .def("get_decimals",  &PyMySQLField::get_decimals)
        .def("get_data_type", &PyMySQLField::get_data_type)
    ;

    py::class_<PyMySQLResultCursor>(wf, "MySQLResultCursor")
        .def(py::init<PyMySQLResponse&>())
        .def("next_result_set",   &PyMySQLResultCursor::next_result_set)
        .def("first_result_set",  &PyMySQLResultCursor::first_result_set)
        .def("fetch_fields",      &PyMySQLResultCursor::fetch_fields)
        .def("fetch_row",         &PyMySQLResultCursor::fetch_row)
        .def("fetch_all",         &PyMySQLResultCursor::fetch_all)
        .def("get_cursor_status", &PyMySQLResultCursor::get_cursor_status)
        .def("get_server_status", &PyMySQLResultCursor::get_server_status)
        .def("get_field_count",   &PyMySQLResultCursor::get_field_count)
        .def("get_rows_count",    &PyMySQLResultCursor::get_rows_count)
        .def("get_affected_rows", &PyMySQLResultCursor::get_affected_rows)
        .def("get_insert_id",     &PyMySQLResultCursor::get_insert_id)
        .def("get_warnings",      &PyMySQLResultCursor::get_warnings)
        .def("get_info",          &PyMySQLResultCursor::get_info)
        .def("rewind",            &PyMySQLResultCursor::rewind)
    ;

    py::class_<PyWFMySQLTask, PySubTask>(wf, "MySQLTask")
        .def("is_null",             &PyWFMySQLTask::is_null)
        .def("start",               &PyWFMySQLTask::start)
        .def("dismiss",             &PyWFMySQLTask::dismiss)
        .def("noreply",             &PyWFMySQLTask::noreply)
        .def("get_req",             &PyWFMySQLTask::get_req)
        .def("get_resp",            &PyWFMySQLTask::get_resp)
        .def("get_state",           &PyWFMySQLTask::get_state)
        .def("get_error",           &PyWFMySQLTask::get_error)
        .def("get_timeout_reason",  &PyWFMySQLTask::get_timeout_reason)
        .def("get_task_seq",        &PyWFMySQLTask::get_task_seq)
        .def("set_send_timeout",    &PyWFMySQLTask::set_send_timeout)
        .def("set_receive_timeout", &PyWFMySQLTask::set_receive_timeout)
        .def("set_keep_alive",      &PyWFMySQLTask::set_keep_alive)
        .def("get_peer_addr",       &PyWFMySQLTask::get_peer_addr)
        .def("set_callback",        &PyWFMySQLTask::set_callback)
        .def("set_user_data",       &PyWFMySQLTask::set_user_data)
        .def("get_user_data",       &PyWFMySQLTask::get_user_data)
    ;

    py::class_<PyWFMySQLConnection>(wf, "MySQLConnection")
        .def(py::init<int>())
        .def("init",                   &PyWFMySQLConnection::init)
        .def("deinit",                 &PyWFMySQLConnection::deinit)
        .def("create_query_task",      &PyWFMySQLConnection::create_query_task)
        .def("create_disconnect_task", &PyWFMySQLConnection::create_disconnect_task)
    ;

    py::class_<PyWFMySQLServer>(wf, "MySQLServer")
        .def(py::init<py_mysql_process_t>())
        .def(py::init<WFServerParams, py_mysql_process_t>())
        .def("start",       &PyWFMySQLServer::start_1, py::arg("port"), py::arg("cert_file") = std::string(),
                             py::arg("key_file") = std::string())
        .def("start",       &PyWFMySQLServer::start_2, py::arg("family"), py::arg("host"), py::arg("port"),
                             py::arg("cert_file") = std::string(), py::arg("key_file") = std::string())
        .def("shutdown",    &PyWFMySQLServer::shutdown, py::call_guard<py::gil_scoped_release>())
        .def("wait_finish", &PyWFMySQLServer::wait_finish, py::call_guard<py::gil_scoped_release>())
        .def("stop",        &PyWFMySQLServer::stop, py::call_guard<py::gil_scoped_release>())
    ;

    wf.def("mysql_datatype2str", &mysql_datatype2str, py::arg("datatype"));

    wf.def("create_mysql_task", &create_mysql_task, py::arg("url"), py::arg("retry_max"),
                                 py::arg("callback"));
}
