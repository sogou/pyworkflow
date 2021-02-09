import signal
import sys
import threading

import pywf as wf


def process(task):
    request = task.get_req()
    response = task.get_resp()

    response.append_body(b"<html>\n")  # as bytes
    response.append_body(
        f"<p>{request.get_method()} "
        f"{request.get_request_uri()} "
        f"{request.get_http_version()}</p>\n"
    )
    headers = request.get_headers()
    for header in headers:
        response.append_body(f"<p>{header[0]}: {header[1]}</p>\n")
    response.append_body("</html>\n")  # as str

    response.set_http_version("HTTP/1.1")
    response.set_status_code("200")
    response.set_reason_phrase("OK")
    response.add_header_pair("Content-Type", "text/html")
    response.add_header_pair("Server", "Sogou Python3 WFHttpServer")
    seq = task.get_task_seq()
    if seq == 9:  # close after 10 requests, seq start from 0
        resp.add_header_pair("Connection", "close")


def main():

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
        sys.exit(code)


if __name__ == "__main__":
    main()
