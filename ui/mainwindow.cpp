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
// 【紫区】运行日志栏
// 功能：显示程序运行状态、错误信息、处理进度等文本日志
// 包含组件：textEdit_log（Qt原生QTextEdit）
// 说明：使用Qt原生控件，保持文本显示的最佳性能
// ============================================================================
    // 当前版本：紫区通过其他区域的槽函数被动更新，无自定义信号槽



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
