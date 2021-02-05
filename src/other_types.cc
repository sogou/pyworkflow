#include "other_types.h"

PyWFFileIOTask create_pread_task(int fd, size_t count, off_t offset, py_fio_callback_t cb) {
    void *buf = malloc(count);
    auto ptr = WFTaskFactory::create_pread_task(fd, buf, count, offset, [cb, buf](WFFileIOTask *p) mutable {
        py_callback_wrapper(cb, PyWFFileIOTask(p));
        free(buf);
        release_wrapped_function(cb);
    });
    return PyWFFileIOTask(ptr);
}
PyWFTimerTask create_timer_task(unsigned int microseconds, py_timer_callback_t cb) {
    auto ptr = WFTaskFactory::create_timer_task(microseconds, [cb](WFTimerTask *p) mutable {
        py_callback_wrapper(cb, PyWFTimerTask(p));
        release_wrapped_function(cb);
    });
    return PyWFTimerTask(ptr);
}
PyWFCounterTask create_counter_task(const std::string &name, unsigned int target, py_counter_callback_t cb) {
    auto ptr = WFTaskFactory::create_counter_task(name, target, [cb](WFCounterTask *p) mutable {
        py_callback_wrapper(cb, PyWFCounterTask(p));
        release_wrapped_function(cb);
    });
    return PyWFCounterTask(ptr);
}
void count_by_name(const std::string &name, unsigned int n) {
    WFTaskFactory::count_by_name(name, n);
}
PyWFGoTask create_go_task(const std::string &name, py::function f, py::args a, py::kwargs kw) {
    py::function *pf = new py::function(f);
    py::args *pa = new py::args(a);
    py::kwargs *pkw = new py::kwargs(kw);
    auto ptr = WFTaskFactory::create_go_task(name, [pf, pa, pkw](){
        // TODO make it more clear
        if(has_gil()) {
            std::cout << "Error: I have GIL now" << std::endl;
        }
        {
            py::gil_scoped_acquire acquire;
            (*pf)(*(*pa), **(*pkw));
            delete pkw;
            delete pa;
            delete pf;
        }
    });
    return PyWFGoTask(ptr);
}

void init_other_types(py::module_ &wf) {
    py::class_<PyWFFileIOTask, PySubTask>(wf, "FileIOTask")
        .def("start", &PyWFFileIOTask::start)
        .def("get_state", &PyWFFileIOTask::get_state)
        .def("get_error", &PyWFFileIOTask::get_error)
        .def("get_args", &PyWFFileIOTask::get_args)
        .def("get_retval", &PyWFFileIOTask::get_retval)
    ;
    py::class_<PyFileIOArgs, PyWFBase>(wf, "FileIOArgs")
        .def("copy", &PyFileIOArgs::copy)
        .def("get_fd", &PyFileIOArgs::get_fd)
        .def("get_content", &PyFileIOArgs::get_content)
        .def("get_offset", &PyFileIOArgs::get_offset)
    ;
    py::class_<CopyableFileIOArgs>(wf, "CopyableFileIOArgs")
        .def("get_fd", &CopyableFileIOArgs::get_fd)
        .def("get_content", &CopyableFileIOArgs::get_content)
        .def("get_offset", &CopyableFileIOArgs::get_offset)
    ;
    py::class_<PyWFTimerTask, PySubTask>(wf, "TimerTask")
        .def("start", &PyWFTimerTask::start)
        .def("get_state", &PyWFTimerTask::get_state)
        .def("get_error", &PyWFTimerTask::get_error)
    ;
    py::class_<PyWFCounterTask, PySubTask>(wf, "CounterTask")
        .def("start", &PyWFCounterTask::start)
        .def("count", &PyWFCounterTask::count)
    ;
    py::class_<PyWFGoTask, PySubTask>(wf, "GoTask")
        .def("start", &PyWFGoTask::start)
    ;
    py::class_<PyWaitGroup>(wf, "WaitGroup")
        .def(py::init<int>())
        .def("done", &PyWaitGroup::done, py::call_guard<py::gil_scoped_release>())
        .def("wait", &PyWaitGroup::wait, py::call_guard<py::gil_scoped_release>())
    ;
    wf.def("create_pread_task", create_pread_task, py::arg("fd"), py::arg("count"), py::arg("offset"), py::arg("callback"));
    wf.def("create_timer_task", create_timer_task, py::arg("microseconds"), py::arg("callback"));
    wf.def("create_counter_task", create_counter_task);
    wf.def("count_by_name", count_by_name);
    wf.def("create_go_task", create_go_task);
}
