#encoding: utf-8
from connectors.redis_queue import RedisQueue
import os

def redis_queque_test():
    try:
        _host = os.environ["REDISHOST"]
    except:
        _host = "localhost"

    try:
        _db = os.environ["REDISDB"]
    except:
        _db = 0

    try:
        _port = os.environ["REDISPORT"]
    except:
        _port = 6379

    try:
        _password = os.environ["REDISPASSWORD"]
    except:
        _password = None

    redis_test = RedisQueue(_host, _port, _db, _password)

    assert(redis_test)
    redis_test.set("R1","REDISTEST1")
    redis_test.set("R2","REDISTEST2")
    redis_test.set("R3","REDISTEST3")
    print redis_test.get("R1")
    assert(redis_test.get("R1") == "REDISTEST1")
    assert(redis_test.get("R2") == "REDISTEST2")
    assert(redis_test.get("R3") == "REDISTEST3")
    assert(redis_test.get("R4") == None)
