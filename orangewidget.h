#ifndef ORANGEWIDGET_H
#define ORANGEWIDGET_H

#include <QWidget>

namespace Ui {
class orangewidget;
}

class orangewidget : public QWidget
{
    Q_OBJECT

public:
    explicit orangewidget(QWidget *parent = nullptr);
    ~orangewidget();

private:
    Ui::orangewidget *ui;
};

#endif // ORANGEWIDGET_H
