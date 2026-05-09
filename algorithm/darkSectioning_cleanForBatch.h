#ifndef DARKSECTIONING_CLEANFORBATCH_H
#define DARKSECTIONING_CLEANFORBATCH_H

#include "darkSectioning.h"  // 继承父类 DarkSectioning，复用 paramsBasicSet/paramsExpertSet/imageStack/final_images 等公共成员
#include <QDir>              // 用于文件夹遍历（processFolder 中列出图片文件）
#include <QFileInfo>         // 用于文件信息提取（processFolder 中获取文件路径/名称）
#include <QFileInfoList>     // 用于 QDir::entryInfoList 返回类型

// ============================================================================
// DarkSectioningBatch：批量处理专用子类
// 继承自 DarkSectioning，专为批量处理设计，与主窗口UI完全解耦：
//   1. 构造函数无需 Ui::MainWindow 指针 → 完全独立于主窗口
//   2. 输入/输出路径通过 m_inputPath / m_outputPath 成员变量直接传入
//   3. process() 使用 std::cout 输出日志 → 不依赖 textEdit_log
//   4. 不使用 OrangeBar 进度条 → 无 UI 事件处理开销
//   5. 固定全帧处理模式 → isQuick 恒为 0（批量无需单帧快速预览）
// ============================================================================
class DarkSectioningBatch : public DarkSectioning
{
public:
    // 批量处理专用构造函数：不需要 Ui::MainWindow 指针
    // 父类构造传递 nullptr → 父类仅保存指针不做任何 UI 操作
    DarkSectioningBatch();
    ~DarkSectioningBatch();

    // 覆盖父类的 process()：批量处理版本（无任何 UI 依赖）
    // 每次调用处理 m_inputPath 指向的单张图片，结果保存到 m_outputPath
    void process();

    // 批量处理入口：遍历 folderPath 下所有支持的图片文件，逐个调用 process()
    // 参数 folderPath - 输入文件夹的绝对路径
    // 功能：列出文件夹下所有图片 → 循环设置 m_inputPath → 调用 process() → 打印汇总统计
    void processFolder(const QString &folderPath);

    // 设置输入图片路径（每次调用 process() 前单独设置）
    void setInputPath(const QString &path)  { m_inputPath  = path; }

    // 设置输出目录路径
    void setOutputPath(const QString &path) { m_outputPath = path; }

private:
    QString m_inputPath;   // 当前处理的输入图片完整路径
    QString m_outputPath;  // 输出目录路径

    // 辅助函数：获取图像尺寸（覆盖/隐藏父类同名私有方法）
    void getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc);
};

#endif // DARKSECTIONING_CLEANFORBATCH_H
