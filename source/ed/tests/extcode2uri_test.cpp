#include "ed/connectors/extcode2uri.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_extcode2uri
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(exception) {

    std::string host("localhost");
    std::string db("0");
    std::string password("password");
    std::string port("6379");
    std::string timeout("2");

    if (getenv("REDISHOST") != nullptr){
        host = getenv("REDISHOST");
    }
    if (getenv("REDISDB") != nullptr){
        db = getenv("REDISDB");
    }
    if (getenv("REDISPASSWORD") != nullptr){
        password = getenv("REDISPASSWORD");
    }
    if (getenv("REDISPORT") != nullptr){
        host = getenv("REDISPORT");
    }
    if (getenv("REDISTIMEOUT") != nullptr){
        timeout = getenv("REDISTIMEOUT");
    }
    host = "AAAAAAA";
    ed::connectors::ExtCode2Uri redis_test("host=" + host + " db=" + db + " password=" + password + " port=" + port + " timeout=" + timeout);    
    BOOST_REQUIRE_THROW(redis_test.redis.redis_connect_with_timeout(), navitia::exception);

}
BOOST_AUTO_TEST_CASE(test1) {

    std::string host("localhost");
    std::string db("0");
    std::string password("password");
    std::string port("6379");
    std::string timeout("2");

    if (getenv("REDISHOST") != nullptr){
        host = getenv("REDISHOST");
    }
    if (getenv("REDISDB") != nullptr){
        db = getenv("REDISDB");
    }
    if (getenv("REDISPASSWORD") != nullptr){
        password = getenv("REDISPASSWORD");
    }
    if (getenv("REDISPORT") != nullptr){
        host = getenv("REDISPORT");
    }
    if (getenv("REDISTIMEOUT") != nullptr){
        timeout = getenv("REDISTIMEOUT");
    }

    ed::connectors::ExtCode2Uri redis_test("host=" + host + " db=" + db + " password=" + password + " port=" + port + " timeout=" + timeout);
    redis_test.redis.redis_connect_with_timeout();
    redis_test.redis.set("t1", "test1");
    redis_test.redis.set("t2", "test2");
    BOOST_REQUIRE_EQUAL(redis_test.redis.get("t1"), "test1");
    BOOST_REQUIRE_EQUAL(redis_test.redis.get("t2"), "test2");

}
