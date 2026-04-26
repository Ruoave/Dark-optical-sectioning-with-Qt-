#include "greenwidget.h"
#include "ui_greenwidget.h"

greenwidget::greenwidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::greenwidget)
{
    ui->setupUi(this);
}

greenwidget::~greenwidget()
{
    delete ui;
}
