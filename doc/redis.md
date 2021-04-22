## Redis Client/Server

### Tutorials
- [tutorial02-redis_cli.py](../tutorial/tutorial02-redis_cli.py)
- [tutorial03-wget_to_redis.py](../tutorial/tutorial03-wget_to_redis.py)

### RedisValue
- 构造函数
  - RedisValue()
  - RedisValue(RedisValue)
- copy() -> RedisValue
  - 获得当前对象的一份深拷贝
- move_to(RedisValue) -> None
  - 将当前对象移动至另一对象，模仿C++的std::move
- move_from(RedisValue) -> None
  - 将另一个对象的内容移动至当前对象，模仿C++的std::move

- set_nil() -> None
- set_int(int) -> None
- set_array(uint newsize) -> None
  - 设置RedisValue为数组，并指定大小
- set_string(str/bytes) -> None
- set_status(str/bytes) -> None
- set_error(str/bytes) -> None

- is_ok() -> bool
- is_error() -> bool
- is_nil() -> bool
- is_int() -> bool
- is_array() -> bool
- is_string() -> bool

- string_value() -> bytes
- int_value() -> int
- arr_size() -> int
- arr_clear() -> None
- arr_resize(uint newsize) -> None

- clear() -> None
  - 清空对象，等价于`set_nil`
- debug_string() -> bytes
  - 获取当前对象的字符串表示
- arr_at(uint pos) -> wf.RedisValue
  - 若当前对象为数组，返回位于pos位置的RedisValue的拷贝，否则返回None
- arr_at_ref(uint pos) -> wf.RedisValue
  - 若当前对象为数组，返回位于pos位置的RedisValue的引用，用户自行保证生命周期，否则返回None
- arr_at_object(uint pos) -> object
  - 若当前对象为数组，返回位于pos位置的RedisValue的Python对象表示，否则返回None
- as_object() -> object
  - 返回当前对象的Python对象表示
  - string、error、status均为Python bytes
  - nil为Python None
  - array为Python list

### RedisRequest
- move_to(RedisRequest) -> None
  - 移动当前对象至新对象，模仿C++的std::move
- set_request(str cmd, list[str/bytes]) -> None
- get_command() -> str
- get_params() -> list[bytes]
  - 获取参数列表，注意参数类型均为bytes
- set_size_limit(uint)
- get_size_limit() -> int

### RedisResponse
- move_to(RedisResponse) -> None
  - 移动当前对象至新对象，模仿C++的std::move
- get_result() -> wf.RedisValue
  - 返回RedisValue的拷贝
- set_result(RedisValue) -> None
  - 将RedisValue设置到RedisResponse中
- set_size_limit(uint)
- get_size_limit() -> int

### RedisTask
- start() -> None
- dismiss() -> None
- noreply() -> None
- get_req() -> wf.RedisRequest
- get_resp() -> wf.RedisResponse
- get_state() -> int
- get_error() -> int
- get_timeout_reason() -> int
  - wf.TOR_NOT_TIMEOUT
  - wf.TOR_WAIT_TIMEOUT
  - wf.TOR_CONNECT_TIMEOUT
  - wf.TOR_TRANSMIT_TIMEOUT
- get_task_seq() -> int
- set_send_timeout(int) -> None
- set_receive_timeout(int) -> None
- set_keep_alive(int) -> None
- set_callback(Callable[[wf.RedisTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object

### RedisServer
- RedisServer(Callable[[wf.RedisTask], None])
- RedisServer(wf.ServerParams, Callable[[wf.RedisTask], None])
- start(int port, str cert_file = '', str key_file = '') -> int
  - 启动server，cert_file和key_file任意一个未指定时等同于`start(port)`
  - 函数返回0表示启动成功
- start(int family, str host, int port, str cert_file = '', str key_file = '') -> int
- stop() -> None
  - 停止server，该函数同步等待当前处理中的请求完成

### 任务工厂等
- wf.create_redis_task(str url, int retry_max, Callable[[wf.RedisTask], None]) -> wf.RedisTask

### 示例

RedisTask使用较为简单，直接参考tutorials即可。

```py
# RedisValue
import pywf as wf

def construct():
    v1 = wf.RedisValue()

    print(v1.as_object()) # None

def set_check():
    v1 = wf.RedisValue()
    v2 = wf.RedisValue()
    v1.set_int(1024)
    v2.set_status(b'status')

    print(v1.is_int(), v1.int_value()) # True 1024
    print(v2.is_ok(), v2.is_string(), v2.string_value()) # True True b'status'

def array():
    v1 = wf.RedisValue()
    v1.set_array(3)

    v1[0].set_string('string')
    v11_ref = v1.arr_at_ref(1)
    v11_ref.set_array(2)
    v11_ref[0].set_int(1)
    v11_ref[1].set_string(b'value')
    v1[2] = v1[0] # v1[2] is a copy of v1[0]
    v1[0].set_nil()

    print(v1.as_object()) # [None, [1, b'value'], b'string']

    v2 = wf.RedisValue()
    v3 = wf.RedisValue()
    v3.set_string('string')
    v2.set_array(2)
    v3.move_to(v2[0])
    v21 = v2.arr_at(1) # arr_at returns a copy
    v21.set_error('error') # So v2[1] not changed
    print(v2.as_object()) # [b'string', None]

if __name__ == "__main__":
    construct()
    set_check()
    array()
```
