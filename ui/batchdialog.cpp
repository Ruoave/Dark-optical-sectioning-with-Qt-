#include "batchdialog.h"
#include "ui_batchdialog.h"               // uic 从 batchdialog.ui 自动生成的头文件
#include "darkSectioning_cleanForBatch.h"  // 批量处理专用 DarkSectioningBatch 类（algorithm/ 在 INCLUDEPATH 中）

#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>

// ============================================================================
// 【构造函数】
// ============================================================================
BatchDialog::BatchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchDialog)               // 创建由 uic 生成的 UI 对象
{
    ui->setupUi(this);                    // 初始化 UI（根据 batchdialog.ui 创建所有控件）
}

// ============================================================================
// 【析构函数】
// ============================================================================
BatchDialog::~BatchDialog()
{
    delete ui;                            // 释放 UI 对象（Qt 父子机制会自动回收子控件）
}

// ============================================================================
// 【槽函数：浏览输入图片目录】
// 信号源：ui->pushButton_batchBrowseInput clicked()信号
// 功能：打开文件夹选择对话框 → 将路径显示到 lineEdit_batchInputDir
// ============================================================================
void BatchDialog::on_pushButton_batchBrowseInput_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8("选择输入图片目录"),                               // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)    // 默认打开桌面
    );
    if (!dirPath.isEmpty()) {
        ui->lineEdit_batchInputDir->setText(dirPath);                       // 将选择的目录路径显示到输入框
        ui->textEdit_batchLog->append(QString::fromUtf8("已选择输入目录: ") + dirPath);
    }
}

// ============================================================================
// 【槽函数：浏览输出图片目录】
// 信号源：ui->pushButton_batchBrowseOutput clicked()信号
// 功能：打开文件夹选择对话框 → 将路径显示到 lineEdit_batchOutputDir
// ============================================================================
void BatchDialog::on_pushButton_batchBrowseOutput_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8("选择输出图片目录"),                               // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)    // 默认打开桌面
    );
    if (!dirPath.isEmpty()) {
        ui->lineEdit_batchOutputDir->setText(dirPath);                      // 将选择的目录路径显示到输入框
        ui->textEdit_batchLog->append(QString::fromUtf8("已选择输出目录: ") + dirPath);
    }
}

// ============================================================================
// 【槽函数：浏览参数txt文件】
// 信号源：ui->pushButton_batchBrowseParam clicked()信号
// 功能：打开文件选择对话框 → 将路径显示到 lineEdit_batchParamsPath
// ============================================================================
void BatchDialog::on_pushButton_batchBrowseParam_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择参数文件"),                                   // 对话框标题
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),   // 默认打开桌面
        QString::fromUtf8("文本文件 (*.txt)")                                // 文件过滤器：仅显示txt
    );
    if (!filePath.isEmpty()) {
        ui->lineEdit_batchParamsPath->setText(filePath);                    // 将选择的文件路径显示到输入框
        ui->textEdit_batchLog->append(QString::fromUtf8("已选择参数文件: ") + filePath);
    }
}

