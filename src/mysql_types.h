#ifndef PYWF_MYSQL_TYPES_H
#define PYWF_MYSQL_TYPES_H

#include "network_types.h"
#include "workflow/MySQLMessage.h"
#include "workflow/MySQLResult.h"
#include <datetime.h>

class PyMySQLCell {
public:
    using OriginType = protocol::MySQLCell;
    using ulonglong = unsigned long long;
    PyMySQLCell() {}
    PyMySQLCell(OriginType &&c) : cell(std::move(c)) {}

    int get_data_type()      { return cell.get_data_type(); }
    bool is_null()           { return cell.is_null(); }
    bool is_int()            { return cell.is_int(); }
    bool is_string()         { return cell.is_string(); }
    bool is_float()          { return cell.is_float(); }
    bool is_double()         { return cell.is_double(); }
    bool is_ulonglong()      { return cell.is_ulonglong(); }
    bool is_date()           { return cell.is_date(); }
    bool is_time()           { return cell.is_time(); }
    bool is_datetime()       { return cell.is_datetime(); }

    int as_int()             { return cell.as_int(); }
    float as_float()         { return cell.as_float(); }
    double as_double()       { return cell.as_double(); }
    ulonglong as_ulonglong() { return cell.as_ulonglong(); }

    py::bytes as_string() {
        if(is_string() || is_time() || is_date() || is_datetime()) {
            return as_bytes();
        }
        return py::bytes();
    }

    py::object to_datetime() {
        std::string str = cell.as_string();
        int year, month, day, hour, min, sec, usec, ret;
        if(is_date()) {
            if(str.size() != 10) return py::none();
            ret = sscanf(str.c_str(), "%4d-%2d-%2d", &year, &month, &day);
            if(ret != 3) return py::none();
            return py::reinterpret_steal<py::object>(PyDate_FromDate(year, month, day));
        }
        else if(is_time()) {
            if(str.size() < 8) return py::none();
            usec = 0;
            ret = sscanf(str.c_str(), "%d:%2d:%2d.%d", &hour, &min, &sec, &usec);
            if(ret != 3 && ret != 4) return py::none();
            return py::reinterpret_steal<py::object>(
                PyDelta_FromDSU(0, hour * 3600 + min * 60 + sec, usec)
            );
        }
        else if(is_datetime()) {
            if(str.size() < 19) return py::none();
            usec = 0;
            ret = sscanf(str.c_str(), "%4d-%2d-%2d %2d:%2d:%2d.%d",
                &year, &month, &day, &hour, &min, &sec, &usec);
            if(ret != 6 && ret != 7) return py::none();
            return py::reinterpret_steal<py::object>(
                PyDateTime_FromDateAndTime(year, month, day, hour, min, sec, usec)
            );
        }
        return py::none();
    }
    py::object as_date()     { return to_datetime(); }
    py::object as_time()     { return to_datetime(); }
    py::object as_datetime() { return to_datetime(); }

    py::bytes as_bytes() {
        const void *data;
        size_t len;
        int data_type;
        cell.get_cell_nocopy(&data, &len, &data_type);
        return py::bytes(static_cast<const char*>(data), len);
    }

    py::object as_object() {
        switch (get_data_type()) {
        case MYSQL_TYPE_NULL:
            return py::none();
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
            return py::int_(as_int());
        case MYSQL_TYPE_LONGLONG:
            return py::int_(as_ulonglong());
        case MYSQL_TYPE_FLOAT:
            return py::float_(as_float());
        case MYSQL_TYPE_DOUBLE:
            return py::float_(as_double());
        case MYSQL_TYPE_DATE:
            return as_date();
        case MYSQL_TYPE_TIME:
            return as_time();
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
            return as_datetime();
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_JSON:
            return as_string();
        default:
            return as_bytes();
        }
    }

    py::str __str__() {
        py::object obj = as_object();
        py::str result;
        bool str_is_set = false;
        if(py::bytes::check_(obj)) {
            try {
                py::str s(static_cast<py::bytes>(obj));
                result = s;
                str_is_set = true;
            }
            // this py::bytes cannot convert to py::str
            // but we can also try bytes' __str__
            catch(const std::runtime_error&) {
                PyErr_Clear();
            }
        }
        if(!str_is_set) {
            try {
                result = py::str(obj.ptr());
                str_is_set = true;
            }
            catch(const std::runtime_error&) {
                PyErr_Clear();
            }
        }
        if(!str_is_set) result = py::str("Exception");
        return result;
    }
private:
    OriginType cell;
};

