#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <boost/function.hpp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_go_button_clicked()
{
    QUrl url(this->ui->query_input->text());
    std::cout << QString(url.encodedQuery()).toStdString() << std::endl;
    ui->textBrowser->setPlainText(wrapper.run(this->ui->query_input->text()));
}
