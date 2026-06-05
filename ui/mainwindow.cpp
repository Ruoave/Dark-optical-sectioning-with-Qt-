// Dark Sectioning 主程序（精简版）
// 基于Material Design风格UI改造版本
// 橙区功能已完全迁移到OrangeWidget自定义控件中

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "paramsBasic.h"
#include "paramsExpert.h"
#include "qtmaterialautocomplete.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <windows.h>
#include <ViewMat.h>
#include <vector>
#include <QFileDialog>
#include <QFile>            // 用于读写参数txt文件
#include <QTextStream>      // 用于文本流读写键值对
#include <QMessageBox>      // 用于弹出警告/错误提示框
#include <QString>
#include <QStandardPaths>
#include <QImage>
#include <QPixmap>
#include <QLayout>
#include <QApplication>
#include <QTimer>
#include <QPalette>

// 引入批量处理对话框头文件（独立功能，菜单"批量处理"弹出模态窗口）
#include "batchdialog.h"

// 引入关于对话框头文件（菜单"关于此软件"弹出模态窗口）
#include "aboutdialog.h"

// 引入 QDesktopServices（用于"帮助"菜单各按钮调用系统默认浏览器打开 help.html）
#include <QDesktopServices>

using namespace cv;
using namespace std;
using namespace chrono;


// ============================================================================
// 【构造函数：初始化窗口】
// ============================================================================
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    darkSectioning(new DarkSectioning(ui))       // 初始化业务对象
{
    // 第1步：调用Qt Designer生成的UI设置
    ui->setupUi(this);

    // 设置DarkSectioning的OrangeBar指针（供进度更新时直接调用setProgress）
    darkSectioning->setOrangeBar(ui->widget_orangePlaceholder->orangeBar());
    

    //第2步：主窗口按设计图分为5个区域：红区、蓝区、紫区、橙区、绿区
    // 2.1【红区】菜单栏初始化（使用Qt原生MenuBar，无需Material组件）
    // 红区包含：文件、分析、设置、帮助 菜单项
    // 此处无需额外代码，Qt Designer已配置完成

    // 2.2：【蓝区】路径与操作按钮区 - Material组件初始化+信号槽

    // 2.3：【紫区】运行日志栏 - 使用原生QTextEdit，无需Material组件
    // 紫区包含：textEdit_log日志显示框
    // 此处无需额外代码，Qt Designer已配置完成

    // 2.4：【绿区】GreenWidget已通过方法B（容器提升法）在.ui中提升
    // ui->widget_greenPlaceholder 的类型已是 GreenWidget*，由uic在setupUi时自动创建

    // 用 ParamsBasic 默认值初始化 GreenWidget 控件显示（程序启动时控件显示默认值）
    ParamsBasic defaults;  // 此对象自带默认值：background=0, pad=1, denoise=0, NA=1.49, emwavelength=610, pixelsize=65, factor=2
    ui->widget_greenPlaceholder->setBackground(defaults.background);
    ui->widget_greenPlaceholder->setPad(defaults.pad);
    ui->widget_greenPlaceholder->setDenoise(defaults.denoise);
    ui->widget_greenPlaceholder->setNA(defaults.NA);
    ui->widget_greenPlaceholder->setEmwavelength(defaults.emwavelength);
    ui->widget_greenPlaceholder->setPixelsize(defaults.pixelsize);
    ui->widget_greenPlaceholder->setFactor(defaults.factor);

    // 第二页高级参数 lineEdit~lineEdit_6 启动时不显示内容（文本为空）
    // 算法默认值保留在 ParamsExpert 结构体中，用户未输入时算法使用默认值
    
    // 2.5：【橙区】OrangeWidget已通过方法B（容器提升法）在.ui中提升
    // ui->widget_orangePlaceholder 的类型已是 OrangeWidget*，由uic在setupUi时自动创建
    // 连接橙区日志信号即可
    connect(ui->widget_orangePlaceholder, &OrangeWidget::logMessage, [this](const QString &message) {
        ui->textEdit_log->append(message);  }   );


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
    
    // Qt父子对象机制会自动回收，无需手动delete
}


// ============================================================================
// 【红区】菜单栏区域
// 功能：提供文件、分析、设置、帮助等顶层菜单
// 说明：使用Qt原生QMenuBar，无需Material组件
// ============================================================================



