// Dark Sectioning 主程序
// 基于Material Design风格UI改造版本
// 完整实现图像嵌入显示、Material组件集成、UI响应保持

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "params.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <windows.h>
#include <ViewMat.h>
#include <vector>
#include <QFileDialog>
#include <QString>
#include <QStandardPaths>
#include <QImage>
#include <QPixmap>
#include <QLayout>
#include <QApplication>

using namespace cv;
using namespace std;
using namespace chrono;


// ==================== 构造函数：初始化窗口 ====================
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    darkSectioning(new DarkSectioning(ui)),
    currentOriginalFrame(0),
    currentProcessedFrame(0),
    isSyncMode(false)
{
    // 第1步：调用Qt Designer生成的UI设置
    ui->setupUi(this);
    
    // 第2步：初始化所有Material组件
    initMaterialComponents();
    
    // 第3步：将Material组件嵌入到UI布局中
    setupMaterialWidgetsInLayout();
    
    // 第4步：应用Material主题颜色#55aaff
    applyMaterialTheme();
    
    // 第5步：绑定信号槽连接
    connectSignalsAndSlots();
}


// ==================== 析构函数：释放资源 ====================
MainWindow::~MainWindow()
{
    // 释放DarkSectioning业务对象
    delete darkSectioning;
    
    // 释放UI对象
    delete ui;
    
    // 释放所有Material组件内存
    delete m_runButton;
    delete m_browseInputButton;
    delete m_browseOutputButton;
    delete m_toggleParam;
    delete m_checkboxParam;
    qDeleteAll(m_paramTextFields);
    delete m_prevLeftButton;
    delete m_nextLeftButton;
    delete m_prevRightButton;
    delete m_nextRightButton;
    delete m_progressBar;
    delete m_sliderOriginal;
    delete m_sliderProcessed;
    delete m_syncButton;
}


// ==================== 初始化Material组件 ====================
void MainWindow::initMaterialComponents()
{
    // ---------- 蓝色区：路径与操作按钮区的Material组件 ----------
    
    // 创建Run Dark Sectioning凸起按钮（主操作按钮）
    m_runButton = new QtMaterialRaisedButton("Run Dark Sectioning");
    
    // 创建输入路径浏览扁平按钮
    m_browseInputButton = new QtMaterialFlatButton("浏览");
    
    // 创建输出目录浏览扁平按钮
    m_browseOutputButton = new QtMaterialFlatButton("浏览");
    
    
    // ---------- 绿色区：参数设置面板的Material组件 ----------
    
    // 创建开关控件（Toggle）- 参数1
    m_toggleParam = new QtMaterialToggle();
    
    // 创建复选框控件（CheckBox）- 参数2
    m_checkboxParam = new QtMaterialCheckBox();
    m_checkboxParam->setText("启用选项");
    
    // 创建11个文本输入框（TextField）- 参数3至13
    for (int i = 0; i < 11; i++) {
        QtMaterialTextField *textField = new QtMaterialTextField();
        textField->setPlaceholderText(QString("参数%1").arg(i + 3));
        m_paramTextFields.append(textField);
    }
    
    
    // ---------- 橙色区：双图显示区和控制条的Material组件 ----------
    
    // 创建左侧区域箭头按钮（控制处理前图片帧）- 使用扁平按钮显示文本
    m_prevLeftButton = new QtMaterialFlatButton("◀");
    m_prevLeftButton->setMinimumSize(40, 30);
    m_prevLeftButton->setMaximumSize(40, 30);
    
    m_nextLeftButton = new QtMaterialFlatButton("▶");
    m_nextLeftButton->setMinimumSize(40, 30);
    m_nextLeftButton->setMaximumSize(40, 30);
    
    // 创建右侧区域箭头按钮（控制处理后图片帧）
    m_prevRightButton = new QtMaterialFlatButton("◀");
    m_prevRightButton->setMinimumSize(40, 30);
    m_prevRightButton->setMaximumSize(40, 30);
    
    m_nextRightButton = new QtMaterialFlatButton("▶");
    m_nextRightButton->setMinimumSize(40, 30);
    m_nextRightButton->setMaximumSize(40, 30);
    
    // 创建处理进度条
    m_progressBar = new QtMaterialProgress();
    m_progressBar->setRange(0, 100);           // 设置进度条范围0-100
    m_progressBar->setValue(0);                 // 初始值为0
    
    // 创建处理前图片帧滑块（初始隐藏，处理完成后显示）
    m_sliderOriginal = new QtMaterialSlider();
    m_sliderOriginal->setRange(0, 0);          // 初始范围（无数据时为0-0）
    m_sliderOriginal->setValue(0);
    m_sliderOriginal->hide();                  // 处理完成前隐藏
    
    // 创建处理后图片帧滑块（初始隐藏，处理完成后显示）
    m_sliderProcessed = new QtMaterialSlider();
    m_sliderProcessed->setRange(0, 0);         // 初始范围（无数据时为0-0）
    m_sliderProcessed->setValue(0);
    m_sliderProcessed->hide();                 // 处理完成前隐藏
    
    // 创建同步帧按钮
    m_syncButton = new QtMaterialRaisedButton("同步帧");
}


