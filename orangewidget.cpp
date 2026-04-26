#include "orangewidget.h"
#include "ui_orangewidget.h"

orangewidget::orangewidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::orangewidget)
{
    ui->setupUi(this);
}

orangewidget::~orangewidget()
{
    delete ui;
}
