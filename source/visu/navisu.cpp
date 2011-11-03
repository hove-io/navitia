#include "navisu.h"
#include "ui_navisu.h"

#include <iostream>
#include <boost/foreach.hpp>
#include <QFileDialog>
#include <QErrorMessage>
using namespace navitia::type;
navisu::navisu(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::navisu)
{
    ui->setupUi(this);
  //  MyMarbleWidget * my = new MyMarbleWidget(ui->carto);
    ui->my->setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
    ui->my->show();
  //  ui->my->setHome(2.36, 48.84, 2500);

    //ui->carto->model()->
    QCompleter * completer = new QCompleter(new NavitiaItemModel(this->d));
//    ui->first_letter_line_edit->setCompleter(completer);
}

void MyMarbleWidget::customPaint(GeoPainter* painter)
{
    GeoDataCoordinates home(8.4, 49.0, 0.0, GeoDataCoordinates::Degree);
    painter->setPen(Qt::green);
    painter->drawEllipse(home, 7, 7);
    painter->setPen(Qt::black);
    painter->drawText(home, "Hello Marble!");
}

template<class T>
std::string formatHeader(const T & t){
    std::stringstream ss;
    ss << t.name << "(ext=" << t.external_code << "; id=" << t.id << "; idx=" << t.idx << ")";
    return ss.str();
}

