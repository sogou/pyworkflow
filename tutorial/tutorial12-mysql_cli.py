import readline
import sys

import pywf as wf

default_column_width = 20
retry_max = 2

def header_line(sz):
    return '+' + '+'.join(['-' * default_column_width for i in range(sz)]) + '+'

def as_str(obj):
    try:
        return obj.decode() if type(obj) is bytes else str(obj)
    except:
        return 'Exception'

def format_row(row):
    return '|' + '|'.join([f"{str(x):^{default_column_width}}" for x in row]) + '|'

def next_line():
    sql = ''
    while True:
        line = ''
        prompt = 'mysql> ' if len(sql) == 0 else '-----> '
        try:
            line = input(prompt).strip()
        except:
            return None
        sql = line if len(sql) == 0 else sql + ' ' + line

        if sql[:4].lower() == 'quit' or sql[:4].lower() == 'exit':
            return None
        if len(sql) > 0 and sql[-1] == ';':
            return sql

def create_next_task(url, callback):
    sql = next_line()
    if sql is None:
        print('\nBye')
        return None
    readline.add_history(sql)

    task = wf.create_mysql_task(url, retry_max, callback)
    task.get_req().set_query(sql)
    return task

def mysql_callback(task):
    state = task.get_state()
    error = task.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print(wf.get_error_string(state, error))
        return

    resp = task.get_resp()
    cursor = wf.MySQLResultCursor(resp)

    for result_set in wf.MySQLResultSetIterator(cursor):
        if result_set.get_cursor_status() == wf.MYSQL_STATUS_GET_RESULT:
            fields = result_set.fetch_fields()

            print(header_line(len(fields)))
            print(format_row(fields))
            print(header_line(len(fields)))

            for row in wf.MySQLRowIterator(result_set):
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
                as_str(result_set.get_info())
            ))

    if resp.get_packet_type() == wf.MYSQL_PACKET_OK:
        print("OK. {} {} affected. {} warnings. insert_id={}. {}".format(
            resp.get_affected_rows(),
            "row" if resp.get_affected_rows() == 1 else "rows",
            resp.get_warnings(),
            resp.get_last_insert_id(),
            as_str(resp.get_info())
        ))

    elif resp.get_packet_type() == wf.MYSQL_PACKET_ERROR:
        print("ERROR. error_code={} {}".format(
            resp.get_error_code(),
            as_str(resp.get_error_msg())
        ))

    url = wf.series_of(task).get_context()
    next_task = create_next_task(url, mysql_callback)
    if next_task is not None:
        wf.series_of(task).push_back(next_task)
    pass


def main():
    if len(sys.argv) != 2:
        print("USAGE: {} <url>".format(sys.argv[0]))
        sys.exit(1)

    url = sys.argv[1]
    if url[:8].lower() != "mysql://" and url[:9].lower() != "mysqls://":
        url = "mysql://" + url

    readline.set_auto_history(False)

    mysql_task = wf.create_mysql_task(url, retry_max, mysql_callback)

    first_sql = "show databases;"
    req = mysql_task.get_req()
    req.set_query(first_sql)
    readline.add_history(first_sql)

    series = wf.create_series_work(mysql_task, None)
    series.set_context(url)
    series.start()
    wf.wait_finish()

# Usage: python3 tutorial12-mysql_cli.py mysql://user:password@host:port/dbname
if __name__ == '__main__':
    main()
