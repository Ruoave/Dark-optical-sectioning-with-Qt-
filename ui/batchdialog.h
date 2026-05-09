#ifndef BATCHDIALOG_H
#define BATCHDIALOG_H

#include <QDialog>                    // 弹出式模态对话框（不是 QWidget）
#include "paramsBasic.h"              // 参数解析目标结构体：NA/emwavelength/pixelsize/factor/background/pad/denoise
#include "paramsExpert.h"             // 参数解析目标结构体：thres/divide/padsize/deg/dep/hl

namespace Ui {
class BatchDialog;                    // 对应 batchdialog.ui 中的 <class>BatchDialog</class>
}

class BatchDialog : public QDialog    // 批量处理对话框（独立模态窗口，与主窗口解耦）
{
    Q_OBJECT

public:
    explicit BatchDialog(QWidget *parent = nullptr);
    ~BatchDialog();

private slots:
    // 槽函数：浏览输入图片目录
    // 信号源：ui->pushButton_batchBrowseInput clicked()信号
    // 功能：打开文件夹选择对话框 → 将路径显示到 lineEdit_batchInputDir
    void on_pushButton_batchBrowseInput_clicked();

    // 槽函数：浏览输出图片目录
    // 信号源：ui->pushButton_batchBrowseOutput clicked()信号
    // 功能：打开文件夹选择对话框 → 将路径显示到 lineEdit_batchOutputDir
    void on_pushButton_batchBrowseOutput_clicked();

    // 槽函数：浏览参数txt文件
    // 信号源：ui->pushButton_batchBrowseParam clicked()信号
    // 功能：打开文件选择对话框 → 将路径显示到 lineEdit_batchParamsPath
    void on_pushButton_batchBrowseParam_clicked();

    // 槽函数：运行批量处理
    // 信号源：ui->pushButton_batchRun clicked()信号
    // 流程：验证路径 → 解析参数txt → 扫描输入目录 → 逐文件循环处理
    // 功能：批量处理多张图片，参数从txt文件直接注入结构体（绕过主窗口UI参数栏）
    void on_pushButton_batchRun_clicked();

private:
    Ui::BatchDialog *ui;              // 由 uic 从 batchdialog.ui 自动生成的 UI 对象

    // ========== 辅助函数：从txt参数文件直接解析到结构体（绕过UI参数栏）==========
    // 功能：读取键值对格式txt → 直接填充 ParamsBasic / ParamsExpert
    // 与 mainwindow 中 on_actionImport_Parameters_triggered() 不同：
    //   本函数不经过 GreenWidget 中转，直接写入结构体
    // 排除字段：Nx/Ny（运行时从图像尺寸自动获取）、isQuick（批量处理固定全帧模式=0）
    bool parseParamsTxtToStruct(const QString &filePath,
                                 ParamsBasic  &paramsBasicSet,
                                 ParamsExpert &paramsExpertSet);
};

#endif // BATCHDIALOG_H