// ==================== 将Material组件嵌入到UI布局中 ====================
void MainWindow::setupMaterialWidgetsInLayout()
{
    // ---------- 蓝色区：替换原有按钮 ----------
    
    // 替换Run按钮：从父布局移除原QPushButton，添加MaterialRaisedButton
    QHBoxLayout *runButtonLayout = qobject_cast<QHBoxLayout*>(ui->pushButton_run->parentWidget()->layout());
    if (runButtonLayout) {
        int index = runButtonLayout->indexOf(ui->pushButton_run);
        if (index >= 0) {
            runButtonLayout->removeWidget(ui->pushButton_run);
            ui->pushButton_run->deleteLater();
            runButtonLayout->insertWidget(index, m_runButton);
        }
    }
    
    // 替换输入路径浏览按钮
    QHBoxLayout *browseInputLayout = qobject_cast<QHBoxLayout*>(ui->pushButton_browse->parentWidget()->layout());
    if (browseInputLayout) {
        int index = browseInputLayout->indexOf(ui->pushButton_browse);
        if (index >= 0) {
            browseInputLayout->removeWidget(ui->pushButton_browse);
            ui->pushButton_browse->deleteLater();
            browseInputLayout->insertWidget(index, m_browseInputButton);
        }
    }
    
    // 替换输出目录浏览按钮
    QHBoxLayout *browseOutputLayout = qobject_cast<QHBoxLayout*>(ui->pushButton_browseOutput->parentWidget()->layout());
    if (browseOutputLayout) {
        int index = browseOutputLayout->indexOf(ui->pushButton_browseOutput);
        if (index >= 0) {
            browseOutputLayout->removeWidget(ui->pushButton_browseOutput);
            ui->pushButton_browseOutput->deleteLater();
            browseOutputLayout->insertWidget(index, m_browseOutputButton);
        }
    }
    
    
    // ---------- 绿色区：将Toggle和Checkbox嵌入容器 ----------
    
    // 将Toggle控件嵌入widget_toggleContainer容器
    QVBoxLayout *toggleContainerLayout = new QVBoxLayout(ui->widget_toggleContainer);
    toggleContainerLayout->addWidget(m_toggleParam);
    toggleContainerLayout->setContentsMargins(0, 0, 0, 0);
    
    // 将Checkbox控件嵌入widget_checkboxContainer容器
    QVBoxLayout *checkboxContainerLayout = new QVBoxLayout(ui->widget_checkboxContainer);
    checkboxContainerLayout->addWidget(m_checkboxParam);
    checkboxContainerLayout->setContentsMargins(0, 0, 0, 0);
    
    // 将11个TextField替换原有的QLineEdit
    QList<QLineEdit*> paramLineEdits = {
        ui->lineEdit_param1, ui->lineEdit_param2, ui->lineEdit_param3,
        ui->lineEdit_param4, ui->lineEdit_param5, ui->lineEdit_param6,
        ui->lineEdit_param7, ui->lineEdit_param8, ui->lineEdit_param9,
        ui->lineEdit_param10, ui->lineEdit_param11
    };
    
    // 直接使用UI文件中的网格布局对象
    QGridLayout *gridLayout = ui->gridLayout_params;
    
    for (int i = 0; i < 11 && i < paramLineEdits.size() && i < m_paramTextFields.size(); i++) {
        QLineEdit *oldLineEdit = paramLineEdits[i];
        
        if (gridLayout) {
            int row, col, rowSpan, colSpan;
            gridLayout->getItemPosition(gridLayout->indexOf(oldLineEdit), &row, &col, &rowSpan, &colSpan);
            
            gridLayout->removeWidget(oldLineEdit);
            oldLineEdit->deleteLater();
            gridLayout->addWidget(m_paramTextFields[i], row, col, rowSpan, colSpan);
        }
    }
    
    
    // ---------- 橙色区：替换控制条中的箭头按钮和进度条 ----------
    
    // 替换左侧箭头按钮（处理前图片切帧）
    QHBoxLayout *leftArrowsLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_leftArrows);
    if (leftArrowsLayout) {
        // 替换左箭头
        int prevIndex = leftArrowsLayout->indexOf(ui->pushButton_prevLeft);
        if (prevIndex >= 0) {
            leftArrowsLayout->removeWidget(ui->pushButton_prevLeft);
            ui->pushButton_prevLeft->deleteLater();
            leftArrowsLayout->insertWidget(prevIndex, m_prevLeftButton);
        }
        
        // 替换右箭头
        int nextIndex = leftArrowsLayout->indexOf(ui->pushButton_nextLeft);
        if (nextIndex >= 0) {
            leftArrowsLayout->removeWidget(ui->pushButton_nextLeft);
            ui->pushButton_nextLeft->deleteLater();
            leftArrowsLayout->insertWidget(nextIndex, m_nextLeftButton);
        }
    }
    
    // 替换右侧箭头按钮（处理后图片切帧）
    QHBoxLayout *rightArrowsLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_rightArrows);
    if (rightArrowsLayout) {
        // 替换左箭头
        int prevIndex = rightArrowsLayout->indexOf(ui->pushButton_prevRight);
        if (prevIndex >= 0) {
            rightArrowsLayout->removeWidget(ui->pushButton_prevRight);
            ui->pushButton_prevRight->deleteLater();
            rightArrowsLayout->insertWidget(prevIndex, m_prevRightButton);
        }
        
        // 替换右箭头
        int nextIndex = rightArrowsLayout->indexOf(ui->pushButton_nextRight);
        if (nextIndex >= 0) {
            rightArrowsLayout->removeWidget(ui->pushButton_nextRight);
            ui->pushButton_nextRight->deleteLater();
            rightArrowsLayout->insertWidget(nextIndex, m_nextRightButton);
        }
    }
    
    // 替换进度条
    QHBoxLayout *controlBarLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_controlBar);
    if (controlBarLayout) {
        int progressIndex = controlBarLayout->indexOf(ui->progressBar_processing);
        if (progressIndex >= 0) {
            controlBarLayout->removeWidget(ui->progressBar_processing);
            ui->progressBar_processing->deleteLater();
            controlBarLayout->insertWidget(progressIndex, m_progressBar);
        }
    }
    
    // 替换同步帧按钮（在进度条左侧位置）
    if (controlBarLayout) {
        int syncIndex = controlBarLayout->indexOf(ui->pushButton_syncFrames);
        if (syncIndex >= 0) {
            controlBarLayout->removeWidget(ui->pushButton_syncFrames);
            ui->pushButton_syncFrames->deleteLater();
            controlBarLayout->insertWidget(syncIndex, m_syncButton);
        }
    }
}


