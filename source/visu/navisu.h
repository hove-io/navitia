#ifndef NAVISU_H
#define NAVISU_H

#include <QMainWindow>

namespace Ui {
    class navisu;
}

class navisu : public QMainWindow
{
    Q_OBJECT

public:
    explicit navisu(QWidget *parent = 0);
    ~navisu();

private:
    Ui::navisu *ui;
};

#endif // NAVISU_H
