#include "greenwidget.h"
#include "ui_greenwidget.h"

#include <QFont>
#include <QTabBar>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QApplication>
#include <qtmaterialradiobutton.h>
#include <qtmaterialtextfield.h>

GreenWidget::GreenWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenWidget)
{
    ui->setupUi(this);

    //==========toggle按钮外观设置==========
    ui->isSevere_para_background->setUseThemeColors(false); 
    ui->padMethodSelect_para_pad->setUseThemeColors(false);
    ui->isSevere_para_background->setActiveColor(QColor("#55aaff"));
    ui->padMethodSelect_para_pad->setActiveColor(QColor("#55aaff"));

    // ========== Toggle 两侧 QLabel 颜色联动 ==========
    // 逻辑：滑块拨到哪一侧，那一侧的 QLabel 显示主题色 #55aaff（选中），另一侧显示 #9e9e9e（未选中）
    // QtMaterialToggle unchecked 时滑块在左侧，checked 时滑块在右侧

    // isSevere toggle 联动：左侧 notSevereLabel，右侧 isSevereLabel
    //   unchecked(滑块在左)→notSevereLabel=#55aaff(选中), isSevereLabel=#9e9e9e(未选中)
    //   checked(滑块在右)→notSevereLabel=#9e9e9e(未选中), isSevereLabel=#55aaff(选中)
    connect(ui->isSevere_para_background, &QAbstractButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->notSevereLabel->setStyleSheet("color: #9e9e9e;");   // 左侧未选中：灰色
            ui->isSevereLabel->setStyleSheet("color: #55aaff;");    // 右侧选中：主题蓝色
        } else {
            ui->notSevereLabel->setStyleSheet("color: #55aaff;");   // 左侧选中：主题蓝色
            ui->isSevereLabel->setStyleSheet("color: #9e9e9e;");    // 右侧未选中：灰色
        }
    });

    // pad toggle 联动：左侧 zeroPad(0填充)，右侧 synPad(对称填充)
    //   unchecked(滑块在左)→zeroPad=#55aaff(选中), synPad=#9e9e9e(未选中)
    //   checked(滑块在右)→zeroPad=#9e9e9e(未选中), synPad=#55aaff(选中)
    connect(ui->padMethodSelect_para_pad, &QAbstractButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->synPad->setStyleSheet("color: #9e9e9e;");     // 左侧选中：主题蓝色
            ui->zeroPad->setStyleSheet("color: #55aaff;");      // 左侧未选中：灰色
        } else {
            ui->synPad->setStyleSheet("color: #55aaff;");     // 右侧选中：主题蓝色
            ui->zeroPad->setStyleSheet("color: #9e9e9e;");      // 右侧未选中：灰色
        }
    });

    // 设置 QLabel 初始颜色（toggle 默认 unchecked，滑块在左侧，左侧选中）
    ui->notSevereLabel->setStyleSheet("color: #55aaff;");     // 初始选中：主题蓝色
    ui->isSevereLabel->setStyleSheet("color: #9e9e9e;");      // 初始未选中：灰色
    ui->synPad->setStyleSheet("color: #55aaff;");             // 初始选中：主题蓝色
    ui->zeroPad->setStyleSheet("color: #9e9e9e;");              // 初始未选中：灰色


    //==========AutoComplete Editline外观设置==========
    ui->Line_para_emwave->setUseThemeColors(false);
    ui->Line_para_NA->setUseThemeColors(false);
    ui->Line_para_pixelsize->setUseThemeColors(false);
    ui->Line_para_factor->setUseThemeColors(false);
    ui->Line_para_emwave->setInkColor(QColor("#55aaff"));
    ui->Line_para_NA->setInkColor(QColor("#55aaff"));
    ui->Line_para_pixelsize->setInkColor(QColor("#55aaff"));
    ui->Line_para_factor->setInkColor(QColor("#55aaff"));

    // ========== AutoComplete 获得焦点时，左侧 QLabel 加粗+变主题色 ==========
    // 为4个 AutoComplete 输入框安装事件过滤器，捕获焦点进入/离开事件
    ui->Line_para_emwave->installEventFilter(this);     // 发射波长输入框
    ui->Line_para_NA->installEventFilter(this);         // 数值孔径输入框
    ui->Line_para_pixelsize->installEventFilter(this);  // 像素尺寸输入框
    ui->Line_para_factor->installEventFilter(this);     // 分辨率比例因子输入框



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


    //==========RadioButton外观设置==========
    denoiseMethodGauss_para_denoise->setUseThemeColors(false);   // 关闭主题色，使用自定义颜色
    denoiseMethodMid_para_denoise->setUseThemeColors(false);     // 关闭主题色，使用自定义颜色
    denoiseMethodNo_para_denoise->setUseThemeColors(false);      // 关闭主题色，使用自定义颜色
    denoiseMethodGauss_para_denoise->setCheckedColor(QColor("#55aaff"));  // 选中颜色：主题蓝色
    denoiseMethodMid_para_denoise->setCheckedColor(QColor("#55aaff"));    // 选中颜色：主题蓝色
    denoiseMethodNo_para_denoise->setCheckedColor(QColor("#55aaff"));     // 选中颜色：主题蓝色


    //========== 第二页参数组件外观设置 ==========
    // lineEdit ~ lineEdit_6 已在 .ui 中提升为 QtMaterialTextField
    // 设置外观：关闭主题色、显示浮动标签、标签颜色#55aaff、墨水颜色#55aaff
    ui->lineEdit->setUseThemeColors(false);        // 关闭主题色，使用自定义颜色
    ui->lineEdit_2->setUseThemeColors(false);      // 关闭主题色，使用自定义颜色
    ui->lineEdit_3->setUseThemeColors(false);      // 关闭主题色，使用自定义颜色
    ui->lineEdit_4->setUseThemeColors(false);      // 关闭主题色，使用自定义颜色
    ui->lineEdit_5->setUseThemeColors(false);      // 关闭主题色，使用自定义颜色
    ui->lineEdit_6->setUseThemeColors(false);      // 关闭主题色，使用自定义颜色

    ui->lineEdit->setInkColor(QColor("#55aaff"));     // 墨水颜色（焦点线条颜色）：主题蓝色
    ui->lineEdit_2->setInkColor(QColor("#55aaff"));   // 墨水颜色（焦点线条颜色）：主题蓝色
    ui->lineEdit_3->setInkColor(QColor("#55aaff"));   // 墨水颜色（焦点线条颜色）：主题蓝色
    ui->lineEdit_4->setInkColor(QColor("#55aaff"));   // 墨水颜色（焦点线条颜色）：主题蓝色
    ui->lineEdit_5->setInkColor(QColor("#55aaff"));   // 墨水颜色（焦点线条颜色）：主题蓝色
    ui->lineEdit_6->setInkColor(QColor("#55aaff"));   // 墨水颜色（焦点线条颜色）：主题蓝色

    ui->lineEdit->setShowLabel(true);              // 显示浮动标签
    ui->lineEdit_2->setShowLabel(true);            // 显示浮动标签
    ui->lineEdit_3->setShowLabel(true);            // 显示浮动标签
    ui->lineEdit_4->setShowLabel(true);            // 显示浮动标签
    ui->lineEdit_5->setShowLabel(true);            // 显示浮动标签
    ui->lineEdit_6->setShowLabel(true);            // 显示浮动标签

    ui->lineEdit->setLabelColor(QColor("#9e9e9e"));    // 标签颜色：灰色（未聚焦时）
    ui->lineEdit_2->setLabelColor(QColor("#9e9e9e"));  // 标签颜色：灰色（未聚焦时）
    ui->lineEdit_3->setLabelColor(QColor("#9e9e9e"));  // 标签颜色：灰色（未聚焦时）
    ui->lineEdit_4->setLabelColor(QColor("#9e9e9e"));  // 标签颜色：灰色（未聚焦时）
    ui->lineEdit_5->setLabelColor(QColor("#9e9e9e"));  // 标签颜色：灰色（未聚焦时）
    ui->lineEdit_6->setLabelColor(QColor("#9e9e9e"));  // 标签颜色：灰色（未聚焦时）

    // 设置 lineEdit ~ lineEdit_6 的浮动标签字体为微软雅黑
    // QtMaterialTextField 无公共接口设置标签字体，但 setShowLabel(true) 后标签已作为 QWidget 子控件存在
    // 通过遍历直接子对象，找到标签 QWidget 并设置字体（标签是唯一的 QWidget 子控件）
    QFont labelFont("微软雅黑", 9.8);              // 标签字体：微软雅黑 
    for (auto *textField : {ui->lineEdit, ui->lineEdit_2, ui->lineEdit_3,
                            ui->lineEdit_4, ui->lineEdit_5, ui->lineEdit_6}) {
        for (QObject *child : textField->children()) {
            QWidget *w = qobject_cast<QWidget*>(child);  // 尝试转换为QWidget
            if (w) {
                w->setFont(labelFont);            // 标签是唯一的QWidget子控件，设置字体
            }
        }
    }

    ui->lineEdit->setLabel("推荐2~15");            // 浮动标签显示推荐值
    ui->lineEdit_2->setLabel("推荐0.5");           // 浮动标签显示推荐值    
    ui->lineEdit_3->setLabel("推荐15~40");         // 浮动标签标签推荐值
    ui->lineEdit_4->setLabel("推荐6,3");           // 浮动标签显示推荐值
    ui->lineEdit_5->setLabel("推荐3,2或2,2");      // 浮动标签显示推荐值
    ui->lineEdit_6->setLabel("推荐1,1");           // 浮动标签显示推荐值

    // 第二页 lineEdit~lineEdit_6 事件过滤器：获得焦点时左侧 QLabel 加粗+变主题色
    ui->lineEdit->installEventFilter(this);             // thres 输入框
    ui->lineEdit_2->installEventFilter(this);           // divide 输入框
    ui->lineEdit_3->installEventFilter(this);           // padsize 输入框
    ui->lineEdit_4->installEventFilter(this);           // deg 输入框
    ui->lineEdit_5->installEventFilter(this);           // dep 输入框
    ui->lineEdit_6->installEventFilter(this);           // hl 输入框


    // 第二页SingleFrameRunning checkbox组件外观设置
    ui->SingleFrameRunning->setUseThemeColors(false);                   // 关闭主题颜色
    ui->SingleFrameRunning->setCheckedColor(QColor("#55aaff"));         // 选中时图标显示蓝色
    
    // ========== SingleFrameRunning 选中状态文字颜色切换 ==========
    // 信号源：ui->SingleFrameRunning（QtMaterialCheckBox）的toggled(bool)信号
    // 功能：选中时文字颜色变为#55aaff，未选中时为黑色
    void (QAbstractButton::*toggledSignal)(bool) = &QAbstractButton::toggled;
    connect(ui->SingleFrameRunning, toggledSignal, this, [this](bool checked) {
        if (checked) {
            // 选中时：文字颜色变为主题蓝色 并且加粗
            ui->SingleFrameRunning->setTextColor(QColor("#55aaff"));
            QFont font = ui->SingleFrameRunning->font();
            font.setBold(true);
            ui->SingleFrameRunning->setFont(font);
        } else {
            // 未选中时：文字颜色变更为黑色，恢复普通字体
            ui->SingleFrameRunning->setTextColor(Qt::black);
            QFont font = ui->SingleFrameRunning->font();
            font.setBold(false);
            ui->SingleFrameRunning->setFont(font);
        }
    });
    

    // ========== QTabWidget 样式表设置 ==========
    // 设置 tabWidget 的整体外观
    ui->tabWidget->setStyleSheet(

        // ---------- 标签栏整体容器（面板区域） ----------
        "QTabWidget::pane {"
        "   border: 2px solid #9e9e9e;"           // 主题色
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
        "   outline: none;"                        // 去掉焦点虚线框
        "}"

        // ---------- 悬停状态的标签（非选中时的悬停） ----------
        "QTabBar::tab:hover:!selected {"
        "   background-color: #d5d5d5;"            // 悬停背景色：稍深的浅灰色
        "   color: #333333;"                       // 悬停文字颜色：深黑色
        "   border: 1px solid #9e9e9e;"            // 悬停边框：#9e9e9e 灰色
        "}"
    );

    // ========== QTabWidget 焦点联动：内部任意控件获得焦点时边框变蓝 ==========
    // 通过 QApplication::focusChanged 全局信号监听焦点变化
    // 判断新焦点控件是否属于 tabWidget 的后代，动态切换 pane 边框颜色
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now) {
        if (now && ui->tabWidget->isAncestorOf(now)) {
            // 焦点在 tabWidget 内部 → pane 边框变主题蓝色
            ui->tabWidget->setStyleSheet(
                "QTabWidget::pane {"
                "   border: 2px solid #55aaff;"           // 焦点在内：主题蓝色边框
                "   border-radius: 4px;"
                "   background-color: #ffffff;"
                "}"
                "QTabBar::tab {"
                "   min-width: 136px;"
                "   max-width: 136px;"
                "   height: 24px;"
                "   font-size: 18px;"
                "   font-family: \"Microsoft YaHei\";"
                "   color: #888888;"
                "   background-color: #e0e0e0;"
                "   border: 1px solid #3a3a3a;"
                "   border-bottom-left-radius: 2px;"
                "   border-bottom-right-radius: 2px;"
                "   border-top-left-radius: 0px;"
                "   border-top-right-radius: 0px;"
                "   padding: 2px 8px;"
                "   outline: none;"
                "}"
                "QTabBar::tab:selected {"
                "   background-color: #c8e4ff;"
                "   color: #1565C0;"
                "   border: 2px solid #55aaff;"
                "   font-weight: bold;"
                "   padding: 6px 8px;"
                "   outline: none;"
                "}"
                "QTabBar::tab:hover:!selected {"
                "   background-color: #d5d5d5;"
                "   color: #333333;"
                "   border: 1px solid #9e9e9e;"
                "}"
            );
        } else {
            // 焦点在 tabWidget 外部 → pane 边框变灰色
            ui->tabWidget->setStyleSheet(
                "QTabWidget::pane {"
                "   border: 2px solid #9e9e9e;"           // 焦点在外：灰色边框
                "   border-radius: 4px;"
                "   background-color: #ffffff;"
                "}"
                "QTabBar::tab {"
                "   min-width: 136px;"
                "   max-width: 136px;"
                "   height: 24px;"
                "   font-size: 18px;"
                "   font-family: \"Microsoft YaHei\";"
                "   color: #888888;"
                "   background-color: #e0e0e0;"
                "   border: 1px solid #3a3a3a;"
                "   border-bottom-left-radius: 2px;"
                "   border-bottom-right-radius: 2px;"
                "   border-top-left-radius: 0px;"
                "   border-top-right-radius: 0px;"
                "   padding: 2px 8px;"
                "   outline: none;"
                "}"
                "QTabBar::tab:selected {"
                "   background-color: #c8e4ff;"
                "   color: #1565C0;"
                "   border: 2px solid #55aaff;"
                "   font-weight: bold;"
                "   padding: 6px 8px;"
                "   outline: none;"
                "}"
                "QTabBar::tab:hover:!selected {"
                "   background-color: #d5d5d5;"
                "   color: #333333;"
                "   border: 1px solid #9e9e9e;"
                "}"
            );
        }
    });

    // ========== 设置 tabWidget 初始索引为 0（第一个标签页） ==========
    ui->tabWidget->setCurrentIndex(0);
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