// ==================== 应用Material主题颜色 ====================
void MainWindow::applyMaterialTheme()
{
    // 设置主题主色调为#55aaff（RGB: 85, 170, 255）
    QColor themeColor(85, 170, 255);
    
    // 为所有Material组件应用主题色
    m_runButton->setForegroundColor(themeColor);
    m_browseInputButton->setForegroundColor(themeColor);
    m_browseOutputButton->setForegroundColor(themeColor);
    
    m_toggleParam->setTrackColor(themeColor);
    m_checkboxParam->setCheckedColor(themeColor);
    m_checkboxParam->setTextColor(QColor(51, 51, 51));  // 设置Checkbox文字颜色为深灰色
    
    foreach (QtMaterialTextField *textField, m_paramTextFields) {
        textField->setInkColor(themeColor);
    }
    
    // 设置箭头按钮的前景色（文字颜色）
    m_prevLeftButton->setForegroundColor(themeColor);
    m_nextLeftButton->setForegroundColor(themeColor);
    m_prevRightButton->setForegroundColor(themeColor);
    m_nextRightButton->setForegroundColor(themeColor);
    
    m_progressBar->setProgressColor(themeColor);
    m_sliderOriginal->setTrackColor(themeColor);
    m_sliderProcessed->setTrackColor(themeColor);
    m_sliderOriginal->setThumbColor(themeColor);
    m_sliderProcessed->setThumbColor(themeColor);
    
    m_syncButton->setForegroundColor(themeColor);
    
    // 设置窗口整体样式表
    this->setStyleSheet(
        "QMainWindow { background-color: #fafafa; }"
        "QGroupBox { font-weight: bold; color: #55aaff; border: 2px solid #55aaff; "
        "border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
        "QLabel { color: #333333; }"
        "QMenuBar { background-color: #ffffff; color: #55aaff; border-bottom: 2px solid #55aaff; }"
        "QMenuBar::item:selected { background-color: #e6f2ff; }"
        "QMenu { background-color: #ffffff; border: 1px solid #cccccc; }"
        "QMenu::item:selected { background-color: #e6f2ff; color: #55aaff; }"
    );
}


