// Dark Sectioning 主程序（精简版）
// 基于Material Design风格UI改造版本
// 橙区功能已完全迁移到OrangeWidget自定义控件中

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
    m_orangeWidget(nullptr)                      // 初始化橙区控件指针为空（稍后创建）
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
    
    // 第4步：【紫区】运行日志栏 - 使用原生QTextEdit，无需Material组件
    // 紫区包含：textEdit_log日志显示框
    // 此处无需额外代码，Qt Designer已配置完成
    
    // 第5步：【橙区】将OrangeWidget嵌入主窗口指定位置
    // 橙区所有功能已封装在OrangeWidget自定义控件中，此处只需创建并嵌入即可
    embedOrangeWidget();
    
    // 第6步：应用Material主题颜色#55aaff（全局统一应用，仅非橙区组件）
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
    
    // 释放【橙区】OrangeWidget对象内存
    // 注意：OrangeWidget内部会自动清理其所有子控件（包括Material组件）
    delete m_orangeWidget;
}


// ============================================================================
// 【红区】菜单栏区域
// 功能：提供文件、分析、设置、帮助等顶层菜单
// 说明：使用Qt原生QMenuBar，无需Material组件
// ============================================================================

// 【红区】Material组件初始化
void MainWindow::initRedAreaComponents()
{
    // 当前版本：红区保持Qt原生样式，无需Material组件
}

// 【红区】将Material组件嵌入UI布局
void MainWindow::setupRedAreaInLayout()
{
    // 当前版本：红区布局由Qt Designer管理，无需额外代码
}

// 【红区】绑定信号槽连接
void MainWindow::connectRedAreaSignals()
{
    // 当前版本：红区无自定义信号槽连接需求
}


// ============================================================================
// 【蓝区】路径与操作按钮区
// 功能：输入/输出路径选择、Run主操作按钮
// 说明：蓝区按钮使用Qt原生QPushButton（ui->pushButton_run等），无需Material组件初始化
// ============================================================================

// ---------- 【蓝区】Material组件初始化 ----------
void MainWindow::initBlueAreaComponents()
{
    // 蓝区按钮使用Qt原生QPushButton，无需创建Material组件
}

// ---------- 【蓝区】将Material组件嵌入UI布局 ----------
void MainWindow::setupBlueAreaInLayout()
{
    // 蓝区按钮使用Qt原生QPushButton，无需替换布局中的组件
}

// ---------- 【蓝区】绑定信号槽连接 ----------
void MainWindow::connectBlueAreaSignals()
{
    // 蓝区按钮使用Qt原生QPushButton，槽函数采用on_objectName_signal命名规范
    // Qt的setupUi()内部会调用QMetaObject::connectSlotsByName()自动连接
    // 因此无需手动connect，否则槽函数会被调用两次（如浏览对话框弹出两次）
}


// ==================== 【蓝区】槽函数实现：路径与操作按钮 ====================

// Qt原生公共槽函数：输入路径浏览
// 信号源：ui->pushButton_browse clicked()信号
// 流程：打开文件选择对话框 -> 用户选择文件 -> 更新输入路径显示 -> 预加载图像信息到橙区
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
        
        // 预加载图像预览信息到橙区（调用OrangeWidget的公共接口）
        // OrangeWidget会自动获取帧数信息并保存文件路径
        m_orangeWidget->preloadImagePreview(filePath);
    }
}

// Qt原生公共槽函数：输出目录浏览
// 信号源：ui->pushButton_browseOutput clicked()信号
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
// 【紫区】运行日志栏
// 功能：显示程序运行状态、错误信息、处理进度等文本日志
// 包含组件：textEdit_log（Qt原生QTextEdit）
// 说明：使用Qt原生控件，保持文本显示的最佳性能
// ============================================================================

// 【紫区】Material组件初始化
void MainWindow::initPurpleAreaComponents()
{
    // 当前版本：紫区保持Qt原生样式，无需Material组件
}

// 【紫区】将Material组件嵌入UI布局
void MainWindow::setupPurpleAreaInLayout()
{
    // 当前版本：紫区布局由Qt Designer管理，无需额外代码
}

// 【紫区】绑定信号槽连接
void MainWindow::connectPurpleAreaSignals()
{
    // 当前版本：紫区通过其他区域的槽函数被动更新，无自定义信号槽
}


// ============================================================================
// 【橙区嵌入与管理】
// 功能：创建OrangeWidget并将其嵌入主窗口右侧区域（原橙区位置）
// 说明：橙区所有UI和逻辑已完全封装在OrangeWidget中，主窗口只需管理其生命周期
// ============================================================================

