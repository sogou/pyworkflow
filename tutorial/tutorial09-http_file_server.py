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


def pread_callback(t):
    state = t.get_state()
    error = t.get_error()
    resp = t.get_user_data()
    args = t.get_args()

    os.close(args.get_fd())
    if state != wf.WFT_STATE_SUCCESS or t.get_retval() < 0:
        resp.set_status_code("503")
        resp.append_body("<html>503 Internal Server Error</html>")
    else:
        resp.append_body(args.get_content())

def process(t):
    req = t.get_req()
    resp = t.get_resp()

    request_uri = req.get_request_uri()
    if request_uri == "/":
        path = "/index.html"
    else:
        index = request_uri.find("?")
        if index >= 0:
            request_uri = request_uri[0:index]
        path = "." + request_uri

    try:
        fd = os.open(path, os.O_RDONLY)
    except:
        resp.set_status_code("404")
        resp.append_body("<html>404 Not Found</html>")
        return

    series = wf.series_of(t)
    size = os.lseek(fd, 0, os.SEEK_END)
    pread_task = wf.create_pread_task(fd, size, 0, pread_callback)
    pread_task.set_user_data(resp)
    series.push_back(pread_task)

def main():
    argc = len(sys.argv)
    if argc != 2 and argc != 3 and argc != 5:
        print("Usage {} <port> [root path] [cert file] [key file]".format(sys.argv[0]))
        sys.exit(1)
    port = int(sys.argv[1])
    signal.signal(signal.SIGINT, Stop)
    server = wf.HttpServer(process)

    if argc >= 4:
        ret = server.start(port, sys.argv[3], sys.argv[4])
    else:
        ret = server.start(port)

    if ret == 0:
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
