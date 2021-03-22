## Other Tasks
本文档介绍结构较为简单的任务

### FileTasks
文件相关任务用于对打开的fd进行异步读写等操作，包括`FileIOTask`, `FileVIOTask`, `FileSyncTask`

#### FileIOTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int
- get_retval() -> int
  - 文件读写操作的返回值
- set_callback(Callable[[wf.FileIOTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object
- get_fd() -> int
  - 获取和该任务相关的fd
- get_offset() -> int
- get_count() -> int
  - 文件操作的字节数
- get_data() -> bytes
  - 文件读取操作的结果

#### FileVIOTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int
- get_retval() -> int
  - 文件写操作的返回值
- set_callback(Callable[[wf.FileVIOTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object
- get_fd() -> int
  - 获取和该任务相关的fd
- get_offset() -> int
- get_data() -> list[bytes]
  - 文件读取操作的结果

#### FileSyncTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int
- get_retval() -> int
  - 文件操作的返回值
- set_callback(Callable[[wf.FileSyncTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object
- get_fd() -> int
  - 获取和该任务相关的fd

### TimerTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int
- set_callback(Callable[[wf.TimerTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object

### CounterTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int
- set_callback(Callable[[wf.CounterTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object
- count() -> None
  - 增加一次计数

### GoTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int
- set_callback(Callable[[wf.GoTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object

### EmptyTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int

### DynamicTask
- start() -> None
- dismiss() -> None
- get_state() -> int
- get_error() -> int

### 任务工厂等
- wf.create_pread_task(int fd, int count, int offset, callback) -> wf.FileIOTask
- wf.create_pwrite_task(int fd, bytes data, int count, int offset, callback) -> wf.FileIOTask
  - 若data长度小于count，则以真实长度为准
- wf.create_pwritev_task(int fd, list[bytes], int offset, callback) -> wf.FileVIOTask
- wf.create_fsync_task(int fd, callback) -> wf.FileSyncTask
- wf.create_fdsync_task(int fd, callback) -> wf.FileSyncTask
- wf.create_timer_task(int microseconds, callback) -> wf.TimerTask
- wf.create_counter_task(int target, callback) -> wf.CounterTask
- wf.create_counter_task(str name, int target, callback) -> wf.TimerTask
- wf.count_by_name(str name, int n) -> None
  - 通过任务名称进行n次计数
- wf.create_go_task(Callable[[*args, **kwargs], None], args, kwargs) -> wf.GoTask
- wf.create_empty_task() -> wf.EmptyTask
- wf.create_dynamic_task(Callable[[wf.DynamicTask], wf.SubTask]) -> wf.DynamicTask

### 示例
```py
# GoTask
import pywf as wf

def go_callback(task):
    print("state: {}, error: {}".format(
        task.get_state(), task.get_error()
    ))
    print("user_data: ", task.get_user_data())
    pass

def go(a, b, c):
    print(a, b, c)

go_task = wf.create_go_task(go, 1, "pywf", 3.1415926)
go_task.set_user_data({"key": "value"})
go_task.set_callback(go_callback)
go_task.start()
wf.wait_finish()
```

```py
# CounterTask
import pywf as wf

def counter_callback(task):
    print('counter callback')

task = wf.create_counter_task("counter", 2, counter_callback)
task.count()
task.start()
wf.count_by_name("counter", 1)
wf.wait_finish()
```

```py
# TimerTask
import pywf as wf
import time

start = 0

def timer_callback(task):
    stop = time.time()
    print(stop - start)

# sleep for 10 ms
task = wf.create_timer_task(10 * 1000, timer_callback)
start = time.time()
task.start()
wf.wait_finish()
```
