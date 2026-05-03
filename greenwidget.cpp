#include "greenwidget.h"
#include "ui_greenwidget.h"

#include <QFont>
#include <QTabBar>
#include <QHBoxLayout>
#include <qtmaterialradiobutton.h>

GreenWidget::GreenWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenWidget)
{
    ui->setupUi(this);

    // ========== 去噪方式单选按钮组（在 widget_denoiseRadioButtonPlaceholder 占位容器中添加） ==========
    // 为占位容器创建水平布局，三个单选按钮从左到右排列
    QHBoxLayout *hLayout_denoiseRadioBtn = new QHBoxLayout(ui->widget_denoiseRadioButtonPlaceholder);
    hLayout_denoiseRadioBtn->setSpacing(10);        // 控件之间的间距为10像素
    hLayout_denoiseRadioBtn->setContentsMargins(0, 0, 0, 0);  // 去掉四周边距

    // 第一个单选按钮：高斯平滑（使用成员变量，默认不选中，由 MainWindow 通过 setDenoise 设置）
    denoiseMethodGauss_para_denoise = new QtMaterialRadioButton(ui->widget_denoiseRadioButtonPlaceholder);
    denoiseMethodGauss_para_denoise->setText("高斯平滑");  // 设置显示文本
    denoiseMethodGauss_para_denoise->setFont(QFont("微软雅黑", 9));  // 设置字体：微软雅黑9号

    // 第二个单选按钮：中值滤波（使用成员变量，默认不选中）
    denoiseMethodMid_para_denoise = new QtMaterialRadioButton(ui->widget_denoiseRadioButtonPlaceholder);
    denoiseMethodMid_para_denoise->setText("中值滤波");    // 设置显示文本
    denoiseMethodMid_para_denoise->setFont(QFont("微软雅黑", 9));   // 设置字体：微软雅黑9号

    // 第三个单选按钮：不去噪（使用成员变量，默认不选中）
    denoiseMethodNo_para_denoise = new QtMaterialRadioButton(ui->widget_denoiseRadioButtonPlaceholder);
    denoiseMethodNo_para_denoise->setText("不去噪");      // 设置显示文本
    denoiseMethodNo_para_denoise->setFont(QFont("微软雅黑", 9));   // 设置字体：微软雅黑9号

    // 将三个单选按钮依次添加到水平布局中（从左到右排列）
    hLayout_denoiseRadioBtn->addWidget(denoiseMethodGauss_para_denoise);   // 左侧：高斯平滑
    hLayout_denoiseRadioBtn->addWidget(denoiseMethodMid_para_denoise);     // 中间：中值滤波
    hLayout_denoiseRadioBtn->addWidget(denoiseMethodNo_para_denoise);      // 右侧：不去噪


    // ========== QTabWidget 样式表设置 ==========
    // 设置 tabWidget 的整体外观
    ui->tabWidget->setStyleSheet(
        //---------- QTabWidget 整体：底部留出空间供标签下沉显示 ----------
        "QTabWidget {"
        "   border: 2px solid #55aaff;"            // 选中边框：#55aaff 蓝色（2px加粗更醒目）
        "}"

        // ---------- 标签栏整体容器（面板区域） ----------
        "QTabWidget::pane {"
        "   border: 2px solid #55aaff;"           // 主题色
        "   border-radius: 4px;"                   // 边框圆角：4像素
        "   background-color: #ffffff;"            // 面板背景色：浅灰白
        "}"

        // ---------- 标签按钮通用样式（未选中状态） ----------
        "QTabBar::tab {"
        "   min-width: 136px;"                     // 最小宽度：136像素
        "   max-width: 136px;"                     // 最大宽度：136像素（固定宽度）
        "   height: 24px;"                         // 高度：24像素
        "   font-size: 18px;"                      // 文字大小（实际字体由C++代码设置）
        "   font-family: \"Microsoft YaHei\";"     // 字体：微软雅黑
        "   color: #888888;"                       // 未选中文字颜色：中灰色
        "   background-color: #e0e0e0;"            // 未选中背景色：浅灰色
        "   border: 1px solid #3a3a3a;"            // 边框：深灰色（保持原有风格）
        "   border-bottom-left-radius: 2px;"       // 左下角圆角：2像素
        "   border-bottom-right-radius: 2px;"      // 右下角圆角：2像素
        "   border-top-left-radius: 0px;"          // 左上角无圆角（贴合面板顶部）
        "   border-top-right-radius: 0px;"         // 右上角无圆角（贴合面板顶部）
        "   padding: 2px 8px;"                     // 内边距：上下2px、左右8px
        "   outline: none;"                        // 去掉焦点虚线框
        "}"

        // ---------- 选中状态的标签（带下沉效果） ----------
        "QTabBar::tab:selected {"
        "   background-color: #c8e4ff;"            // 选中背景色：浅蓝色
        "   color: #1565C0;"                       // 选中文字颜色：深蓝色
        "   border: 2px solid #55aaff;"            // 选中边框：#55aaff 蓝色（2px加粗更醒目）
        "   font-weight: bold;"                    // 选中时文字加粗
        "   padding: 6px 8px;"                     // 内边距：上下6px、左右8px
        "}"

        // ---------- 悬停状态的标签（非选中时的悬停） ----------
        "QTabBar::tab:hover:!selected {"
        "   background-color: #d5d5d5;"            // 悬停背景色：稍深的浅灰色
        "   color: #333333;"                       // 悬停文字颜色：深黑色
        "   border: 1px solid #9e9e9e;"            // 悬停边框：#9e9e9e 灰色
        "}"
    );
}

