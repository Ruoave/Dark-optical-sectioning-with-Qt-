#ifndef GREENWIDGET_H
#define GREENWIDGET_H

#include <QWidget>

namespace Ui {
class GreenWidget;
}

class GreenWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GreenWidget(QWidget *parent = nullptr);
    ~GreenWidget();

private:
    Ui::GreenWidget *ui;
};

#endif // GREENWIDGET_H
