#include "greenwidget.h"
#include "ui_greenwidget.h"

#include <QFont>
#include <QTabBar>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <qtmaterialradiobutton.h>

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
    }

    // 继续传递事件给基类处理，不截断事件流
    return QWidget::eventFilter(watched, event);
}
