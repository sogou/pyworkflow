#ifndef PYWF_OTHER_TYPES_H
#define PYWF_OTHER_TYPES_H
#include "common_types.h"
#include "workflow/WFTask.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

class FileTaskData {
public:
    FileTaskData() : obj(nullptr) {}
    FileTaskData(const FileTaskData&) = delete;
    FileTaskData& operator=(const FileTaskData&) = delete;
    virtual ~FileTaskData() {
        if(obj) {
            py::gil_scoped_acquire acquire;
            delete obj;
            obj = nullptr;
        }
    }
    void set_obj(const py::object &o) {
        void *old = obj;
        if(old != nullptr) {
            delete static_cast<py::object*>(old);
        }
        obj = nullptr;
        if(o.is_none() == false) obj = new py::object(o);
    }
    py::object get_obj() const {
        if(obj == nullptr) return py::none();
        else return *obj;
    }
private:
    py::object *obj;
};

/**
 * FileIOTaskData OWNS buf and bytes, you need this class
 * to destruct buf and bytes on the end of task's destructor.
 **/
class FileIOTaskData : public FileTaskData {
public:
    FileIOTaskData(void *buf, py::object *bytes)
        : buf(buf), bytes(bytes) { }
    ~FileIOTaskData() {
        if(buf) free(buf);
        if(bytes) {
            py::gil_scoped_acquire acquire;
            delete bytes;
        }
    }
private:
    void *buf;
    py::object *bytes;
};

/**
 * FileVIOTaskData OWNS buf[] and bytes[], you need this class
 * to destruct buf[] and bytes[] on the end of task's destructor.
 **/
class FileVIOTaskData : public FileTaskData {
public:
    FileVIOTaskData(struct iovec *iov, bool with_buf, py::object *bytes, size_t count)
        : iov(iov), with_buf(with_buf), bytes(bytes), count(count) {}
    ~FileVIOTaskData() {
        if(iov && with_buf) {
            for(size_t i = 0; i < count; i++) {
                free(iov[i].iov_base);
            }
        }
        if(iov) delete[] iov;
        if(bytes) {
            py::gil_scoped_acquire acquire;
            delete[] bytes;
        }
    }
private:
    struct iovec *iov;
    bool with_buf;
    py::object *bytes;
    size_t count;
};

template<typename Func, typename Arg>
class TaskDeleterWrapper<Func, WFFileTask<Arg>> {
    using Task = WFFileTask<Arg>;
public:
    TaskDeleterWrapper(Func &&f, Task *t)
        : f(std::forward<Func>(f)), t(t) { }
    TaskDeleterWrapper(const TaskDeleterWrapper&) = delete;
    TaskDeleterWrapper& operator=(const TaskDeleterWrapper&) = delete;
    Func& get_func() {
        return f;
    }
    ~TaskDeleterWrapper() {
        if(t->user_data) {
            delete static_cast<FileTaskData*>(t->user_data);
            t->user_data = nullptr;
        }
    }
private:
    Func f;
    Task *t{nullptr};
};

class __file_helper {
public:
    template<typename Task>
    static int get_fd(Task*)             { return -1; }
    static int get_fd(WFFileIOTask *t)   { return t->get_args()->fd; }
    static int get_fd(WFFileVIOTask *t)  { return t->get_args()->fd; }
    static int get_fd(WFFileSyncTask *t) { return t->get_args()->fd; }

    template<typename Task>
    static off_t get_offset(Task*)            { return -1; }
    static off_t get_offset(WFFileIOTask *t)  { return t->get_args()->offset; }
    static off_t get_offset(WFFileVIOTask *t) { return t->get_args()->offset; }

    template<typename Task>
    static size_t get_count(Task*)           { return 0; }
    static size_t get_count(WFFileIOTask *t) { return t->get_args()->count; }