// ============================================================================
// 【蓝区】路径与操作按钮区
// 功能：输入/输出路径选择、Run主操作按钮
// 说明：蓝区按钮使用Qt原生QPushButton（ui->pushButton_run等），无需Material组件初始化
// ============================================================================

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
        ui->widget_orangePlaceholder->preloadImagePreview(filePath);
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
// 【Qt原生公共槽函数：运行处理】
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
    ui->widget_orangePlaceholder->setInputFilePath(inputFilePath);
    
    // ========== 第2步：通知橙区开始处理（显示进度条） ==========
    
    // 调用OrangeWidget的startProcessing()接口
    // 橙区会自动显示进度条、重置进度值为0%
    ui->widget_orangePlaceholder->startProcessing();
    
    // 强制刷新UI（确保界面立刻更新）
    QApplication::processEvents();
    
    // ========== 第3步：从 GreenWidget 控件读取当前参数值，写入 DarkSectioning 的 paramsBasicSet ==========
    
    // 从 GreenWidget 控件读取用户当前设置的值，写入 DarkSectioning 的参数
    darkSectioning->paramsBasicSet.background = ui->widget_greenPlaceholder->getBackground();
    darkSectioning->paramsBasicSet.pad = ui->widget_greenPlaceholder->getPad();
    darkSectioning->paramsBasicSet.denoise = ui->widget_greenPlaceholder->getDenoise();
    darkSectioning->paramsBasicSet.NA = ui->widget_greenPlaceholder->getNA();
    darkSectioning->paramsBasicSet.emwavelength = ui->widget_greenPlaceholder->getEmwavelength();
    darkSectioning->paramsBasicSet.pixelsize = ui->widget_greenPlaceholder->getPixelsize();
    darkSectioning->paramsBasicSet.factor = ui->widget_greenPlaceholder->getFactor();
    
    // 从 GreenWidget 第二页控件读取高级参数值，写入 DarkSectioning 的 paramsExpertSet
    // 每个参数独立判断：有填写就写入，没填写保持 ParamsExpert 结构体的默认值
    if (!ui->widget_greenPlaceholder->getDeg().isEmpty()) {
        darkSectioning->paramsExpertSet.deg = ui->widget_greenPlaceholder->getDeg().toStdString();
    }
    if (!ui->widget_greenPlaceholder->getDep().isEmpty()) {
        darkSectioning->paramsExpertSet.dep = ui->widget_greenPlaceholder->getDep().toStdString();
    }
    if (!ui->widget_greenPlaceholder->getHl().isEmpty()) {
        darkSectioning->paramsExpertSet.hl = ui->widget_greenPlaceholder->getHl().toStdString();
    }
    if (!ui->widget_greenPlaceholder->isThresEmpty()) {  // thres
        darkSectioning->paramsExpertSet.thres = ui->widget_greenPlaceholder->getThres();
    }
    if (!ui->widget_greenPlaceholder->isDivideEmpty()) {  // divide
        darkSectioning->paramsExpertSet.divide = ui->widget_greenPlaceholder->getDivide();
    }
    if (!ui->widget_greenPlaceholder->isPadsizeEmpty()) {  // padsize
        darkSectioning->paramsExpertSet.padsize = ui->widget_greenPlaceholder->getPadsize();
    }
    darkSectioning->paramsExpertSet.isQuick = ui->widget_greenPlaceholder->getIsQuick();  // 单帧处理模式：0-多帧(默认), 1-单帧快速
    
    // ========== 第4步：调用原有的DarkSectioning处理函数（保持原有业务逻辑不变） ==========
    
    // darkSectioning->process() 内部会：
    // 1. 读取输入文件并执行Dark Sectioning算法
    // 2. 将结果保存到输出目录的xxx_Darked.tif文件
    // 3. 定期调用QApplication::processEvents()保持UI响应
    darkSectioning->process();
    
    QApplication::processEvents();
    
    // ========== 第5步：构建输出文件路径并传递给橙区 ==========
    
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
    ui->widget_orangePlaceholder->setOutputFilePath(outputFilePath);
    
    // ========== 第6步：获取帧数信息并通知橙区处理完成 ==========
    
    // 从darkSectioning的业务对象中读取帧数统计信息
    int totalOriginalFrames = darkSectioning->imageStack.size();      // 处理前总帧数
    int totalProcessedFrames = darkSectioning->final_images.size();   // 处理后总帧数
    
    
    // 调用OrangeWidget的finishProcessing()接口通知处理完成
    // 橙区会自动：
    // 1. 配置两个滑块的有效范围（0 到 总帧数-1）
    // 2. 显示滑块（之前被startProcessing隐藏了）
    // 3. 更新进度条到100%
    // 4. 显示第一帧的处理前/后对比图像
    ui->widget_orangePlaceholder->finishProcessing(totalOriginalFrames, totalProcessedFrames);
    
    // 重新启用Run按钮（允许再次处理其他图片）
    ui->pushButton_run->setEnabled(true);
    // 将按钮文本恢复为"Run"
    ui->pushButton_run->setText("运行");
    
    // 强制最终UI刷新（确保所有变更都立刻呈现给用户）
    QApplication::processEvents();
}


