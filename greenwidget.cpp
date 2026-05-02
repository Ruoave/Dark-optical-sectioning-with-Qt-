#include "greenwidget.h"
#include "ui_greenwidget.h"

#include <QFont>
#include <QTabBar>

GreenWidget::GreenWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenWidget)
{
    ui->setupUi(this);

    // ========== 设置 Tab 标签栏字体（必须用 C++ 代码设置） ==========


    // ========== QTabWidget 样式表设置 ==========
    // 设置 tabWidget 的整体外观
    ui->tabWidget->setStyleSheet(
        // ---------- QTabWidget 整体：底部留出空间供标签下沉显示 ----------
        // "QTabWidget {"
        // "   border: 2px solid #55aaff;"            // 选中边框：#55aaff 蓝色（2px加粗更醒目）
        // "}"

        // ---------- 标签栏整体容器（面板区域） ----------
        "QTabWidget::pane {"
        "   border: 2px solid #55aaff;"           // 主题色
        "   border-radius: 4px;"                   // 边框圆角：4像素
        "   background-color: #ffffff;"            // 面板背景色：浅灰白
        "}"

        // ---------- 标签按钮通用样式（未选中状态） ----------
        "QTabBar::tab {"
        "   min-width: 120px;"                     // 最小宽度：120像素
        "   max-width: 120px;"                     // 最大宽度：120像素（固定宽度）
        "   height: 24px;"                         // 高度：24像素
        "   font-size: 20px;"                      // 文字大小（实际字体由C++代码设置）
        "   font-family: \"Microsoft YaHei\";"     // 字体：微软雅黑
        "   color: #888888;"                       // 未选中文字颜色：中灰色
        "   background-color: #e0e0e0;"            // 未选中背景色：浅灰色
        "   border: 1px solid #3a3a3a;"            // 边框：深灰色（保持原有风格）
        "   border-bottom-left-radius: 2px;"       // 左下角圆角：2像素
        "   border-bottom-right-radius: 2px;"      // 右下角圆角：2像素
        "   border-top-left-radius: 0px;"          // 左上角无圆角（贴合面板顶部）
        "   border-top-right-radius: 0px;"         // 右上角无圆角（贴合面板顶部）
        "   padding: 2px 8px;"                     // 内边距：上下2px、左右8px
        "}"

        // ---------- 选中状态的标签（带下沉效果） ----------
        "QTabBar::tab:selected {"
        "   background-color: #c8e4ff;"            // 选中背景色：浅蓝色
        "   color: #1565C0;"                       // 选中文字颜色：深蓝色
        "   border: 2px solid #55aaff;"            // 选中边框：#55aaff 蓝色（2px加粗更醒目）
        "   font-weight: bold;"                    // 选中时文字加粗
        "   padding: 6px 8px;"                     // 内边距：上下2px、左右8px
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