    template<typename Task>
    static py::object get_data(Task*) { return py::none(); }
    static py::object get_data(WFFileIOTask *t) {
        auto *arg = t->get_args();
        const char *buf = static_cast<const char*>(arg->buf);
        return py::bytes(buf, arg->count);
    }
    static py::object get_data(WFFileVIOTask *t) {
        py::list contents;
        auto *arg = t->get_args();
        for(int i = 0; i < arg->iovcnt; i++) {
            const iovec &iov = arg->iov[i];
            contents.append(py::bytes((const char*)iov.iov_base, iov.iov_len));
        }
        return contents;
    }
};

template<typename Arg>
class PyWFFileTask : public PySubTask {
    using _py_callback_t = std::function<void(PyWFFileTask<Arg>)>;
public:
    using ArgType = Arg;
    using OriginType = WFFileTask<typename ArgType::OriginType>;
    PyWFFileTask()                      : PySubTask()  {}
    PyWFFileTask(OriginType *p)         : PySubTask(p) {}
    PyWFFileTask(const PyWFFileTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss() {
        this->get()->dismiss();
    }
    int get_fd() const          { return __file_helper::get_fd(this->get()); }
    off_t get_offset() const    { return __file_helper::get_offset(this->get()); }
    size_t get_count() const    { return __file_helper::get_count(this->get()); }
    py::object get_data() const { return __file_helper::get_data(this->get()); }
    long get_retval() const { return this->get()->get_retval();        }
    int get_state()   const { return this->get()->get_state();         }
    int get_error()   const { return this->get()->get_error();         }
    void set_callback(_py_callback_t cb) {
        auto *task = this->get();
        void *user_data = task->user_data;
        task->user_data = nullptr;
        auto deleter = std::make_shared<TaskDeleterWrapper<_py_callback_t, OriginType>>(
            std::move(cb), task);
        task->set_callback([deleter](OriginType *p) {
            py_callback_wrapper(deleter->get_func(), PyWFFileTask<Arg>(p));
        });
        task->user_data = user_data;
    }
    void set_user_data(const py::object &obj) {
        auto *data = static_cast<FileTaskData*>(this->get()->user_data);
        data->set_obj(obj);
    }
    py::object get_user_data() const {
        auto *data = static_cast<FileTaskData*>(this->get()->user_data);
        return data->get_obj();
    }
};

class PyFileIOArgs : public PyWFBase {
public:
    using OriginType = FileIOArgs;
    PyFileIOArgs()                      : PyWFBase()  {}
    PyFileIOArgs(OriginType *p)         : PyWFBase(p) {}
    PyFileIOArgs(const PyFileIOArgs &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
};

class PyFileVIOArgs : public PyWFBase {
public:
    using OriginType = FileVIOArgs;
    PyFileVIOArgs()                       : PyWFBase()  {}
    PyFileVIOArgs(OriginType *p)          : PyWFBase(p) {}
    PyFileVIOArgs(const PyFileVIOArgs &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
};

class PyFileSyncArgs : public PyWFBase {
public:
    using OriginType = FileSyncArgs;
    PyFileSyncArgs()                        : PyWFBase()  {}
    PyFileSyncArgs(OriginType *p)           : PyWFBase(p) {}
    PyFileSyncArgs(const PyFileSyncArgs &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
};

class PyWFTimerTask : public PySubTask {
public:
    using OriginType = WFTimerTask;
    using _py_callback_t = std::function<void(PyWFTimerTask)>;
    PyWFTimerTask()                       : PySubTask()  {}
    PyWFTimerTask(OriginType *p)          : PySubTask(p) {}
    PyWFTimerTask(const PyWFTimerTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss()        { return this->get()->dismiss(); }
    int get_state() const { return this->get()->get_state(); }
    int get_error() const { return this->get()->get_error(); }
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
    void set_callback(_py_callback_t cb) {
        auto *task = this->get();
        void *user_data = task->user_data;
        task->user_data = nullptr;
        auto deleter = std::make_shared<TaskDeleterWrapper<_py_callback_t, OriginType>>(
            std::move(cb), this->get());
        this->get()->set_callback([deleter](OriginType *p) {
            py_callback_wrapper(deleter->get_func(), PyWFTimerTask(p));
        });
        task->user_data = user_data;
    }
};

class PyWFCounterTask : public PySubTask {
public:
    using OriginType = WFCounterTask;
    using _py_callback_t = std::function<void(PyWFCounterTask)>;
    PyWFCounterTask()                         : PySubTask()  {}
    PyWFCounterTask(OriginType *p)            : PySubTask(p) {}
    PyWFCounterTask(const PyWFCounterTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss()        { return this->get()->dismiss(); }
    int get_state() const { return this->get()->get_state(); }
    int get_error() const { return this->get()->get_error(); }
    void count()          { this->get()->count(); }
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
    void set_callback(_py_callback_t cb) {
        auto *task = this->get();
        void *user_data = task->user_data;
        task->user_data = nullptr;
        auto deleter = std::make_shared<TaskDeleterWrapper<_py_callback_t, OriginType>>(
            std::move(cb), this->get());
        this->get()->set_callback([deleter](OriginType *p) {
            py_callback_wrapper(deleter->get_func(), PyWFCounterTask(p));
        });
        task->user_data = user_data;
    }
};

class PyWFGoTask : public PySubTask {
public:
    using OriginType = WFGoTask;
    using _py_callback_t = std::function<void(PyWFGoTask)>;
    PyWFGoTask()                    : PySubTask()  {}
    PyWFGoTask(OriginType *p)       : PySubTask(p) {}
    PyWFGoTask(const PyWFGoTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss()        { this->get()->dismiss(); }
    int get_state() const { return this->get()->get_state(); }
    int get_error() const { return this->get()->get_error(); }
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
    void set_callback(_py_callback_t cb) {
        auto *task = this->get();
        void *user_data = task->user_data;
        task->user_data = nullptr;
        auto deleter = std::make_shared<TaskDeleterWrapper<_py_callback_t, OriginType>>(
            std::move(cb), this->get());
        this->get()->set_callback([deleter](OriginType *p) {
            py_callback_wrapper(deleter->get_func(), PyWFGoTask(p));
        });
        task->user_data = user_data;
    }
};

class PyWFEmptyTask : public PySubTask {
public:
    using OriginType = WFEmptyTask;
    using _py_callback_t = std::function<void(PyWFEmptyTask)>;
    PyWFEmptyTask()                       : PySubTask()  {}
    PyWFEmptyTask(OriginType *p)          : PySubTask(p) {}
    PyWFEmptyTask(const PyWFEmptyTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void dismiss()        { this->get()->dismiss(); }
    int get_state() const { return this->get()->get_state(); }
    int get_error() const { return this->get()->get_error(); }
};

// TODO Cannot call done and wait in same (main) thread
class PyWaitGroup {
public:
    using OriginType = WFFacilities::WaitGroup;
    PyWaitGroup(int n) : wg(n) {}
    void done()       { wg.done(); }
    void wait() const { wg.wait(); }
private:
    OriginType wg;
};

using PyWFFileIOTask        = PyWFFileTask<PyFileIOArgs>;
using PyWFFileVIOTask       = PyWFFileTask<PyFileVIOArgs>;
using PyWFFileSyncTask      = PyWFFileTask<PyFileSyncArgs>;
using py_fio_callback_t     = std::function<void(PyWFFileIOTask)>;
using py_fvio_callback_t    = std::function<void(PyWFFileVIOTask)>;
using py_fsync_callback_t   = std::function<void(PyWFFileSyncTask)>;
using py_timer_callback_t   = std::function<void(PyWFTimerTask)>;
using py_counter_callback_t = std::function<void(PyWFCounterTask)>;

#endif // PYWF_OTHER_TYPES_H