// ============================================================================
// 【菜单栏槽函数：导出算法所用参数】
// 信号源：ui->actionExport_Parameters triggered()信号
// 流程：弹出保存文件对话框 → 用户选择路径 → 写入键值对格式txt文件
// 功能：将darkSectioning中paramsBasicSet/paramsExpertSet的当前值导出为txt
// 说明：所有参数字段均有默认值，无需"已运行过"检查
// ============================================================================
void MainWindow::on_actionExport_Parameters_triggered()
{
    // 弹出保存文件对话框，让用户自选导出路径和文件名
    QString filePath = QFileDialog::getSaveFileName(
        this,                                              // 父窗口
        QString::fromUtf8("导出算法所用参数"),               // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),  // 默认保存到桌面
        QString::fromUtf8("文本文件 (*.txt)")               // 文件过滤器：仅显示txt
    );

    // 用户取消选择时filePath为空字符串，直接返回
    if (filePath.isEmpty()) {
        return;
    }

    // 打开文件准备写入（覆盖模式）
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 无法创建文件（如路径权限问题），弹出错误提示
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("无法创建文件，请检查路径权限"));
        return;
    }

    // 创建文本输出流，连接文件进行写入
    QTextStream out(&file);

    // ========== 写入 [ParamsBasic] 段 ==========
    out << "[ParamsBasic]\n";
    out << "Nx=" << darkSectioning->paramsBasicSet.Nx << "\n";
    out << "Ny=" << darkSectioning->paramsBasicSet.Ny << "\n";
    out << "NA=" << darkSectioning->paramsBasicSet.NA << "\n";
    out << "emwavelength=" << darkSectioning->paramsBasicSet.emwavelength << "\n";
    out << "pixelsize=" << darkSectioning->paramsBasicSet.pixelsize << "\n";
    out << "factor=" << darkSectioning->paramsBasicSet.factor << "\n";
    out << "background=" << darkSectioning->paramsBasicSet.background << "\n";
    out << "pad=" << darkSectioning->paramsBasicSet.pad << "\n";
    out << "denoise=" << darkSectioning->paramsBasicSet.denoise << "\n";

    out << "\n";  // 两个段之间用空行分隔，方便人类阅读

    // ========== 写入 [ParamsExpert] 段 ==========
    out << "[ParamsExpert]\n";
    out << "thres=" << darkSectioning->paramsExpertSet.thres << "\n";
    out << "divide=" << darkSectioning->paramsExpertSet.divide << "\n";
    out << "padsize=" << darkSectioning->paramsExpertSet.padsize << "\n";
    // deg/dep/hl是std::string类型，需要用QString::fromStdString()转换
    out << "deg=" << QString::fromStdString(darkSectioning->paramsExpertSet.deg) << "\n";
    out << "dep=" << QString::fromStdString(darkSectioning->paramsExpertSet.dep) << "\n";
    out << "hl=" << QString::fromStdString(darkSectioning->paramsExpertSet.hl) << "\n";
    //isQuick不导出，GreenWidget无对应setter，导入要额外写一个，而且从需求分析上来说不需要导出，这是用来测试参数的快处理选项，用户想要快处理自己点选择框即可
    // 关闭文件（确保数据写入磁盘）
    file.close();

    // 日志提示导出成功
    ui->textEdit_log->append(QString::fromUtf8("参数已导出到: ") + filePath);
}


// ============================================================================
// 【菜单栏槽函数：导入参数到参数栏】
// 信号源：ui->actionImport_Parameters triggered()信号
// 流程：弹出打开文件对话框 → 用户选择txt → 逐行解析键值对 → 设置GreenWidget控件值
// 功能：从txt文件读取参数值并显示在GreenWidget参数控件上
// 说明：导入的值写入GreenWidget控件显示，不写入darkSectioning，
//       后续用户点击"运行"时参数将通过on_pushButton_run_clicked()自动传入
// ============================================================================
void MainWindow::on_actionImport_Parameters_triggered()
{
    // 弹出打开文件对话框，让用户选择要导入的txt文件
    QString filePath = QFileDialog::getOpenFileName(
        this,                                              // 父窗口
        QString::fromUtf8("导入参数文件"),                   // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),  // 默认打开桌面
        QString::fromUtf8("文本文件 (*.txt)")               // 文件过滤器：仅显示txt
    );

    // 用户取消选择时filePath为空字符串，直接返回
    if (filePath.isEmpty()) {
        return;
    }

    // 打开文件准备读取
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 无法打开文件（如文件不存在、权限问题），弹出错误提示
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("无法打开文件，请检查文件是否存在且可读"));
        return;
    }

    // 创建文本输入流，连接文件进行逐行读取
    QTextStream in(&file);
    QString currentSection;  // 记录当前解析到的段："Basic" 或 "Expert"，空字符串表示尚未遇到段标题

    // 逐行读取文件内容
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();  // 读取一行并去除首尾空白字符

        // 跳过空行（不包含任何内容）
        if (line.isEmpty()) {
            continue;
        }

        // 检测段标题：[ParamsBasic] 或 [ParamsExpert]
        if (line == "[ParamsBasic]") {
            currentSection = "Basic";  // 切换到基本参数字段
            continue;  // 段标题本身不需要解析键值
        }
        if (line == "[ParamsExpert]") {
            currentSection = "Expert";  // 切换到高级参数字段
            continue;  // 段标题本身不需要解析键值
        }

        // 解析键=值格式
        int eqPos = line.indexOf('=');  // 查找等号位置
        if (eqPos <= 0) {
            // 等号不存在或等号在行首（key为空），跳过该行
            continue;
        }

        // 提取键和值（等号左边为键，右边为值）
        QString key = line.left(eqPos).trimmed();        // 等号左边的部分是键名
        QString value = line.mid(eqPos + 1).trimmed();   // 等号右边的部分是值

        // 根据当前段和键名，将值设置到GreenWidget对应的参数控件上
        if (currentSection == "Basic") {
            // GreenWidget没有Nx/Ny的控件，这两个值跳过不设置
            if (key == "NA") {
                ui->widget_greenPlaceholder->setNA(value.toDouble());
            } else if (key == "emwavelength") {
                ui->widget_greenPlaceholder->setEmwavelength(value.toDouble());
            } else if (key == "pixelsize") {
                ui->widget_greenPlaceholder->setPixelsize(value.toDouble());
            } else if (key == "factor") {
                ui->widget_greenPlaceholder->setFactor(value.toInt());
            } else if (key == "background") {
                ui->widget_greenPlaceholder->setBackground(value.toInt());
            } else if (key == "pad") {
                ui->widget_greenPlaceholder->setPad(value.toInt());
            } else if (key == "denoise") {
                ui->widget_greenPlaceholder->setDenoise(value.toInt());
            }
            // Nx、Ny：GreenWidget无对应控件，跳过不处理
        }
        else if (currentSection == "Expert") {
            if (key == "thres") {
                ui->widget_greenPlaceholder->setThres(value.toInt());
            } else if (key == "divide") {
                ui->widget_greenPlaceholder->setDivide(value.toDouble());
            } else if (key == "padsize") {
                ui->widget_greenPlaceholder->setPadsize(value.toInt());
            } else if (key == "deg") {
                ui->widget_greenPlaceholder->setDeg(value);
            } else if (key == "dep") {
                ui->widget_greenPlaceholder->setDep(value);
            } else if (key == "hl") {
                ui->widget_greenPlaceholder->setHl(value);
            }
            // isQuick：GreenWidget无对应setter（对应SingleFrameRunning复选框），跳过不处理
        }
    }

    // 关闭文件（释放文件句柄）
    file.close();

    // 日志提示导入成功
    ui->textEdit_log->append(QString::fromUtf8("参数已从文件导入并显示在参数控件上: ") + filePath);
}


