#include "other_types.h"

class GoTaskWrapper {
    struct Params {
        Params(const py::function &f, const py::args &a, const py::kwargs &kw)
            : f(f), a(a), kw(kw) {}
        ~Params() = default;
        py::function f;
        py::args a;
        py::kwargs kw;
    };
public:
    GoTaskWrapper(const py::function &f, const py::args &a, const py::kwargs &kw)
        : params(new Params(f, a, kw)) {}
    GoTaskWrapper(const GoTaskWrapper&) = delete;
    GoTaskWrapper& operator=(const GoTaskWrapper&) = delete;
    void go() {
        py::gil_scoped_acquire acquire;
        (params->f)(*(params->a), **(params->kw));
    }
    ~GoTaskWrapper() {
        py::gil_scoped_acquire acquire;
        delete params;
    }
private:
    Params *params;
};

PyWFFileIOTask create_pread_task(int fd, size_t count, off_t offset, py_fio_callback_t cb) {
    void *buf = malloc(count);
    FileIOTaskData *data = new FileIOTaskData(buf, nullptr);
    auto ptr = WFTaskFactory::create_pread_task(fd, buf, count, offset, nullptr);
    ptr->user_data = data;
    PyWFFileIOTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

PyWFFileIOTask create_pwrite_task(int fd, const py::bytes &b, size_t count, off_t offset,
    py_fio_callback_t cb) {
    char *buffer;
    ssize_t length;
    if(PYBIND11_BYTES_AS_STRING_AND_SIZE(b.ptr(), &buffer, &length)) {
        // there is an error
        return nullptr;
    }
    py::bytes *bytes = new py::bytes(b);
    size_t write_size = std::min(count, (size_t)length);
    FileIOTaskData *data = new FileIOTaskData(nullptr, bytes);
    auto ptr = WFTaskFactory::create_pwrite_task(fd, buffer, write_size, offset, nullptr);
    ptr->user_data = data;
    PyWFFileIOTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

PyWFFileVIOTask create_pwritev_task(int fd, const std::vector<py::bytes> &b, off_t offset,
    py_fvio_callback_t cb) {
    size_t size = b.size();
    struct iovec *iov = new iovec[size];
    py::object *bytes = new py::object[size];
    bool failed = false;
    for(size_t i = 0; i < size; i++) {
        char *buffer;
        ssize_t length;
        if(PYBIND11_BYTES_AS_STRING_AND_SIZE(b[i].ptr(), &buffer, &length)) {
            failed = true;
            break;
        }
        iov[i].iov_base = buffer;
        iov[i].iov_len = (size_t)length;
        bytes[i] = b[i];
    }
    if(failed) {
        delete iov;
        delete bytes;
        return nullptr;
    }
    FileVIOTaskData *data = new FileVIOTaskData(iov, false, bytes, size);
    auto ptr = WFTaskFactory::create_pwritev_task(fd, iov, (int)size, offset, nullptr);
    ptr->user_data = data;
    PyWFFileVIOTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

PyWFFileSyncTask create_fsync_task(int fd, py_fsync_callback_t cb) {
    FileTaskData *data = new FileTaskData();
    auto ptr = WFTaskFactory::create_fsync_task(fd, nullptr);
    ptr->user_data = data;
    PyWFFileSyncTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

PyWFFileSyncTask create_fdsync_task(int fd, py_fsync_callback_t cb) {
    FileTaskData *data = new FileTaskData();
    auto ptr = WFTaskFactory::create_fdsync_task(fd, nullptr);
    ptr->user_data = data;
    PyWFFileSyncTask t(ptr);
    t.set_callback(std::move(cb));
    return t;
}

PyWFTimerTask create_timer_task(unsigned int microseconds, py_timer_callback_t cb) {
    auto ptr = WFTaskFactory::create_timer_task(microseconds, nullptr);
    PyWFTimerTask task(ptr);
    task.set_callback(std::move(cb));
    return task;
}

PyWFCounterTask create_counter_task(const std::string &name, unsigned int target, py_counter_callback_t cb) {
    auto ptr = WFTaskFactory::create_counter_task(name, target, nullptr);
    PyWFCounterTask task(ptr);
    task.set_callback(std::move(cb));
    return task;
}

PyWFCounterTask create_counter_task_no_name(unsigned int target, py_counter_callback_t cb) {
    auto ptr = WFTaskFactory::create_counter_task(target, nullptr);
    PyWFCounterTask task(ptr);
    task.set_callback(std::move(cb));
    return task;
}

void count_by_name(const std::string &name, unsigned int n) {
    WFTaskFactory::count_by_name(name, n);
}

PyWFGoTask create_go_task_with_name(const std::string &name, py::function f, py::args a, py::kwargs kw) {
    auto deleter = std::make_shared<GoTaskWrapper>(f, a, kw);
    auto ptr = WFTaskFactory::create_go_task(name, [deleter]() {
        deleter->go();
    });
    PyWFGoTask task(ptr);
    task.set_callback(nullptr); // We need callback to delete user_data
    return task;
}

PyWFGoTask create_go_task(py::function f, py::args a, py::kwargs kw) {
    return create_go_task_with_name("pywf-default", f, a, kw);
}

PyWFEmptyTask create_empty_task() {
    auto ptr = WFTaskFactory::create_empty_task();
    PyWFEmptyTask task(ptr);
    return task;
}

void init_other_types(py::module_ &wf) {
    py::class_<PyWFFileIOTask, PySubTask>(wf, "FileIOTask")
        .def("is_null",       &PyWFFileIOTask::is_null)
        .def("start",         &PyWFFileIOTask::start)
        .def("dismiss",       &PyWFFileIOTask::dismiss)
        .def("get_state",     &PyWFFileIOTask::get_state)
        .def("get_error",     &PyWFFileIOTask::get_error)
        .def("get_retval",    &PyWFFileIOTask::get_retval)
        .def("set_callback",  &PyWFFileIOTask::set_callback)
        .def("set_user_data", &PyWFFileIOTask::set_user_data)
        .def("get_user_data", &PyWFFileIOTask::get_user_data)
        .def("get_fd",        &PyWFFileIOTask::get_fd)
        .def("get_offset",    &PyWFFileIOTask::get_offset)
        .def("get_count",     &PyWFFileIOTask::get_count)
        .def("get_data",      &PyWFFileIOTask::get_data)
    ;
    py::class_<PyWFFileVIOTask, PySubTask>(wf, "FileVIOTask")
        .def("is_null",       &PyWFFileVIOTask::is_null)
        .def("start",         &PyWFFileVIOTask::start)
        .def("dismiss",       &PyWFFileVIOTask::dismiss)
        .def("get_state",     &PyWFFileVIOTask::get_state)
        .def("get_error",     &PyWFFileVIOTask::get_error)
        .def("get_retval",    &PyWFFileVIOTask::get_retval)
        .def("set_callback",  &PyWFFileVIOTask::set_callback)
        .def("set_user_data", &PyWFFileVIOTask::set_user_data)
        .def("get_user_data", &PyWFFileVIOTask::get_user_data)
        .def("get_fd",        &PyWFFileVIOTask::get_fd)
        .def("get_offset",    &PyWFFileVIOTask::get_offset)
        .def("get_data",      &PyWFFileVIOTask::get_data)
    ;
    py::class_<PyWFFileSyncTask, PySubTask>(wf, "FileSyncTask")
        .def("is_null",       &PyWFFileSyncTask::is_null)
        .def("start",         &PyWFFileSyncTask::start)
        .def("dismiss",       &PyWFFileSyncTask::dismiss)
        .def("get_state",     &PyWFFileSyncTask::get_state)
        .def("get_error",     &PyWFFileSyncTask::get_error)
        .def("get_retval",    &PyWFFileSyncTask::get_retval)
        .def("set_callback",  &PyWFFileSyncTask::set_callback)
        .def("set_user_data", &PyWFFileSyncTask::set_user_data)
        .def("get_user_data", &PyWFFileSyncTask::get_user_data)
        .def("get_fd",        &PyWFFileSyncTask::get_fd)
    ;

    py::class_<PyWFTimerTask, PySubTask>(wf, "TimerTask")
        .def("is_null",       &PyWFTimerTask::is_null)
        .def("start",         &PyWFTimerTask::start)
        .def("dismiss",       &PyWFTimerTask::dismiss)
        .def("get_state",     &PyWFTimerTask::get_state)
        .def("get_error",     &PyWFTimerTask::get_error)
        .def("set_user_data", &PyWFTimerTask::set_user_data)
        .def("get_user_data", &PyWFTimerTask::get_user_data)
        .def("set_callback",  &PyWFTimerTask::set_callback)
    ;

    py::class_<PyWFCounterTask, PySubTask>(wf, "CounterTask")
        .def("is_null",       &PyWFCounterTask::is_null)
        .def("start",         &PyWFCounterTask::start)
        .def("dismiss",       &PyWFCounterTask::dismiss)
        .def("get_state",     &PyWFCounterTask::get_state)
        .def("get_error",     &PyWFCounterTask::get_error)
        .def("set_user_data", &PyWFCounterTask::set_user_data)
        .def("get_user_data", &PyWFCounterTask::get_user_data)
        .def("set_callback",  &PyWFCounterTask::set_callback)
        .def("count",         &PyWFCounterTask::count)
    ;

    py::class_<PyWFGoTask, PySubTask>(wf, "GoTask")
        .def("is_null",       &PyWFGoTask::is_null)
        .def("start",         &PyWFGoTask::start)
        .def("dismiss",       &PyWFGoTask::dismiss)
        .def("get_state",     &PyWFGoTask::get_state)
        .def("get_error",     &PyWFGoTask::get_error)
        .def("set_user_data", &PyWFGoTask::set_user_data)
        .def("get_user_data", &PyWFGoTask::get_user_data)
        .def("set_callback",  &PyWFGoTask::set_callback)
    ;

    py::class_<PyWFEmptyTask, PySubTask>(wf, "EmptyTask")
        .def("is_null",       &PyWFEmptyTask::is_null)
        .def("start",         &PyWFEmptyTask::start)
        .def("dismiss",       &PyWFEmptyTask::dismiss)
        .def("get_state",     &PyWFEmptyTask::get_state)
        .def("get_error",     &PyWFEmptyTask::get_error)
    ;

    py::class_<PyWaitGroup>(wf, "WaitGroup")
        .def(py::init<int>())
        .def("done", &PyWaitGroup::done, py::call_guard<py::gil_scoped_release>())
        .def("wait", &PyWaitGroup::wait, py::call_guard<py::gil_scoped_release>())
    ;

    wf.def("create_pread_task",   &create_pread_task, py::arg("fd"), py::arg("count"),
                                   py::arg("offset"), py::arg("callback"));
    wf.def("create_pwrite_task",  &create_pwrite_task, py::arg("fd"), py::arg("data"),
                                   py::arg("count"), py::arg("offset"), py::arg("callback"));
    wf.def("create_pwritev_task", &create_pwritev_task, py::arg("fd"), py::arg("data_list"),
                                   py::arg("offset"), py::arg("callback"));
    wf.def("create_fsync_task",   &create_fsync_task, py::arg("fd"), py::arg("callback"));
    wf.def("create_fdsync_task",  &create_fdsync_task, py::arg("fd"), py::arg("callback"));

    wf.def("create_timer_task",   &create_timer_task, py::arg("microseconds"), py::arg("callback"));
    wf.def("create_counter_task", &create_counter_task_no_name, py::arg("target"), py::arg("callback"));
    wf.def("create_counter_task", &create_counter_task, py::arg("name"), py::arg("target"),
                                   py::arg("callback"));
    wf.def("count_by_name",       &count_by_name, py::arg("name"), py::arg("n"));
    wf.def("create_go_task",      &create_go_task, py::arg("function"));
    wf.def("create_go_task",      &create_go_task_with_name, py::arg("name"), py::arg("function"));
    wf.def("create_empty_task",   &create_empty_task);
}
