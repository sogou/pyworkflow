import pywf as wf
import sys, signal
import threading

cv = threading.Condition()
def Stop(signum, frame):
    print("Stop server:", signum)
    cv.acquire()
    cv.notify()
    cv.release()

def process(t):
    req = t.get_req()
    resp = t.get_resp()
    seq = t.get_task_seq()
    resp.append_body(b"<html>\n") # as bytes
    resp.append_body("<p>{} {} {}</p>\n".format(req.get_method(), req.get_request_uri(), req.get_http_version()))
    headers = req.get_headers()
    for h in headers:
        resp.append_body("<p>{}: {}</p>\n".format(h[0], h[1]))
    resp.append_body("</html>\n") # as str
    resp.set_http_version("HTTP/1.1")
    resp.set_status_code("200")
    resp.set_reason_phrase("OK")
    resp.add_header_pair("Content-Type", "text/html")
    resp.add_header_pair("Server", "Sogou Python3 WFHttpServer")
    if seq == 9:
        resp.add_header_pair("Connection", "close")
    pass

def main():
    if(len(sys.argv) != 2):
        print("Usage {} <port>".format(sys.argv[0]))
        sys.exit(1)
    port = int(sys.argv[1])
    signal.signal(signal.SIGINT, Stop)
    server = wf.HttpServer(process)
    # You can use server.start(socket.AddressFamily.AF_INET, "localhost", port) too
    if server.start(port) == 0:
        cv.acquire()
        cv.wait()
        cv.release()
        server.stop()
        ''' server.stop() equal to:
        server.shutdown()
        server.wait_finish()
        '''
    else:
        print("Cannot start server")
        sys.exit(1)

if __name__ == "__main__":
    main()