// ============================================================================
// 【菜单栏槽函数：批量处理】
// 信号源：ui->actionBatch_Process triggered()信号
// 流程：弹出 BatchWidget 模态对话框 → 用户选择目录和参数 → 点击运行 → 批量处理
// 功能：批量处理多张图片，参数从txt文件直接注入（独立于主窗口UI）
// 说明：批量处理完全独立，不使用主窗口的 DarkSectioning，也不经过 GreenWidget 参数栏
// ============================================================================
void MainWindow::on_actionBatch_Process_triggered()
{
    // 创建批量处理对话框（模态窗口，处理期间阻塞主窗口交互）
    BatchDialog dlg(this);
    dlg.exec();  // exec() = 模态显示，用户关闭对话框后才返回
}


// ============================================================================
// 【菜单栏槽函数：关于此软件】
// 信号源：ui->actionAbout triggered()信号
// 流程：弹出 AboutDialog 模态对话框 → 显示软件版本/作者声明/仓库链接
// 功能：显示关于对话框，包含项目介绍、开源声明和可点击的仓库链接
// ============================================================================
void MainWindow::on_actionAbout_triggered()
{
    // 创建关于对话框（模态窗口，关闭后才能继续操作主窗口）
    AboutDialog dlg(this);
    dlg.exec();  // exec() = 模态显示，用户关闭对话框后才返回
}


// ============================================================================
// 【菜单栏槽函数：开始使用】
// 信号源：ui->actionHowToUse triggered()信号
// 流程：调用系统默认浏览器打开 help.html，URL 带锚点自动跳转到"开始使用"章节
// 功能：在浏览器中打开帮助文档并定位到"开始使用"章节
// ============================================================================
void MainWindow::on_actionHowToUse_triggered()
{
    // 使用 applicationDirPath()：从 exe 所在目录查找 help 文件夹
    // 开发时需将 help 文件夹复制到 build 输出目录；打包时 help 文件夹与 exe 放在一起
    QString helpHtmlPath = QCoreApplication::applicationDirPath() + "/help/help.html";
    QString urlStr = QUrl::fromLocalFile(helpHtmlPath).toString() + "#quickstart";
    QDesktopServices::openUrl(QUrl(urlStr));
}

// ============================================================================
// 【菜单栏槽函数：参数说明】（已移除对应菜单项，保留代码以防后续恢复）
// 信号源：ui->actionExplainParams triggered()信号
// 流程：调用系统默认浏览器打开 help.html，URL 带锚点自动跳转到"参数说明"章节
// 功能：在浏览器中打开帮助文档并定位到"参数说明"章节
// ============================================================================
//void MainWindow::on_actionExplainParams_triggered()
//{
//    QString helpHtmlPath = QCoreApplication::applicationDirPath() + "/help/help.html";
//    QString urlStr = QUrl::fromLocalFile(helpHtmlPath).toString() + "#paramsexplain";
//    QDesktopServices::openUrl(QUrl(urlStr));
//}

