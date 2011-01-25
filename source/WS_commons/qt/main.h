#pragma once

#include <QtGui/QApplication>
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "mainwindow.h"

namespace webservice {typedef void RequestHandle; /**< Handle de la requête*/}

#include "data_structures.h"

template<class Data, class Worker>
struct Wrapper {
    Data data;
    Worker worker;
    Wrapper() : worker(data) {}
    QString run(QString request){
        webservice::RequestData query;
        query.method = webservice::GET;

        int pos = request.indexOf("?");
        if(pos < 1){
            query.path = request.toStdString();
        }else{
            query.path = request.left(pos).toStdString();
            query.raw_params = request.right(request.length() - (pos+1)).toStdString();
        }
        return worker(query, data).response.c_str();
    }
};



#define MAKE_WEBSERVICE(Data, Worker) int main(int argc , char** argv){QApplication a(argc, argv); \
    Wrapper<Data, Worker> wrap; \
    MainWindow w(boost::bind(&Wrapper<Data, Worker>::run, &wrap, _1)); \
    w.show(); \
    return a.exec(); \
}