void navisu::menuAction(QAction * action){
    if(action == ui->actionOuvrir){
        QString filename;
        try{
            filename = QFileDialog::getOpenFileName(this, "Ouvrir un fichier de données NAViTiA2");
            ui->statusbar->showMessage("Loading " + filename + "...");
            d.load_flz(filename.toStdString());
            ui->statusbar->showMessage("Loading done " + filename);
        }catch(std::exception e){
            QErrorMessage * err = new QErrorMessage(this);
            err->showMessage(QString("Impossible d'ouvrir") + filename + " : " + e.what());
            ui->statusbar->showMessage("Load error " + filename);
            ui->tableWidget->clear();
        }catch(...){
            QErrorMessage * err = new QErrorMessage(this);
            err->showMessage(QString("Impossible d'ouvrir") + filename + " : exception inconnue");
            ui->statusbar->showMessage("Load error " + filename);
            ui->tableWidget->clear();
        }
    }
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
    resetTable(4, d.networks.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Nb lines");
    for(size_t i=0; i < d.networks.size(); ++i){
        Network & n = d.networks[i];
        setItem(i, 0, n.external_code);
        setItem(i, 1, n.id);
        setItem(i, 2, n.name);
        setItem(i, 3, n.line_list.size());
    }
}

void navisu::show_mode() {
    resetTable(4, d.modes.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Mode Type");
    for(size_t i=0; i < d.modes.size(); ++i){
        Mode & m = d.modes[i];
        setItem(i, 0, m.external_code);
        setItem(i, 1, m.id);
        setItem(i, 2, m.name);
        if(m.mode_type_idx < d.mode_types.size())
            setItem(i, 3, formatHeader(d.mode_types[m.mode_type_idx]));
    }
}

void navisu::show_mode_type(){
    resetTable(5, d.mode_types.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Nb Modes" << "Nb lines");
    for(size_t i=0; i < d.mode_types.size(); ++i){
        ModeType & mt = d.mode_types[i];
        setItem(i, 0, mt.external_code);
        setItem(i, 1, mt.id);
        setItem(i, 2, mt.name);
        setItem(i, 3, mt.mode_list.size());
        setItem(i, 4, mt.line_list.size());
    }
}

void navisu::show_city() {
    resetTable(6, d.cities.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Postal Code" << "Department" << "X" << "Y");
    for(size_t i=0; i < d.cities.size(); ++i){
        City & c = d.cities[i];
        setItem(i, 0, c.external_code);
        setItem(i, 1, c.id);
        setItem(i, 2, c.name);
        setItem(i, 3, c.main_postal_code);
        if(c.department_idx < d.departments.size())
            setItem(i, 4, formatHeader(d.departments[c.department_idx]));
        setItem(i, 5, c.coord.x);
        setItem(i, 6, c.coord.y);
    }
}

void navisu::show_connection(){
    resetTable(6, d.connections.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Departure" << "Destination" << "Duration" << "Max Duration");
    for(size_t i=0; i < d.connections.size(); ++i){
        Connection & c = d.connections[i];
        setItem(i, 0, c.external_code);
        setItem(i, 1, c.id);
        if(c.departure_stop_point_idx < d.stop_points.size())
            setItem(i, 2, formatHeader(d.stop_points[c.departure_stop_point_idx]));
        if(c.destination_stop_point_idx < d.stop_points.size())
            setItem(i, 3, formatHeader(d.stop_points[c.destination_stop_point_idx]));
        setItem(i, 4, c.duration);
        setItem(i, 5, c.max_duration);
    }
}

void navisu::show_district(){
    resetTable(6, d.districts.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Main City" << "Country" << "Nb departments");
    for(size_t i=0; i < d.districts.size(); ++i){
        District & di = d.districts[i];
        setItem(i, 0, di.external_code);
        setItem(i, 1, di.id);
        setItem(i, 2, di.name);
        if(di.main_city_idx < d.cities.size())
            setItem(i, 3, formatHeader(d.cities[di.main_city_idx]));
        if(di.country_idx < d.countries.size())
            setItem(i, 4, formatHeader(d.countries[di.country_idx]));
        setItem(i, 5, di.department_list.size());
    }

}

void navisu::show_route_point(){
    resetTable(5, d.route_points.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Order" << "Stop Point" << "Route");
    for(size_t i=0; i < d.route_points.size(); ++i){
        RoutePoint & rp = d.route_points[i];
        setItem(i, 0, rp.external_code);
        setItem(i, 1, rp.id);
        setItem(i, 2, rp.order);
        if(rp.stop_point_idx < d.stop_points.size())
            setItem(i, 3, formatHeader(d.stop_points[rp.stop_point_idx]));
        if(rp.route_idx < d.routes.size())
            setItem(i, 4, formatHeader(d.routes[rp.route_idx]));

    }
}

void navisu::show_department(){
    resetTable(6, d.departments.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Main City" << "District" << "Nb Cities");
    for(size_t i=0; i < d.departments.size(); ++i){
        Department & dpt = d.departments[i];
        setItem(i, 0, dpt.external_code);
        setItem(i, 1, dpt.id);
        setItem(i, 2, dpt.name);
        if(dpt.main_city_idx < d.cities.size())
            setItem(i, 3, formatHeader(d.cities[dpt.main_city_idx]));
        if(dpt.district_idx < d.districts.size())
            setItem(i, 4, formatHeader(d.districts[dpt.district_idx]));
        setItem(i, 5, dpt.city_list.size());
    }
}

void navisu::show_company(){
    resetTable(4, d.companies.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Nb lines");
    for(size_t i = 0; i < d.companies.size(); ++i){
        Company & c = d.companies[i];
        setItem(i, 0, c.external_code);
        setItem(i, 1, c.id);
        setItem(i, 2, c.name);
        setItem(i, 3, c.line_list.size());
    }
}

void navisu::show_vehicle(){
    resetTable(3, d.vehicles.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name");
    for(size_t i = 0; i < d.vehicles.size(); ++i){
        Vehicle & v = d.vehicles[i];
        setItem(i, 0, v.external_code);
        setItem(i, 1, v.id);
        setItem(i, 2, v.name);
    }

}

void navisu::show_country(){
    resetTable(5, d.countries.size());
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "External Code" << "Id" << "Name" << "Main City" << "Nb districts");
    for(size_t i = 0; i < d.countries.size(); ++i){
        Country & c = d.countries[i];
        setItem(i, 0, c.external_code);
        setItem(i, 1, c.id);
        setItem(i, 2, c.name);
        if(c.main_city_idx < d.cities.size())
            setItem(i, 3, formatHeader(d.cities[c.main_city_idx]));
        setItem(i, 4, c.district_list.size());
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    navisu w;
    w.show();

    return a.exec();
}