// ========== getter 方法实现（高级参数：第二页 lineEdit ~ lineEdit_6） ==========

// 读取 thres 输入框 → 返回 int
int GreenWidget::getThres() const
{
    return ui->lineEdit->text().toInt();
}

// 读取 divide 输入框 → 返回 double
double GreenWidget::getDivide() const
{
    return ui->lineEdit_2->text().toDouble();
}

// 读取 padsize 输入框 → 返回 int
int GreenWidget::getPadsize() const
{
    return ui->lineEdit_3->text().toInt();
}

// 读取 deg 输入框 → 返回 QString（逗号分隔，如 "6,3"）
QString GreenWidget::getDeg() const
{
    return ui->lineEdit_4->text();
}

// 读取 dep 输入框 → 返回 QString（逗号分隔，如 "3,3"）
QString GreenWidget::getDep() const
{
    return ui->lineEdit_5->text();
}

// 读取 hl 输入框 → 返回 QString（逗号分隔，如 "1,1"）
QString GreenWidget::getHl() const
{
    return ui->lineEdit_6->text();
}

// 读取 SingleFrameRunning 选中状态 → 返回 0(未选中) 或 1(选中)
int GreenWidget::getIsQuick() const
{
    return ui->SingleFrameRunning->isChecked() ? 1 : 0;
}