// ==================== 绑定信号槽连接 ====================
void MainWindow::connectSignalsAndSlots()
{
    // ---------- 蓝色区：路径与操作按钮的信号槽 ----------
    
    // Run按钮点击 -> 运行Dark Sectioning处理
    connect(m_runButton, &QtMaterialRaisedButton::clicked, 
            this, &MainWindow::on_pushButton_run_clicked);
    
    // 输入路径浏览按钮点击 -> 打开文件选择对话框
    connect(m_browseInputButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::on_pushButton_browse_clicked);
    
    // 输出目录浏览按钮点击 -> 打开文件夹选择对话框
    connect(m_browseOutputButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::on_pushButton_browseOutput_clicked);
    
    
    // ---------- 橙色区：控制条的信号槽 ----------
    
    // 左侧箭头按钮 -> 控制imageStack（处理前图片）帧切换
    connect(m_prevLeftButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::onPrevFrameLeft);
    connect(m_nextLeftButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::onNextFrameLeft);
    
    // 右侧箭头按钮 -> 控制final_images（处理后图片）帧切换
    connect(m_prevRightButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::onPrevFrameRight);
    connect(m_nextRightButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::onNextFrameRight);
    
    // 同步帧按钮点击 -> 切换同步模式
    connect(m_syncButton, &QtMaterialRaisedButton::clicked,
            this, &MainWindow::onSyncFramesClicked);
    
    // 处理前图片滑块值改变 -> 更新当前帧并刷新显示
    connect(m_sliderOriginal, &QtMaterialSlider::valueChanged, [this](int value) {
        currentOriginalFrame = value;
        if (isSyncMode) {
            currentProcessedFrame = value;
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(value);
            m_sliderProcessed->blockSignals(false);
        }
        updateImageDisplay();
    });
    
    // 处理后图片滑块值改变 -> 更新当前帧并刷新显示
    connect(m_sliderProcessed, &QtMaterialSlider::valueChanged, [this](int value) {
        currentProcessedFrame = value;
        if (isSyncMode) {
            currentOriginalFrame = value;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(value);
            m_sliderOriginal->blockSignals(false);
        }
        updateImageDisplay();
    });
}


