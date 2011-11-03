#ifndef NAVISU_H
#define NAVISU_H

#include <QMainWindow>
#include <marble/MarbleWidget.h>
#include <marble/GeoPainter.h>
#include <QCompleter>
#include <QStringListModel>
#include "../type/data.h"

using namespace Marble;
namespace nt = navitia::type;

/** Représente les données NAViTiA sous forme Qt
  *
  * QModelIndex représente un objet, on a
  * row = le type de l'objet TC considéré
  * column = l'indexe de l'objet
  */
class NavitiaItemModel : public QAbstractItemModel
{
    Q_OBJECT
    nt::Data * d;
public:
    NavitiaItemModel(nt::Data & data) : d(&data) {}

    QModelIndex index (int row, int column, const QModelIndex & = QModelIndex() ) const{
        return createIndex(row, column);
    }

    QModelIndex parent ( const QModelIndex & ) const {
        return QModelIndex();
    }

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const {
        if(parent == QModelIndex())
            return 18;
        else
            return 0;
    }

    int columnCount ( const QModelIndex & parent  = QModelIndex() ) const {
        if(parent == QModelIndex())
            return 0;
        else
            switch(parent.row()) {
            case nt::eStopArea: return d->stop_areas.size(); break;
            default: return 0; break;
            }

    }

    QVariant data ( const QModelIndex & index, int = Qt::DisplayRole ) const {
        QString result("%1: %2 idx=%3");
        switch(index.row()){
        case nt::eStopArea:
            result.arg("StopArea")
                    .arg(QString::fromStdString(d->stop_areas[index.column()].name))
                    .arg(index.column());
            break;
        }
        return result;
    }
};

class MyMarbleWidget : public MarbleWidget
{
public:
    virtual void customPaint(GeoPainter* painter);
    MyMarbleWidget(QWidget * parent = 0) : MarbleWidget(parent){}
};

#include "type/data.h"
namespace Ui {
    class navisu;
}

class MyCompleter : public QCompleter
{
    Q_OBJECT

public:
    inline MyCompleter(QObject * parent) :
        QCompleter(parent),  m_model()
    {
        setModel(&m_model);
    }

    inline void update(QString word)
    {
        // Do any filtering you like.
        // Here we just include all items that contain word.
        QStringList filtered;
        filtered << "Hello" << "World";
        m_model.setStringList(filtered);
        m_word = word;
        complete();
    }

    inline QString word()
    {
        return m_word;
    }

private:
    QStringList m_list;
    QStringListModel m_model;
    QString m_word;
};

class navisu : public QMainWindow
{
    Q_OBJECT
private slots:
    void tableSelected(QString);
    void menuAction(QAction *);
    /*void firstLetterChanges(QString);
    void firstLetterSelected(int);*/
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
