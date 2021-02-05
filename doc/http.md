## Http Client/Server

### Tutorials
- [tutorial01-wget.py](../tutorial/tutorial01-wget.py)
- [tutorial04-http_echo_server.py](../tutorial/tutorial04-http_echo_server.py)
- [tutorial05-http_proxy.py](../tutorial/tutorial05-http_proxy.py)
- [tutorial06-parallel_wget.py](../tutorial/tutorial06-parallel_wget.py)

### HttpRequest
- is_chunked() -> bool
- is_keep_alive() -> bool
- get_method() -> str
- get_request_uri() -> str
- get_http_version() -> str
- get_headers() -> list[tuple]
- get_body() -> bytes
  - 获取body，注意返回类型为bytes
- set_method(str) -> bool
- set_request_uri(str) -> bool
- set_http_version(str) -> bool
- add_header_pair(str key, str value) -> bool
- set_header_pair(str key, str value) -> bool
- append_body(bytes) -> bool
  - 追加http body，此bytes会被内部引用一份，不会发生拷贝
- append_body(str) -> bool
  - 追加http body，会发生拷贝
- clear_body() -> None
  - 清空body
- get_body_size() -> int
- set_size_limit(int) -> None
- get_size_limit() -> int
- move_to(wf.HttpRequest) -> None
  - 将当前HttpRequest内容移动至另一个HttpRequest中，在实现Http Proxy时可以用来减少拷贝
  - 移动完成后当前对象已经不可使用，即使回调函数还未结束

### HttpResponse
- is_chunked() -> bool
- is_keep_alive() -> bool
- get_status_code() -> str
- get_reason_phrase() -> str
- get_http_version() -> str
- get_headers() -> list[tuple]
- get_body() -> bytes
  - 获取body，注意返回类型为bytes
- set_status_code(str) -> bool
- set_reason_phrase(str) -> bool
- set_http_version(str) -> bool
- add_header_pair(str key, str value) -> bool
- set_header_pair(str key, str value) -> bool
- append_body(bytes) -> bool
  - 追加http body，此bytes会被内部引用一份，不会发生拷贝
- append_body(str) -> bool
  - 追加http body，会发生拷贝
- clear_body() -> None
  - 清空body
- get_body_size() -> int
- set_size_limit(int) -> None
- get_size_limit() -> int
- move_to(wf.HttpRequest) -> None
  - 将当前HttpRequest内容移动至另一个HttpRequest中，在实现Http Proxy时可以用来减少拷贝
  - 移动完成后当前对象已经不可使用，即使回调函数还未结束

### HttpTask
- start() -> None
- dismiss() -> None
- noreply() -> None
- get_req() -> wf.HttpRequest
- get_resp() -> wf.HttpResponse
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
- set_callback(Callable[[wf.HttpTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object

### HttpServer
- HttpServer(Callable[[wf.HttpTask], None])
- HttpServer(wf.ServerParams, Callable[[wf.HttpTask], None])
- start(int port, str cert_file = '', str key_file = '') -> int
  - 启动server，cert_file和key_file任意一个未指定时等同于`start(port)`
  - 函数返回0表示启动成功
- start(int family, str host, int port, str cert_file = '', str key_file = '') -> int
- stop() -> None
  - 停止server，该函数同步等待当前处理中的请求完成

### 任务工厂等
- wf.create_http_task(str url, int redirect_max, int retry_max, Callable[[wf.HttpTask], None]) -> wf.HttpTask
- wf.ServerParams同workflow的WFServerParams

Workflow中关于Server Params的定义，wf.ServerParams的默认构造会返回SERVER_PARAMS_DEFAULT
```cpp
struct WFServerParams
{
    size_t max_connections;
    int peer_response_timeout; /* timeout of each read or write operation */
    int receive_timeout;       /* timeout of receiving the whole message */
    int keep_alive_timeout;
    size_t request_size_limit;
    int ssl_accept_timeout;    /* if not ssl, this will be ignored */
};

static constexpr struct WFServerParams SERVER_PARAMS_DEFAULT =
{
    .max_connections        = 2000,
    .peer_response_timeout  = 10 * 1000,
    .receive_timeout        = -1,
    .keep_alive_timeout     = 60 * 1000,
    .request_size_limit     = (size_t)-1,
    .ssl_accept_timeout     = 10 * 1000,
};
```
### 示例
```py
# 获取Http body示例
import pywf as wf
def series_callback(x):
    pass

def http_callback(x):
    # x的类型是wf.HttpTask，作用域仅限于此函数内
    # req 和 resp仅限于此函数中，函数结束时会立刻被回收
    req = x.get_req()
    resp = x.get_resp()
    # 此时get_body获取到的是http请求的结果
    # body的生命周期由Python管理，可以随意引用至函数外部
    body = resp.get_body()
    # print(body)
    # 一般情况下不需要向resp里添加内容，若此时向resp追加内容，则请求结果会被清空，仅保留新追加的内容
    resp.append_body("<html>") # 发生拷贝
    resp.append_body(b"</html>") # 不会拷贝，bytes会被resp引用一份
    print(resp.get_body()) # 输出 b'<html></html>'

def create_http_task():
    return wf.create_http_task("http://www.sogou.com/", 1, 1, http_callback)

series = wf.create_series_work(create_http_task(), series_callback)
series.start()
wf.wait()
```