// ========== setter 方法实现（高级参数：第二页 lineEdit ~ lineEdit_6） ==========

// 设置 thres 输入框文本
void GreenWidget::setThres(int value)
{
    ui->lineEdit->setText(QString::number(value));
}

// 设置 divide 输入框文本
void GreenWidget::setDivide(double value)
{
    ui->lineEdit_2->setText(QString::number(value));
}

// 设置 padsize 输入框文本
void GreenWidget::setPadsize(int value)
{
    ui->lineEdit_3->setText(QString::number(value));
}

// 设置 deg 输入框文本（逗号分隔字符串）
void GreenWidget::setDeg(const QString &value)
{
    ui->lineEdit_4->setText(value);
}

// 设置 dep 输入框文本（逗号分隔字符串）
void GreenWidget::setDep(const QString &value)
{
    ui->lineEdit_5->setText(value);
}

// 设置 hl 输入框文本（逗号分隔字符串）
void GreenWidget::setHl(const QString &value)
{
    ui->lineEdit_6->setText(value);
}

// ========== 检查方法实现（高级参数）：供 MainWindow 判断控件是否为空 ==========

// 判断 thres 输入框是否为空
bool GreenWidget::isThresEmpty() const
{
    return ui->lineEdit->text().isEmpty();
}

// 判断 divide 输入框是否为空
bool GreenWidget::isDivideEmpty() const
{
    return ui->lineEdit_2->text().isEmpty();
}

