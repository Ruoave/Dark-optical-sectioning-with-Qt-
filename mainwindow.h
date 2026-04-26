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

// Material组件头文件引入（严格使用libs文件夹下的公共接口头文件）
// 仅包含蓝区使用的Material组件（绿区和橙区组件已移除/迁移）
#include "qtmaterialraisedbutton.h"
#include "qtmaterialflatbutton.h"

// 引入橙区自定义控件头文件（用于在主窗口中嵌入OrangeWidget）
#include "orangewidget.h"

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
    // 信号源：ui->pushButton_run clicked信号
    // 功能：运行Dark Sectioning处理，保持原有业务逻辑不变
    void on_pushButton_run_clicked();
    
    // Qt原生公共槽函数：响应浏览按钮点击信号
    // 信号源：ui->pushButton_browse clicked信号
    // 功能：打开文件选择对话框选择输入图片路径
    void on_pushButton_browse_clicked();
    
    // Qt原生公共槽函数：响应浏览按钮点击信号
    // 信号源：ui->pushButton_browseOutput clicked信号
    // 功能：打开文件夹选择对话框选择输出目录
    void on_pushButton_browseOutput_clicked();
    
protected:
    // Qt原生事件重载：窗口大小改变时自动调用
    // 信号源：系统resizeEvent
    // 功能：窗口缩放时自动重新计算图片大小并更新显示（无需手动点击"同步帧"）
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
    DarkSectioning *darkSectioning;
    
    // ==================== 橙区自定义控件指针 ====================
    
    // OrangeWidget对象指针（橙区所有功能已封装在此自定义控件中）
    // 主窗口通过此指针调用橙区的公共接口（如setInputFilePath、startProcessing等）
    OrangeWidget *m_orangeWidget;                // 橙区双图显示+控制条组件
    
    // ==================== Material组件声明 ===
    
    // ==================== 辅助函数声明（按区域分块）====================
    
    // ---------- 【红区】菜单栏区域 ----------
    void initRedAreaComponents();                 // 红区Material组件初始化
    void setupRedAreaInLayout();                  // 红区组件嵌入UI布局
    void connectRedAreaSignals();                 // 红区信号槽连接
    
    // ---------- 【蓝区】路径与操作按钮区 ----------
    void initBlueAreaComponents();                // 蓝区Material组件初始化
    void setupBlueAreaInLayout();                 // 蓝区组件嵌入UI布局
    void connectBlueAreaSignals();                // 蓝区信号槽连接
    
    // ---------- 【紫区】运行日志栏 ----------
    void initPurpleAreaComponents();              // 紫区Material组件初始化
    void setupPurpleAreaInLayout();               // 紫区组件嵌入UI布局
    void connectPurpleAreaSignals();              // 紫区信号槽连接
    
    // ---------- 全局功能函数 ----------
    void applyMaterialTheme();                    // 应用Material主题颜色#55aaff
    QImage applyStandardToneMapping(const cv::Mat& mat);            // 色调映射函数
    
    // ---------- 【新增】橙区嵌入与管理函数 ----------
    void embedOrangeWidget();                     // 将OrangeWidget嵌入主窗口指定位置
};

#endif // MAINWINDOW_H