class PyMySQLField : public PyWFBase {
public:
    using OriginType = protocol::MySQLField;
    PyMySQLField()                      : PyWFBase()  {}
    PyMySQLField(OriginType *p)         : PyWFBase(p) {}
    PyMySQLField(const PyMySQLField &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    py::str as_str(const std::string &s) {
        try {
            return py::str(s);
        }
        catch(const std::runtime_error&) {
            PyErr_Clear();
        }
        try {
            py::bytes b(s);
            return py::str(b.ptr());
        }
        catch(const std::runtime_error&) {
            PyErr_Clear();
        }
        return py::str();
    }
    py::str __str__() {
        return as_str(this->get()->get_name());
    }
    py::str __repr__() {
        return py::str("pywf.MySQLField of ") + __str__();
    }

    py::bytes get_name()      { return this->get()->get_name(); }
    py::bytes get_org_name()  { return this->get()->get_org_name(); }
    py::bytes get_table()     { return this->get()->get_table(); }
    py::bytes get_org_table() { return this->get()->get_org_table(); }
    py::bytes get_db()        { return this->get()->get_db(); }
    py::bytes get_catalog()   { return this->get()->get_catalog(); }
    py::bytes get_def()       { return this->get()->get_def(); }

    int get_charsetnr() { return this->get()->get_charsetnr(); }
    int get_length()    { return this->get()->get_length(); }
    int get_flags()     { return this->get()->get_flags(); }
    int get_decimals()  { return this->get()->get_decimals(); }
    int get_data_type() { return this->get()->get_data_type(); }
};

class PyMySQLRequest : public PyWFBase {
public:
    using OriginType = protocol::MySQLRequest;
    PyMySQLRequest()                        : PyWFBase()  {}
    PyMySQLRequest(OriginType *p)           : PyWFBase(p) {}
    PyMySQLRequest(const PyMySQLRequest &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    void move_to(PyMySQLRequest &o) {
        *(o.get()) = std::move(*(this->get()));
    }

    void set_query(const std::string &query) { this->get()->set_query(query); }
    py::bytes get_query() const { return this->get()->get_query(); }
    bool query_is_unset() const { return this->get()->query_is_unset(); }

    int get_seqid() const   { return this->get()->get_seqid(); }
    void set_seqid(int id)  { this->get()->set_seqid(id); }
    int get_command() const { return this->get()->get_command(); }

    void set_size_limit(size_t limit) { this->get()->set_size_limit(limit); }
    size_t get_size_limit() const     { return this->get()->get_size_limit(); }
};

class PyMySQLResponse : public PyWFBase {
public:
    using OriginType = protocol::MySQLResponse;
    PyMySQLResponse()                         : PyWFBase()  {}
    PyMySQLResponse(OriginType *p)            : PyWFBase(p) {}
    PyMySQLResponse(const PyMySQLResponse &o) : PyWFBase(o) {}
    OriginType* get() const { return static_cast<OriginType*>(ptr); }

    void move_to(PyMySQLResponse &o) {
        *(o.get()) = std::move(*(this->get()));
    }

    bool is_ok_packet() const    { return this->get()->is_ok_packet(); }
    bool is_error_packet() const { return this->get()->is_error_packet(); }
    int get_packet_type() const  { return this->get()->get_packet_type(); }
    unsigned long long get_affected_rows() const {
        return this->get()->get_affected_rows();
    }
    unsigned long long get_last_insert_id() const {
        return this->get()->get_last_insert_id();
    }

    int get_warnings() const        { return this->get()->get_warnings(); }
    int get_status_flags() const    { return this->get()->get_status_flags(); }
    int get_error_code() const      { return this->get()->get_error_code(); }
    py::bytes get_error_msg() const { return this->get()-> get_error_msg(); }
    py::bytes get_sql_state() const { return this->get()->get_sql_state(); }
    py::bytes get_info() const      { return this->get()->get_info(); }
    void set_ok_packet()            { return this->get()->set_ok_packet(); }

    void set_size_limit(size_t limit) { this->get()->set_size_limit(limit); }
    size_t get_size_limit() const     { return this->get()->get_size_limit(); }
};

class PyMySQLResultCursor {
public:
    using OriginType = protocol::MySQLResultCursor;
    PyMySQLResultCursor() {}
    PyMySQLResultCursor(OriginType &&o) : csr(std::move(o)) {}
    PyMySQLResultCursor(PyMySQLResponse &resp) : csr(resp.get()) {}

    bool next_result_set()  { return csr.next_result_set(); }
    void first_result_set() { return csr.first_result_set(); }

    std::vector<PyMySQLField> fetch_fields() {
        const protocol::MySQLField * const * fields = csr.fetch_fields();
        std::vector<PyMySQLField> fields_vec;
        fields_vec.reserve(csr.get_field_count());
        for(int i = 0; i < csr.get_field_count(); i++)
            fields_vec.push_back(PyMySQLField(const_cast<protocol::MySQLField*>(fields[i])));
        return fields_vec;
    }

    py::object fetch_row() {
        py::list lst;
        std::vector<protocol::MySQLCell> cells;
        if(csr.fetch_row(cells)) {
            for(auto &cell : cells)
                lst.append(PyMySQLCell(std::move(cell)));
            return static_cast<py::object>(lst);
        }
        else {
            return py::none();
        }
    }
    py::object fetch_all() {
        py::list all;
        while(true) {
            py::object row;
            row = this->fetch_row();
            if(row.is_none()) break;
            all.append(row);
        }
        return all;
    }

    int get_field_count()   { return csr.get_field_count(); }
    int get_rows_count()    { return csr.get_rows_count(); }
    int get_cursor_status() { return csr.get_cursor_status(); }
    void rewind()           { csr.rewind(); }
private:
    OriginType csr;
};

using PyWFMySQLTask       = PyWFNetworkTask<PyMySQLRequest, PyMySQLResponse>;
using PyWFMySQLServer     = PyWFServer<PyMySQLRequest, PyMySQLResponse>;
using py_mysql_callback_t = std::function<void(PyWFMySQLTask)>;
using py_mysql_process_t  = std::function<void(PyWFMySQLTask)>;

#endif // PYWF_MYSQL_TYPES_H
