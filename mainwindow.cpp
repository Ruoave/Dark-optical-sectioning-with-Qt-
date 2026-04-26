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


// ============================================================================
// 【构造函数：初始化窗口】
// ============================================================================
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
    
    // 第2步：【红区】菜单栏初始化（使用Qt原生MenuBar，无需Material组件）
    // 红区包含：文件、分析、设置、帮助 菜单项
    // 此处无需额外代码，Qt Designer已配置完成
    
    // 第3步：【蓝区】路径与操作按钮区 - Material组件初始化+布局嵌入+信号槽
    initBlueAreaComponents();
    setupBlueAreaInLayout();
    connectBlueAreaSignals();
    
    // 第4步：【绿区】参数设置面板 - Material组件初始化+布局嵌入+信号槽
    initGreenAreaComponents();
    setupGreenAreaInLayout();
    connectGreenAreaSignals();
    
    // 第5步：【紫区】运行日志栏 - 使用原生QTextEdit，无需Material组件
    // 紫区包含：textEdit_log日志显示框
    // 此处无需额外代码，Qt Designer已配置完成
    
    // 第6步：【橙区】双图显示区和控制条 - Material组件初始化+布局嵌入+信号槽
    initOrangeAreaComponents();
    setupOrangeAreaInLayout();
    connectOrangeAreaSignals();
    
    // 第7步：应用Material主题颜色#55aaff（全局统一应用）
    applyMaterialTheme();
}


// ============================================================================
// 【析构函数：释放资源】
// ============================================================================
MainWindow::~MainWindow()
{
    // 释放DarkSectioning业务对象
    delete darkSectioning;
    
    // 释放UI对象
    delete ui;
    
    // 释放【蓝区】Material组件内存
    delete m_runButton;
    delete m_browseInputButton;
    delete m_browseOutputButton;
    
    // 释放【绿区】Material组件内存
    delete m_toggleParam;
    delete m_checkboxParam;
    qDeleteAll(m_paramTextFields);
    
    // 释放【橙区】Material组件内存
    delete m_prevLeftButton;
    delete m_nextLeftButton;
    delete m_prevRightButton;
    delete m_nextRightButton;
    delete m_progressBar;
    delete m_sliderOriginal;
    delete m_sliderProcessed;
    delete m_syncButton;
}


// ============================================================================
// 【红区】菜单栏区域
// 功能：提供文件、分析、设置、帮助等顶层菜单
// 说明：使用Qt原生QMenuBar，无需Material组件替换
// ============================================================================

// 【红区】Material组件初始化
// 说明：红区使用Qt原生菜单栏，无需创建Material组件
// 如果未来需要美化菜单栏，可在此处添加QtMaterialMenu相关代码
void MainWindow::initRedAreaComponents()
{
    // 当前版本：红区保持Qt原生样式，无需Material组件
}

// 【红区】将Material组件嵌入UI布局
// 说明：菜单栏已在Qt Designer中配置完成
void MainWindow::setupRedAreaInLayout()
{
    // 当前版本：红区布局由Qt Designer管理，无需额外代码
}

// 【红区】绑定信号槽连接
// 说明：菜单项的信号槽连接可在需要时添加
void MainWindow::connectRedAreaSignals()
{
    // 当前版本：红区无自定义信号槽连接需求
}


// ============================================================================
// 【蓝区】路径与操作按钮区
// 功能：输入/输出路径选择、Run主操作按钮
// 包含组件：m_runButton, m_browseInputButton, m_browseOutputButton
// ============================================================================

// ---------- 【蓝区】Material组件初始化 ----------
void MainWindow::initBlueAreaComponents()
{
    // 创建Run Dark Sectioning凸起按钮（主操作按钮）
    m_runButton = new QtMaterialRaisedButton("Run Dark Sectioning");
    
    // 创建输入路径浏览扁平按钮
    m_browseInputButton = new QtMaterialFlatButton("浏览");
    
    // 创建输出目录浏览扁平按钮
    m_browseOutputButton = new QtMaterialFlatButton("浏览");
}

// ---------- 【蓝区】将Material组件嵌入UI布局 ----------
void MainWindow::setupBlueAreaInLayout()
{
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
}

