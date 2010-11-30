#define ISAPI

#include "../threadpool.h"

using namespace webservice;
using namespace boost::posix_time;
struct Data{int count; boost::mutex mut; Data() : count(0){}};
class Worker{
    int i;
    public:
    Worker() : i(0) {}
        ResponseData operator()(const RequestData & data, Data & d){
            i++;
            ResponseData rd;
            std::stringstream ss;
            ss << "Hello world!!! Exécuté par ce thread : " << i << " executé au total : ";
            d.mut.lock();
            ss << d.count++;
            d.mut.unlock();

            ss << " compté par httpproc : " << count;

            rd.response = ss.str();
            rd.content_type = "text/html";
            rd.status_code = 200;
            return rd;
        }
};

static Data d = Data();
static ThreadPool<Data, Worker> tp(d, 16);
