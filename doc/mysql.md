## MySQL Client/Server

### Tutorials

- [tutorial12-mysql_cli.py](../tutorial/tutorial12-mysql_cli.py)

### MySQL URL
mysql://username:password@host:port/dbname?character_set=charset&character_set_results=charset

- username和password按需填写
- port默认为3306
- dbname为要用的数据库名，一般如果SQL语句只操作一个db的话建议填写
- character_set为client的字符集，等价于使用官方客户端启动时的参数`--default-character-set`的配置，默认utf8，具体可以参考MySQL官方文档[character-set.html](https://dev.mysql.com/doc/internals/en/character-set.html)
- character_set_results为client、connection和results的字符集，如果想要在SQL语句里使用`SET NAME`来指定这些字符集的话，请把它配置到url的这个位置

MySQL URL示例

mysql://root:password@127.0.0.1

mysql://@test.mysql.com:3306/db1?character_set=utf8&character_set_results=utf8

### MySQL结果集
与workflow其他任务类似，可以用`task.get_resp()`拿到`MySQLResponse`，我们可以通过`MySQLResultCursor`遍历结果集及其中的每个列的信息`MySQLField`、每行和每个`MySQLCell`

**注意**：用户在遍历MySQL结果集的过程中获得的`MySQLResultCursor`、`MySQLField`、`MySQLCell`都是`MySQLResponse`内部对象的引用，即仅可以在回调函数中使用

一次请求所对应的回复中，其数据是一个三维结构
- 一个回复中包含了一个或多个结果集（result set）
- 一个结果集包含了一行或多行（row）
- 一行包含了一到多个阈（Field/Cell）

- 具体使用从外到内的步骤应该是：

1. 判断任务状态（代表通信层面状态）：用户通过判断 **task.get_state()** 是否为WFT_STATE_SUCCESS来查看任务执行是否成功；

2. 判断回复包类型（代表返回包解析状态）：调用 **resp.get_packet_type()** 查看最后一条MySQL语句的返回包类型，常见的几个类型为：
  - MYSQL_PACKET_OK：返回非结果集的请求: 解析成功；
  - MYSQL_PACKET_EOF：返回结果集的请求: 解析成功；
  - MYSQL_PACKET_ERROR：请求失败；

3. 判断结果集状态（代表结果集读取状态）：用户可以使用MySQLResultCursor读取结果集中的内容，因为MySQL server返回的数据是多结果集的，因此一开始cursor会**自动指向第一个结果集**的读取位置。通过 **cursor.get_cursor_status()** 可以拿到的几种状态：
  - MYSQL_STATUS_GET_RESULT：有数据可读；
  - MYSQL_STATUS_END：当前结果集已读完最后一行；
  - MYSQL_STATUS_OK：此回复包为非结果集包，无需通过结果集接口读数据；
  - MYSQL_STATUS_ERROR：解析错误；

4. 读取columns中每个field：
  - `get_field_count()`
  - `fetch_fields()`

5. 读取每一行：按行读取可以使用 **cursor.fetch_row()** 直到返回值为false。其中会移动cursor内部对当前结果集的指向每行的offset：
  - `get_rows_count()`
  - `fetch_row()`

6. 直接把当前结果集的所有行拿出：所有行的读取可以使用 **cursor.fetch_all()** ，内部用来记录行的cursor会直接移动到最后；cursor状态会变成MYSQL_STATUS_END：
  - `fetch_all()`: 返回 list[list[MySQLCell]]

7. 返回当前结果集的头部：如果有必要重读这个结果集，可以使用 **cursor.rewind()** 回到当前结果集头部，再通过第5步或第6步进行读取；

8. 拿到下一个结果集：因为MySQL server返回的数据包可能是包含多结果集的（比如每个select语句为一个结果集；或者call procedure返回的多结果集数据），因此用户可以通过 **cursor.next_result_set()** 跳到下一个结果集，返回值为false表示所有结果集已取完。

9. 返回第一个结果集：**cursor.first_result_set()** 可以让我们返回到所有结果集的头部，然后可以从第3步开始重新拿数据；

10. 每列具体数据MySQLCell：第5步中读取到的一行，由多列组成，每列结果为MySQLCell，基本使用接口有：
  - `get_data_type()` 返回MYSQL_TYPE_LONG、MYSQL_TYPE_STRING...
  - `is_TYPE()` TYPE为int、string、ulonglong，判断是否是某种类型
  - `as_TYPE()` 以某种类型读出MySQLCell的数据
  - `as_object()` 按数据类型返回Python对象

### MySQLCell
- get_data_type() -> int
  - 返回Cell中的数据类型，即`wf.MYSQL_TYPE_*`
- is_null() -> bool
  - 判断是否为`MYSQL_TYPE_NULL`
- is_int() -> bool
  - 判断是否为`MYSQL_TYPE_TINY`、`MYSQL_TYPE_SHORT`、`MYSQL_TYPE_INT24`、`MYSQL_TYPE_LONG`
- is_string() -> bool
  - 判断是否为`MYSQL_TYPE_DECIMAL`、`MYSQL_TYPE_STRING`、`MYSQL_TYPE_VARCHAR`、`MYSQL_TYPE_VAR_STRING`、`MYSQL_TYPE_JSON`
- is_float() -> bool
  - 判断是否为`MYSQL_TYPE_FLOAT`
- is_double() -> bool
  - 判断是否为`MYSQL_TYPE_DOUBLE`
- is_ulonglong() -> bool
  - 判断是否为`MYSQL_TYPE_LONGLONG`
- is_date() -> bool
  - 判断是否为`MYSQL_TYPE_DATE`
- is_time() -> bool
  - 判断是否为`MYSQL_TYPE_TIME`
- is_datetime() -> bool
  - 判断是否为`MYSQL_TYPE_DATETIME`、`MYSQL_TYPE_TIMESTAMP`
- as_int() -> int
- as_float() -> float
- as_double() -> float
- as_ulonglong() -> int
- as_string() -> bytes
  - `is_string()`时以bytes类型返回底层数据
  - 非`is_string()`不应该调用该接口，若调用则返回长度为零的bytes
- as_date() -> datetime.date
- as_time() -> datetime.timedelta
  - MySQL的time类型可以表示一个时间段，`datetime.timedelta`更相符
- as_datetime() -> datetime.datetime
- as_bytes() -> bytes
  - 返回底层数据的bytes类型表示，不考虑真实类型
- as_object() -> object
  - 返回该Cell的Python对象表示，如同正确调用了`as_*`函数
- `__str__` -> str
  - 返回该Cell的字符串表示，如同正确调用了`str(cell.as_object())`
  - 例外: 若`as_object()`返回了`utf-8`编码的bytes，则会尝试将其解码为str类型，否则会调用`bytes.__str__`

### MySQLField
- get_name() -> bytes
  - 字段名称，如果为字段提供了带有AS子句的别名，则值为别名。对于过程参数，为参数名称。
- get_org_name() -> bytes
  - 字段名称，忽略别名
- get_table() -> bytes
  - 包含此字段的表的名称。对于计算字段，该值为空字符串。如果为表或视图提供了带有AS子句的别名，则值为别名。对于过程参数，为过程名称。
- get_org_table() -> bytes
  - 表的名称，忽略别名
- get_db() -> bytes
  - 字段来源的数据库的名称。如果该字段是计算字段，则为空字符串。对于过程参数，是包含该过程的数据库的名称。
- get_catalog() -> bytes
  - 此值始终为b"def"
- get_def() -> bytes
  - 此字段的默认值。仅在使用`mysql_list_fields()`时设置此值
- get_charsetnr() -> int
- get_length() -> int
- get_flags() -> int
- get_decimals() -> int
- get_data_type() -> int
  - 返回该列的数据类型，即`wf.MYSQL_TYPE_*`
- `__str__()` -> str
  - 返回`get_name()`的str表示

### MySQLResultCursor
- 构造函数
  - MySQLResultCursor(wf.MySQLResponse)
- fetch_fields() -> list[wf.MySQLField]
  - 返回当前ResultSet的所有Fields
- next_result_set() -> bool
- first_result_set() -> None
- fetch_row() -> list[wf.MySQLCell]/None
  - 若当前ResultSet还有未返回的行，则返回下一行并移动下标，否则返回None
- fetch_all() -> list[list[wf.MySQLCell]]
  - 返回当前ResultSet中所有剩余的行
- get_cursor_status() -> int
  - 返回cursor状态，即`wf.MYSQL_STATUS_*`
- get_server_status() -> int
- get_field_count() -> int
  - 返回当前ResultSet的field个数
- get_rows_count() -> int
  - 返回当前ResultSet的行数
- get_affected_rows() -> int
- get_insert_id() -> int
- get_warnings() -> int
- get_info() -> bytes
- rewind()
  - 将行标移动至当前ResultSet开始处

### MySQLRequest
- move_to(MySQLRequest) -> None
  - 移动当前对象至新对象，模仿C++的std::move
- set_query(bytes/str) -> None
  - 设置sql请求
- get_query() -> bytes
  - 获取已设置的请求
- query_is_unset() -> bool
- get_seqid() -> int
- set_seqid(int) -> None
- get_command() -> int
- set_size_limit(int) -> None
- get_size_limit() -> int

### MySQLResponse
- move_to(MySQLResponse) -> None
  - 移动当前对象至新对象，模仿C++的std::move
- is_ok_packet() -> bool
- is_error_packet() -> bool
- get_packet_type() -> int
  - 返回Response状态，即`wf.MYSQL_PACKET_*`
- get_affected_rows() -> int
- get_last_insert_id() -> int
- get_warnings() -> int
- get_error_code() -> int
- get_error_msg() -> bytes
- get_sql_state() -> bytes
- get_info() -> bytes
- set_ok_packet() -> None
- get_seqid() -> int
- set_seqid(int)
- get_command() -> int
- set_size_limit(int) -> None
- get_size_limit() -> int

### MySQLTask
- start() -> None
- dismiss() -> None
- noreply() -> None
- get_req() -> wf.MySQLRequest
- get_resp() -> wf.MySQLResponse
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
- set_callback(Callable[[wf.MySQLTask], None]) -> None
- set_user_data(object) -> None
- get_user_data() -> object

### MySQLServer
- MySQLServer(Callable[[wf.MySQLTask], None])
- MySQLServer(wf.ServerParams, Callable[[wf.MySQLTask], None])
- start(int port, str cert_file = '', str key_file = '') -> int
  - 启动server，cert_file和key_file任意一个未指定时等同于`start(port)`
  - 函数返回0表示启动成功
- start(int family, str host, int port, str cert_file = '', str key_file = '') -> int
- stop() -> None
  - 停止server，该函数同步等待当前处理中的请求完成

### 其他
- wf.mysql_datatype2str(int) -> str
  - 返回`wf.MYSQL_TYPE_*`的str表示
- wf.create_mysql_task(str url, int retry_max, Callable[[wf.MySQLTask], None]) -> wf.MySQLTask

- wf.MySQLRowIterator(wf.MySQLResultCursor)
  - 获得一个遍历当前ResultSet所有行的可迭代对象，每一次迭代返回一个`list[wf.MySQLCell]`
- wf.MySQLRowObjectIterator(wf.MySQLResultCursor)
  - 获得一个遍历当前ResultSet所有行的可迭代对象，每一次迭代返回一个`list[object]`，其中object为`wf.MySQLCell.as_object()`的结果
- wf.MySQLResultSetIterator(wf.MySQLResultCursor)
  - 获得一个遍历当前MySQLResultCursor所有ResultSet的可迭代对象
  - 每一次迭代返回的结果都是wf.MySQLResultCursor的一个引用，该cursor在遍历过程中含有状态，用户不应该在遍历过程中施加额外的操作

#### MySQL status
- wf.MYSQL_STATUS_NOT_INIT
- wf.MYSQL_STATUS_OK
- wf.MYSQL_STATUS_GET_RESULT
- wf.MYSQL_STATUS_ERROR
- wf.MYSQL_STATUS_END

#### MySQL type
- wf.MYSQL_TYPE_DECIMAL
- wf.MYSQL_TYPE_TINY
- wf.MYSQL_TYPE_SHORT
- wf.MYSQL_TYPE_LONG
- wf.MYSQL_TYPE_FLOAT
- wf.MYSQL_TYPE_DOUBLE
- wf.MYSQL_TYPE_NULL
- wf.MYSQL_TYPE_TIMESTAMP
- wf.MYSQL_TYPE_LONGLONG
- wf.MYSQL_TYPE_INT24
- wf.MYSQL_TYPE_DATE
- wf.MYSQL_TYPE_TIME
- wf.MYSQL_TYPE_DATETIME
- wf.MYSQL_TYPE_YEAR
- wf.MYSQL_TYPE_NEWDATE
- wf.MYSQL_TYPE_VARCHAR
- wf.MYSQL_TYPE_BIT
- wf.MYSQL_TYPE_TIMESTAMP2
- wf.MYSQL_TYPE_DATETIME2
- wf.MYSQL_TYPE_TIME2
- wf.MYSQL_TYPE_TYPED_ARRAY
- wf.MYSQL_TYPE_JSON
- wf.MYSQL_TYPE_NEWDECIMAL
- wf.MYSQL_TYPE_ENUM
- wf.MYSQL_TYPE_SET
- wf.MYSQL_TYPE_TINY_BLOB
- wf.MYSQL_TYPE_MEDIUM_BLOB
- wf.MYSQL_TYPE_LONG_BLOB
- wf.MYSQL_TYPE_BLOB
- wf.MYSQL_TYPE_VAR_STRING
- wf.MYSQL_TYPE_STRING
- wf.MYSQL_TYPE_GEOMETRY

#### MySQL packet
- wf.MYSQL_PACKET_OTHER
- wf.MYSQL_PACKET_OK
- wf.MYSQL_PACKET_NULL
- wf.MYSQL_PACKET_EOF
- wf.MYSQL_PACKET_ERROR
- wf.MYSQL_PACKET_GET_RESULT
- wf.MYSQL_PACKET_LOCAL_INLINE

### 示例
```python
# 发起一个MySQL请求
import pywf as wf

def mysql_callback(task):
    print(task.get_state(), task.get_error())

url = "mysql://user:password@host:port/database"
mysql_task = wf.create_mysql_task(url=url, retry_max=1, callback=mysql_callback)
req = mysql_task.get_req()
req.set_query("select * from your_table limit 10;")
mysql_task.start()
wf.wait_finish()
```

```python
# 遍历MySQL请求结果
default_column_width = 20
def header_line(sz):
    return '+' + '+'.join(['-' * default_column_width for i in range(sz)]) + '+'

def format_row(row):
    return '|' + '|'.join([f"{str(x):^{default_column_width}}" for x in row]) + '|'

def mysql_callback(task):
    state = task.get_state()
    error = task.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print(wf.get_error_string(state, error))
        return

    resp = task.get_resp()
    cursor = wf.MySQLResultCursor(resp)

    # 使用迭代语法遍历所有结果集
    for result_set in wf.MySQLResultSetIterator(cursor):
        # 1. 遍历每个结果集
        if result_set.get_cursor_status() == wf.MYSQL_STATUS_GET_RESULT:
            fields = result_set.fetch_fields()

            print(header_line(len(fields)))
            print(format_row(fields))
            print(header_line(len(fields)))
            # 2. 遍历每一行
            for row in wf.MySQLRowIterator(result_set):
                # 3. 遍历每个Cell
                print(format_row(row))

            print(header_line(len(fields)))
            print("{} {} in set\n".format(
                result_set.get_rows_count(),
                "row" if result_set.get_rows_count() == 1 else "rows"
            ))
        elif result_set.get_cursor_status() == wf.MYSQL_STATUS_OK:
            print("OK. {} {} affected. {} warnings. insert_id={}. {}".format(
                result_set.get_affected_rows(),
                "row" if result_set.get_affected_rows() == 1 else "rows",
                result_set.get_warnings(),
                result_set.get_insert_id(),
                str(result_set.get_info())
            ))

# ...
```

```python
# 遍历MySQL请求结果
import pywf as wf

default_column_width = 20
def header_line(sz):
    return '+' + '+'.join(['-' * default_column_width for i in range(sz)]) + '+'

def format_row(row):
    return '|' + '|'.join([f"{str(x):^{default_column_width}}" for x in row]) + '|'

def mysql_callback(task):
    state = task.get_state()
    error = task.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print(wf.get_error_string(state, error))
        return

    resp = task.get_resp()
    cursor = wf.MySQLResultCursor(resp)

    # 遍历所有结果集
    while True:
        # 1. 遍历每个结果集
        if cursor.get_cursor_status() == wf.MYSQL_STATUS_GET_RESULT:
            fields = cursor.fetch_fields()

            print(header_line(len(fields)))
            print(format_row(fields))
            print(header_line(len(fields)))

            # 2. 遍历每一行，两种方案任选其一
            # 2.1 使用fetch_all
            all_row = cursor.fetch_all()
            for row in all_row:
                print(format_row(row))
            # 2.2 使用fetch_row
            cursor.rewind() # 回到当前结果集起始位置
            while True:
                row = cursor.fetch_row()
                if row == None:
                    break
                # 3. 遍历每个Cell
                print(format_row(row))
            print(header_line(len(fields)))
            print("{} {} in set\n".format(
                cursor.get_rows_count(),
                "row" if cursor.get_rows_count() == 1 else "rows"
            ))
        elif cursor.get_cursor_status() == wf.MYSQL_STATUS_OK:
            print("OK. {} {} affected. {} warnings. insert_id={}. {}".format(
                cursor.get_affected_rows(),
                "row" if cursor.get_affected_rows() == 1 else "rows",
                cursor.get_warnings(),
                cursor.get_insert_id(),
                str(cursor.get_info())
            ))
        if cursor.next_result_set() == False:
            break

# ...
```