// ---------- 【蓝区】绑定信号槽连接 ----------
void MainWindow::connectBlueAreaSignals()
{
    // Run按钮点击 -> 运行Dark Sectioning处理
    connect(m_runButton, &QtMaterialRaisedButton::clicked,
            this, &MainWindow::on_pushButton_run_clicked);
    
    // 输入路径浏览按钮点击 -> 打开文件选择对话框
    connect(m_browseInputButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::on_pushButton_browse_clicked);
    
    // 输出目录浏览按钮点击 -> 打开文件夹选择对话框
    connect(m_browseOutputButton, &QtMaterialFlatButton::clicked,
            this, &MainWindow::on_pushButton_browseOutput_clicked);
}


// ==================== 【蓝区】槽函数实现：路径与操作按钮 ====================

// Qt原生公共槽函数：输入路径浏览
// 信号源：m_browseInputButton clicked()信号
// 流程：打开文件选择对话框 -> 用户选择文件 -> 更新输入路径显示 -> 预加载图像信息
// 功能：让用户选择输入的图像文件路径
void MainWindow::on_pushButton_browse_clicked()
{
    // 打开文件选择对话框（支持常见图像格式和多帧TIFF）
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "请选择输入图像文件",                          // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),  // 默认打开桌面目录
        "图像文件 (*.tif *.tiff *.png *.jpg *.jpeg *.bmp);;所有文件 (*)"   // 文件过滤器
    );
    
    // 检查用户是否选择了文件（取消选择时filePath为空字符串）
    if (!filePath.isEmpty()) {
        // 将选择的文件路径显示到输入路径输入框中
        ui->lineEdit_inputPath->setText(filePath);
        
        // 在日志区记录用户操作
        ui->textEdit_log->append("已选择输入文件: " + filePath);
        
        // 预加载图像预览（方案B：仅获取帧数信息，不读取全部像素数据）
        preloadImagePreview(filePath);
    }
}

// 项目自定义辅助函数：预加载图像预览（方案B：仅获取帧数信息）
// 功能：快速获取多帧TIFF文件的帧数信息，用于初始化滑块范围
//       不读取实际像素数据，因此速度极快（毫秒级）
// 参数：filePath - 图像文件路径
void MainWindow::preloadImagePreview(const QString& filePath)
{
    // 使用Qt的QImageReader获取图像基本信息（不读取像素数据）
    QImageReader reader(filePath);
    
    // 检查文件是否可以正常读取
    if (!reader.canRead()) {
        ui->textEdit_log->append("警告: 无法读取文件 " + filePath);
        return;
    }
    
    // 获取图像总帧数（对于单帧图像返回1，多帧TIFF返回实际帧数）
    int frameCount = reader.imageCount();
    
    // 处理某些特殊格式返回-1或0的情况（视为单帧图像）
    if (frameCount <= 0) {
        frameCount = 1;
    }
    
    // 更新处理前图片的总帧数（用于滑块范围设置）
    totalOriginalFrames = frameCount;
    
    // 保存输入文件路径（后续运行处理时使用）
    m_inputFilePath = filePath;
    
    // 重置当前帧索引到第一帧
    currentOriginalFrame = 0;
    
    // 在日志中记录预加载结果
    ui->textEdit_log->append(QString("预加载完成: %1 帧图像").arg(frameCount));
    
    // 注意：此处不调用updateImageDisplay()，因为只是预加载帧数信息
    // 实际的图像显示将在用户点击"Run"按钮后进行
}

// Qt原生公共槽函数：输出目录浏览
// 信号源：m_browseOutputButton clicked()信号
// 流程：打开文件夹选择对话框 -> 用户选择目录 -> 更新输出路径显示
// 功能：让用户选择处理后图像的保存目录
void MainWindow::on_pushButton_browseOutput_clicked()
{
    // 打开文件夹选择对话框
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "请选择输出目录",                              // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),  // 默认打开桌面目录
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks  // 仅显示目录选项
    );
    
    // 检查用户是否选择了目录（取消选择时dirPath为空字符串）
    if (!dirPath.isEmpty()) {
        // 将选择的目录路径显示到输出路径输入框中
        ui->lineEdit_outputPath->setText(dirPath);
        
        // 在日志区记录用户操作
        ui->textEdit_log->append("已选择输出目录: " + dirPath);
    }
}


// ============================================================================
// 【绿区】参数设置面板
// 功能：提供13个参数输入控件（1个Toggle + 1个Checkbox + 11个TextField）
// 包含组件：m_toggleParam, m_checkboxParam, m_paramTextFields[11]
// ============================================================================

// ---------- 【绿区】Material组件初始化 ----------
void MainWindow::initGreenAreaComponents()
{
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
}

