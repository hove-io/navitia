#include "navisu.h"
#include "ui_navisu.h"

#include <iostream>
#include <boost/foreach.hpp>
using namespace navitia::type;
navisu::navisu(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::navisu)
{
    ui->setupUi(this);
    d.load_flz("/home/tristram/idf.flz");
    std::cout << "loading done" << std::endl;

}

template<class T>
std::string formatHeader(const T & t){
    std::stringstream ss;
    ss << t.name << "(ext=" << t.external_code << "; id=" << t.id << "; idx=" << t.idx << ")";
    return ss.str();
}

navisu::~navisu()
{
    delete ui;
}

void navisu::resetTable(int nb_cols, int nb_rows){
    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(nb_cols);
    ui->tableWidget->setRowCount(nb_rows);
}

template<class T>
void navisu::setItem(int col, int row, const T & t){
    ui->tableWidget->setItem(col, row, new QTableWidgetItem(QString::fromStdString(boost::lexical_cast<std::string>(t))));
}

template<>
void navisu::setItem<bool>(int col, int row, const bool & b){
    if(b)
        setItem(col, row, "True");
    else
        setItem(col, row, "False");
}

void navisu::tableSelected(QString table){
    if(table == "StopArea") show_stop_area();
    else if(table == "ValidityPattern") show_validity_pattern();
    else if(table == "Line") show_line();
    else if(table == "Route") show_route();
    else if(table == "VehicleJourney") show_vehicle_journey();
    else if(table == "StopPoint") show_stop_point();
    else if(table == "StopArea") show_stop_area();
    else if(table == "StopTime") show_stop_time();
    else if(table == "Network") show_network();
    else if(table == "Mode") show_mode();
    else if(table == "ModeType") show_mode_type();
    else if(table == "City") show_city();
    else if(table == "Connection") show_connection();
    else if(table == "RoutePoint") show_route_point();
    else if(table == "District") show_district();
    else if(table == "Department") show_department();
    else if(table == "Company") show_company();
    else if(table == "Vehicle") show_vehicle();
    else if(table == "Country") show_country();
}