GreenWidget::~GreenWidget()
{
    delete ui;
}

// ========== getter 方法实现 ==========

// 读取"离焦严重"toggle状态 → 返回 0(不严重) 或 1(严重)
int GreenWidget::getBackground() const
{
    return ui->isSevere_para_background->isChecked() ? 1 : 0;
}

// 读取"对称填充"toggle状态 → 返回 1(对称填充/未选中) 或 0(零填充/选中)
// toggle布局：[对称填充(左)] [toggle] [0填充(右)]
// 未选中(disabled/左)→对称填充→pad=1，选中(enabled/右)→0填充→pad=0
int GreenWidget::getPad() const
{
    return ui->padMethodSelect_para_pad->isChecked() ? 0 : 1;
}

// 读取去噪方式单选按钮 → 返回 0(不去噪)/1(高斯平滑)/2(中值滤波)
int GreenWidget::getDenoise() const
{
    if (denoiseMethodNo_para_denoise->isChecked()) {
        return 0;       // 不去噪
    } else if (denoiseMethodGauss_para_denoise->isChecked()) {
        return 1;       // 高斯平滑
    } else if (denoiseMethodMid_para_denoise->isChecked()) {
        return 2;       // 中值滤波
    }
    return 0;           // 默认不去噪
}

// 读取数值孔径输入框 → 返回 double
double GreenWidget::getNA() const
{
    return ui->Line_para_NA->text().toDouble();
}

// 读取发射波长输入框 → 返回 double
double GreenWidget::getEmwavelength() const
{
    return ui->Line_para_emwave->text().toDouble();
}

// 读取像素尺寸输入框 → 返回 double
double GreenWidget::getPixelsize() const
{
    return ui->Line_para_pixelsize->text().toDouble();
}

// 读取分辨率比例因子输入框 → 返回 int
int GreenWidget::getFactor() const
{
    return ui->Line_para_factor->text().toInt();
}

// ========== setter 方法实现 ==========

// 设置"离焦严重"toggle状态：1→选中(严重), 0→未选中(不严重)
void GreenWidget::setBackground(int value)
{
    ui->isSevere_para_background->setChecked(value == 1);
}

// 设置"对称填充"toggle状态：1→未选中(对称填充/左), 0→选中(零填充/右)
// toggle布局：[对称填充(左)] [toggle] [0填充(右)]
void GreenWidget::setPad(int value)
{
    // pad=1(对称填充)→setChecked(false/左), pad=0(零填充)→setChecked(true/右)
    ui->padMethodSelect_para_pad->setChecked(value == 0);
}

// 设置去噪方式单选按钮：0→不去噪选中, 1→高斯平滑选中, 2→中值滤波选中
void GreenWidget::setDenoise(int value)
{
    switch (value) {
    case 0:
        denoiseMethodNo_para_denoise->setChecked(true);
        break;
    case 1:
        denoiseMethodGauss_para_denoise->setChecked(true);
        break;
    case 2:
        denoiseMethodMid_para_denoise->setChecked(true);
        break;
    default:
        denoiseMethodNo_para_denoise->setChecked(true);
        break;
    }
}

// 设置数值孔径输入框文本
void GreenWidget::setNA(double value)
{
    ui->Line_para_NA->setText(QString::number(value));
}

// 设置发射波长输入框文本
void GreenWidget::setEmwavelength(double value)
{
    ui->Line_para_emwave->setText(QString::number(value));
}

// 设置像素尺寸输入框文本
void GreenWidget::setPixelsize(double value)
{
    ui->Line_para_pixelsize->setText(QString::number(value));
}

// 设置分辨率比例因子输入框文本
void GreenWidget::setFactor(int value)
{
    ui->Line_para_factor->setText(QString::number(value));
}