// ==================== Qt原生公共槽函数：运行处理（解决UI冻结问题）====================
// 信号源：m_runButton clicked信号
// 功能：调用DarkSectioning处理逻辑，使用processEvents保持UI响应
void MainWindow::on_pushButton_run_clicked()
{
    // 在日志区输出开始信息
    ui->textEdit_log->append("开始图像处理...");
    
    // 禁用Run按钮防止重复点击
    m_runButton->setEnabled(false);
    
    // 显示进度条，隐藏滑块
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_sliderOriginal->hide();
    m_sliderProcessed->hide();
    
    // 强制刷新UI
    QApplication::processEvents();
    
    // 更新进度条到10%
    m_progressBar->setValue(10);
    QApplication::processEvents();
    
    // 调用原有的DarkSectioning处理函数（不修改任何业务逻辑）
    // darkSectioning->process() 内部会：
    // 1. 读取imageStack到darkSectioning->imageStack成员变量
    // 2. 执行处理算法，结果保存到darkSectioning->final_images成员变量
    // 3. 定期调用QApplication::processEvents()保持UI响应
    darkSectioning->process();
    
    // 处理完成后，从darkSectioning获取数据到MainWindow成员变量
    imageStack = darkSectioning->imageStack;
    final_images = darkSectioning->final_images;
    
    // 更新进度条到90%
    m_progressBar->setValue(90);
    QApplication::processEvents();
    
    // 在日志中记录完成信息
    ui->textEdit_log->append("图像处理完成!");
    ui->textEdit_log->append(QString("处理前帧数: %1, 处理后帧数: %2")
                             .arg(imageStack.size())
                             .arg(final_images.size()));
    
    // ---------- 处理完成后：更新界面状态 ----------
    
    // 隐藏进度条，显示滑块
    m_progressBar->hide();
    m_sliderOriginal->show();
    m_sliderProcessed->show();
    
    // 如果imageStack有数据，更新处理前图片的滑块范围
    if (!imageStack.empty()) {
        m_sliderOriginal->setRange(0, imageStack.size() - 1);
        m_sliderOriginal->setValue(0);
        currentOriginalFrame = 0;
    }
    
    // 更新处理后图片的滑块范围
    if (!final_images.empty()) {
        m_sliderProcessed->setRange(0, final_images.size() - 1);
        m_sliderProcessed->setValue(0);
        currentProcessedFrame = 0;
    }
    
    // 进度条完成
    m_progressBar->setValue(100);
    QApplication::processEvents();
    
    // 立即更新图像显示（在QLabel中嵌入显示，不是弹出窗口）
    updateImageDisplay();
    
    // 恢复Run按钮
    m_runButton->setEnabled(true);
}


