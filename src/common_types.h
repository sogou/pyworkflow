#ifndef PYWF_COMMON_H
#define PYWF_COMMON_H
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "workflow/Workflow.h"
#include <iostream>
#include <string>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <cstdlib>

namespace py = pybind11;

inline bool has_gil() {
    return PyGILState_Check();
}

/**
 * All call of python function from workflow threads need to acquire
 * gil first. It is not allowed to throw exceptions from python
 * callback functions, if it does, the program print the error info to
 * stderr and exit immediately.
 */
template<typename Callable, typename... Args>
void py_callback_wrapper(Callable &&C, Args&& ...args) {
    py::gil_scoped_acquire acquire;
    try {
        if(C) C(std::forward<Args>(args)...);
    }
    catch(py::error_already_set &e) {
        std::cerr << e.what() << std::endl;
#ifdef __APPLE__
        std::_Exit(1);
#else
        std::quick_exit(1);
#endif
    }
}

/**
 * This is used to destruct a std::function object, we need to acquire
 * gil because there may be python object captured by std::function.
 */
template<typename R, typename... Args>
void release_wrapped_function(std::function<R(Args...)> &f) {
    py::gil_scoped_acquire acquire;
    if(f) f = nullptr;
}

template<typename T>
struct pytype {
    using type = void;
};

template<typename T>
using pytype_t = typename pytype<T>::type;

template<typename Func, typename Task>
class TaskDeleterWrapper {
public:
    TaskDeleterWrapper(Func &&f, Task *t)
        : f(std::forward<Func>(f)), t(t) { }
    TaskDeleterWrapper(const TaskDeleterWrapper&) = delete;
    TaskDeleterWrapper& operator=(const TaskDeleterWrapper&) = delete;
    Func& get_func() {
        return f;
    }
    void *get_context() {
        if(t) return t->user_data;
        return nullptr;
    }
    ~TaskDeleterWrapper() {
        py::gil_scoped_acquire acquire;
        if(f) f = nullptr;
        void *context = this->get_context();
        if(context != nullptr) {
            delete static_cast<py::object*>(context);
        }
        t->user_data = nullptr;
    }
private:
    Func f;
    Task *t{nullptr};
};

/**
 * PyWFBase is the Base wrapper class of raw pointers in Workflow.
 * All wrapper class has prefix Py, but no need for exported python class names.
 * The wrapper does not own the pointer, which is managed by Workflow.
 * OriginType is the wrapped type.
 */
class PyWFBase {
public:
    using OriginType = void;
    PyWFBase()                  : ptr(nullptr) {}
    PyWFBase(void *p)           : ptr(p)       {}
    PyWFBase(const PyWFBase &o) : ptr(o.ptr)   {}
    PyWFBase& operator=(const PyWFBase &o) {
        ptr = o.ptr;
        return *this;
    }
    void* get() const { return ptr; }
    bool is_null() const { return ptr == nullptr; }
    virtual ~PyWFBase() = default;
protected:
    void *ptr;
};

class PySubTask : public PyWFBase {
public:
    using OriginType = SubTask;
    PySubTask()                    : PyWFBase()  {}
    PySubTask(OriginType *p)       : PyWFBase(p) {}
    PySubTask(const PySubTask &o)  : PyWFBase(o) {}
    OriginType* get()        const { return static_cast<OriginType*>(ptr); }
};

// Derive SeriesWork, release something in destructor
class CountableSeriesWork final : public SeriesWork {
public:
    static std::mutex series_mtx;
    static size_t series_counter;
    static std::condition_variable series_cv;
    static void wait_finish() {
        std::unique_lock<std::mutex> lk(series_mtx);
        series_cv.wait(lk, [&](){ return series_counter == 0; });
    }
    static bool wait_finish_timeout(double seconds) {
        auto dur = std::chrono::duration<double>(seconds);
        std::unique_lock<std::mutex> lk(series_mtx);
        return series_cv.wait_for(lk, dur, [&](){ return series_counter == 0; });
    }
    static SeriesWork* create_series_work(SubTask *first, series_callback_t cb) {
        return new CountableSeriesWork(first, std::move(cb));
    }
    static void start_series_work(SubTask *first, series_callback_t cb) {
        new CountableSeriesWork(first, std::move(cb));
        first->dispatch();
    }
    static SeriesWork* create_series_work(SubTask *first, SubTask *last, series_callback_t cb) {
        SeriesWork *series = new CountableSeriesWork(first, std::move(cb));
        series->set_last_task(last);
        return series;
    }
    static void start_series_work(SubTask *first, SubTask *last, series_callback_t cb) {
        SeriesWork *series = new CountableSeriesWork(first, std::move(cb));
        series->set_last_task(last);
        first->dispatch();
    }
protected:
    CountableSeriesWork(SubTask *first, series_callback_t &&callback)
        : SeriesWork(first, std::move(callback)) {
        std::unique_lock<std::mutex> lk(series_mtx);
        ++series_counter;
    }
    ~CountableSeriesWork() {
        {
            py::gil_scoped_acquire acquire;
            void *context = get_context();
            if(context != nullptr) {
                delete static_cast<py::object*>(context);
            }
            SeriesWork::callback = nullptr;
        }
        {
            std::unique_lock<std::mutex> lk(series_mtx);
            --series_counter;
            if(series_counter == 0) series_cv.notify_all();
        }
    }
};

