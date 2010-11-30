#define FCGI

#include "../threadpool.h"
#include <iostream>
#include "boost/date_time/posix_time/posix_time_types.hpp"

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

            rd.response = ss.str();
            rd.content_type = "text/html";
            rd.status_code = 200;
            return rd;
        }
};

int main(int, char**){

    Data d;
    ThreadPool<Data, Worker> tp(d, 16);
    tp.run_fastcgi();
    /*for(int threads = 1; threads <= 64; threads *= 2){
        for(int size = 1000; size <= 100*1000*1000; size *= 10){
            //Un gros paquet de données pour être sur qu'il n'y a pas de ralentissements liés au passage des données
            RequestHandle<FCGI> req;
            std::string str;
            str.resize(size, ' ');
            req.data.data = str;

            Data d;

            ThreadPool<Data, Worker, FCGI> tp(d, 10, 1);
            ptime start = (microsec_clock::local_time());
            for(int i=0; i<10000; i++){
                tp.push(req);
            }

            tp.stop();
            std::cout << "Nb threads : " << threads << " — Taille requête : "  << size/1000 << " ko"
                    << " – Durée : " << (microsec_clock::local_time() - start).total_milliseconds() << " ms" << std::endl;
        }
    }
    //tp.join();*/
    return 0;
}

