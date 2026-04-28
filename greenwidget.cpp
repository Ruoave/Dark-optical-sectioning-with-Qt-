#include "greenwidget.h"
#include "ui_greenwidget.h"

GreenWidget::GreenWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenWidget)
{
    ui->setupUi(this);
}

GreenWidget::~GreenWidget()
{
    delete ui;
}
