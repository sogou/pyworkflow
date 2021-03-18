#include "redis_types.h"
using namespace std;

using protocol::RedisValue;
py::object redis_as_object(RedisValue &value) {
    switch(value.get_type()) {
    case REDIS_REPLY_TYPE_STRING:
    case REDIS_REPLY_TYPE_STATUS:
    case REDIS_REPLY_TYPE_ERROR:
        return py::bytes(value.string_value());
    case REDIS_REPLY_TYPE_ARRAY:
    {
        py::list lst;
        for(size_t i = 0; i < value.arr_size(); i++) {
            lst.append(redis_as_object(value[i]));
        }
        return static_cast<py::object>(lst);
    }
    case REDIS_REPLY_TYPE_INTEGER:
        return py::int_(value.int_value());
    case REDIS_REPLY_TYPE_NIL:
    default:
        return py::none();
    }
}
void redis_set_string(RedisValue &value, const string &s) {
    value.set_string(s);
}
void redis_set_status(RedisValue &value, const string &s) {
    value.set_status(s);
}
void redis_set_error(RedisValue &value, const string &s) {
    value.set_error(s);
}

py::bytes redis_bytes_value(RedisValue &value) {
    const std::string *sv = value.string_view();
    if(sv == nullptr) return py::bytes();
    else return py::bytes(*sv);
}
py::bytes redis_debug_bytes(RedisValue &value) {
    std::string str = value.debug_string();
    return py::bytes(str);
}

py::object redis_arr_at_ref(RedisValue &value, size_t pos) {
    if(pos >= value.arr_size()) return py::none();
    auto &v = value.arr_at(pos);
    return py::cast(v, py::return_value_policy::reference);
}
py::object redis_arr_at_object(RedisValue &value, size_t pos) {
    if(pos >= value.arr_size()) return py::none();
    return redis_as_object(value.arr_at(pos));
}
py::object redis_arr_at(RedisValue &value, size_t pos) {
    if(pos >= value.arr_size()) return py::none();
    return py::cast(value.arr_at(pos), py::return_value_policy::copy);
}
void redis_arr_set(RedisValue &value, size_t pos, RedisValue &v) {
    // if pos is invalid, do nothing
    if(pos < value.arr_size()) {
        value.arr_at(pos) = v;
    }
}

RedisValue redis_copy(RedisValue &value) {
    return value;
}
void redis_move_to(RedisValue &value, RedisValue &o) {
    o = std::move(value);
}
void redis_move_from(RedisValue &value, RedisValue &o) {
    value = std::move(o);
}

