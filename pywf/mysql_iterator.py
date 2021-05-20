from .cpp_pyworkflow import MySQLResultCursor
from .cpp_pyworkflow import MYSQL_STATUS_GET_RESULT
from .cpp_pyworkflow import MYSQL_STATUS_OK

def MySQLRowIterator(cursor):
    assert type(cursor) == MySQLResultCursor
    cursor.rewind()
    while True:
        row = cursor.fetch_row()
        if row is None:
            break
        yield row

def MySQLRowObjectIterator(cursor):
    assert type(cursor) == MySQLResultCursor
    cursor.rewind()
    while True:
        row = cursor.fetch_row()
        if row is None:
            break
        yield [cell.as_object() for cell in row]

def MySQLResultSetIterator(cursor):
    assert type(cursor) == MySQLResultCursor
    if cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT and \
        cursor.get_cursor_status() != MYSQL_STATUS_OK:
        return
    cursor.first_result_set()
    yield cursor
    while cursor.next_result_set():
        yield cursor
