#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ui_mainwindow.h"
#include "paramsBasic.h"
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

    // Qt原生公共槽函数：响应"导出算法所用参数"菜单项点击
    // 信号源：ui->actionExport_Parameters triggered()信号
    // 流程：弹出保存文件对话框 → 用户选择路径 → 写入键值对格式txt
    // 功能：将darkSectioning中paramsBasicSet/paramsExpertSet的值导出为txt
    void on_actionExport_Parameters_triggered();

    // Qt原生公共槽函数：响应"导入参数到参数栏"菜单项点击
    // 信号源：ui->actionImport_Parameters triggered()信号
    // 流程：弹出打开文件对话框 → 用户选择txt → 解析键值对 → 设置GreenWidget控件值
    // 功能：从txt文件读取参数并显示在GreenWidget参数控件上
    void on_actionImport_Parameters_triggered();

    // Qt原生公共槽函数：响应"批量处理"菜单项点击
    // 信号源：ui->actionBatch_Process triggered()信号
    // 流程：弹出BatchDialog模态对话框 → 用户选择目录和参数 → 点击运行 → 批量处理
    // 功能：批量处理多张图片，参数从txt文件直接注入（绕过主窗口UI）
    void on_actionBatch_Process_triggered();

    // Qt原生公共槽函数：响应"当前帧另存为"菜单项点击
    // 信号源：ui->actionSave_ImageFrame triggered()信号
    // 流程：安全检查final_images非空 → 获取当前帧索引 → 弹出保存对话框 → 根据格式写出图像
    // 功能：将当前显示的处理后图片帧另存为用户指定格式（tif/jpg/png）
    void on_actionSave_ImageFrame_triggered();

    // Qt原生公共槽函数：响应"图片另存为"菜单项点击
    // 信号源：ui->actionSave_Image triggered()信号
    // 流程：安全检查final_images非空 → 弹出保存对话框 → 多帧用imwritemulti存tif/单帧可选格式 → 写出图像
    // 功能：将整个处理后图像栈另存（多帧存为多页TIFF，单帧可选tif/jpg/png）
    void on_actionSave_Image_triggered();
    
protected:
    // Qt原生事件重载：窗口大小改变时自动调用
    // 信号源：系统resizeEvent
    // 功能：窗口缩放时自动重新计算图片大小并更新显示（无需手动点击"同步帧"）
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
    DarkSectioning *darkSectioning;
    
    // ==================== 橙区自定义控件 ====================
    
    // 橙区已通过方法B（容器提升法）在.ui中将widget_orangePlaceholder提升为OrangeWidget
    
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
    QImage applyStandardToneMapping(const cv::Mat& mat);            // 色调映射函数
};

#endif // MAINWINDOW_H
