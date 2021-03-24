# PyWorkflow(PyWF) - A Python Binding of C++ Workflow
[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://github.com/sogou/pyworkflow/blob/master/LICENSE)
[![Language](https://img.shields.io/badge/language-c++-red.svg)](https://en.cppreference.com/)
[![PyPI - Python Version](https://img.shields.io/pypi/pyversions/pywf.svg)](https://pypi.python.org/pypi/pywf)
[![PyPI](https://img.shields.io/pypi/v/pywf.svg)](https://pypi.python.org/pypi/pywf)

## 概览
[C++ Workflow](https://github.com/sogou/workflow)是一个高性能的异步引擎，本项目着力于实现一个Python版的Workflow，让Python用户也能享受Workflow带来的绝佳体验。

### 快速上手
在用户深入了解Workflow相关概念之前，先来看几个简单的示例，可以对使用方法有一个初步印象。pywf是本项目Python包的名称，在文档中有时会直接使用`wf`作为其简称。


#### 发起一个Http请求
```py
import pywf as wf

def http_callback(http_task):
    resp = http_task.get_resp()
    print("Http status:{}\n{}".format(
        resp.get_status_code(), resp.get_body())) # body is bytes

http_task = wf.create_http_task("http://www.sogou.com/", redirect_max=4, retry_max=2, callback=http_callback)
http_task.start()
wf.wait_finish()
```

#### 依次发起多个Http请求
```py
import pywf as wf

def series_callback(s):
    print("All task in this series is done")

def http_callback(http_task):
    req = http_task.get_req()
    resp = http_task.get_resp()
    print("uri:{} status:{}".format(
        req.get_request_uri(),
        resp.get_status_code()))

def create_http_task(url):
    return wf.create_http_task(url, 4, 2, http_callback)

first_task = create_http_task("http://www.sogou.com")
series = wf.create_series_work(first_task, series_callback)
series.push_back(create_http_task("https://www.zhihu.com/people/kedixa"))
series.push_back(create_http_task("https://fanyi.sogou.com/document"))
series.start()
wf.wait_finish()
```

#### 同时发起多个Http请求
```py
import pywf as wf

def parallel_callback(p):
    print("All series in this parallel is done")

def http_callback(http_task):
    req = http_task.get_req()
    resp = http_task.get_resp()
    print("uri:{} status:{}".format(
        req.get_request_uri(),
        resp.get_status_code()))

url = [
    "http://www.sogou.com",
    "https://www.zhihu.com/people/kedixa",
    "https://fanyi.sogou.com/document"
]
parallel = wf.create_parallel_work(parallel_callback)
for u in url:
    task = wf.create_http_task(u, 4, 2, http_callback)
    series = wf.create_series_work(task, None) # without callback
    parallel.add_series(series)
parallel.start()

wf.wait_finish()
```

### 基本概念
#### 任务
通过`create_xxx_task`等工厂函数创建的对象称作任务(task)，例如`create_http_task`。一个任务被创建后，必须被启动或取消，通过执行`http_task.start()`，会自动以`http_task`为`first_task`创建一个串行并立即启动任务。如果用户指定了回调函数，当任务完成时回调函数会被调用，但在任务启动后且回调函数前，用户不能再操作该任务。当回调函数结束后，该任务被立即释放，用户也不能再操作该任务。

#### 串行
通过`create_series_work`创建的对象称作串行(series)，用户在创建时需要指定一个`first_task`来作为启动该`series`启动时应当执行的任务，用户可选地指定一个回调函数，当所有任务执行完成后，回调函数会被调用。

`series`的回调函数用于通知用户该串行中的任务均已完成，不能再继续添加新的任务，且回调函数结束后，该串行会立即被销毁。

#### 并行
通过`create_parallel_work`创建的对象称作并行(parallel)，用户可以创建一个空的并行，然后通过`add_series`接口向并行中添加串行，也可以在创建时指定一组串行。并行本身也是一种任务，所以并行也可以放到串行中。`parallel.start()`就会自动创建一个串行，并将`parallel`作为`first_task`立即开始执行。

`parallel`的回调函数用于通知用户该并行中的串行均已完成，不能再继续添加新的串行，且回调函数结束后，该并行会立即被销毁。

有了上述三个概念，就可以构建出各种复杂的任务结构，并在Workflow的管理下高效执行。

### 详细说明
- [任务流](./doc/pywf.md)
- [http任务](./doc/http.md)
- [redis任务](./doc/redis.md)
- [mysql任务](./doc/mysql.md)
- [其他任务](./doc/others.md)

### 设计理念
Workflow认为，一个典型的后端程序由三个部分组成，并且完全独立开发。即：程序=协议+算法+任务流。

- 协议
  - 大多数情况下，用户使用的是内置的通用网络协议，例如http，redis或各种rpc。
  - PyWF未支持用户自定义协议。
- 算法
  - 算法是与协议对称的概念。
  - 如果说协议的调用是rpc，算法的调用就是一次apc（Async Procedure Call）。
  - 任何一次边界清晰的复杂计算，都应该包装成算法。
- 任务流
  - 任务流就是实际的业务逻辑，就是把开发好的协议与算法放在流程图里使用起来。
  - 典型的任务流是一个闭合的串并联图。复杂的业务逻辑，可能是一个非闭合的DAG。
  - 任务流图可以直接构建，也可以根据每一步的结果动态生成。所有任务都是异步执行的。

Python Workflow将会逐步支持Workflow的六种基础任务：通讯，文件IO，CPU，GPU，定时器，计数器。

### 注意事项
- 框架本身不抛出异常，也未处理任何异常，所以用户需要保证回调函数不会抛出异常，context的构造和析构不抛出异常。
- 所有通过工厂函数创建出的task，必须start、dismiss或添加至一个series中。
- 所有创建出的series必须start、dismiss或添加至一个parallel中。
- 所有创建出的parallel必须start、dismiss或添加至一个series中。
- 由PyWF工厂函数创建的对象的生命周期均由内部管理，在Python层面仅是一个引用，用户不能使用超出生命周期的对象。
- 用户使用大部分`get`接口获取的对象可以自由使用，例如Http中的`get_body`。

## 构建和安装

### 通过pip安装
本项目仅支持Python3.6以上，正在准备发布一组`manylinux2014`版本，用户即将可以通过较高版本的pip直接安装。

```bash
# We are working on it
pip3 install pywf
```

### 编译安装
用户可以下载本项目源码进行编译安装。

```bash
# CentOS 7
yum install cmake3 ninja-build python36 python36-devel python36-pip
yum install gcc-c++ # if needed
git clone https://github.com/sogou/pyworkflow --recursive
cd pyworkflow
pip3 install wheel
python3 setup.py bdist_wheel
pip3 install dist/*.whl --user
```

```bash
# CentOS 8
yum install cmake ninja-build python36 python36-devel python3-pip
git clone https://github.com/sogou/pyworkflow --recursive
cd pyworkflow
pip3 install wheel
python3 setup.py bdist_wheel
pip3 install dist/*.whl --user
```

```bash
# Mac
# Build Requirement
brew install ninja cmake openssl
pip3 install wheel

# OpenSSL Env, see 'brew info openssl' for more infomation
echo 'export PATH="/usr/local/opt/openssl@1.1/bin:$PATH"' >> ~/.bash_profile
echo 'export LDFLAGS="-L/usr/local/opt/openssl@1.1/lib"' >> ~/.bash_profile
echo 'export CPPFLAGS="-I/usr/local/opt/openssl@1.1/include"' >> ~/.bash_profile
echo 'export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"' >> ~/.bash_profile
echo 'export OPENSSL_ROOT_DIR="/usr/local/opt/openssl@1.1/"' >> ~/.bash_profile

# zsh Env, if needed
echo 'test -f ~/.bash_profile  && source ~/.bash_profile' >> ~/.zshrc
source ~/.zshrc

# Build
git clone https://github.com/sogou/pyworkflow --recursive
cd pyworkflow
python3 setup.py bdist_wheel
pip3 install dist/*.whl --user
```
