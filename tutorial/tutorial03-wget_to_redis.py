import sys

import pywf as wf

class Context:
    def __init__(self):
        pass

def redis_callback(t):
    req = t.get_req()
    resp = t.get_resp()
    state = t.get_state()
    error = t.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print(
            "redis: state:{} error:{} errstr:{}".format(
                state, error, wf.get_error_string(state, error)
            )
        )
        return
    else:
        val = resp.get_result()
        if val.is_error():
            print(
                "Error reply. Need a password?"
            )
            return

    context = wf.series_of(t).get_context()
    print("redis SET success: key: {}, value size: {}".format(context.http_url, context.body_len))
    context.success = True

def http_callback(t):
    req = t.get_req()
    resp = t.get_resp()
    state = t.get_state()
    error = t.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print(
            "http: state:{} error:{} errstr:{}".format(
                state, error, wf.get_error_string(state, error)
            )
        )
        return

    body_len = resp.get_body_size()
    if body_len == 0:
        print("Error: empty Http body!")
        return

    series = wf.series_of(t)
    context = series.get_context()
    context.body_len = body_len
    redis_url = context.redis_url
    redis_task = wf.create_redis_task(redis_url, retry_max=2, callback=redis_callback)
    redis_task.get_req().set_request("SET", [context.http_url, resp.get_body()])
    series.push_back(redis_task)

def series_callback(s):
    context = s.get_context()
    print("Series finished.", end=" ")
    if context.success:
        print("all success")
    else:
        print("failed")

def main():
    if len(sys.argv) != 3:
        print("USAGE: {} <http URL> <redis URL>".format(sys.argv[0]))
        sys.exit(1)
    context = Context()
    context.success = False

    url = sys.argv[1]
    if url[:7].lower() != "http://" and url[:8].lower() != "https://":
        url = "https://" + url
    context.http_url = url

    url = sys.argv[2]
    if url[:8].lower() != "redis://" and url[:9].lower() != "rediss://":
        url = "redis://" + url
    context.redis_url = url

    task = wf.create_http_task(context.http_url, 3, 2, http_callback)
    req = task.get_req()
    req.add_header_pair("Accept", "*/*")
    req.add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)")
    req.add_header_pair("Connection", "close")

    # Limit the http response to size 20M
    task.get_resp().set_size_limit(20 * 1024 * 1024)

    # no more than 30 seconds receiving http response
    task.set_receive_timeout(30 * 1000)

    series = wf.create_series_work(task, series_callback)
    series.set_context(context)
    series.start()

    wf.wait_finish()


# Usage: python3 tutorial03-wget_to_redis <http url> <redis url>
if __name__ == "__main__":
    main()