// 判断 padsize 输入框是否为空
bool GreenWidget::isPadsizeEmpty() const
{
    return ui->lineEdit_3->text().isEmpty();
}

// ========== 事件过滤器：AutoComplete 获得焦点时左侧 QLabel 加粗+变主题色 ==========
// 捕获4个 AutoComplete 输入框的 FocusIn/FocusOut 事件
// FocusIn → 对应左侧 QLabel 文本加粗+变#55aaff
// FocusOut → 对应左侧 QLabel 恢复正常字重+默认颜色
bool GreenWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 只处理焦点进入和焦点离开事件
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
        // 根据被监视的控件，找到对应的左侧 QLabel
        QLabel *label = nullptr;
        if (watched == ui->Line_para_emwave) {
            label = ui->emwaveLabel;            // 发射波长标签
        } else if (watched == ui->Line_para_NA) {
            label = ui->NALabel;                // 数值孔径标签
        } else if (watched == ui->Line_para_pixelsize) {
            label = ui->pixelLabel;             // 像素尺寸标签
        } else if (watched == ui->Line_para_factor) {
            label = ui->factorLabel;            // 分辨率比例因子标签
        } else if (watched == ui->lineEdit) {
            label = ui->Label;                  // thres 标签
        } else if (watched == ui->lineEdit_2) {
            label = ui->Label_2;                // divide 标签
        } else if (watched == ui->lineEdit_3) {
            label = ui->Label_3;                // padsize 标签
        } else if (watched == ui->lineEdit_4) {
            label = ui->Label_4;                // deg 标签
        } else if (watched == ui->lineEdit_5) {
            label = ui->Label_5;                // dep 标签
        } else if (watched == ui->lineEdit_6) {
            label = ui->Label_6;                // hl 标签
        }

        if (label) {
            QFont font = label->font();         // 获取当前字体
            if (event->type() == QEvent::FocusIn) {
                // 获得焦点：加粗+主题蓝色
                font.setBold(true);             // 设置字体为加粗
                label->setFont(font);           // 应用字体
                label->setStyleSheet("color: #55aaff;");  // 选中颜色：主题蓝色
            } else {
                // 失去焦点：恢复正常字重+默认颜色
                font.setBold(false);            // 恢复字体为正常字重
                label->setFont(font);           // 应用字体
                label->setStyleSheet("");       // 清除样式，恢复默认颜色
            }
        }

        // 第二页 lineEdit~lineEdit_6：焦点变化时切换浮动标签颜色
        // 获得焦点 → 浮动标签变主题蓝色#55aaff；失去焦点 → 浮动标签变灰色#9e9e9e
        QtMaterialTextField *textField = nullptr;
        if (watched == ui->lineEdit) {
            textField = ui->lineEdit;           // thres 输入框
        } else if (watched == ui->lineEdit_2) {
            textField = ui->lineEdit_2;         // divide 输入框
        } else if (watched == ui->lineEdit_3) {
            textField = ui->lineEdit_3;         // padsize 输入框
        } else if (watched == ui->lineEdit_4) {
            textField = ui->lineEdit_4;         // deg 输入框
        } else if (watched == ui->lineEdit_5) {
            textField = ui->lineEdit_5;         // dep 输入框
        } else if (watched == ui->lineEdit_6) {
            textField = ui->lineEdit_6;         // hl 输入框
        }

        if (textField) {
            if (event->type() == QEvent::FocusIn) {
                textField->setLabelColor(QColor("#55aaff"));  // 获得焦点：标签变主题蓝色
            } else {
                textField->setLabelColor(QColor("#9e9e9e"));  // 失去焦点：标签变灰色
            }
        }
    }

    // 继续传递事件给基类处理，不截断事件流
    return QWidget::eventFilter(watched, event);
}
