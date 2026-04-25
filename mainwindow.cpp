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
#include <QImageReader>
#include <QLayout>
#include <QApplication>
#include <QTimer>

using namespace cv;
using namespace std;
using namespace chrono;


// ==================== 构造函数：初始化窗口 ====================
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    darkSectioning(new DarkSectioning(ui)),
    m_inputFilePath(""),                        // 初始化输入文件路径为空
    m_outputFilePath(""),                       // 初始化输出文件路径为空
    currentOriginalFrame(0),                    // 当前处理前图片帧索引=0
    currentProcessedFrame(0),                   // 当前处理后图片帧索引=0
    totalOriginalFrames(0),                     // 处理前总帧数=0
    totalProcessedFrames(0),                    // 处理后总帧数=0
    isSyncMode(false)                           // 同步模式默认关闭
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
    
    // ========== 方案B：保存文件路径而非Mat数据 ==========
    
    // 第1步：获取并保存输入文件路径
    m_inputFilePath = ui->lineEdit_inputPath->text();
    if (m_inputFilePath.isEmpty()) {
        ui->textEdit_log->append("错误: 请先选择输入图片路径");
        m_runButton->setEnabled(true);
        return;
    }
    
    // 更新进度条到10%
    m_progressBar->setValue(10);
    QApplication::processEvents();
    
    // 第2步：调用原有的DarkSectioning处理函数（不修改任何业务逻辑）
    // darkSectioning->process() 内部会：
    // 1. 读取输入文件并处理
    // 2. 将结果保存到输出目录的Dark.tif文件
    // 3. 定期调用QApplication::processEvents()保持UI响应
    darkSectioning->process();
    
    // 更新进度条到90%
    m_progressBar->setValue(90);
    QApplication::processEvents();
    
    // 第3步：构建输出文件路径（方案B核心：从该路径读取显示）
    QString outputDir = ui->lineEdit_outputPath->text();
    if (outputDir.isEmpty()) {
        outputDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    
    // 确保路径以斜杠结尾
    if (!outputDir.endsWith('/') && !outputDir.endsWith('\\')) {
        outputDir += '/';
    }
    
    m_outputFilePath = outputDir + "Dark.tif";  // 输出文件固定名为Dark.tif
    
    // 第4步：获取帧数信息（从darkSectioning的成员变量读取）
    totalOriginalFrames = darkSectioning->imageStack.size();
    totalProcessedFrames = darkSectioning->final_images.size();
    
    // 在日志中记录完成信息
    ui->textEdit_log->append("图像处理完成!");
    ui->textEdit_log->append(QString("输入文件: %1").arg(m_inputFilePath));
    ui->textEdit_log->append(QString("输出文件: %1").arg(m_outputFilePath));
    ui->textEdit_log->append(QString("处理前帧数: %1, 处理后帧数: %2")
                             .arg(totalOriginalFrames)
                             .arg(totalProcessedFrames));
    
    // ---------- 处理完成后：更新界面状态 ----------
    
    // 隐藏进度条，显示滑块
    m_progressBar->hide();
    m_sliderOriginal->show();
    m_sliderProcessed->show();
    
    // 更新处理前图片的滑块范围
    if (totalOriginalFrames > 0) {
        m_sliderOriginal->setRange(0, totalOriginalFrames - 1);
        m_sliderOriginal->setValue(0);
        currentOriginalFrame = 0;
    }
    
    // 更新处理后图片的滑块范围
    if (totalProcessedFrames > 0) {
        m_sliderProcessed->setRange(0, totalProcessedFrames - 1);
        m_sliderProcessed->setValue(0);
        currentProcessedFrame = 0;
    }
    
    // 进度条完成
    m_progressBar->setValue(100);
    QApplication::processEvents();
    
    // 立即更新图像显示（方案B：从文件路径直接读取，效果与Windows一致）
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


// ==================== 预加载图像预览（方案B：仅获取帧数信息）====================
// 功能：在选择文件后立即显示第一帧预览
void MainWindow::preloadImagePreview(const QString& filePath)
{
    // 保存输入文件路径
    m_inputFilePath = filePath;
    
    // 使用Qt原生QImageReader获取帧数信息（不加载图像数据，只读取元数据）
    QImageReader reader(filePath);
    
    // 检查文件是否可以读取
    if (!reader.canRead()) {
        ui->textEdit_log->append("错误: 无法读取文件 " + filePath);
        return;
    }
    
    // 获取总帧数
    int frameCount = reader.imageCount();
    
    if (frameCount <= 0) {
        // 单帧图像或无法确定帧数（某些TIFF格式可能返回-1或0）
        // 尝试读取第一帧来验证文件有效性
        QImage testImage = reader.read();
        if (!testImage.isNull()) {
            // 单帧图像
            totalOriginalFrames = 1;
            currentOriginalFrame = 0;
            updateImageDisplay();
            
            ui->textEdit_log->append("已预加载单帧图像");
            m_sliderOriginal->setRange(0, 0);
        } else {
            ui->textEdit_log->append("错误: 无法读取图像数据");
        }
        return;
    }
    
    // 多帧图像：保存总帧数（方案B：只保存帧数，不保存Mat数据）
    totalOriginalFrames = frameCount;
    
    // 显示第一帧到左侧QLabel
    currentOriginalFrame = 0;
    updateImageDisplay();
    
    ui->textEdit_log->append("已预加载 " + QString::number(totalOriginalFrames) + " 帧");
    
    // 更新滑块范围
    m_sliderOriginal->setRange(0, totalOriginalFrames - 1);
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
// 功能：处理前图片当前帧索引减1，显示前一张图片
void MainWindow::onPrevFrameLeft()
{
    // 边界检查：确保不超出总帧数范围（方案B：使用totalOriginalFrames）
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
// 功能：处理前图片当前帧索引加1，显示下一张图片
void MainWindow::onNextFrameLeft()
{
    // 边界检查：确保不超过总帧数（方案B：使用totalOriginalFrames）
    if (currentOriginalFrame < totalOriginalFrames - 1) {
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
// 功能：处理后图片当前帧索引减1，显示前一张图片
void MainWindow::onPrevFrameRight()
{
    // 边界检查（方案B：使用totalProcessedFrames）
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
// 功能：处理后图片当前帧索引加1，显示下一张图片
void MainWindow::onNextFrameRight()
{
    // 边界检查（方案B：使用totalProcessedFrames）
    if (currentProcessedFrame < totalProcessedFrames - 1) {
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


// ==================== 辅助函数：更新图像显示（方案B核心实现）====================
// 功能：根据currentOriginalFrame和currentProcessedFrame，
//       从文件路径直接读取图像并显示（效果与Windows照片查看器一致）
void MainWindow::updateImageDisplay()
{
    // ---------- 显示处理前图片（左侧QLabel）- 从输入文件读取 ----------
    
    // 检查文件路径和帧索引是否有效
    if (!m_inputFilePath.isEmpty() && 
        currentOriginalFrame >= 0 && 
        currentOriginalFrame < totalOriginalFrames) {
        
        // 方案B核心：从文件路径读取指定帧
        QImage qOriginalImg = readTiffFrame(m_inputFilePath, currentOriginalFrame);
        
        if (!qOriginalImg.isNull()) {
            // 缩放以适应QLabel大小，使用KeepAspectRatio保持原始比例（不变形）
            QPixmap pixmapOriginal = QPixmap::fromImage(qOriginalImg);
            QPixmap scaledOriginal = pixmapOriginal.scaled(
                ui->label_originalImage->size(),
                Qt::KeepAspectRatio,             // 保持宽高比，不拉伸变形
                Qt::SmoothTransformation         // 平滑缩放
            );
            
            // 显示在左侧QLabel上（嵌入橙色区，不是弹出窗口）
            ui->label_originalImage->setPixmap(scaledOriginal);
        }
    }
    
    
    // ---------- 显示处理后图片（右侧QLabel）- 从输出文件读取 ----------
    
    // 检查输出文件路径和帧索引是否有效
    if (!m_outputFilePath.isEmpty() && 
        currentProcessedFrame >= 0 && 
        currentProcessedFrame < totalProcessedFrames) {
        
        // 方案B核心：从输出文件路径读取指定帧
        QImage qProcessedImg = readTiffFrame(m_outputFilePath, currentProcessedFrame);
        
        if (!qProcessedImg.isNull()) {
            // 缩放以适应QLabel大小，使用KeepAspectRatio保持原始比例（不变形）
            QPixmap pixmapProcessed = QPixmap::fromImage(qProcessedImg);
            QPixmap scaledProcessed = pixmapProcessed.scaled(
                ui->label_processedImage->size(),
                Qt::KeepAspectRatio,             // 保持宽高比，不拉伸变形
                Qt::SmoothTransformation         // 平滑缩放
            );
            
            // 显示在右侧QLabel上（嵌入橙色区，不是弹出窗口）
            ui->label_processedImage->setPixmap(scaledProcessed);
        }
    }
}


// ==================== 窗口大小改变事件处理函数 ====================
// 功能：当窗口大小改变时自动重新缩放图片以适应新的窗口尺寸
// 参数：event - Qt resize事件对象
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 调用父类的resizeEvent处理（确保正常的布局更新）
    QMainWindow::resizeEvent(event);
    
    // 延迟调用图片更新，等待布局完成后再重新计算大小
    // 使用单次定时器延迟50ms，确保QLabel的新尺寸已经计算完成
    QTimer::singleShot(50, this, [this]() {
        updateImageDisplay();
    });
}


// ==================== 核心函数：从多帧TIFF文件中读取指定帧（Qt原生方式）====================
// 功能：使用Qt原生QImageReader读取TIFF文件的特定帧
//       直接返回QImage，无需OpenCV转换，显示效果与Windows照片查看器一致
// 参数：filePath - TIFF文件路径，frameIndex - 帧索引（从0开始）
// 返回：QImage对象（如果读取失败返回空QImage）
QImage MainWindow::readTiffFrame(const QString& filePath, int frameIndex)
{
    // 使用Qt的QImageReader直接读取图像文件（支持TIFF多帧）
    QImageReader reader(filePath);
    
    // 检查文件是否可以读取
    if (!reader.canRead()) {
        ui->textEdit_log->append("错误: 无法读取文件 " + filePath);
        return QImage();
    }
    
    // 获取总帧数（用于边界检查）
    int totalFrames = reader.imageCount();
    
    if (totalFrames <= 0) {
        // 单帧图像或无法确定帧数（某些TIFF格式可能返回-1或0）
        // 直接读取第一帧并返回
        QImage image = reader.read();
        
        if (frameIndex == 0 || frameIndex < 0) {
            return image;
        } else {
            ui->textEdit_log->append(QString("警告: 请求帧 %1 但图像为单帧").arg(frameIndex));
            return image;
        }
    }
    
    // 边界检查：确保帧索引在有效范围内
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        ui->textEdit_log->append(QString("错误: 帧索引 %1 超出范围 (总帧数: %2)")
                                 .arg(frameIndex)
                                 .arg(totalFrames));
        return QImage();
    }
    
    // 跳转到指定帧
    bool jumpSuccess = reader.jumpToImage(frameIndex);
    
    if (!jumpSuccess) {
        // 某些TIFF格式可能不支持随机访问，尝试顺序读取
        reader.setFileName(filePath);  // 重置读取器
        
        for (int i = 0; i < frameIndex; i++) {
            if (!reader.jumpToNextImage()) {
                ui->textEdit_log->append(QString("错误: 无法跳转到帧 %1 (在第 %2 帧失败)")
                                         .arg(frameIndex).arg(i));
                return QImage();
            }
        }
    }
    
    // 读取指定帧并返回（Qt自动处理颜色空间转换）
    QImage image = reader.read();
    
    if (image.isNull()) {
        ui->textEdit_log->append(QString("错误: 读取帧 %1 失败 (可能是TIFF格式问题)")
                                 .arg(frameIndex));
    }
    
    return image;
}


// ==================== 色调映射函数：模拟Windows照片查看器效果 ====================
// 功能：将OpenCV Mat数据转换为QImage，应用标准色调映射算法
//       确保显示效果与Windows照片查看器一致
// 参数：mat - OpenCV Mat对象（支持CV_8U/CV_16U/CV_32F等格式）
// 返回：QImage对象（RGB888格式）
QImage MainWindow::applyStandardToneMapping(const cv::Mat& mat)
{
    // 检查Mat是否为空
    if (mat.empty()) {
        return QImage();
    }
    
    cv::Mat displayMat;
    
    // ---------- 根据Mat类型进行不同的处理 ----------
    
    switch (mat.depth()) {
        case CV_8U:
            // 8位图像：直接使用，无需特殊处理
            displayMat = mat.clone();
            break;
            
        case CV_16U:
        {
            // 16位图像：使用百分位数裁剪 + 线性拉伸（模拟Windows照片查看器）
            
            double minVal, maxVal;
            cv::minMaxLoc(mat, &minVal, &maxVal);
            
            if (maxVal <= minVal) {
                // 常量图像或异常情况
                mat.convertTo(displayMat, CV_8U);
                break;
            }
            
            // 计算直方图并找到1%和99%的百分位数（排除极端值）
            int histSize = 65536;
            float range[] = {0, 65536};
            const float* histRange = {range};
            cv::Mat hist;
            cv::calcHist(&mat, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
            
            // 找到1%和99%百分位数对应的像素值
            int totalPixels = mat.rows * mat.cols;
            int lowThreshold = static_cast<int>(totalPixels * 0.01);
            int highThreshold = static_cast<int>(totalPixels * 0.99);
            
            int sum = 0;
            int pLow = 0;   // 1%百分位
            int pHigh = 65535;  // 99%百分位
            
            for (int i = 0; i < histSize; i++) {
                sum += static_cast<int>(hist.at<float>(i));
                if (sum < lowThreshold) pLow = i;
                if (sum < highThreshold) pHigh = i;
            }
            
            // 如果百分位范围太小，使用全局范围
            if (pHigh - pLow < 256) {
                pLow = static_cast<int>(minVal);
                pHigh = static_cast<int>(maxVal);
            }
            
            // 应用线性拉伸到0-255范围
            double alpha = 255.0 / (pHigh - pLow);  // 缩放因子
            double beta = -pLow * alpha;             // 偏移量
            
            mat.convertTo(displayMat, CV_8U, alpha, beta);
            break;
        }
            
        case CV_16S:
        {
            // 16位有符号图像：转为无符号后再处理
            cv::Mat absMat = cv::abs(mat);
            double minVal, maxVal;
            cv::minMaxLoc(absMat, &minVal, &maxVal);
            
            if (maxVal > 255.0 && maxVal > minVal) {
                absMat.convertTo(displayMat, CV_8U, 255.0 / maxVal);
            } else {
                absMat.convertTo(displayMat, CV_8U);
            }
            break;
        }
            
        case CV_32F:
        case CV_64F:
        {
            // 浮点图像：标准化到0-255范围
            double minVal, maxVal;
            cv::minMaxLoc(mat, &minVal, &maxVal);
            double range = maxVal - minVal;
            
            if (range > 0) {
                mat.convertTo(displayMat, CV_8U, 255.0 / range, -255.0 * minVal / range);
            } else {
                displayMat = cv::Mat(mat.size(), CV_8U, cv::Scalar(128));
            }
            break;
        }
            
        default:
            mat.convertTo(displayMat, CV_8U);
            break;
    }
    
    // ---------- 根据通道数转换颜色空间 ----------
    
    QImage result;
    
    switch (displayMat.channels()) {
        case 1:
        {
            // 单通道灰度图：转换为RGB（灰度值的R=G=B）
            cv::Mat rgbMat;
            cv::cvtColor(displayMat, rgbMat, cv::COLOR_GRAY2RGB);
            
            result = QImage(rgbMat.data, rgbMat.cols, rgbMat.rows,
                           static_cast<int>(rgbMat.step), QImage::Format_RGB888)
                    .copy();  // 深拷贝，避免悬空指针
            break;
        }
            
        case 3:
        {
            // 三通道彩色图：BGR转RGB（OpenCV默认是BGR，Qt需要RGB）
            cv::Mat rgbMat;
            cv::cvtColor(displayMat, rgbMat, cv::COLOR_BGR2RGB);
            
            result = QImage(rgbMat.data, rgbMat.cols, rgbMat.rows,
                           static_cast<int>(rgbMat.step), QImage::Format_RGB888)
                    .copy();  // 深拷贝，避免悬空指针
            break;
        }
            
        case 4:
        {
            // 四通道带Alpha：BGRA转RGBA
            cv::Mat rgbaMat;
            cv::cvtColor(displayMat, rgbaMat, cv::COLOR_BGRA2RGBA);
            
            result = QImage(rgbaMat.data, rgbaMat.cols, rgbaMat.rows,
                           static_cast<int>(rgbaMat.step), QImage::Format_RGBA8888)
                    .copy();  // 深拷贝，避免悬空指针
            break;
        }
            
        default:
            // 其他通道数，尝试用灰度方式处理
            cv::Mat grayMat;
            if (displayMat.channels() > 1) {
                cv::cvtColor(displayMat, grayMat, cv::COLOR_BGR2GRAY);
            } else {
                grayMat = displayMat;
            }
            
            cv::Mat rgbMat;
            cv::cvtColor(grayMat, rgbMat, cv::COLOR_GRAY2RGB);
            
            result = QImage(rgbMat.data, rgbMat.cols, rgbMat.rows,
                           static_cast<int>(rgbMat.step), QImage::Format_RGB888)
                    .copy();
            break;
    }
    
    return result;
}