// ============================================================================
// 【菜单栏槽函数：参数说明】（已移除对应菜单项，保留代码以防后续恢复）
// 信号源：ui->actionUseBatch triggered()信号
// 流程：调用系统默认浏览器打开 help.html，URL 带锚点自动跳转到"批量处理"章节
// 功能：在浏览器中打开帮助文档并定位到"批量处理"章节
// ============================================================================
//void MainWindow::on_actionUseBatch_triggered()
//{
//    QString helpHtmlPath = QCoreApplication::applicationDirPath() + "/help/help.html";
//    QString urlStr = QUrl::fromLocalFile(helpHtmlPath).toString() + "#batchuse";
//    QDesktopServices::openUrl(QUrl(urlStr));
//}

// ============================================================================
// 【菜单栏槽函数：打开帮助文档】
// 信号源：ui->actionHelp_md triggered()信号
// 流程：资源管理器打开 help 文件夹，取消系统默认编辑器打开 help.md动作
// 功能：显示帮助文件位置
// ============================================================================
void MainWindow::on_actionHelp_md_triggered()
{
    // 打开资源管理器，定位到 help 文件夹（让用户知道文件在哪）
    // 使用 applicationDirPath()：从 exe 所在目录查找 help 文件夹
    QString helpDir = QCoreApplication::applicationDirPath() + "/help";
    QDesktopServices::openUrl(QUrl::fromLocalFile(helpDir));

    // 已注释：不再自动打开 help.md
    //QString helpMdPath = QCoreApplication::applicationDirPath() + "/help/help.md";
    //if (QFile::exists(helpMdPath)) {
    //    QDesktopServices::openUrl(QUrl::fromLocalFile(helpMdPath));
    //}
}


// ============================================================================
// 【菜单栏槽函数：当前帧另存为】
// 信号源：ui->actionSave_ImageFrame triggered()信号
// 流程：安全检查final_images非空 → 获取当前帧索引 → 弹出保存对话框 → 根据格式写出图像
// 功能：将当前显示的处理后图片帧另存为用户指定格式（tif/jpg/png）
// 说明：直接从darkSectioning->final_images[帧索引]获取cv::Mat数据，
//       使用cv::imwrite写出，无需darkSectioning参与任何操作
//       final_images是CV_16U精度（16位），tif/png支持16位，jpg需降为8位
// ============================================================================
void MainWindow::on_actionSave_ImageFrame_triggered()
{
    // ========== 第1步：安全检查 - final_images是否为空 ==========
    // 如果用户尚未运行处理，final_images为空，无法另存
    if (darkSectioning->final_images.empty()) {
        QMessageBox::information(this,
            QString::fromUtf8("提示"),
            QString::fromUtf8("请先运行处理，获得处理后图像栈"));
        return;
    }

    // ========== 第2步：获取当前帧索引 ==========
    // currentProcessedFrame从0开始，与final_images数组索引一致，无需偏移
    int frameIndex = ui->widget_orangePlaceholder->getCurrentProcessedFrame();

    // 边界安全检查：确保帧索引在final_images的有效范围内
    if (frameIndex < 0 || frameIndex >= static_cast<int>(darkSectioning->final_images.size())) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("帧索引超出范围，请检查图像栈状态"));
        return;
    }

    // ========== 第3步：弹出保存对话框 ==========
    // 让用户选择保存路径和文件格式
    // 过滤器提供三种格式：TIFF（16位无损）、PNG（16位无损）、JPEG（8位有损）
    QString savePath = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("另存当前帧图像"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        QString::fromUtf8("TIFF图像 (*.tif);;PNG图像 (*.png);;JPEG图像 (*.jpg)")
    );

    // 用户取消选择时savePath为空字符串，直接返回
    if (savePath.isEmpty()) {
        return;
    }

    // ========== 第4步：根据格式处理数据并写出 ==========
    // 从final_images中取出当前帧的cv::Mat数据（CV_16U精度）
    cv::Mat frameToSave = darkSectioning->final_images[frameIndex];

    // 检查Mat数据是否有效（非空）
    if (frameToSave.empty()) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("当前帧图像数据为空，无法保存"));
        return;
    }

    // 判断用户选择的格式：根据文件扩展名决定是否需要转换
    // toLower()统一转小写，避免.TIF/.Tif等大小写差异导致判断失败
    QString ext = QFileInfo(savePath).suffix().toLower();

    if (ext == "jpg" || ext == "jpeg") {
        // JPEG格式不支持16位深度，需要转换为8位
        // 转换方式：将CV_16U（0~65535）线性映射到CV_8U（0~255）
        // 除以256.0实现近似映射：65535/256.0 ≈ 255.99，截断后为255
        cv::Mat frame8u;
        frameToSave.convertTo(frame8u, CV_8U, 1.0 / 256.0);

        // 调用cv::imwrite写出8位JPEG图像
        // imwrite根据文件扩展名自动选择JPEG编码器
        bool success = cv::imwrite(savePath.toStdString(), frame8u);

        if (success) {
            ui->textEdit_log->append(
                QString::fromUtf8("已另存第 %1 帧为JPEG格式（8位精度）: %2")
                    .arg(frameIndex + 1).arg(savePath));
        } else {
            QMessageBox::warning(this,
                QString::fromUtf8("错误"),
                QString::fromUtf8("保存JPEG图像失败，请检查输出路径是否含中文字符"));
        }
    } else {
        // TIFF或PNG格式：直接写出16位原始精度数据，无需转换
        // imwrite根据文件扩展名自动选择TIFF或PNG编码器
        bool success = cv::imwrite(savePath.toStdString(), frameToSave);

        if (success) {
            // 根据扩展名显示不同的精度提示
            if (ext == "tif" || ext == "tiff") {
                ui->textEdit_log->append(
                    QString::fromUtf8("已另存第 %1 帧为TIFF格式（16位精度）: %2")
                        .arg(frameIndex + 1).arg(savePath));
            } else if (ext == "png") {
                ui->textEdit_log->append(
                    QString::fromUtf8("已另存第 %1 帧为PNG格式（16位精度）: %2")
                        .arg(frameIndex + 1).arg(savePath));
            } else {
                ui->textEdit_log->append(
                    QString::fromUtf8("已另存第 %1 帧: %2")
                        .arg(frameIndex + 1).arg(savePath));
            }
        } else {
            QMessageBox::warning(this,
                QString::fromUtf8("错误"),
                QString::fromUtf8("保存图像失败，请检查输出路径是否含中文字符"));
        }
    }
}