// ==================== Qt原生公共槽函数：输入路径浏览 ====================
// 信号源：m_browseInputButton clicked信号
// 功能：打开文件选择对话框，选择输入图片文件
void MainWindow::on_pushButton_browse_clicked()
{
    // 打开文件选择对话框（支持多种图片格式）
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择输入图片",
        "",
        "图片文件 (*.tif *.tiff *.png *.jpg *.jpeg *.bmp);;所有文件 (*.*)"
    );

    // 如果用户选择了文件
    if (!filePath.isEmpty()) {
        // 检查路径是否包含中文字符（中文路径可能导致OpenCV读取失败）
        bool hasChinese = false;
        for (int i = 0; i < filePath.length(); i++) {
            QChar ch = filePath.at(i);
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                hasChinese = true;
                break;
            }
        }

        // 如果包含中文字符，在日志中警告用户
        if (hasChinese) {
            ui->textEdit_log->append("警告: 路径包含中文字符，可能导致处理失败");
        }

        // 将选择的路径显示在输入框中
        ui->lineEdit_inputPath->setText(filePath);
        
        // 在日志中记录选择的文件
        ui->textEdit_log->append("已选择文件: " + filePath);
        
        // 预加载图像预览（不等待处理）
        preloadImagePreview(filePath);
    }
}


// ==================== 预加载图像预览 ====================
// 功能：在选择文件后立即显示第一帧预览
void MainWindow::preloadImagePreview(const QString& filePath)
{
    std::string path = filePath.toStdString();
    
    // 尝试读取多帧图像
    std::vector<cv::Mat> tempStack;
    bool success = cv::imreadmulti(path, tempStack, cv::IMREAD_UNCHANGED);
    
    if (success && !tempStack.empty()) {
        // 保存到成员变量
        imageStack = tempStack;
        
        // 显示第一帧到左侧QLabel
        currentOriginalFrame = 0;
        updateImageDisplay();
        
        ui->textEdit_log->append("已预加载 " + QString::number(imageStack.size()) + " 帧");
    }
}


// ==================== Qt原生公共槽函数：输出目录浏览 ====================
// 信号源：m_browseOutputButton clicked信号
// 功能：打开文件夹选择对话框，选择输出目录
void MainWindow::on_pushButton_browseOutput_clicked()
{
    // 打开文件夹选择对话框
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        "选择输出目录",
        ""
    );

    // 如果用户选择了目录
    if (!folderPath.isEmpty()) {
        // 检查路径是否包含中文字符
        bool hasChinese = false;
        for (int i = 0; i < folderPath.length(); i++) {
            QChar ch = folderPath.at(i);
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                hasChinese = true;
                break;
            }
        }

        // 如果包含中文字符，在日志中警告用户
        if (hasChinese) {
            ui->textEdit_log->append("警告: 路径包含中文字符，可能导致处理失败");
        }

        // 将选择的路径显示在输入框中
        ui->lineEdit_outputPath->setText(folderPath);
        
        // 在日志中记录选择的目录
        ui->textEdit_log->append("已选择输出目录: " + folderPath);
    }
}


// ==================== 项目自定义槽函数：左侧上一帧 ====================
// 信号源：m_prevLeftButton clicked信号
// 功能：imageStack当前帧索引减1，显示前一张处理前图片
void MainWindow::onPrevFrameLeft()
{
    // 边界检查：确保不超出图像栈范围
    if (currentOriginalFrame > 0) {
        currentOriginalFrame--;
        
        // 更新滑块位置（会触发valueChanged信号自动刷新显示）
        m_sliderOriginal->setValue(currentOriginalFrame);
        
        // 如果处于同步模式，同步更新右侧滑块
        if (isSyncMode) {
            currentProcessedFrame = currentOriginalFrame;
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(currentProcessedFrame);
            m_sliderProcessed->blockSignals(false);
        }
    }
}


// ==================== 项目自定义槽函数：左侧下一帧 ====================
// 信号源：m_nextLeftButton clicked信号
// 功能：imageStack当前帧索引加1，显示下一张处理前图片
void MainWindow::onNextFrameLeft()
{
    // 边界检查：确保不超过图像栈大小
    if (currentOriginalFrame < static_cast<int>(imageStack.size()) - 1) {
        currentOriginalFrame++;
        
        // 更新滑块位置
        m_sliderOriginal->setValue(currentOriginalFrame);
        
        // 如果处于同步模式，同步更新右侧滑块
        if (isSyncMode) {
            currentProcessedFrame = currentOriginalFrame;
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(currentProcessedFrame);
            m_sliderProcessed->blockSignals(false);
        }
    }
}


