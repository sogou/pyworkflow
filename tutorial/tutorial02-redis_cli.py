import sys

import pywf as wf

class UserData:
    def __init__(self):
        pass


def redis_callback(t):
    req = t.get_req()
    resp = t.get_resp()
    state = t.get_state()
    error = t.get_error()

    if state != wf.WFT_STATE_SUCCESS:
        print(
            "state:{} error:{} errstr:{}".format(
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

    cmd = req.get_command()
    if cmd == "SET":
        user_data = t.get_user_data()
        n = wf.create_redis_task(user_data.url, retry_max=2, callback=redis_callback)
        n.get_req().set_request("GET", [user_data.key])
        wf.series_of(t).push_back(n)
        print("Redis SET request sucess. Trying to GET...")
    else:
        if val.is_string():
            print("Redis GET success. Value = {}".format(val.string_value()))
        else:
            print("Error. Not a string value")

        print("Finish")

def main():
    if len(sys.argv) != 4:
        print("USAGE: {} <redis URL> <key> <value>".format(sys.argv[0]))
        sys.exit(1)
    user_data = UserData()
    url = sys.argv[1]
    if url[:8].lower() != "redis://" and url[:9].lower() != "rediss://":
        url = "redis://" + url
    user_data.url = url
    task = wf.create_redis_task(url, retry_max=2, callback=redis_callback)
    req = task.get_req()
    req.set_request("SET", [sys.argv[2], sys.argv[3]])
    user_data.key = sys.argv[2]
    task.set_user_data(user_data)
    task.start()
    wf.wait_finish()


# Usage: python3 tutorial01-wget.py http://www.sogou.com/
if __name__ == "__main__":
    main()
