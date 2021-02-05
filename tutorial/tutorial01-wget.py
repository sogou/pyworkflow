import pywf as wf
import sys, os

def wget_callback(t):
    req = t.get_req()
    resp = t.get_resp()
    state = t.get_state()
    error = t.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print("state:{} error:{} errstr:{}".format(state, error, wf.get_error_string(state, error)))
        return
    print("{} {} {}".format(req.get_method(), req.get_http_version(), req.get_request_uri()))
    req_headers = req.get_headers()
    for h in req_headers:
        print("{}: {}".format(h[0], h[1]))
    print("")
    print("{} {} {}".format(
        resp.get_http_version(), resp.get_status_code(), resp.get_reason_phrase()))
    headers = resp.get_headers()
    for h in headers:
        print("{}: {}".format(h[0], h[1]))
    print("\nbody size:{} (ignore body for a clear output)".format(len(resp.get_body())))
    #print(resp.get_body()) # body of resp is bytes type

def main():
    if(len(sys.argv) != 2):
        print("USAGE: {} <http URL>".format(sys.argv[0]))
        sys.exit(1)
    url = sys.argv[1]
    if(url[:7].lower() != "http://" and url[:8].lower() != "https://"):
        url = "http://" + url
    task = wf.create_http_task(url, redirect_max=4, retry_max=2, callback=wget_callback)
    req = task.get_req()
    req.add_header_pair("Accept", "*/*")
    req.add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)")
    req.add_header_pair("Connection", "close")
    task.start()
    wf.wait()

# Usage: python3 tutorial01-wget.py http://www.sogou.com/
if __name__ == "__main__":
    main()