void navisu::show_stop_area(){
    resetTable(7, d.stop_areas.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" <<"Name" <<"City" << "id" << "X" << "Y");
    int count = 0;
    BOOST_FOREACH(StopArea & sa, d.stop_areas){

        setItem(count ,0, sa.external_code);
        setItem(count, 1, sa.id);
        setItem(count ,2, sa.name);
        if(sa.city_idx < d.cities.size())
            setItem(count ,3, d.cities[sa.city_idx].name);
        setItem(count ,4, sa.id);
        setItem(count ,5, sa.coord.x);
        setItem(count ,6, sa.coord.y);
        count++;
    }
}

void navisu::show_validity_pattern(){
    resetTable(3, d.validity_patterns.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External code" << "Id" << "Pattern");

    for(size_t i=0; i < d.validity_patterns.size(); ++i){
        ValidityPattern & vp = d.validity_patterns[i];
        setItem(i, 0, vp.external_code);
        setItem(i, 1, vp.id);
        setItem(i, 2, vp.str());
    }
}

void navisu::show_line(){
    resetTable(13, d.lines.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External code"<< "Id" << "Name" << "code" << "Forward Name" << "Backward Name"
                                               << "Additional data" << "Color" << "Sort" << "Mode Type"
                                               << "Network" << "Foward Direction" << "Backward Direction");
    for(size_t i=0; i < d.lines.size(); ++i){
        Line & l = d.lines[i];
        setItem(i, 0, l.external_code);
        setItem(i, 1, l.id);
        setItem(i, 2, l.name);
        setItem(i, 3, l.code);
        setItem(i, 4, l.forward_name);
        setItem(i, 5, l.backward_name);
        setItem(i, 6, l.additional_data);
        setItem(i, 7, l.color);
        setItem(i, 8, l.sort);
        if(l.mode_type_idx < d.mode_types.size())
            setItem(i, 9, formatHeader(d.mode_types[l.mode_type_idx]));
        if(l.network_idx < d.networks.size())
            setItem(i, 10, formatHeader(d.networks[l.network_idx]));
        if(l.forward_direction_idx < d.stop_areas.size())
            setItem(i, 11, formatHeader(d.stop_areas[l.forward_direction_idx]));
        if(l.backward_direction_idx < d.stop_areas.size())
            setItem(i, 12, formatHeader(d.stop_points[l.backward_direction_idx]));
    }
}

void navisu::show_route(){
    resetTable(8, d.routes.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Is Frequence" << "Is Foward" << "Is Adapted" << "Line" << "Mode Type" );

    for(size_t i=0; i < d.routes.size(); ++i){
        Route & r = d.routes[i];
        setItem(i, 0, r.external_code);
        setItem(i, 1, r.id);
        setItem(i, 2, r.name);
        setItem(i, 3, r.is_frequence);
        setItem(i, 4, r.is_forward);
        setItem(i, 5, r.is_adapted);
        if(r.line_idx < d.lines.size())
            setItem(i, 6, formatHeader(d.lines[r.line_idx]));
        if(r.mode_type_idx < d.mode_types.size())
            setItem(i, 7, formatHeader(d.mode_types[r.mode_type_idx]));
    }
}

void navisu::show_vehicle_journey(){
    resetTable(8, d.vehicle_journeys.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() <<"External Code" << "Id" << "Name" << "Route" << "Company" << "Mode" << "Vehicle" << "Is adapted");

    for(size_t i=0; i < d.vehicle_journeys.size(); ++i){
        VehicleJourney & vj = d.vehicle_journeys[i];
        setItem(i, 0, vj.external_code);
        setItem(i, 1, vj.id);
        setItem(i, 2, vj.name);
        if(vj.route_idx < d.routes.size())
            setItem(i, 3, formatHeader(d.routes[vj.route_idx]));
        if(vj.company_idx < d.companies.size())
            setItem(i, 4, formatHeader(d.companies[vj.company_idx]));
        if(vj.mode_idx < d.modes.size())
            setItem(i, 5, formatHeader(d.modes[vj.mode_idx]));
        if(vj.vehicle_idx < d.vehicles.size())
            setItem(i, 6, formatHeader(d.vehicles[vj.vehicle_idx]));
        setItem(i, 7, vj.is_adapted);
    }
}

void navisu::show_stop_point(){
    resetTable(7, d.stop_points.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "StopArea" << "City" << "Mode"
                                               << "Network");
    for(size_t i=0; i < d.stop_points.size(); ++i){
        StopPoint & sp = d.stop_points[i];
        setItem(i, 0, sp.external_code);
        setItem(i, 1, sp.id);
        setItem(i, 2, sp.name);
        if(sp.stop_area_idx < d.stop_areas.size())
            setItem(i, 3, formatHeader(d.stop_areas[sp.stop_area_idx]));
        if(sp.city_idx < d.cities.size())
            setItem(i, 4, formatHeader(d.cities[sp.city_idx]));
        if(sp.mode_idx < d.modes.size())
            setItem(i, 5, formatHeader(d.modes[sp.mode_idx]));
        if(sp.network_idx < d.networks.size())
            setItem(i, 6, formatHeader(d.networks[sp.network_idx]));
    }
}

void navisu::show_stop_time(){
    resetTable(8, d.stop_times.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Arrival Time" << "Departure Time"
                                               << "Vehicle Journey" << "Order" << "ODT" << "Zone");

    for(size_t i=0; i < d.stop_times.size(); ++i){
        StopTime & st = d.stop_times[i];
        setItem(i, 0, st.external_code);
        setItem(i, 1, st.id);
        setItem(i, 2, st.arrival_time);
        setItem(i, 3, st.departure_time);
        if(st.vehicle_journey_idx < d.vehicle_journeys.size())
            setItem(i, 4, formatHeader(d.vehicle_journeys[st.vehicle_journey_idx]));
        setItem(i, 5, st.order);
        setItem(i, 6, st.ODT);
        setItem(i, 7, st.zone);
    }

}

void navisu::show_network(){

}

void navisu::show_mode() {

}

void navisu::show_mode_type(){

}

void navisu::show_city() {

}

void navisu::show_connection(){

}

void navisu::show_district(){

}

void navisu::show_route_point(){

}

void navisu::show_department(){

}

void navisu::show_company(){

}

void navisu::show_vehicle(){

}

void navisu::show_country(){

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    navisu w;
    w.show();

    return a.exec();
}