// ==================== 项目自定义槽函数：右侧上一帧 ====================
// 信号源：m_prevRightButton clicked信号
// 功能：final_images当前帧索引减1，显示前一张处理后图片
void MainWindow::onPrevFrameRight()
{
    // 边界检查
    if (currentProcessedFrame > 0) {
        currentProcessedFrame--;
        
        // 更新滑块位置
        m_sliderProcessed->setValue(currentProcessedFrame);
        
        // 如果处于同步模式，同步更新左侧滑块
        if (isSyncMode) {
            currentOriginalFrame = currentProcessedFrame;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(currentOriginalFrame);
            m_sliderOriginal->blockSignals(false);
        }
    }
}


// ==================== 项目自定义槽函数：右侧下一帧 ====================
// 信号源：m_nextRightButton clicked信号
// 功能：final_images当前帧索引加1，显示下一张处理后图片
void MainWindow::onNextFrameRight()
{
    // 边界检查
    if (currentProcessedFrame < static_cast<int>(final_images.size()) - 1) {
        currentProcessedFrame++;
        
        // 更新滑块位置
        m_sliderProcessed->setValue(currentProcessedFrame);
        
        // 如果处于同步模式，同步更新左侧滑块
        if (isSyncMode) {
            currentOriginalFrame = currentProcessedFrame;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(currentOriginalFrame);
            m_sliderOriginal->blockSignals(false);
        }
    }
}


// ==================== 项目自定义槽函数：同步帧切换 ====================
// 信号源：m_syncButton clicked信号
// 功能：切换同步模式，同步后两边帧数联动变化
void MainWindow::onSyncFramesClicked()
{
    // 切换同步模式状态
    isSyncMode = !isSyncMode;
    
    if (isSyncMode) {
        // 进入同步模式：
        // 1. 强制将处理前图片帧数设置为与处理后图片当前帧数相同
        currentOriginalFrame = currentProcessedFrame;
        m_sliderOriginal->blockSignals(true);
        m_sliderOriginal->setValue(currentOriginalFrame);
        m_sliderOriginal->blockSignals(false);
        
        // 2. 更新按钮文字提示用户
        m_syncButton->setText("同步中");
        m_syncButton->setBackgroundColor(QColor(85, 170, 255));
        m_syncButton->setForegroundColor(Qt::white);
        
        // 3. 在日志中记录
        ui->textEdit_log->append("同步模式已开启：前后图片帧数将联动");
    } else {
        // 退出同步模式：
        // 1. 恢复按钮默认样式
        m_syncButton->setText("同步帧");
        m_syncButton->setBackgroundColor(Qt::transparent);
        m_syncButton->setForegroundColor(QColor(85, 170, 255));
        
        // 2. 在日志中记录
        ui->textEdit_log->append("同步模式已关闭：前后图片帧数独立控制");
    }
    
    // 无论是否同步，都立即刷新图像显示
    updateImageDisplay();
}


