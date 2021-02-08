# pywf

## 基础类型
在C++ Workflow中，所有由Workflow创建的`xxxTask`或`xxxMessage`等资源均由Workflow负责释放，用户只需要在资源生命周期结束之前通过指针或引用等方式访问公开接口。在Python Workflow中，为了实现这一特性，所有的原生指针被包装成一个含有该指针的Python对象，且均派生自`WFBase`类。Python用户同样需要注意，所有由被包装的指针指向的对象的生命周期均由Workflow管理，在对应的任务`callback`结束后就会立即被释放，虽然Python层面仍保留有一个指针，但此时已经成为野指针，用户切不可再次使用。可能有一些奇技淫巧可以在运行时报告这种错误，但我们暂时没有计划这样做。如果用户要判断哪种对象需要注意生命周期的问题，可以进行如下判断
```py
t = wf.create_http_task("http://www.sogou.com/", 1, 1, None)
isinstance(t, wf.WFBase) # True
issubclass(wf.HttpTask, wf.WFBase) # True
t.dismiss()
# 在一个task被直接或间接 dismiss/start 之后，用户不再拥有其所有权
# 此后用户只能在该task的回调函数内部进行操作
```

### SubTask
`SubTask`是所有Task类型的基类，大部分情况下用户不需要关心它。除非用户需要了解某个对象是否是一个Task
```py
t = wf.create_http_task("http://www.sogou.com/", 1, 1, None)
p = wf.create_parallel_work(None)
isinstance(t, wf.SubTask) # 所有task都是SubTask
isinstance(p, wf.SubTask) # parallel work是一种task
```

### SeriesWork
- is_null() -> bool
  - 判断该对象是否为空指针
- start() -> None
  - 启动串行
- dismiss() -> None
  - 放弃串行中的所有任务
- push_back(wf.SubTask) -> None
  - 在串行尾部添加任务
- push_front(wf.SubTask) -> None
  - 在串行头部添加任务
- cancel() -> None
  - 取消串行
- is_canceled() -> bool
  - 串行是否被取消
- set_callback(Callable[[wf.SeriesWork], None]) -> None
  - 为串行设置回调函数，将参数设置为`None`可以取消回调
- set_context(object) -> None
  - 用户可以为串行设置一个Python object作为context，在串行生命周期存续的任意时刻将object取回任意次
  - 串行将在自身被释放时取消对该object的引用
  - 将参数设置为`None`可以提前取消串行对该object的引用
- get_context() -> object
  - 取回context，若从未设置过context，此调用返回`None`
- `__lshift__`(wf.SubTask) -> wf.SeriesWork
  - 将一个任务加入到串行中，作用同`push_back`
  - 但用户可以方便地执行 `series << task1 << task2 << task3`

### ParallelWork
- is_null() -> bool
  - 判断该对象是否为空指针
- start() -> None
  - 并行是一种任务，启动并行会将其放入串行，并启动串行
- dismiss() -> None
  - 取消并行中的所有任务
- add_series(wf.SeriesWork)
  - 将串行加入到并行中，并行由一组串行构成
- series_at(int) -> wf.SeriesWork
  - 获取指定位置上的Series，用户需保证该位置合法
- set_context(object) -> None
  - 用户可以为并行设置一个Python object作为context，在并行生命周期存续的任意时刻将object取回任意次
  - 并行将在自身被释放时取消对该object的引用
  - 将参数设置为`None`可以提前取消并行对该object的引用
- get_context() -> object
  - 取回context，若从未设置过context，此调用返回`None`
- size() -> int
  - 获取并行中串行的个数
- set_callback(Callable[[wf.ParallelWork], None]) -> None
  - 为并行设置回调函数，将参数设置为`None`可以取消回调

### 几个全局函数
- wf.WORKFLOW_library_init(wf.GlobalSettings) -> None
  - 初始化workflow全局参数
- wf.create_series_work(wf.SubTask, Callable[[wf.SeriesWork], None]) -> None
- wf.create_series_work(wf.SubTask, wf.SubTask, Callable[[wf.SeriesWork], None]) -> None
- wf.start_series_work(wf.SubTask, Callable[[wf.SeriesWork], None]) -> None
- wf.start_series_work(wf.SubTask, wf.SubTask, Callable[[wf.SeriesWork], None]) -> None
- wf.create_parallel_work(Callable[[wf.ParallelWork], None]) -> None
- wf.create_parallel_work(list[wf.SeriesWork], Callable[[wf.ParallelWork], None]) -> None
- wf.start_parallel_work(list[wf.SeriesWork], Callable[[wf.ParallelWork], None]) -> None
- wf.series_of(wf.SubTask) -> wf.SeriesWork
  - 获取task所在的series
  - 未被加入到sereis的task会返回空指针，用series.is_null()来检查
- wf.wait_finish() -> None
  - **重要**
  - workflow设计了一套严密的管理体系来保证所有的任务都会在正确的时机被释放，但在PyWF中，一些Python对象会被引用在Series或者Task中，需要保证在Python线程退出前，这些对象均已被成功释放
  - 在PyWF中，用户只需在需要等待所有串行完成的地方调用`wf.wait_finish()`，该函数返回时所有被启动的串行均已被执行，被放弃的串行均已被析构
  - `wf.wait_finish()`可以作为一种序列点，若用户创建了串行但既不启动也不放弃，则`wf.wait_finish()`不会返回
- wf.wait_finish_timeout(float seconds) -> None
  - 等待串行完成，可以传入一个以秒计时的参数
  - 函数返回`True`时表示所有串行执行完成
- wf.get_error_string(int state, int error) -> None
  - 获取`state, error`状态码对应的可读的字符串表示

### 其他
- 状态码，同workflow
  - wf.WFT_STATE_UNDEFINED
  - wf.WFT_STATE_SUCCESS
  - wf.WFT_STATE_TOREPLY
  - wf.WFT_STATE_NOREPLY
  - wf.WFT_STATE_SYS_ERROR
  - wf.WFT_STATE_SSL_ERROR
  - wf.WFT_STATE_DNS_ERROR
  - wf.WFT_STATE_TASK_ERROR
  - wf.WFT_STATE_ABORTED
- wf.EndpointParams同workflow的EndpointParams
- wf.GlobalSettings同workflow的WFGlobalSettings
