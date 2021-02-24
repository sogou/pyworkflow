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


//#define DEBUG(x) do { x } while(0)
#define DEBUG(x)
namespace py = pybind11;

inline std::string charptr2str(const char *p) {
    std::string s;
    if(p) s.assign(p);
    return s;
}
inline bool has_gil() {
    return PyGILState_Check();
}
inline void check_gil(const std::string &s) {
    if (PyGILState_Check()) {
        std::cout << s << " have GIL" << std::endl;
    }
    else {
        std::cout << s << " dont have GIL" << std::endl;
    }
}
template<typename Callable, typename... Args>
void py_callback_wrapper(Callable &&C, Args&& ...args) {
    // All callback by workflow thread need to acquire gil
    if(has_gil()) { // TODO: but maybe nothing todo
    }
    py::gil_scoped_acquire acquire;
    // C should not throw any exception
    if(C) C(args...); // TODO C may not be std::function
}
template<typename R, typename... Args>
void release_wrapped_function(std::function<R(Args...)> &f) {
    py::gil_scoped_acquire acquire;
    // There may be python object in f
    // Release it under GIL
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
    virtual ~PyWFBase() = default; // TODO what if not virtual
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
    // TODO
    // get_parent_task, get_pointer, set_pointer
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
            DEBUG(std::cout << "CountableSeriesWork:" << series_counter << std::endl;);
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
    void start() {
        this->get()->start();
    }
    void dismiss() {
        this->get()->dismiss();
    }
    void push_back(PySubTask &t) {
        this->get()->push_back(t.get());
    }
    void push_front(PySubTask &t) {
        this->get()->push_front(t.get());
    }
    void cancel() {
        this->get()->cancel();
    }
    bool is_canceled() const {
        return this->get()->is_canceled();
    }
    void set_callback(std::function<void(const PySeriesWork)> cb) {
        this->get()->set_callback([cb](const SeriesWork *p) mutable {
            py_callback_wrapper(cb, PySeriesWork(const_cast<SeriesWork*>(p)));
        });
    }
    void set_context(py::object obj) {
        // I must have GIL now
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
private:
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
    void dismiss() {
        this->get()->dismiss();
    }
    void add_series(PySeriesWork &series) {
        this->get()->add_series(series.get());
    }
    // TODO series_at python does not has const
    PySeriesWork series_at(size_t index) const {
        auto p = this->get()->series_at(index);
        return PySeriesWork(const_cast<SeriesWork*>(p));
    }
    void set_context(py::object obj) {
        // I must have GIL now
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
    void set_callback(std::function<void(const PyParallelWork)> cb) {
        this->get()->set_callback([cb](const ParallelWork *p) {
            py_callback_wrapper(cb, PyParallelWork(const_cast<ParallelWork*>(p)));
        });
    }
private:
};

using py_series_callback_t   = std::function<void(const PySeriesWork)>;
using py_parallel_callback_t = std::function<void(const PyParallelWork)>;

#endif // PYWF_COMMON_H