// ---------- 【绿区】将Material组件嵌入UI布局 ----------
void MainWindow::setupGreenAreaInLayout()
{
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
}

// ---------- 【绿区】绑定信号槽连接 ----------
void MainWindow::connectGreenAreaSignals()
{
    // 绿区参数控件的信号槽连接（可根据业务需求扩展）
    // 例如：参数改变时实时更新预览、参数验证等
    
    // 当前版本：参数值在点击"Run"时统一读取，无需实时监听
    // 如需实时响应，可在此处添加类似以下代码：
    // connect(m_toggleParam, &QtMaterialToggle::toggled, this, &MainWindow::onParamChanged);
    // connect(m_checkboxParam, &QtMaterialCheckBox::toggled, this, &MainWindow::onParamChanged);
}


// ============================================================================
// 【紫区】运行日志栏
// 功能：显示程序运行状态、错误信息、处理进度等文本日志
// 包含组件：textEdit_log（Qt原生QTextEdit）
// 说明：使用Qt原生控件，保持文本显示的最佳性能
// ============================================================================

// 【紫区】Material组件初始化
// 说明：紫区使用Qt原生QTextEdit，无需创建Material组件
void MainWindow::initPurpleAreaComponents()
{
    // 当前版本：紫区保持Qt原生样式，无需Material组件
}

// 【紫区】将Material组件嵌入UI布局
// 说明：日志栏已在Qt Designer中配置完成
void MainWindow::setupPurpleAreaInLayout()
{
    // 当前版本：紫区布局由Qt Designer管理，无需额外代码
}

// 【紫区】绑定信号槽连接
// 说明：日志栏为被动显示组件，无主动信号槽需求
void MainWindow::connectPurpleAreaSignals()
{
    // 当前版本：紫区通过其他区域的槽函数被动更新，无自定义信号槽
}


// ============================================================================
// 【橙区】双图显示区和控制条
// 功能：显示处理前/后对比图，提供帧切换控制和进度显示
// 包含组件：箭头按钮(4)、进度条、滑块(2)、同步帧按钮、进度标签
// ============================================================================

// ---------- 【橙区】Material组件初始化 ----------
void MainWindow::initOrangeAreaComponents()
{
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
    
    // 创建进度数值显示标签
    m_progressValueLabel = new QLabel("100%");
    m_progressValueLabel->setAlignment(Qt::AlignCenter);
    m_progressValueLabel->setStyleSheet("QLabel { color: #55aaff; font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 12px; }");
    
    // 创建处理前图片帧滑块（初始隐藏，处理完成后显示）
    m_sliderOriginal = new QtMaterialSlider(ui->widget_sliderOriginal);
    m_sliderOriginal->setRange(0, 0);          // 初始范围（无数据时为0-0）
    m_sliderOriginal->setValue(0);
    m_sliderOriginal->hide();                  // 处理完成前隐藏
    
    // 创建处理后图片帧滑块（初始隐藏，处理完成后显示）
    m_sliderProcessed = new QtMaterialSlider(ui->widget_sliderProcessed);
    m_sliderProcessed->setRange(0, 0);         // 初始范围（无数据时为0-0）
    m_sliderProcessed->setValue(0);
    m_sliderProcessed->hide();                 // 处理完成前隐藏
    
    // 创建同步帧按钮
    m_syncButton = new QtMaterialRaisedButton("同步帧");
    // 设置同步帧按钮的大小策略，使其能够拉伸填充空间
    m_syncButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_syncButton->setMinimumHeight(30);
}

// ---------- 【橙区】将Material组件嵌入UI布局 ----------
void MainWindow::setupOrangeAreaInLayout()
{
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
    
    // 替换进度条（移到顶部控制条）
    QHBoxLayout *topControlBarLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_topControlBar);
    if (topControlBarLayout) {
        // 移除原始进度条
        QLayoutItem *progressItem = topControlBarLayout->itemAt(1);
        if (progressItem && progressItem->widget() == ui->progressBar_processing) {
            topControlBarLayout->removeWidget(ui->progressBar_processing);
            ui->progressBar_processing->deleteLater();
            // 添加Material进度条
            topControlBarLayout->insertWidget(1, m_progressBar);
        }
        
        // 添加进度数值显示标签
        QLayoutItem *progressValueItem = topControlBarLayout->itemAt(2);
        if (progressValueItem && progressValueItem->widget() == ui->widget_progressValue) {
            // 创建布局并添加标签
            QVBoxLayout *progressValueLayout = new QVBoxLayout(ui->widget_progressValue);
            progressValueLayout->setContentsMargins(0, 0, 0, 0);
            progressValueLayout->addWidget(m_progressValueLabel);
        }
    }
    
    // 替换同步帧按钮（在底部控制条）
    QHBoxLayout *controlBarLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_controlBar);
    if (controlBarLayout) {
        int syncIndex = controlBarLayout->indexOf(ui->pushButton_syncFrames);
        if (syncIndex >= 0) {
            controlBarLayout->removeWidget(ui->pushButton_syncFrames);
            ui->pushButton_syncFrames->deleteLater();
            controlBarLayout->insertWidget(syncIndex, m_syncButton);
        }
    }
}

