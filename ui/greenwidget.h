#ifndef GREENWIDGET_H
#define GREENWIDGET_H

#include <QWidget>
#include <qtmaterialradiobutton.h>

namespace Ui {
class GreenWidget;
}

class GreenWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GreenWidget(QWidget *parent = nullptr);
    ~GreenWidget();

    // ========== getter 方法：供 MainWindow 读取控件当前值 ==========
    int getBackground() const;       // 读取 toggle 状态 → 返回 0(不严重) 或 1(严重)
    int getPad() const;              // 读取 toggle 状态 → 返回 0(零填充) 或 1(对称填充)
    int getDenoise() const;          // 读取三个单选按钮 → 返回 0(不去噪)/1(高斯)/2(中值)
    double getNA() const;            // 读取输入框文本 → 返回 double
    double getEmwavelength() const;  // 读取输入框文本 → 返回 double
    double getPixelsize() const;     // 读取输入框文本 → 返回 double
    int getFactor() const;           // 读取输入框文本 → 返回 int

    // ========== setter 方法：供 MainWindow 设置控件默认值 ==========
    void setBackground(int value);       // 1→setChecked(true), 0→setChecked(false)
    void setPad(int value);              // 1→setChecked(true), 0→setChecked(false)
    void setDenoise(int value);          // 0→No选中, 1→Gauss选中, 2→Mid选中
    void setNA(double value);            // setText(QString::number(value))
    void setEmwavelength(double value);  // setText(QString::number(value))
    void setPixelsize(double value);     // setText(QString::number(value))
    void setFactor(int value);           // setText(QString::number(value))

    // ========== getter 方法（高级参数）：供 MainWindow 读取第二页控件当前值 ==========
    int getThres() const;                // 读取 lineEdit 文本 → 返回 int
    double getDivide() const;            // 读取 lineEdit_2 文本 → 返回 double
    int getPadsize() const;              // 读取 lineEdit_3 文本 → 返回 int
    QString getDeg() const;              // 读取 lineEdit_4 文本 → 返回 QString（逗号分隔）
    QString getDep() const;              // 读取 lineEdit_5 文本 → 返回 QString（逗号分隔）
    QString getHl() const;               // 读取 lineEdit_6 文本 → 返回 QString（逗号分隔）
    int getIsQuick() const;              // 读取 SingleFrameRunning 选中状态 → 返回 0(未选中) 或 1(选中)

    // ========== setter 方法（高级参数）：供 MainWindow 设置第二页控件默认值 ==========
    void setThres(int value);            // setText(QString::number(value))
    void setDivide(double value);        // setText(QString::number(value))
    void setPadsize(int value);          // setText(QString::number(value))
    void setDeg(const QString &value);   // setText(value)
    void setDep(const QString &value);   // setText(value)
    void setHl(const QString &value);    // setText(value)

    // ========== 检查方法（高级参数）：供 MainWindow 判断控件是否为空 ==========
    bool isThresEmpty() const;           // 判断 lineEdit（thres）是否为空
    bool isDivideEmpty() const;          // 判断 lineEdit_2（divide）是否为空
    bool isPadsizeEmpty() const;         // 判断 lineEdit_3（padsize）是否为空

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::GreenWidget *ui;

    QtMaterialRadioButton *denoiseMethodGauss_para_denoise;
    QtMaterialRadioButton *denoiseMethodMid_para_denoise;
    QtMaterialRadioButton *denoiseMethodNo_para_denoise;
};

#endif // GREENWIDGET_H