// 将OrangeWidget嵌入主窗口指定位置
// 流程：创建OrangeWidget对象 -> 从布局中移除原橙区占位容器 -> 在相同位置插入OrangeWidget
// 功能：在主窗口右侧区域（verticalLayout_right的第1个位置）显示橙区的双图显示+控制条界面
void MainWindow::embedOrangeWidget()
{
    // 第1步：创建OrangeWidget对象（传入this作为父窗口，确保自动内存管理）
    m_orangeWidget = new OrangeWidget(this);

    // 第2步：获取右侧区域的垂直布局对象
    QVBoxLayout *rightLayout = ui->verticalLayout_right;

    // 第3步：移除.ui中的橙区占位容器（widget_orangePlaceholder），避免占用多余空间
    int placeholderIndex = rightLayout->indexOf(ui->widget_orangePlaceholder);
    if (placeholderIndex >= 0) {
        rightLayout->removeWidget(ui->widget_orangePlaceholder);
        ui->widget_orangePlaceholder->deleteLater();
    }

    if (rightLayout) {
        // 第4步：在原位置插入OrangeWidget
        rightLayout->insertWidget(placeholderIndex >= 0 ? placeholderIndex : 0, m_orangeWidget);
    }

    // ========== 关键：在代码中强制设置布局属性（.ui属性可能被动态操作覆盖）==========

    // 设置内容区水平布局的stretch因子：左列固定宽度(0)，橙区填充所有额外宽度(1)
    QHBoxLayout *contentLayout = ui->horizontalLayout_content;
    contentLayout->setStretch(0, 0);   // 左列（绿区+紫区）：不分配额外宽度
    contentLayout->setStretch(1, 1);   // 右列（橙区）：独占所有额外宽度

    // 设置左列垂直布局的stretch因子：绿区和紫区初始高度比例
    QVBoxLayout *leftLayout = ui->verticalLayout_left;
    leftLayout->setStretch(0, 1);   // 绿区groupBox_params（Fixed 350×360）：占1份高度
    leftLayout->setStretch(1, 1);   // 紫区textEdit_log：占1份高度（实际会吸收所有剩余空间）

    // 设置绿区为Fixed大小策略，固定尺寸350×360（窗口放大时保持不变）
    ui->widget_greenPlaceholder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->widget_greenPlaceholder->setMinimumSize(350, 360);
    ui->widget_greenPlaceholder->setMaximumSize(350, 360);

    // 连接OrangeWidget的日志消息信号到主窗口的日志显示槽
    connect(m_orangeWidget, &OrangeWidget::logMessage, [this](const QString &message) {
        ui->textEdit_log->append(message);
    });
}