// ---------- 【橙区】绑定信号槽连接 ----------
void MainWindow::connectOrangeAreaSignals()
{
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


// ==================== 【橙区】槽函数实现：双图显示与帧控制 ====================

// 项目自定义槽函数：左侧上一帧
// 信号源：m_prevLeftButton clicked()信号
// 流程：检查边界 -> 帧索引减1 -> 更新滑块位置 -> 刷新图像显示
// 功能：切换到处理前图片的上一帧（如果不在第一帧）
void MainWindow::onPrevFrameLeft()
{
    // 边界检查：确保不会超出第一帧（索引<0）
    if (currentOriginalFrame > 0) {
        // 帧索引减1（切换到上一帧）
        currentOriginalFrame--;
        
        // 同步模式下，处理后图片也跟着切换
        if (isSyncMode) {
            currentProcessedFrame = currentOriginalFrame;
            // 阻止右侧滑块的信号发射（避免循环触发）
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(currentProcessedFrame);
            m_sliderProcessed->blockSignals(false);
        }
        
        // 更新左侧滑块位置（会触发valueChanged信号刷新显示）
        m_sliderOriginal->setValue(currentOriginalFrame);
    }
}

// 项目自定义槽函数：左侧下一帧
// 信号源：m_nextLeftButton clicked()信号
// 流程：检查边界 -> 帧索引加1 -> 更新滑块位置 -> 刷新图像显示
// 功能：切换到处理前图片的下一帧（如果不在最后一帧）
void MainWindow::onNextFrameLeft()
{
    // 边界检查：确保不会超出最后一帧
    if (currentOriginalFrame < totalOriginalFrames - 1) {
        // 帧索引加1（切换到下一帧）
        currentOriginalFrame++;
        
        // 同步模式下，处理后图片也跟着切换
        if (isSyncMode) {
            currentProcessedFrame = currentOriginalFrame;
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(currentProcessedFrame);
            m_sliderProcessed->blockSignals(false);
        }
        
        // 更新左侧滑块位置
        m_sliderOriginal->setValue(currentOriginalFrame);
    }
}

// 项目自定义槽函数：右侧上一帧
// 信号源：m_prevRightButton clicked()信号
// 流程：检查边界 -> 帧索引减1 -> 更新滑块位置 -> 刷新图像显示
// 功能：切换到处理后图片的上一帧（如果不在第一帧）
void MainWindow::onPrevFrameRight()
{
    // 边界检查：确保不会超出第一帧
    if (currentProcessedFrame > 0) {
        // 帧索引减1
        currentProcessedFrame--;
        
        // 同步模式下，处理前图片也跟着切换
        if (isSyncMode) {
            currentOriginalFrame = currentProcessedFrame;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(currentOriginalFrame);
            m_sliderOriginal->blockSignals(false);
        }
        
        // 更新右侧滑块位置
        m_sliderProcessed->setValue(currentProcessedFrame);
    }
}

// 项目自定义槽函数：右侧下一帧
// 信号源：m_nextRightButton clicked()信号
// 流程：检查边界 -> 帧索引加1 -> 更新滑块位置 -> 刷新图像显示
// 功能：切换到处理后图片的下一帧（如果不在最后一帧）
void MainWindow::onNextFrameRight()
{
    // 边界检查：确保不会超出最后一帧
    if (currentProcessedFrame < totalProcessedFrames - 1) {
        // 帧索引加1
        currentProcessedFrame++;
        
        // 同步模式下，处理前图片也跟着切换
        if (isSyncMode) {
            currentOriginalFrame = currentProcessedFrame;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(currentOriginalFrame);
            m_sliderOriginal->blockSignals(false);
        }
        
        // 更新右侧滑块位置
        m_sliderProcessed->setValue(currentProcessedFrame);
    }
}

// 项目自定义槽函数：同步帧切换
// 信号源：m_syncButton clicked()信号
// 流程：切换同步模式标志位 -> 更新按钮文字提示 -> 记录日志
// 功能：开启/关闭前后图片帧同步模式
//       开启后：操作一侧会自动同步另一侧
//       关闭后：两侧可独立切帧
void MainWindow::onSyncFramesClicked()
{
    // 切换同步模式标志位（true <-> false取反）
    isSyncMode = !isSyncMode;
    
    // 根据新模式状态更新按钮文字和日志提示
    if (isSyncMode) {
        // 开启同步模式
        m_syncButton->setText("取消同步");           // 按钮文字改为"取消同步"
        ui->textEdit_log->append("同步模式已开启: 前后图片帧数将联动");  // 日志提示
        
        // 立即同步两侧到同一帧（以左侧为准）
        currentProcessedFrame = currentOriginalFrame;
        m_sliderProcessed->blockSignals(true);
        m_sliderProcessed->setValue(currentProcessedFrame);
        m_sliderProcessed->blockSignals(false);
    } else {
        // 关闭同步模式
        m_syncButton->setText("同步帧");              // 按钮文字恢复为"同步帧"
        ui->textEdit_log->append("同步模式已关闭: 前后图片帧数独立控制");  // 日志提示
    }
    
    // 无论是否同步，都刷新一次图像显示
    updateImageDisplay();
}

// 辅助函数：更新图像显示（方案B核心实现）
// 功能：根据当前帧索引，从文件中读取对应帧并显示到两个QLabel上
//       自动计算缩放比例以适应显示区域大小
void MainWindow::updateImageDisplay()
{
    // ========== 显示处理前图片（左侧区域） ==========

    // 检查是否有有效的输入文件路径
    if (!m_inputFilePath.isEmpty()) {
        // 从TIFF文件中读取当前帧（核心函数调用）
        QImage qOriginalImg = readTiffFrame(m_inputFilePath, currentOriginalFrame);

        // 检查读取是否成功
        if (!qOriginalImg.isNull()) {
            // 获取左侧显示区域（label_originalImage）的实际尺寸
            QSize labelSize = ui->label_originalImage->size();

            // 计算缩放比例：保持宽高比，适应显示区域（等比缩放）
            QPixmap pixmapOriginal = QPixmap::fromImage(qOriginalImg);
            QPixmap scaledPixmapOriginal = pixmapOriginal.scaled(
                labelSize,                            // 目标尺寸（ QLabel的大小）
                Qt::KeepAspectRatio,                 // 保持宽高比
                Qt::SmoothTransformation             // 平滑转换（高质量缩放算法）
            );

            // 将缩放后的图像显示到左侧QLabel
            ui->label_originalImage->setPixmap(scaledPixmapOriginal);
        } else {
            // 读取失败时显示占位文字
            ui->label_originalImage->setText("处理前图片\n(帧 " + QString::number(currentOriginalFrame + 1) + "/" + QString::number(totalOriginalFrames) + ")");
        }
    }


    // ========== 显示处理后图片（右侧区域） ==========

    // 检查是否有有效的输出文件路径（处理完成后才会设置）
    if (!m_outputFilePath.isEmpty()) {
        // 从输出TIFF文件中读取当前帧
        QImage qProcessedImg = readTiffFrame(m_outputFilePath, currentProcessedFrame);

        // 检查读取是否成功
        if (!qProcessedImg.isNull()) {
            // 获取右侧显示区域（label_processedImage）的实际尺寸
            QSize labelSize = ui->label_processedImage->size();

            // 计算缩放比例：保持宽高比，适应显示区域
            QPixmap pixmapProcessed = QPixmap::fromImage(qProcessedImg);
            QPixmap scaledPixmapProcessed = pixmapProcessed.scaled(
                labelSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );

            // 将缩放后的图像显示到右侧QLabel
            ui->label_processedImage->setPixmap(scaledPixmapProcessed);
        } else {
            // 读取失败时显示占位文字
            ui->label_processedImage->setText("处理后图片\n(帧 " + QString::number(currentProcessedFrame + 1) + "/" + QString::number(totalProcessedFrames) + ")");
        }
    }
}


// ============================================================================
// 【Qt原生公共槽函数：运行处理（解决UI冻结问题）】
// 信号源：m_runButton clicked信号
// 功能：调用DarkSectioning处理逻辑，使用processEvents保持UI响应
// ============================================================================
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
    
    // 第3步：设置输出文件路径（方案B：从文件路径读取显示）
    QString outputDir = ui->lineEdit_outputPath->text();
    if (outputDir.isEmpty()) {
        outputDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    
    // 确保路径以斜杠结尾
    if (!outputDir.endsWith('/') && !outputDir.endsWith('\\')) {
        outputDir += '/';
    }
    
    // 构建输出文件名：原始文件名 + _Darked.tif
    QFileInfo inputFileInfo(m_inputFilePath);
    QString baseName = inputFileInfo.baseName();  // 去除扩展名的文件名
    QString outputFileName = baseName + "_Darked.tif";
    m_outputFilePath = outputDir + outputFileName;  // 输出文件名为：原始文件名_Darked.tif
    
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
    
    // 第5步：配置滑块范围（如果有有效帧数）
    if (totalOriginalFrames > 0) {
        // 设置处理前图片滑块的范围（0 到 总帧数-1）
        m_sliderOriginal->setRange(0, totalOriginalFrames - 1);
        m_sliderOriginal->setValue(0);              // 重置到第一帧
        m_sliderOriginal->show();                  // 显示滑块
    }
    
    if (totalProcessedFrames > 0) {
        // 设置处理后图片滑块的范围
        m_sliderProcessed->setRange(0, totalProcessedFrames - 1);
        m_sliderProcessed->setValue(0);            // 重置到第一帧
        m_sliderProcessed->show();                 // 显示滑块
    }
    
    // 第6步：重置帧索引并显示第一帧图像
    currentOriginalFrame = 0;
    currentProcessedFrame = 0;
    
    // 更新进度条到100%（表示全部完成）
    m_progressBar->setValue(100);
    
    // 更新进度数值显示标签
    m_progressValueLabel->setText("100%");
    
    // 刷新图像显示（调用辅助函数显示第一帧）
    updateImageDisplay();
    
    // 重新启用Run按钮（允许再次处理）
    m_runButton->setEnabled(true);
    
    // 强制最终UI刷新
    QApplication::processEvents();
}


// ============================================================================
// 窗口大小改变事件处理函数
// ============================================================================
// Qt原生事件重载：窗口大小改变时自动调用
// 信号源：系统resizeEvent
// 功能：窗口缩放时自动重新计算图片大小并更新显示（无需手动点击"同步帧"）
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


// ============================================================================
// 【应用Material主题颜色】
// 功能：统一设置所有Material组件的主题色为#55aaff（天蓝色）
// ============================================================================
void MainWindow::applyMaterialTheme()
{
    // 设置主题主色调为#55aaff（RGB: 85, 170, 255）
    QColor themeColor(85, 170, 255);
    
    // 为【蓝区】组件应用主题色
    m_runButton->setForegroundColor(themeColor);
    m_browseInputButton->setForegroundColor(themeColor);
    m_browseOutputButton->setForegroundColor(themeColor);
    
    // 为【绿区】组件应用主题色
    m_toggleParam->setTrackColor(themeColor);
    m_checkboxParam->setCheckedColor(themeColor);
    m_checkboxParam->setTextColor(QColor(51, 51, 51));  // 设置Checkbox文字颜色为深灰色
    
    foreach (QtMaterialTextField *textField, m_paramTextFields) {
        textField->setInkColor(themeColor);
    }
    
    // 为【橙区】组件应用主题色
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
    
    // 设置窗口整体样式表（全局UI风格）
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


// ============================================================================
// 核心函数：从多帧TIFF文件中读取指定帧（Qt原生方式）
// 功能：使用Qt原生QImageReader读取TIFF文件的特定帧
//       直接返回QImage，无需OpenCV转换，显示效果与Windows照片查看器一致
// 参数：filePath - TIFF文件路径，frameIndex - 帧索引（从0开始）
// 返回：QImage对象（如果读取失败返回空QImage）
// ============================================================================
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


// ============================================================================
// 色调映射函数：模拟Windows照片查看器效果
// 功能：将OpenCV Mat数据转换为QImage，应用标准色调映射算法
//       确保显示效果与Windows照片查看器一致
// 参数：mat - OpenCV Mat对象（支持CV_8U/CV_16U/CV_32F等格式）
// 返回：QImage对象（RGB888格式）
// ============================================================================
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
