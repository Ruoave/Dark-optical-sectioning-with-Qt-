#include "orangebar.h"
#include "ui_orangebar.h"

OrangeBar::OrangeBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrangeBar)
{
    ui->setupUi(this);
}

OrangeBar::~OrangeBar()
{
    delete ui;
}