// Derive ParallelWork, release something in destructor
class CountableParallelWork final : public ParallelWork {
public:
    static ParallelWork* create_parallel_work(parallel_callback_t cb) {
        return new CountableParallelWork(std::move(cb));
    }
    static ParallelWork* create_parallel_work(SeriesWork *const all_series[], size_t n,
        parallel_callback_t cb) {
        return new CountableParallelWork(all_series, n, std::move(cb));
    }
    static void start_parallel_work(SeriesWork *const all_series[], size_t n,
        parallel_callback_t cb) {
        CountableParallelWork *p = new CountableParallelWork(all_series, n, std::move(cb));
        CountableSeriesWork::start_series_work(p, nullptr);
    }
protected:
    CountableParallelWork(parallel_callback_t &&cb) : ParallelWork(std::move(cb)) {}
    CountableParallelWork(SeriesWork *const all_series[], size_t n, parallel_callback_t &&cb)
        : ParallelWork(all_series, n, std::move(cb)) {}
    virtual ~CountableParallelWork() {
        {
            py::gil_scoped_acquire acquire;
            void *context = get_context();
            if(context != nullptr) {
                delete static_cast<py::object*>(context);
            }
            ParallelWork::callback = nullptr;
        }
    }
};

class PyConstSeriesWork : public PyWFBase {
public:
    using OriginType = SeriesWork;
    PyConstSeriesWork()                           : PyWFBase()  {}
    PyConstSeriesWork(OriginType *p)              : PyWFBase(p) {}
    PyConstSeriesWork(const PyConstSeriesWork &o) : PyWFBase(o) {}
    const OriginType* get() const { return static_cast<OriginType*>(ptr); }

    bool is_canceled() const { return this->get()->is_canceled(); }
    py::object get_context() const {
        void *context = this->get()->get_context();
        if(context == nullptr) return py::none();
        return *static_cast<py::object*>(context);
    }
};

class PyConstParallelWork : public PySubTask {
public:
    using OriginType = ParallelWork;
    PyConstParallelWork()                             : PySubTask()  {}
    PyConstParallelWork(OriginType *p)                : PySubTask(p) {}
    PyConstParallelWork(const PyConstParallelWork &o) : PySubTask(o) {}
    const OriginType* get() const { return static_cast<OriginType*>(ptr); }

    PyConstSeriesWork series_at(size_t index) const {
        auto p = this->get()->series_at(index);
        return PyConstSeriesWork(const_cast<SeriesWork*>(p));
    }
    py::object get_context() const {
        void *context = this->get()->get_context();
        if(context == nullptr) return py::none();
        return *static_cast<py::object*>(context);
    }
    size_t size() const {
        return this->get()->size();
    }
};

class PySeriesWork : public PyWFBase {
public:
    using OriginType = SeriesWork;
    PySeriesWork()                      : PyWFBase()  {}
    PySeriesWork(OriginType *p)         : PyWFBase(p) {}
    PySeriesWork(const PySeriesWork &o) : PyWFBase(o) {}
    OriginType* get()        const { return static_cast<OriginType*>(ptr); }
    PySeriesWork& operator<<(PySubTask &t) {
        push_back(t);
        return *this;
    }

    void start()                  { this->get()->start(); }
    void dismiss()                { this->get()->dismiss(); }
    void push_back(PySubTask &t)  { this->get()->push_back(t.get()); }
    void push_front(PySubTask &t) { this->get()->push_front(t.get()); }
    void cancel()                 { this->get()->cancel(); }
    bool is_canceled() const      { return this->get()->is_canceled(); }

    void set_callback(std::function<void(PyConstSeriesWork)> cb) {
        this->get()->set_callback([cb](const SeriesWork *p) mutable {
            py_callback_wrapper(cb, PyConstSeriesWork(const_cast<SeriesWork*>(p)));
        });
    }
    void set_context(py::object obj) {
        void *old = this->get()->get_context();
        if(old != nullptr) {
            delete static_cast<py::object*>(old);
        }
        py::object *p = nullptr;
        if(obj.is_none() == false) p = new py::object(obj);
        this->get()->set_context(static_cast<void*>(p));
    }
    py::object get_context() const {
        void *context = this->get()->get_context();
        if(context == nullptr) return py::none();
        return *static_cast<py::object*>(context);
    }
};

class PyParallelWork : public PySubTask {
public:
    using OriginType = ParallelWork;
    PyParallelWork()                        : PySubTask()  {}
    PyParallelWork(OriginType *p)           : PySubTask(p) {}
    PyParallelWork(const PyParallelWork &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss() { this->get()->dismiss(); }
    void add_series(PySeriesWork &series) { this->get()->add_series(series.get()); }

    PyConstSeriesWork series_at(size_t index) const {
        auto p = this->get()->series_at(index);
        return PyConstSeriesWork(const_cast<SeriesWork*>(p));
    }

    void set_context(py::object obj) {
        void *old = this->get()->get_context();
        if(old != nullptr) {
            delete static_cast<py::object*>(old);
        }
        py::object *p = nullptr;
        if(obj.is_none() == false) p = new py::object(obj);
        this->get()->set_context(static_cast<void*>(p));
    }
    py::object get_context() const {
        void *context = this->get()->get_context();
        if(context == nullptr) return py::none();
        return *static_cast<py::object*>(context);
    }
    size_t size() const {
        return this->get()->size();
    }
    void set_callback(std::function<void(PyConstParallelWork)> cb) {
        this->get()->set_callback([cb](const ParallelWork *p) {
            py_callback_wrapper(cb, PyConstParallelWork(const_cast<ParallelWork*>(p)));
        });
    }
};

using py_series_callback_t   = std::function<void(PyConstSeriesWork)>;
using py_parallel_callback_t = std::function<void(PyConstParallelWork)>;

#endif // PYWF_COMMON_H