// ============================================================================
// 【Qt原生公共槽函数：运行处理（解决UI冻结问题）】
// 信号源：ui->pushButton_run clicked信号
// 功能：调用DarkSectioning处理逻辑，使用processEvents保持UI响应
//         处理完成后通知橙区显示结果图像
// ============================================================================
void MainWindow::on_pushButton_run_clicked()
{
    // 在日志区输出开始信息
    ui->textEdit_log->append("开始图像处理...");
    
    // 禁用Run按钮防止重复点击
    ui->pushButton_run->setEnabled(false);
    // 将按钮文本改为"运行中.."，提示用户处理正在进行
    ui->pushButton_run->setText("运行中..");
    
    // ========== 第1步：获取并验证输入文件路径 ==========
    
    QString inputFilePath = ui->lineEdit_inputPath->text();  // 从输入框获取路径
    if (inputFilePath.isEmpty()) {
        ui->textEdit_log->append("错误: 请先选择输入图片路径");
        ui->pushButton_run->setEnabled(true);  // 重新启用按钮
        ui->pushButton_run->setText("运行");    // 恢复按钮文本
        return;  // 提前返回，不继续执行
    }
    
    // 将输入文件路径传递给橙区（橙区需要此路径来显示处理前图片）
    m_orangeWidget->setInputFilePath(inputFilePath);
    
    // ========== 第2步：通知橙区开始处理（显示进度条、隐藏滑块） ==========
    
    // 调用OrangeWidget的startProcessing()接口
    // 橙区会自动显示进度条、隐藏帧滑块、重置进度值为0%
    m_orangeWidget->startProcessing();
    
    // 强制刷新UI（确保进度条立刻显示）
    QApplication::processEvents();
    
    // 更新进度条到10%（表示开始处理）
    m_orangeWidget->updateProgress(10);
    QApplication::processEvents();
    
    // ========== 第3步：调用原有的DarkSectioning处理函数（保持原有业务逻辑不变） ==========
    
    // darkSectioning->process() 内部会：
    // 1. 读取输入文件并执行Dark Sectioning算法
    // 2. 将结果保存到输出目录的xxx_Darked.tif文件
    // 3. 定期调用QApplication::processEvents()保持UI响应
    darkSectioning->process();
    
    // 更新进度条到90%（表示算法处理完成）
    m_orangeWidget->updateProgress(90);
    QApplication::processEvents();
    
    // ========== 第4步：构建输出文件路径并传递给橙区 ==========
    
    QString outputDir = ui->lineEdit_outputPath->text();  // 获取输出目录
    if (outputDir.isEmpty()) {
        // 如果用户未选择输出目录，默认保存到桌面
        outputDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    
    // 确保路径以斜杠结尾（Windows兼容/和\两种分隔符）
    if (!outputDir.endsWith('/') && !outputDir.endsWith('\\')) {
        outputDir += '/';
    }
    
    // 构建输出文件名：原始文件名 + _Darked.tif
    QFileInfo inputFileInfo(inputFilePath);
    QString baseName = inputFileInfo.baseName();  // 去除扩展名的文件名（如"image"）
    QString outputFileName = baseName + "_Darked.tif";  // 输出文件名（如"image_Darked.tif"）
    QString outputFilePath = outputDir + outputFileName;  // 完整输出路径
    
    // 将输出文件路径传递给橙区（橙区需要此路径来显示处理后图片）
    m_orangeWidget->setOutputFilePath(outputFilePath);
    
    // ========== 第5步：获取帧数信息并通知橙区处理完成 ==========
    
    // 从darkSectioning的业务对象中读取帧数统计信息
    int totalOriginalFrames = darkSectioning->imageStack.size();      // 处理前总帧数
    int totalProcessedFrames = darkSectioning->final_images.size();   // 处理后总帧数
    
    // 在日志中记录完成信息
    ui->textEdit_log->append("图像处理完成!");
    ui->textEdit_log->append(QString("输入文件: %1").arg(inputFilePath));
    ui->textEdit_log->append(QString("输出文件: %1").arg(outputFilePath));
    ui->textEdit_log->append(QString("处理前帧数: %1, 处理后帧数: %2")
                             .arg(totalOriginalFrames)
                             .arg(totalProcessedFrames));
    
    // 调用OrangeWidget的finishProcessing()接口通知处理完成
    // 橙区会自动：
    // 1. 配置两个滑块的有效范围（0 到 总帧数-1）
    // 2. 显示滑块（之前被startProcessing隐藏了）
    // 3. 更新进度条到100%
    // 4. 显示第一帧的处理前/后对比图像
    m_orangeWidget->finishProcessing(totalOriginalFrames, totalProcessedFrames);
    
    // 重新启用Run按钮（允许再次处理其他图片）
    ui->pushButton_run->setEnabled(true);
    // 将按钮文本恢复为"Run"
    ui->pushButton_run->setText("运行");
    
    // 强制最终UI刷新（确保所有变更都立刻呈现给用户）
    QApplication::processEvents();
}


// ============================================================================
// 窗口大小改变事件处理函数
// ============================================================================
// Qt原生事件重载：窗口大小改变时自动调用
// 信号源：系统resizeEvent
// 功能：窗口缩放时自动通知橙区重新计算图片大小并更新显示
//       （无需手动点击"同步帧"按钮）
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 调用父类的resizeEvent处理（这一步必须要有！）
    // 确保Qt框架正常的布局更新、子控件位置调整等基础功能正常工作
    QMainWindow::resizeEvent(event);
    
    // 使用单次定时器延迟50毫秒后再通知橙区更新图像显示
    // 原因：resize事件发生后，布局系统需要一定时间重新计算所有子控件的新尺寸
    //       如果立即调用updateImageDisplay()，此时QLabel可能还没有更新到最终大小，
    //       导致图片缩放尺寸不准确。延迟50ms可以确保布局已经稳定。
    QTimer::singleShot(50, this, [this]() {
        // 通知橙区更新图像显示（OrangeWidget内部会自动重新计算缩放比例）
        if (m_orangeWidget) {
            m_orangeWidget->updateImageDisplay();
        }
    });
}


// ============================================================================
// 【应用Material主题颜色】
// 功能：统一设置所有Material组件的主题色为#55aaff（天蓝色）
// 说明：仅对蓝区组件应用主题色，橙区组件已在OrangeWidget内部自行设置
//       绿区参数面板已清空，无需设置主题色
// ============================================================================
void MainWindow::applyMaterialTheme()
{
    // 设置主题主色调为#55aaff（RGB: 85, 170, 255）
    QColor themeColor(85, 170, 255);
    
    // 注意：橙区组件的主题色已在OrangeWidget::applyMaterialTheme()中设置
    // 无需在此处重复设置
    
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
