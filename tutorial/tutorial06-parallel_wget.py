import sys

import pywf as wf


class Context:
    def __init__(self):
        pass


def parallel_callback(p):
    sz = p.size()
    for i in range(sz):
        ctx = p.series_at(i).get_context()
        if ctx.state == wf.WFT_STATE_SUCCESS:
            print(
                "url:{} chunked:{} keep_alive:{} status_code:{} http_version:{}".format(
                    ctx.url,
                    ctx.chunked,
                    ctx.keep_alive,
                    ctx.status_code,
                    ctx.http_version,
                )
            )
        else:
            print("url:{} state:{} error:{}".format(ctx.url, ctx.state, ctx.error))


def http_callback(t):
    ctx = wf.series_of(t).get_context()
    ctx.state = t.get_state()
    ctx.error = t.get_error()
    # Attention: YOU ARE NOT ALLOW TO OWN RESP AFTER THIS CALLBACK, SO MAKE A COPY
    resp = t.get_resp()
    ctx.chunked = resp.is_chunked()
    ctx.keep_alive = resp.is_keep_alive()
    ctx.status_code = resp.get_status_code()
    ctx.http_version = resp.get_http_version()
    ctx.body = resp.get_body()


def main():
    parallel_work = wf.create_parallel_work(parallel_callback)
    for url in sys.argv[1:]:
        if url[:7].lower() != "http://" and url[:8].lower() != "https://":
            url = "http://" + url
        task = wf.create_http_task(
            url, redirect_max=4, retry_max=2, callback=http_callback
        )
        req = task.get_req()
        req.add_header_pair("Accept", "*/*")
        req.add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)")
        req.add_header_pair("Connection", "close")
        ctx = Context()
        ctx.url = url
        series = wf.create_series_work(task, None)
        series.set_context(ctx)
        parallel_work.add_series(series)
    wf.start_series_work(parallel_work, None)
    wf.wait_finish()


# Usage: python3 tutorial06-parallel_wget.py https://www.sogou.com/ https://zhihu.sogou.com/
if __name__ == "__main__":
    main()
