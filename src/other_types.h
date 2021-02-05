#ifndef PYWF_OTHER_TYPES_H
#define PYWF_OTHER_TYPES_H
#include "common_types.h"
#include "workflow/WFTask.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

template<typename Arg>
class PyWFFileTask : public PySubTask {
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
    ArgType get_args()      { return ArgType(this->get()->get_args()); }
    long get_retval() const { return this->get()->get_retval();        }
    int get_state()   const { return this->get()->get_state();         }
    int get_error()   const { return this->get()->get_error();         }
    // TODO user_data, set_callback
private:
};

class CopyableFileIOArgs {
public:
    CopyableFileIOArgs(int fd, std::string content, off_t offset)
        : fd(fd), content(std::move(content)), offset(offset) {}
    int get_fd()            { return fd;                 }
    py::bytes get_content() { return py::bytes(content); }
    off_t get_offset()      { return offset;             }
private:
    int fd;
    std::string content;
    off_t offset;
};

class PyFileIOArgs : public PyWFBase {
public:
    using OriginType = FileIOArgs;
    PyFileIOArgs()                      : PyWFBase()  {}
    PyFileIOArgs(OriginType *p)         : PyWFBase(p) {}
    PyFileIOArgs(const PyFileIOArgs &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    CopyableFileIOArgs copy() const {
        return CopyableFileIOArgs(get_fd(), _get_content(), get_offset());
    }
    int get_fd() const { return this->get()->fd; }
    py::bytes get_content() const {
        auto p = this->get();
        const char *buf = static_cast<const char*>(p->buf);
        auto count = p->count;
        if(p->count > 0) return py::bytes(buf, count);
        return py::bytes();
    }
    off_t get_offset() const { return this->get()->offset; }
private:
    std::string _get_content() const {
        std::string s;
        auto p = this->get();
        if(p->count > 0) s.assign((const char*)(p->buf), p->count);
        return s;
    }
};

class PyWFTimerTask : public PySubTask {
public:
    using OriginType = WFTimerTask;
    PyWFTimerTask()                       : PySubTask()  {}
    PyWFTimerTask(OriginType *p)          : PySubTask(p) {}
    PyWFTimerTask(const PyWFTimerTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    int get_state() const { return this->get()->get_state(); }
    int get_error() const { return this->get()->get_error(); }
};

class PyWFCounterTask : public PySubTask {
public:
    using OriginType = WFCounterTask;
    PyWFCounterTask()                         : PySubTask()  {}
    PyWFCounterTask(OriginType *p)            : PySubTask(p) {}
    PyWFCounterTask(const PyWFCounterTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
    void count() {
        this->get()->count();
    }
};

class PyWFGoTask : public PySubTask {
public:
    using OriginType = WFGoTask;
    PyWFGoTask()                    : PySubTask()  {}
    PyWFGoTask(OriginType *p)       : PySubTask(p) {}
    PyWFGoTask(const PyWFGoTask &o) : PySubTask(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }
    void start() {
        assert(!series_of(this->get()));
        CountableSeriesWork::start_series_work(this->get(), nullptr);
    }
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
using py_fio_callback_t     = std::function<void(PyWFFileIOTask)>;
using py_timer_callback_t   = std::function<void(PyWFTimerTask)>;
using py_counter_callback_t = std::function<void(PyWFCounterTask)>;

#endif // PYWF_OTHER_TYPES_H
