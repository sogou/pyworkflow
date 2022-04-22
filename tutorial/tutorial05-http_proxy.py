import errno
import os
import signal
import sys
import threading

import pywf as wf


class Context:
    def __init__(self):
        pass


cv = threading.Condition()


def Stop(signum, frame):
    print("Stop server:", signum)
    cv.acquire()
    cv.notify()
    cv.release()


def reply_callback(proxy_task):
    series = wf.series_of(proxy_task)
    ctx = series.get_context()
    proxy_resp = proxy_task.get_resp()
    sz = proxy_resp.get_body_size()
    if proxy_task.get_state() == wf.WFT_STATE_SUCCESS:
        print(
            "{} Success, Http Status:{} Body Length:{}".format(
                ctx.url, proxy_resp.get_status_code(), sz
            )
        )
    else:
        print(
            "{} Reply failed:{} Body Length:{}".format(
                ctx.url, os.strerror(proxy_task.get_error()), sz
            )
        )


def http_callback(t):
    state = t.get_state()
    error = t.get_error()
    resp = t.get_resp()
    series = wf.series_of(t)
    ctx = series.get_context()
    proxy_resp = ctx.proxy_task.get_resp()

    if state == wf.WFT_STATE_SUCCESS:
        ctx.proxy_task.set_callback(reply_callback)
        # move content from resp to proxy_resp, then you cannot use resp
        resp.move_to(proxy_resp)
        if not ctx.is_keep_alive:
            proxy_resp.set_header_pair("Connection", "close")
    else:
        errstr = ""
        if state == wf.WFT_STATE_SYS_ERROR:
            errstr = "system error: {}".format(os.strerror(error))
        elif state == wf.WFT_STATE_DNS_ERROR:
            errstr = "DNS error: {}".format(error)
        elif state == wf.WFT_STATE_SSL_ERROR:
            errstr = "SSL error: {}".format(error)
        else:
            errstr = "URL error (Cannot be a HTTPS proxy)"
        print(
            "{} Fetch failed, state:{} error:{} {}".format(
                ctx.url, state, error, errstr
            )
        )
        proxy_resp.set_status_code("404")
        proxy_resp.append_body(b"<html>404 Not Found.</html>")


def process(t):
    req = t.get_req()
    series = wf.series_of(t)
    ctx = Context()
    ctx.url = req.get_request_uri()
    ctx.proxy_task = t
    series.set_context(ctx)
    ctx.is_keep_alive = req.is_keep_alive()
    http_task = wf.create_http_task(req.get_request_uri(), 0, 0, http_callback)
    req.set_request_uri(http_task.get_req().get_request_uri())
    req.move_to(http_task.get_req())
    http_task.get_resp().set_size_limit(200 * 1024 * 1024)
    series << http_task


def main():
    if len(sys.argv) != 2:
        print("Usage {} <port>".format(sys.argv[0]))
        sys.exit(1)
    port = int(sys.argv[1])
    signal.signal(signal.SIGINT, Stop)
    server_params = wf.ServerParams()
    server_params.request_size_limit = 8 * 1024 * 1024
    server = wf.HttpServer(server_params, process)
    if server.start(port) == 0:
        cv.acquire()
        cv.wait()
        cv.release()
        server.stop()
    else:
        print("Cannot start server")
        sys.exit(1)


# Test it: curl -x http://localhost:10086/ http://sogou.com
if __name__ == "__main__":
    main()
