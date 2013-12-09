#include "ed/connectors/extcode2uri.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_extcode2uri
#include <boost/test/unit_test.hpp>

class Params{
public:
    std::string host;
    std::string db;
    std::string password;
    std::string port;
    std::string timeout;
    Params(){
        this->host = "localhost";
        this->db = "0";
        this->password = "";
        this->port = "6379";
        this->timeout = "2";

        if (getenv("REDISHOST") != nullptr){
            this->host = getenv("REDISHOST");
        }
        if (getenv("REDISDB") != nullptr){
            this->db = getenv("REDISDB");
        }
        if (getenv("REDISPASSWORD") != nullptr){
            this->password = getenv("REDISPASSWORD");
        }
        if (getenv("REDISPORT") != nullptr){
            this->host = getenv("REDISPORT");
        }
        if (getenv("REDISTIMEOUT") != nullptr){
            this->timeout = getenv("REDISTIMEOUT");
        }
    }
};

BOOST_FIXTURE_TEST_CASE(exception, Params) {
    host = "AAAAAAA";
    bool test = true;
    try{
        ed::connectors::ExtCode2Uri redis_test("host=" + host + " db=" + db + " password=" + password
                                           + " port=" + port + " timeout=" + timeout);
    }catch(const navitia::exception& ne){
        test = false;
    }
    BOOST_REQUIRE_EQUAL(test, false);
}

BOOST_FIXTURE_TEST_CASE(test1,Params) {

    ed::connectors::ExtCode2Uri redis_test("host=" + host + " db=" + db + " password=" + password
                                           + " port=" + port + " timeout=" + timeout);
    redis_test.redis.redis_connect_with_timeout();
    redis_test.redis.set("t1", "test1");
    redis_test.redis.set("t2", "test2");
    BOOST_REQUIRE_EQUAL(redis_test.redis.get("t1"), "test1");
    BOOST_REQUIRE_EQUAL(redis_test.redis.get("t2"), "test2");
}