// ============================================================================
// 【菜单栏槽函数：图片另存为】
// 信号源：ui->actionSave_Image triggered()信号
// 流程：安全检查final_images非空 → 弹出保存对话框 → 多帧用imwritemulti存tif/单帧可选格式 → 写出图像
// 功能：将整个处理后图像栈另存（多帧存为多页TIFF，单帧可选tif/jpg/png）
// 说明：直接从darkSectioning->final_images获取全部cv::Mat数据，
//       多帧时cv::imwritemulti只支持tif/tiff格式（OpenCV限制）
//       单帧时cv::imwrite支持tif/png/jpg等多种格式
//       final_images是CV_16U精度（16位），tif/png支持16位，jpg需降为8位
// ============================================================================
void MainWindow::on_actionSave_Image_triggered()
{
    // ========== 第1步：安全检查 - final_images是否为空 ==========
    // 如果用户尚未运行处理，final_images为空，无法另存
    if (darkSectioning->final_images.empty()) {
        QMessageBox::information(this,
            QString::fromUtf8("提示"),
            QString::fromUtf8("请先运行处理，获得处理后图像栈"));
        return;
    }

    // 获取总帧数，用于判断是多帧还是单帧
    int totalFrames = static_cast<int>(darkSectioning->final_images.size());

    // ========== 第2步：弹出保存对话框 ==========
    // 多帧图像只能存为TIFF格式（cv::imwritemulti的限制）
    // 单帧图像可以选择TIFF/PNG/JPEG等多种格式
    QString savePath;

    if (totalFrames > 1) {
        // 多帧图像：过滤器仅提供TIFF格式（imwritemulti只支持多页TIFF）
        savePath = QFileDialog::getSaveFileName(
            this,
            QString::fromUtf8("另存全部帧图像（%1帧，仅支持TIFF格式）").arg(totalFrames),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
            QString::fromUtf8("TIFF图像 (*.tif)")
        );
    } else {
        // 单帧图像：提供多种格式选择
        savePath = QFileDialog::getSaveFileName(
            this,
            QString::fromUtf8("另存图像"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
            QString::fromUtf8("TIFF图像 (*.tif);;PNG图像 (*.png);;JPEG图像 (*.jpg)")
        );
    }

    // 用户取消选择时savePath为空字符串，直接返回
    if (savePath.isEmpty()) {
        return;
    }

    // ========== 第3步：根据帧数和格式写出图像 ==========

    if (totalFrames > 1) {
        // ---------- 多帧模式：使用cv::imwritemulti保存为多页TIFF ----------
        // imwritemulti将所有帧写入同一个TIFF文件，每帧作为独立页面
        // 多页TIFF只支持tif/tiff格式，这是OpenCV的限制
        bool success = cv::imwritemulti(savePath.toStdString(), darkSectioning->final_images);

        if (success) {
            ui->textEdit_log->append(
                QString::fromUtf8("已另存全部 %1 帧为多页TIFF格式（16位精度）: %2")
                    .arg(totalFrames).arg(savePath));
        } else {
            QMessageBox::warning(this,
                QString::fromUtf8("错误"),
                QString::fromUtf8("保存多页TIFF图像失败，请检查输出路径是否含中文字符"));
        }
    } else {
        // ---------- 单帧模式：使用cv::imwrite保存，支持多种格式 ----------
        cv::Mat frameToSave = darkSectioning->final_images[0];

        // 检查Mat数据是否有效（非空）
        if (frameToSave.empty()) {
            QMessageBox::warning(this,
                QString::fromUtf8("错误"),
                QString::fromUtf8("图像数据为空，无法保存"));
            return;
        }

        // 判断用户选择的格式：根据文件扩展名决定是否需要转换
        // toLower()统一转小写，避免.TIF/.Tif等大小写差异导致判断失败
        QString ext = QFileInfo(savePath).suffix().toLower();

        if (ext == "jpg" || ext == "jpeg") {
            // JPEG格式不支持16位深度，需要转换为8位
            // 转换方式：将CV_16U（0~65535）线性映射到CV_8U（0~255）
            // 除以256.0实现近似映射：65535/256.0 ≈ 255.99，截断后为255
            cv::Mat frame8u;
            frameToSave.convertTo(frame8u, CV_8U, 1.0 / 256.0);

            // 调用cv::imwrite写出8位JPEG图像
            bool success = cv::imwrite(savePath.toStdString(), frame8u);

            if (success) {
                ui->textEdit_log->append(
                    QString::fromUtf8("已另存为JPEG格式（8位精度）: %1").arg(savePath));
            } else {
                QMessageBox::warning(this,
                    QString::fromUtf8("错误"),
                    QString::fromUtf8("保存JPEG图像失败，请检查输出路径是否含中文字符"));
            }
        } else {
            // TIFF或PNG格式：直接写出16位原始精度数据，无需转换
            bool success = cv::imwrite(savePath.toStdString(), frameToSave);

            if (success) {
                // 根据扩展名显示不同的精度提示
                if (ext == "tif" || ext == "tiff") {
                    ui->textEdit_log->append(
                        QString::fromUtf8("已另存为TIFF格式（16位精度）: %1").arg(savePath));
                } else if (ext == "png") {
                    ui->textEdit_log->append(
                        QString::fromUtf8("已另存为PNG格式（16位精度）: %1").arg(savePath));
                } else {
                    ui->textEdit_log->append(
                        QString::fromUtf8("已另存图像: %1").arg(savePath));
                }
            } else {
                QMessageBox::warning(this,
                    QString::fromUtf8("错误"),
                    QString::fromUtf8("保存图像失败，请检查输出路径是否含中文字符"));
            }
        }
    }
}



// 功能：显示程序运行状态、错误信息、处理进度等文本日志
// 包含组件：textEdit_log（Qt原生QTextEdit）
// 说明：使用Qt原生控件，保持文本显示的最佳性能
// ============================================================================
    // 当前版本：紫区通过其他区域的槽函数被动更新，无自定义信号槽


// ============================================================================
// 【恢复默认参数】菜单槽函数实现
// 信号源：ui->actionResetParams triggered()信号（位于"设置"菜单下）
// 流程：读取 paramsBasic.h 和 paramsExpert.h 结构体默认值 → 通过 GreenWidget setter 填入参数控件
// 功能：一键将 GreenWidget 参数栏所有控件恢复为头文件中定义的默认值
// 说明：高级参数（thres/divide/padsize/deg/dep/hl/isQuick）也一并恢复，
//       构造函数中启动时高级参数控件为空，但此功能会填入默认值
// ============================================================================
void MainWindow::on_actionResetParams_triggered()
{
    // ========== 第1步：创建 ParamsBasic 默认结构体 ==========
    // ParamsBasic 的默认值在 paramsBasic.h 的结构体定义中：
    //   background=0, pad=1, denoise=0, NA=1.49, emwavelength=610,
    //   pixelsize=65, factor=2, Nx=0, Ny=0
    // Nx和Ny没有对应的控件，会跳过
    ParamsBasic defaultsBasic;

    // ========== 第2步：将基本参数默认值填入 GreenWidget 控件 ==========
    // 调用 GreenWidget 的 setter 方法，这些方法内部会更新对应的 UI 控件
    ui->widget_greenPlaceholder->setBackground(defaultsBasic.background);      // 离焦严重toggle：恢复为"不严重"(0)
    ui->widget_greenPlaceholder->setPad(defaultsBasic.pad);                    // 填充方式toggle：恢复为"对称填充"(1)
    ui->widget_greenPlaceholder->setDenoise(defaultsBasic.denoise);            // 去噪方式单选按钮：恢复为"不去噪"(0)
    ui->widget_greenPlaceholder->setNA(defaultsBasic.NA);                      // 数值孔径：恢复为1.49
    ui->widget_greenPlaceholder->setEmwavelength(defaultsBasic.emwavelength);  // 发射波长：恢复为610nm
    ui->widget_greenPlaceholder->setPixelsize(defaultsBasic.pixelsize);        // 像素尺寸：恢复为65nm
    ui->widget_greenPlaceholder->setFactor(defaultsBasic.factor);              // 分辨率比例因子：恢复为2

    // ========== 第3步：创建 ParamsExpert 默认结构体 ==========
    // ParamsExpert 的默认值在 paramsExpert.h 的结构体定义中：
    //   thres=70, divide=0.5, padsize=15, deg="6,3", dep="3,3", hl="1,1", isQuick=0
    ParamsExpert defaultsExpert;

    // ========== 第4步：将高级参数默认值填入 GreenWidget 第二页控件 ==========
    // 高级参数在构造函数中启动时为空，此功能会填入默认值供用户参考和修改
    ui->widget_greenPlaceholder->setThres(defaultsExpert.thres);                  // 阈值thres：恢复为70
    ui->widget_greenPlaceholder->setDivide(defaultsExpert.divide);                // divide：恢复为0.5
    ui->widget_greenPlaceholder->setPadsize(defaultsExpert.padsize);              // padsize：恢复为15
    // deg/dep/hl 是 std::string 类型，需要转为 QString
    ui->widget_greenPlaceholder->setDeg(QString::fromStdString(defaultsExpert.deg));   // deg：恢复为"6,3"
    ui->widget_greenPlaceholder->setDep(QString::fromStdString(defaultsExpert.dep));   // dep：恢复为"3,3"
    ui->widget_greenPlaceholder->setHl(QString::fromStdString(defaultsExpert.hl));     // hl：恢复为"1,1"

    // ========== 第5步：重置 SingleFrameRunning 复选框 ==========
    // isQuick 默认值为 0（多帧处理模式），通过 setIsQuick 复位复选框
    // setIsQuick(0) 内部调用 setChecked(false)，会触发 toggled 信号，
    // 在 greenwidget.cpp 的 lambda 中自动将文字颜色恢复为黑色、字体恢复为正常
    ui->widget_greenPlaceholder->setIsQuick(defaultsExpert.isQuick);  // 单帧处理：恢复为多帧模式(0)

    // ========== 第6步：写入日志提示 ==========
    ui->textEdit_log->append(QString::fromUtf8("参数已恢复为默认值"));
}


// ============================================================================
// 【清空输入】菜单槽函数实现
// 信号源：ui->actionReset triggered()信号（位于"设置"菜单下）
// 流程：清空输入/输出路径 → 清空绿区参数栏 → 委托橙区清空全部
// 功能：一键清空所有输入内容和显示，恢复到程序刚启动时的空白状态
// 说明：与"恢复默认参数"不同，此功能是清空（抹掉内容），不是填入默认值
// ============================================================================
void MainWindow::on_actionReset_triggered()
{
    // ========== 第1步：清空蓝区输入/输出路径 ==========
    ui->lineEdit_inputPath->clear();                // 清空输入文件路径
    ui->lineEdit_outputPath->clear();               // 清空输出目录路径

    // ========== 第2步：清空绿区参数栏所有输入字段 ==========
    // GreenWidget::clearInputFields() 内部会：
    //   1. 清空 Line_para_NA/emwave/factor/pixelsize 四个AutoComplete输入框文本
    //   2. 重置 isSevere_para_background 和 padMethodSelect_para_pad 为unchecked
    //   3. 重置三个去噪radioButton为全部未选中
    //   4. 清空 lineEdit~lineEdit_6 六个QtMaterialTextField文本（保留浮动标签）
    //   5. 复位 SingleFrameRunning 为未选中（等同于之前的 setIsQuick(0)）
    ui->widget_greenPlaceholder->clearInputFields();

    // ========== 第3步：委托橙区清空全部显示内容 ==========
    // OrangeWidget::clearAll() 内部会：
    //   1. 清空 m_inputFilePath 和 m_outputFilePath
    //   2. 清空 label_originalImage 和 label_processedImage 的图片
    //   3. 重置帧索引为0
    //   4. 禁用同步帧按钮 m_syncButton
    //   5. 委托 OrangeBar::clearLineEdits() 清空帧号输入框
    ui->widget_orangePlaceholder->clearAll();

    // ========== 第4步：写入日志提示 ==========
    ui->textEdit_log->append(QString::fromUtf8("输入已清空"));
}


// ============================================================================
// 【橙区】已通过方法B（容器提升法）在.ui中将widget_orangePlaceholder提升为OrangeWidget
// ui->widget_orangePlaceholder 的类型已是 OrangeWidget*，由uic在setupUi时自动创建
// 布局属性（stretch、sizePolicy等）全部在.ui中设置，由uic保留，无需代码重复设置
// ============================================================================




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
        ui->widget_orangePlaceholder->updateImageDisplay();
    });
}


// ============================================================================

// ============================================================================

    // 设置主题主色调为#55aaff（RGB: 85, 170, 255）
//    QColor themeColor(85, 170, 255);

    
//    // 设置窗口整体样式表（全局UI风格）
//    this->setStyleSheet(
//        "QMainWindow { background-color: #fafafa; }"
//        "QGroupBox { font-weight: bold; color: #55aaff; border: 2px solid #55aaff; "
//        "border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
//        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
//        "QLabel { color: #333333; }"
//        "QMenuBar { background-color: #ffffff; color: #55aaff; border-bottom: 2px solid #55aaff; }"
//        "QMenuBar::item:selected { background-color: #e6f2ff; }"
//        "QMenu { background-color: #ffffff; border: 1px solid #cccccc; }"
//        "QMenu::item:selected { background-color: #e6f2ff; color: #55aaff; }"
//    );
