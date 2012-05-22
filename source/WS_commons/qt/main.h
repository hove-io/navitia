#pragma once

#include <QtGui/QApplication>
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "mainwindow.h"
#include "utils/configuration.h"
#include "type/pb_utils.h"

namespace webservice {typedef void RequestHandle; /**< Handle de la requête*/}

#include "data_structures.h"

template<class Data, class Worker>
struct Wrapper {
    Data data;
    Worker worker;
    Wrapper() : worker(data) {}
    QString run(QString request){
        std::string response = worker.run_query(request.toStdString(), data);
        return QString::fromUtf8(response.c_str());
    }
};


#ifndef WIN32
#define MAKE_WEBSERVICE(Data, Worker) int main(int argc , char** argv){QApplication a(argc, argv); \
Configuration * conf = Configuration::get();\
std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );\
conf->set_string("application", std::string(argv[0]).substr(posSlash+1));\
char buf[256];\
if(getcwd(buf, 256)) conf->set_string("path",std::string(buf) + "/"); else conf->set_string("path", "unknown");\
    Wrapper<Data, Worker> wrap; \
    MainWindow w(boost::bind(&Wrapper<Data, Worker>::run, &wrap, _1)); \
    w.show(); \
    return a.exec(); \
}
#else
#define MAKE_WEBSERVICE(Data, Worker) int main(int argc , char** argv){QApplication a(argc, argv); \
    Wrapper<Data, Worker> wrap; \
    MainWindow w(boost::bind(&Wrapper<Data, Worker>::run, &wrap, _1)); \
    w.show(); \
    return a.exec(); \
}
#endif
