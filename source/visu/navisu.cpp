#include "navisu.h"
#include "ui_navisu.h"
#include "type/data.h"
#include <iostream>
#include <boost/foreach.hpp>
using namespace navitia::type;
navisu::navisu(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::navisu)
{
    ui->setupUi(this);
    Data d;
    d.load_flz("/home/tristram/idf.flz");
    std::cout << "loading done" << std::endl;
    ui->tableWidget->setColumnCount(6);
    ui->tableWidget->setRowCount(d.stop_areas.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" <<"Name" <<"City" << "id" << "X" << "Y");
    int count = 0;
    BOOST_FOREACH(StopArea & sa, d.stop_areas){

        ui->tableWidget->setItem(count ,0, new QTableWidgetItem(QString::fromStdString(sa.external_code)));
        ui->tableWidget->setItem(count ,1, new QTableWidgetItem(QString::fromStdString(sa.name)));
        if(sa.city_idx < d.cities.size())
            ui->tableWidget->setItem(count ,2, new QTableWidgetItem(QString::fromStdString(d.cities[sa.city_idx].name)));
        ui->tableWidget->setItem(count ,3, new QTableWidgetItem(QString("%1").arg(sa.id)));
        ui->tableWidget->setItem(count ,4, new QTableWidgetItem(QString("%1").arg(sa.coord.x)));
        ui->tableWidget->setItem(count ,5, new QTableWidgetItem(QString("%1").arg(sa.coord.y)));
        count++;
    }
}

navisu::~navisu()
{
    delete ui;
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    navisu w;
    w.show();

    return a.exec();
}
