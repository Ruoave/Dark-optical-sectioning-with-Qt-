#ifndef DARKSECTIONING_CLEANFORBATCH_H
#define DARKSECTIONING_CLEANFORBATCH_H

#include "darkSectioning.h"  // 继承父类 DarkSectioning，复用 paramsBasicSet/paramsExpertSet/imageStack/final_images 等公共成员

class QTextEdit;             // 前向声明，避免在头文件中引入 Qt Widget 依赖

// ============================================================================
// DarkSectioningBatch：批量处理专用子类
// 继承自 DarkSectioning，专为批量处理设计，与主窗口UI完全解耦：
//   1. 构造函数无需 Ui::MainWindow 指针 → 完全独立于主窗口
//   2. 输入/输出路径通过 m_inputPath / m_outputPath 成员变量直接传入
//   3. 日志通过 setLogWidget() 注入的 QTextEdit 控件输出（由 BatchDialog 提供）
//   4. 不使用 OrangeBar 进度条 → 无 UI 事件处理开销
//   5. 固定全帧处理模式 → isQuick 恒为 0（批量无需单帧快速预览）
//   6. 参数通过 initBatchParams() 一次性设置（所有文件共用，不必每次重新读取）
// ============================================================================
class DarkSectioningBatch : public DarkSectioning
{
public:
    // ========== 构造/析构 ==========
    // 批量处理专用构造函数：不需要 Ui::MainWindow 指针
    // 父类构造传递 nullptr → 父类仅保存指针不做任何 UI 操作
    DarkSectioningBatch();
    ~DarkSectioningBatch();

    // ========== 核心函数 ==========

    // 一次性初始化批量处理参数（所有文件共用，不必每次重新读取）
    // 从父类 paramsBasicSet / paramsExpertSet 成员变量读取参数值并缓存到本类成员变量
    // 调用时机：BatchDialog 在注入参数后、循环处理前调用一次
    void initBatchParams();

    // 处理单张图片（覆盖父类的 process()）
    // 算法逻辑与原 DarkSectioning::process() 完全相同，仅以下两点不同：
    //   1. 输入/输出路径从 m_inputPath/m_outputPath 成员变量读取（不依赖 UI 控件）
    //   2. 日志通过 batchLog() 输出到 BatchDialog 的 textEdit_batchLog
    void process();

    // ========== 路径设置 ==========

    // 设置输入图片路径（每次调用 process() 前单独设置）
    void setInputPath(const QString &path)  { m_inputPath  = path; }

    // 设置输出目录路径（所有输出文件写到同一目录）
    void setOutputPath(const QString &path) { m_outputPath = path; }

    // ========== 日志控件注入 ==========

    // 设置日志输出控件（来自 BatchDialog 的 textEdit_batchLog）
    // 参数：log - QTextEdit 控件指针，设为 nullptr 则回退到 std::cout
    void setLogWidget(QTextEdit *log) { m_logWidget = log; }

private:
    // ==================== 路径成员变量 ====================
    QString m_inputPath;   // 当前处理的输入图片完整路径（每张图片单独设置）
    QString m_outputPath;  // 输出目录路径（所有文件共用）

    // ==================== 日志控件指针 ====================
    QTextEdit *m_logWidget = nullptr;  // 日志输出控件（由 BatchDialog 注入）

    // ==================== 批量处理参数（由 initBatchParams() 一次性缓存）====================
    // 这些参数从父类 paramsBasicSet/paramsExpertSet 读取一次后缓存，
    // 避免每次调用 process() 时重复从父类成员读取
    int     m_background = 0;    // 背景类型：0-离焦不严重，1-离焦严重
    int     m_pad        = 1;    // 填充方式：0-零填充，1-对称填充
    int     m_denoise    = 0;    // 去噪方式：0-不去噪，1-高斯平滑，2-中值滤波
    int     m_thres      = 70;   // 划分信息和背景的阈值（荧光信号越强阈值越高）
    double  m_divide     = 0.5;  // 划分高频/低频部分的边界值
    int     m_padSize    = 200;  // 边缘渐变填充大小
    int     m_isQuick    = 0;    // 单帧处理模式：0-多帧处理（批量固定），1-单帧快速

    // ==================== 辅助函数 ====================

    // 统一日志输出函数
    // 若 m_logWidget 不为空 → 输出到 BatchDialog 的 textEdit_batchLog
    // 若 m_logWidget 为空 → 回退到 std::cout（调试/独立运行场景）
    void batchLog(const QString &message);

    // 获取图像尺寸（覆盖/隐藏父类同名私有方法）
    void getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc);
};

#endif // DARKSECTIONING_CLEANFORBATCH_H