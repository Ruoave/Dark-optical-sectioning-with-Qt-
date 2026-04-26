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
#include "qtmaterialtoggle.h"
#include "qtmaterialcheckbox.h"
#include "qtmaterialtextfield.h"
#include "qtmaterialraisedbutton.h"
#include "qtmaterialflatbutton.h"
#include "qtmaterialiconbutton.h"
#include "qtmaterialprogress.h"
#include "qtmaterialslider.h"

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
    
    // 项目自定义槽函数：处理左侧箭头按钮点击（控制imageStack帧切换）
    // 信号源：左侧箭头按钮clicked()信号
    // 功能：切换到上一帧/下一帧（处理前图片）
    void onPrevFrameLeft();
    void onNextFrameLeft();
    
    // 项目自定义槽函数：处理右侧箭头按钮点击（控制final_images帧切换）
    // 信号源：右侧箭头按钮clicked()信号
    // 功能：切换到上一帧/下一帧（处理后图片）
    void onPrevFrameRight();
    void onNextFrameRight();
    
    // 项目自定义槽函数：处理同步帧按钮点击
    // 信号源：ui->pushButton_syncFrames clicked信号
    // 功能：同步处理前后图片的显示帧数，按下后两边帧数同步改变
    void onSyncFramesClicked();
    
protected:
    // Qt原生事件重载：窗口大小改变时自动调用
    // 信号源：系统resizeEvent
    // 功能：窗口缩放时自动重新计算图片大小并更新显示（无需手动点击"同步帧"）
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
    DarkSectioning *darkSectioning;
    
    // ==================== Material组件声明 ====================
    
    // 蓝色区：路径与操作按钮区的Material组件
    QtMaterialRaisedButton *m_runButton;          // Run Dark Sectioning凸起按钮
    QtMaterialFlatButton *m_browseInputButton;     // 输入路径浏览扁平按钮
    QtMaterialFlatButton *m_browseOutputButton;    // 输出目录浏览扁平按钮
    
    // 绿色区：参数设置面板的Material组件
    QtMaterialToggle *m_toggleParam;              // 开关控件（参数1）
    QtMaterialCheckBox *m_checkboxParam;           // 复选框控件（参数2）
    QList<QtMaterialTextField*> m_paramTextFields; // 文本输入框列表（参数3-13，共11个）
    
    // 橙色区：双图显示区和控制条的Material组件
    QtMaterialFlatButton *m_prevLeftButton;       // 左侧区域左箭头扁平按钮
    QtMaterialFlatButton *m_nextLeftButton;       // 左侧区域右箭头扁平按钮
    QtMaterialFlatButton *m_prevRightButton;      // 右侧区域左箭头扁平按钮
    QtMaterialFlatButton *m_nextRightButton;      // 右侧区域右箭头扁平按钮
    QtMaterialProgress *m_progressBar;            // 处理进度条
    QtMaterialSlider *m_sliderOriginal;           // 处理前图片帧滑块
    QtMaterialSlider *m_sliderProcessed;          // 处理后图片帧滑块
    QtMaterialRaisedButton *m_syncButton;         // 同步帧凸起按钮
    QLabel *m_progressValueLabel;                 // 进度数值显示标签
    
    // ==================== 图像文件路径（方案B：从文件读取显示）====================
    QString m_inputFilePath;                      // 输入图片文件路径
    QString m_outputFilePath;                     // 输出图片文件路径（处理后）
    int currentOriginalFrame;                     // 当前处理前图片帧索引
    int currentProcessedFrame;                    // 当前处理后图片帧索引
    int totalOriginalFrames;                      // 处理前图片总帧数
    int totalProcessedFrames;                     // 处理后图片总帧数
    bool isSyncMode;                              // 同步模式标志位
    
    // ==================== 辅助函数声明（按区域分块）====================
    
    // ---------- 【红区】菜单栏区域 ----------
    void initRedAreaComponents();                 // 红区Material组件初始化
    void setupRedAreaInLayout();                  // 红区组件嵌入UI布局
    void connectRedAreaSignals();                 // 红区信号槽连接
    
    // ---------- 【蓝区】路径与操作按钮区 ----------
    void initBlueAreaComponents();                // 蓝区Material组件初始化
    void setupBlueAreaInLayout();                 // 蓝区组件嵌入UI布局
    void connectBlueAreaSignals();                // 蓝区信号槽连接
    
    // ---------- 【绿区】参数设置面板 ----------
    void initGreenAreaComponents();               // 绿区Material组件初始化
    void setupGreenAreaInLayout();                // 绿区组件嵌入UI布局
    void connectGreenAreaSignals();               // 绿区信号槽连接
    
    // ---------- 【紫区】运行日志栏 ----------
    void initPurpleAreaComponents();              // 紫区Material组件初始化
    void setupPurpleAreaInLayout();               // 紫区组件嵌入UI布局
    void connectPurpleAreaSignals();              // 紫区信号槽连接
    
    // ---------- 【橙区】双图显示区和控制条 ----------
    void initOrangeAreaComponents();              // 橙区Material组件初始化
    void setupOrangeAreaInLayout();               // 橙区组件嵌入UI布局
    void connectOrangeAreaSignals();              // 橙区信号槽连接
    
    // ---------- 全局功能函数 ----------
    void applyMaterialTheme();                    // 应用Material主题颜色#55aaff
    void updateImageDisplay();                    // 更新图像显示（方案B核心实现）
    QImage readTiffFrame(const QString& filePath, int frameIndex);  // 从TIFF读取指定帧
    QImage applyStandardToneMapping(const cv::Mat& mat);            // 色调映射函数
    void preloadImagePreview(const QString& filePath);              // 预加载图像预览
};

#endif // MAINWINDOW_H