// ============================================================================
// 【槽函数：运行批量处理】
// 信号源：ui->pushButton_batchRun clicked()信号
// 流程：
//   第1步：验证三个路径均非空
//   第2步：从txt参数文件解析参数直接注入 ParamsBasic/ParamsExpert（绕过UI参数栏）
//   第3步：扫描输入目录中所有支持的图片文件
//   第4步：创建 DarkSectioningBatch 实例，注入参数 → 逐文件循环处理
// 功能：批量处理多张图片，参数从txt文件直接读取，全程不依赖主窗口UI
// ============================================================================
void BatchDialog::on_pushButton_batchRun_clicked()
{
    // ========== 第1步：验证所有路径 ==========
    QString inputDir  = ui->lineEdit_batchInputDir->text();     // 输入图片目录路径
    QString outputDir = ui->lineEdit_batchOutputDir->text();    // 输出图片目录路径
    QString paramPath = ui->lineEdit_batchParamsPath->text();   // 参数txt文件路径

    if (inputDir.isEmpty()) {
        ui->textEdit_batchLog->append(QString::fromUtf8("错误: 请先选择输入图片目录"));
        return;
    }
    if (outputDir.isEmpty()) {
        ui->textEdit_batchLog->append(QString::fromUtf8("错误: 请先选择输出图片目录"));
        return;
    }
    if (paramPath.isEmpty()) {
        ui->textEdit_batchLog->append(QString::fromUtf8("错误: 请先选择参数文件路径"));
        return;
    }

    // ========== 第2步：从txt文件解析参数直接注入结构体（绕过GreenWidget UI）==========
    ParamsBasic  paramsBasicSet;   // 结构体自带默认值（NA=1.49, emwavelength=610, pixelsize=65, factor=2, background=0, pad=1, denoise=0）
    ParamsExpert paramsExpertSet;  // 结构体自带默认值（thres=70, divide=0.5, padsize=200, deg="", dep="", hl=""）

    if (!parseParamsTxtToStruct(paramPath, paramsBasicSet, paramsExpertSet)) {
        ui->textEdit_batchLog->append(QString::fromUtf8("错误: 无法打开参数文件，请检查文件是否存在"));
        return;
    }
    ui->textEdit_batchLog->append(QString::fromUtf8("参数解析完成（已自动跳过 Nx/Ny/isQuick）"));

    // ========== 第3步：扫描输入目录中的所有图片文件 ==========
    QDir dir(inputDir);
    QStringList filters;
    filters << "*.tif" << "*.tiff" << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QStringList imageFiles = dir.entryList(filters, QDir::Files, QDir::Name);  // 按文件名排序

    if (imageFiles.isEmpty()) {
        ui->textEdit_batchLog->append(QString::fromUtf8("错误: 输入目录中没有找到支持的图片文件（tif/tiff/png/jpg/jpeg/bmp）"));
        return;
    }

    ui->textEdit_batchLog->append(QString("共找到 %1 个图片文件，开始批量处理...").arg(imageFiles.size()));
    ui->textEdit_batchLog->append("========================================");
    QApplication::processEvents();  // 刷新日志显示（让用户立即看到上述信息）

    // 禁用运行按钮防止重复点击
    ui->pushButton_batchRun->setEnabled(false);

    // ========== 第4步：创建 DarkSectioningBatch 实例 ==========
    // DarkSectioningBatch 构造函数不需要 Ui::MainWindow 指针，完全独立于主窗口
    DarkSectioningBatch batchProcessor;

    // 注入参数：直接写入结构体（从txt解析来的值覆盖默认值）
    batchProcessor.paramsBasicSet  = paramsBasicSet;
    batchProcessor.paramsExpertSet = paramsExpertSet;
    // Nx/Ny 将在 process() 内部从图像尺寸自动获取，不需要预先设置
    // isQuick 默认为0（全帧处理），批量处理不需要单帧模式

    // 设置输出目录（所有输出文件都写到同一目录）
    batchProcessor.setOutputPath(outputDir);

    // ========== 第5步：逐文件循环处理 ==========
    int successCount = 0;  // 成功处理的文件计数
    int failCount    = 0;  // 处理失败的文件计数

    for (int i = 0; i < imageFiles.size(); i++) {
        const QString &fileName = imageFiles[i];
        QString inputFilePath = QDir(inputDir).absoluteFilePath(fileName);  // 拼接完整输入路径
        QFileInfo fi(inputFilePath);

        // 检查中文路径（OpenCV imreadmulti 不支持含中文的路径）
        bool hasChinese = false;
        for (const QChar &ch : inputFilePath) {
            if (ch.unicode() > 127) {  // Unicode码点超出ASCII范围 → 非ASCII字符（如中文）
                hasChinese = true;
                break;
            }
        }
        if (hasChinese) {
            ui->textEdit_batchLog->append(
                QString("[%1/%2] 跳过: %3（路径含中文字符）")
                    .arg(i + 1).arg(imageFiles.size()).arg(fileName));
            failCount++;
            QApplication::processEvents();  // 刷新日志
            continue;
        }

        ui->textEdit_batchLog->append(
            QString("[%1/%2] 处理: %3").arg(i + 1).arg(imageFiles.size()).arg(fileName));
        QApplication::processEvents();  // 刷新日志

        // 设置当前输入图片路径（每次循环更新）
        batchProcessor.setInputPath(inputFilePath);

        // 调用 DarkSectioningBatch::process() 执行算法
        // 输出文件自动命名为 原始文件名_Darked.tif 写入输出目录
        batchProcessor.process();

        // 检查输出文件是否生成（验证处理结果）
        QString expectedOutput = QDir(outputDir).absoluteFilePath(
            fi.baseName() + "_Darked.tif");  // 预期输出文件名：原始文件名_Darked.tif
        if (QFileInfo::exists(expectedOutput)) {
            ui->textEdit_batchLog->append(
                QString("[%1/%2] 完成 → %3")
                    .arg(i + 1).arg(imageFiles.size()).arg(expectedOutput));
            successCount++;
        } else {
            ui->textEdit_batchLog->append(
                QString("[%1/%2] 警告: 输出文件未生成 → %3")
                    .arg(i + 1).arg(imageFiles.size()).arg(expectedOutput));
            failCount++;
        }
        QApplication::processEvents();  // 刷新日志
    }

    // ========== 第6步：输出汇总信息 ==========
    ui->textEdit_batchLog->append("========================================");
    ui->textEdit_batchLog->append(
        QString::fromUtf8("批量处理完成！成功: %1, 失败: %2, 总计: %3")
            .arg(successCount).arg(failCount).arg(imageFiles.size()));

    // 恢复运行按钮
    ui->pushButton_batchRun->setEnabled(true);
}

