#ifndef NAVISU_H
#define NAVISU_H

#include <QMainWindow>

#include "type/data.h"
namespace Ui {
    class navisu;
}

class navisu : public QMainWindow
{
    Q_OBJECT
private slots:
    void tableSelected(QString);
public:
    explicit navisu(QWidget *parent = 0);
    void resetTable(int nb_cols, int nb_rows);
    template<class T> void setItem(int col, int row, const T & str);
    void show_validity_pattern();
    void show_line();
    void show_route();
    void show_vehicle_journey();
    void show_stop_point();
    void show_stop_area();
    void show_stop_time();
    void show_network();
    void show_mode();
    void show_mode_type();
    void show_city();
    void show_connection();
    void show_route_point();
    void show_district();
    void show_department();
    void show_company();
    void show_vehicle();
    void show_country();
    ~navisu();

private:
    Ui::navisu *ui;
    navitia::type::Data d;
};

#endif // NAVISU_H
