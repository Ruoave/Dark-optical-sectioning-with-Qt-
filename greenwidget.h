#ifndef GREENWIDGET_H
#define GREENWIDGET_H

#include <QWidget>

namespace Ui {
class greenwidget;
}

class greenwidget : public QWidget
{
    Q_OBJECT

public:
    explicit greenwidget(QWidget *parent = nullptr);
    ~greenwidget();

private:
    Ui::greenwidget *ui;
};

#endif // GREENWIDGET_H