PyWFRedisTask create_redis_task(const std::string &url, int retry_max, py_redis_callback_t cb) {
    WFRedisTask *ptr = WFTaskFactory::create_redis_task(url, retry_max, nullptr);
    PyWFRedisTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

void init_redis_types(py::module_ &wf) {
    py::class_<RedisValue::StatusTag>(wf, "RedisValueStatusTag")
        .def(py::init())
    ;
    py::class_<RedisValue::ErrorTag>(wf, "RedisValueErrorTag")
        .def(py::init())
    ;

    py::class_<RedisValue>(wf, "RedisValue")
        .def(py::init())
        .def(py::init<const RedisValue&>())
        .def(py::init<int64_t>())
        .def(py::init<std::string>())
        .def(py::init<std::string, RedisValue::StatusTag>())
        .def(py::init<std::string, RedisValue::ErrorTag>())

        .def("__len__",       &RedisValue::arr_size)
        .def("__getitem__",   &redis_arr_at_ref)
        .def("__setitem__",   &redis_arr_set)
        .def("__copy__",      &redis_copy)
        .def("__deepcopy__",  &redis_copy)

        .def("copy",          &redis_copy)
        .def("move_to",       &redis_move_to)
        .def("move_from",     &redis_move_from)

        .def("set_nil",       &RedisValue::set_nil)
        .def("set_int",       &RedisValue::set_int)
        .def("set_array",     &RedisValue::set_array)
        .def("set_string",    &redis_set_string)
        .def("set_status",    &redis_set_status)
        .def("set_error",     &redis_set_error)

        .def("is_ok",         &RedisValue::is_ok)
        .def("is_error",      &RedisValue::is_error)
        .def("is_nil",        &RedisValue::is_nil)
        .def("is_int",        &RedisValue::is_int)
        .def("is_array",      &RedisValue::is_array)
        .def("is_string",     &RedisValue::is_string)

        .def("string_value",  &redis_bytes_value)
        .def("int_value",     &RedisValue::int_value)
        .def("arr_size",      &RedisValue::arr_size)
        .def("arr_clear",     &RedisValue::arr_clear)
        .def("arr_resize",    &RedisValue::arr_resize)

        .def("clear",         &RedisValue::clear)
        .def("debug_string",  &redis_debug_bytes)
        .def("arr_at",        &redis_arr_at)
        .def("arr_at_ref",    &redis_arr_at_ref)
        .def("arr_at_object", &redis_arr_at_object)
        .def("as_object",     &redis_as_object)
    ;

    py::class_<PyWFRedisTask, PySubTask>(wf, "RedisTask")
        .def("is_null",             &PyWFRedisTask::is_null)
        .def("start",               &PyWFRedisTask::start)
        .def("dismiss",             &PyWFRedisTask::dismiss)
        .def("noreply",             &PyWFRedisTask::noreply)
        .def("get_req",             &PyWFRedisTask::get_req)
        .def("get_resp",            &PyWFRedisTask::get_resp)
        .def("get_state",           &PyWFRedisTask::get_state)
        .def("get_error",           &PyWFRedisTask::get_error)
        .def("get_timeout_reason",  &PyWFRedisTask::get_timeout_reason)
        .def("get_task_seq",        &PyWFRedisTask::get_task_seq)
        .def("set_send_timeout",    &PyWFRedisTask::set_send_timeout)
        .def("set_receive_timeout", &PyWFRedisTask::set_receive_timeout)
        .def("set_keep_alive",      &PyWFRedisTask::set_keep_alive)
        .def("set_callback",        &PyWFRedisTask::set_callback)
        .def("set_user_data",       &PyWFRedisTask::set_user_data)
        .def("get_user_data",       &PyWFRedisTask::get_user_data)
    ;

    py::class_<PyRedisRequest, PyWFBase>(wf, "RedisRequest")
        .def("is_null",        &PyRedisRequest::is_null)
        .def("move_to",        &PyRedisRequest::move_to)
        .def("set_request",    &PyRedisRequest::set_request)
        .def("get_command",    &PyRedisRequest::get_command)
        .def("get_params",     &PyRedisRequest::get_params)

        .def("set_size_limit", &PyRedisRequest::set_size_limit)
        .def("get_size_limit", &PyRedisRequest::get_size_limit)
    ;

    py::class_<PyRedisResponse, PyWFBase>(wf, "RedisResponse")
        .def("is_null",        &PyRedisResponse::is_null)
        .def("move_to",        &PyRedisResponse::move_to)
        .def("get_result",     &PyRedisResponse::get_result)
        .def("set_result",     &PyRedisResponse::set_result)

        .def("set_size_limit", &PyRedisResponse::set_size_limit)
        .def("get_size_limit", &PyRedisResponse::get_size_limit)
    ;

    py::class_<PyWFRedisServer>(wf, "RedisServer")
        .def(py::init<py_redis_process_t>())
        .def(py::init<WFServerParams, py_redis_process_t>())
        .def("start",       &PyWFRedisServer::start_1, py::arg("port"), py::arg("cert_file") = std::string(),
                             py::arg("key_file") = std::string())
        .def("start",       &PyWFRedisServer::start_2, py::arg("family"), py::arg("host"), py::arg("port"),
                             py::arg("cert_file") = std::string(), py::arg("key_file") = std::string())
        .def("shutdown",    &PyWFRedisServer::shutdown, py::call_guard<py::gil_scoped_release>())
        .def("wait_finish", &PyWFRedisServer::wait_finish, py::call_guard<py::gil_scoped_release>())
        .def("stop",        &PyWFRedisServer::stop, py::call_guard<py::gil_scoped_release>())
    ;

    wf.def("create_redis_task", &create_redis_task, py::arg("url"), py::arg("retry_max"),
        py::arg("callback"));
}
