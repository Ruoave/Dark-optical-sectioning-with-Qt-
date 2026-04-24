#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ui_mainwindow.h"
#include "params.h"
#include "darkSectioning.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <windows.h>
#include <ViewMat.h>
#include <vector>

// Material组件头文件引入
#include "qtmaterialtoggle.h"
#include "qtmaterialcheckbox.h"
#include "qtmaterialtextfield.h"
#include "qtmaterialraisedbutton.h"
#include "qtmaterialflatbutton.h"
#include "qtmaterialiconbutton.h"
#include "qtmaterialprogress.h"
#include "qtmaterialslider.h"
#include "lib/qtmaterialtheme.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Qt原生公共槽函数：响应界面按钮点击信号
    // 信号源：ui->pushButton clicked信号
    // 功能：运行Dark Sectioning处理
    void on_pushButton_clicked();
    
    // Qt原生公共槽函数：响应浏览按钮点击信号
    // 信号源：ui->pushButton_browse clicked信号
    // 功能：打开文件选择对话框选择输入图片路径
    void on_pushButton_browse_clicked();
    
    // Qt原生公共槽函数：响应浏览按钮点击信号
    // 信号源：ui->pushButton_browseOutput clicked信号
    // 功能：打开文件夹选择对话框选择输出目录
    void on_pushButton_browseOutput_clicked();
    
    // 项目自定义槽函数：处理帧切换
    // 信号源：前后箭头按钮clicked()信号
    // 功能：切换图像帧
    void onFramePrev();
    void onFrameNext();
    void onFramePrev2();
    void onFrameNext2();
    
    // 项目自定义槽函数：处理同步按钮点击
    // 信号源：ui->pushButton_sync clicked信号
    // 功能：同步处理前后图片的显示帧数
    void onSyncFrames();
    
    // 辅助函数：更新图像显示
    void updateImageDisplay();

private:
    Ui::MainWindow *ui;
    DarkSectioning *darkSectioning;
    
    // Material组件声明
    QtMaterialToggle *m_toggle;               // Material设计风格开关控件
    QtMaterialCheckBox *m_checkbox;           // Material设计风格复选框控件
    QList<QtMaterialTextField*> m_textFields; // Material设计风格文本输入框列表
    QtMaterialRaisedButton *m_runButton;      // Material设计风格凸起按钮
    QtMaterialFlatButton *m_browseButton;     // Material设计风格扁平按钮
    QtMaterialFlatButton *m_browseOutputButton; // Material设计风格扁平按钮
    QtMaterialIconButton *m_prevButton;       // Material设计风格图标按钮
    QtMaterialIconButton *m_nextButton;       // Material设计风格图标按钮
    QtMaterialIconButton *m_prevButton2;      // Material设计风格图标按钮
    QtMaterialIconButton *m_nextButton2;      // Material设计风格图标按钮
    QtMaterialProgress *m_progressBar;        // Material设计风格进度条
    QtMaterialSlider *m_slider1;              // Material设计风格滑块
    QtMaterialSlider *m_slider2;              // Material设计风格滑块
    QtMaterialRaisedButton *m_syncButton;     // Material设计风格凸起按钮
    
    // 图像帧数据
    std::vector<cv::Mat> imageStack;          // 处理前图像栈
    std::vector<cv::Mat> final_images;        // 处理后图像栈
    int currentFrame;                         // 当前帧索引
    bool isSyncMode;                          // 同步模式标志
};

#endif // MAINWINDOW_H