// ==================== 辅助函数：更新图像显示 ====================
// 功能：根据currentOriginalFrame和currentProcessedFrame，
//       从imageStack和final_images中读取Mat数据并转换为QImage显示
void MainWindow::updateImageDisplay()
{
    // ---------- 显示处理前图片（左侧QLabel）----------
    
    // 检查当前帧索引是否有效
    if (currentOriginalFrame >= 0 && 
        currentOriginalFrame < static_cast<int>(imageStack.size())) {
        
        // 获取当前帧的Mat数据
        Mat originalImg = imageStack[currentOriginalFrame];
        
        // 转换为QImage格式
        QImage qOriginalImg = matToQImage(originalImg);
        
        // 缩放以适应QLabel大小，使用IgnoreAspectRatio让图片填满整个区域
        QPixmap pixmapOriginal = QPixmap::fromImage(qOriginalImg);
        QPixmap scaledOriginal = pixmapOriginal.scaled(
            ui->label_originalImage->size(),
            Qt::IgnoreAspectRatio,           // 忽略宽高比，填满整个区域
            Qt::SmoothTransformation         // 平滑缩放
        );
        
        // 显示在左侧QLabel上（嵌入橙色区，不是弹出窗口）
        ui->label_originalImage->setPixmap(scaledOriginal);
    }
    
    
    // ---------- 显示处理后图片（右侧QLabel）----------
    
    // 检查当前帧索引是否有效
    if (currentProcessedFrame >= 0 && 
        currentProcessedFrame < static_cast<int>(final_images.size())) {
        
        // 获取当前帧的Mat数据
        Mat processedImg = final_images[currentProcessedFrame];
        
        // 转换为QImage格式
        QImage qProcessedImg = matToQImage(processedImg);
        
        // 缩放以适应QLabel大小，使用IgnoreAspectRatio让图片填满整个区域
        QPixmap pixmapProcessed = QPixmap::fromImage(qProcessedImg);
        QPixmap scaledProcessed = pixmapProcessed.scaled(
            ui->label_processedImage->size(),
            Qt::IgnoreAspectRatio,           // 忽略宽高比，填满整个区域
            Qt::SmoothTransformation         // 平滑缩放
        );
        
        // 显示在右侧QLabel上（嵌入橙色区，不是弹出窗口）
        ui->label_processedImage->setPixmap(scaledProcessed);
    }
}


// ==================== 辅助函数：OpenCV Mat转QImage ====================
// 功能：将OpenCV的Mat数据转换为Qt的QImage格式
// 支持灰度图、BGR彩色图、BGRA带Alpha通道图等多种格式
QImage MainWindow::matToQImage(const cv::Mat& mat)
{
    // 根据Mat的通道数和类型进行转换
    
    switch (mat.type()) {
        // ---------- 单通道灰度图 ----------
        case CV_8UC1:
        {
            QImage image(mat.data, mat.cols, mat.rows, 
                        static_cast<int>(mat.step), QImage::Format_Grayscale8);
            return image.copy();  // 返回深拷贝，避免Mat释放后悬空指针
        }
        
        // ---------- 三通道BGR彩色图（OpenCV默认格式）----------
        case CV_8UC3:
        {
            // OpenCV使用BGR格式，Qt使用RGB格式，需要转换色彩空间
            cv::Mat rgbMat;
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
            
            QImage image(rgbMat.data, rgbMat.cols, rgbMat.rows,
                        static_cast<int>(rgbMat.step), QImage::Format_RGB888);
            return image.copy();
        }
        
        // ---------- 四通道BGRA带Alpha通道图 ----------
        case CV_8UC4:
        {
            // OpenCV使用BGRA格式，Qt使用RGBA格式，需要转换
            cv::Mat rgbaMat;
            cv::cvtColor(mat, rgbaMat, cv::COLOR_BGRA2RGBA);
            
            QImage image(rgbaMat.data, rgbaMat.cols, rgbaMat.rows,
                        static_cast<int>(rgbaMat.step), QImage::Format_RGBA8888);
            return image.copy();
        }
        
        // ---------- 其他格式（如16位深度图等）----------
        default:
        {
            // 对于不支持的格式，先转换为8位三通道再转换
            cv::Mat convertedMat;
            if (mat.channels() == 1) {
                mat.convertTo(convertedMat, CV_8UC1);
            } else {
                mat.convertTo(convertedMat, CV_8UC3);
            }
            return matToQImage(convertedMat);  // 递归调用自身
        }
    }
}