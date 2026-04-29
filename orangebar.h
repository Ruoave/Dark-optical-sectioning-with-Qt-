#ifndef ORANGEBAR_H
#define ORANGEBAR_H

#include <QWidget>

namespace Ui {
class OrangeBar;
}

class OrangeBar : public QWidget
{
    Q_OBJECT

public:
    explicit OrangeBar(QWidget *parent = nullptr);
    ~OrangeBar();

private:
    Ui::OrangeBar *ui;
};

#endif // ORANGEBAR_H