// ============================================================================
// 【辅助函数：从txt参数文件直接解析到结构体（绕过UI参数栏）】
// 与 mainwindow 中 on_actionImport_Parameters_triggered() 的关键区别：
//   现有导入：txt → GreenWidget.setXXX() → 用户点运行 → GreenWidget.getXXX() → paramsBasicSet
//   本函数：  txt → 直接写入 paramsBasicSet / paramsExpertSet（跳过GreenWidget中转）
//
// 排除字段：
//   Nx/Ny  ：运行时从图像尺寸自动获取 → 跳过不解析
//   isQuick：批量处理固定全帧模式 → 跳过不解析（保持默认值0）
// ============================================================================
bool BatchDialog::parseParamsTxtToStruct(const QString &filePath,
                                          ParamsBasic  &paramsBasicSet,
                                          ParamsExpert &paramsExpertSet)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;  // 文件打开失败（不存在/权限不足等）
    }

    QTextStream in(&file);
    QString currentSection;  // 记录当前解析到的段："Basic" 或 "Expert"

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();  // 读取一行并去除首尾空白

        // 跳过空行
        if (line.isEmpty()) {
            continue;
        }

        // 检测段标题 [ParamsBasic] 或 [ParamsExpert]
        if (line == "[ParamsBasic]") {
            currentSection = "Basic";
            continue;
        }
        if (line == "[ParamsExpert]") {
            currentSection = "Expert";
            continue;
        }

        // 解析 key=value 格式
        int eqPos = line.indexOf('=');  // 查找等号位置
        if (eqPos <= 0) {
            continue;  // 无等号或等号在行首，跳过该行
        }

        QString key   = line.left(eqPos).trimmed();       // 等号左边 = 键名
        QString value = line.mid(eqPos + 1).trimmed();    // 等号右边 = 值

        // ========== [ParamsBasic] 段：直接注入结构体 ==========
        if (currentSection == "Basic") {
            // Nx、Ny 跳过不处理（运行时从图像尺寸自动获取）
            if (key == "NA") {
                paramsBasicSet.NA = value.toDouble();
            } else if (key == "emwavelength") {
                paramsBasicSet.emwavelength = value.toDouble();
            } else if (key == "pixelsize") {
                paramsBasicSet.pixelsize = value.toDouble();
            } else if (key == "factor") {
                paramsBasicSet.factor = value.toInt();
            } else if (key == "background") {
                paramsBasicSet.background = value.toInt();
            } else if (key == "pad") {
                paramsBasicSet.pad = value.toInt();
            } else if (key == "denoise") {
                paramsBasicSet.denoise = value.toInt();
            }
            // Nx, Ny: 跳过（运行时自动计算）
        }
        // ========== [ParamsExpert] 段：直接注入结构体 ==========
        else if (currentSection == "Expert") {
            if (key == "thres") {
                paramsExpertSet.thres = value.toInt();
            } else if (key == "divide") {
                paramsExpertSet.divide = value.toDouble();
            } else if (key == "padsize") {
                paramsExpertSet.padsize = value.toInt();
            } else if (key == "deg") {
                paramsExpertSet.deg = value.toStdString();   // std::string 类型，需转换
            } else if (key == "dep") {
                paramsExpertSet.dep = value.toStdString();   // std::string 类型，需转换
            } else if (key == "hl") {
                paramsExpertSet.hl = value.toStdString();    // std::string 类型，需转换
            }
            // isQuick: 跳过（批量处理固定为0，全帧处理）
        }
    }

    file.close();
    return true;  // 解析成功
}