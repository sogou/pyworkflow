import signal
import sys
import threading

import pywf as wf


def process(task):
    req = task.get_req()
    resp = task.get_resp()

    resp.append_body(b"<html>\n")  # as bytes
    resp.append_body(
        f"<p>{req.get_method()} "
        f"{req.get_request_uri()} "
        f"{req.get_http_version()}</p>\n"
    )
    headers = req.get_headers()
    for header in headers:
        resp.append_body(f"<p>{header[0]}: {header[1]}</p>\n")
    resp.append_body("</html>\n")  # as str

    resp.set_http_version("HTTP/1.1")
    resp.set_status_code("200")
    resp.set_reason_phrase("OK")
    resp.add_header_pair("Content-Type", "text/html")
    resp.add_header_pair("Server", "Sogou Python3 WFHttpServer")
    seq = task.get_task_seq()
    if seq == 9:  # close after 10 reqs, seq start from 0
        resp.add_header_pair("Connection", "close")


def main():
    if len(sys.argv) != 2:
        print("Usage {} <port>".format(sys.argv[0]))
        sys.exit(1)

    port = int(sys.argv[1])

    server = wf.HttpServer(process)
    stop_event = threading.Event()

    def stop(*args):
        if not stop_event.is_set():
            stop_event.set()

    for sig in (signal.SIGTERM, signal.SIGINT):
        signal.signal(sig, stop)

    # You can use server.start(socket.AddressFamily.AF_INET, "localhost", port) too
    if server.start(port) == 0:
        stop_event.wait()
        server.stop()
        """ server.stop() equal to:
        server.shutdown()
        server.wait_finish()
        """
    else:
        print("Cannot start server")
        sys.exit(1)


if __name__ == "__main__":
    main()
