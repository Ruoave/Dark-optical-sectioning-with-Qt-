#ifndef ORANGEWIDGET_H
#define ORANGEWIDGET_H

#include <QWidget>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QLabel>                            // Qt原生标签控件（用于显示进度数值文本）

// Material组件头文件引入（严格使用libs文件夹下的公共接口头文件）
// 仅包含橙区实际使用的Material组件，不包含其他区域组件
#include "qtmaterialflatbutton.h"
#include "qtmaterialprogress.h"
#include "qtmaterialslider.h"

namespace Ui {
class OrangeWidget;
}

class OrangeWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：初始化橙区控件
    explicit OrangeWidget(QWidget *parent = nullptr);
    
    // 析构函数：释放橙区资源
    ~OrangeWidget();

signals:
    // 自定义信号：日志消息发送给主窗口
    // 信号源：橙区内部产生需要记录的日志信息时发射
    // 参数：message - 日志文本内容
    void logMessage(const QString &message);

private slots:
    // 项目自定义槽函数：左侧上一帧（处理前图片）
    // 信号源：ui->pushButton_prevLeft clicked()信号
    // 流程：检查边界 -> 帧索引减1 -> 更新滑块位置 -> 刷新图像显示
    // 功能：切换到处理前图片的上一帧（如果不在第一帧）
    void onPrevFrameLeft();
    
    // 项目自定义槽函数：左侧下一帧（处理前图片）
    // 信号源：ui->pushButton_nextLeft clicked()信号
    // 流程：检查边界 -> 帧索引加1 -> 更新滑块位置 -> 刷新图像显示
    // 功能：切换到处理前图片的下一帧（如果不在最后一帧）
    void onNextFrameLeft();
    
    // 项目自定义槽函数：右侧上一帧（处理后图片）
    // 信号源：ui->pushButton_prevRight clicked()信号
    // 功能：切换到处理后图片的上一帧（如果不在第一帧）
    void onPrevFrameRight();
    
    // 项目自定义槽函数：右侧下一帧（处理后图片）
    // 信号源：ui->pushButton_nextRight clicked()信号
    // 功能：切换到处理后图片的下一帧（如果不在最后一帧）
    void onNextFrameRight();
    
    // 项目自定义槽函数：同步帧按钮点击
    // 信号源：m_syncButton clicked()信号
    // 功能：开启/关闭前后图片帧同步模式
    void onSyncFramesClicked();

protected:
    // Qt原生事件重载：窗口大小改变时自动调用
    // 信号源：系统resizeEvent
    // 功能：窗口缩放时自动重新计算图片大小并更新显示
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::OrangeWidget *ui;                        // UI界面指针（来自Qt Designer生成的ui_orangewidget.h）

    // ==================== 橙区Material组件声明 ====================
    
    // 同步帧按钮：控制两侧图片是否同步切帧
    QtMaterialFlatButton *m_syncButton;           // 同步帧扁平按钮（Material风格）
    
    // 进度条组件：显示Dark Sectioning处理进度
    QtMaterialProgress *m_progressBar;            // 处理进度条（Material风格）
    
    // 帧滑块组件（2个）：用于快速跳转到指定帧
    QtMaterialSlider *m_sliderOriginal;           // 处理前图片帧滑块（拖动可切换帧）
    QtMaterialSlider *m_sliderProcessed;          // 处理后图片帧滑块（拖动可切换帧）
    
    // 进度数值显示标签：显示当前进度百分比
    QLabel *m_progressValueLabel;                 // 进度数值显示标签（#55aaff颜色，微软雅黑加粗）

    // ==================== 橙区数据成员 ====================
    
    QString m_inputFilePath;                      // 输入图片文件路径（从蓝区传入）
    QString m_outputFilePath;                     // 输出图片文件路径（处理后，从算法模块获取）
    int currentOriginalFrame;                     // 当前处理前图片帧索引（0开始）
    int currentProcessedFrame;                    // 当前处理后图片帧索引（0开始）
    int totalOriginalFrames;                      // 处理前图片总帧数（用于滑块范围设置）
    int totalProcessedFrames;                     // 处理后图片总帧数（用于滑块范围设置）
    bool isSyncMode;                              // 同步模式标志位（true=同步，false=独立）

    // ==================== 橙区内部辅助函数 ====================
    
    // 初始化所有Material组件（创建对象、设置属性、配置样式）
    void initOrangeAreaComponents();
    
    // 将Material组件嵌入UI布局（替换原有Qt原生控件）
    void setupOrangeAreaInLayout();
    
    // 绑定所有信号槽连接（箭头按钮、滑块、同步按钮等）
    void connectOrangeAreaSignals();

    // ==================== 橙区对外公共接口 ====================
public:
    // 设置输入文件路径（由主窗口在用户选择文件后调用）
    // 参数：filePath - 输入图片文件的完整路径
    void setInputFilePath(const QString &filePath);
    
    // 获取输入文件路径（供其他模块查询使用）
    // 返回值：当前设置的输入文件路径字符串
    QString getInputFilePath() const;
    
    // 设置输出文件路径（由主窗口在处理完成后调用）
    // 参数：filePath - 处理后输出图片文件的完整路径
    void setOutputFilePath(const QString &filePath);
    
    // 获取输出文件路径（供其他模块查询使用）
    // 返回值：当前设置的输出文件路径字符串
    QString getOutputFilePath() const;
    
    // 预加载图像预览信息（仅获取帧数，不读取像素数据）
    // 功能：快速获取多帧TIFF文件的帧数信息，用于初始化滑块范围
    // 参数：filePath - 图像文件路径
    void preloadImagePreview(const QString &filePath);
    
    // 开始处理流程（显示进度条、隐藏滑块）
    // 由主窗口在点击"Run"按钮时调用
    void startProcessing();
    
    // 完成处理流程（配置滑块范围、显示滑块、更新进度到100%）
    // 参数：
    //   originalFrames - 处理前图片的总帧数
    //   processedFrames - 处理后图片的总帧数
    void finishProcessing(int originalFrames, int processedFrames);
    
    // 更新处理进度（实时刷新进度条和进度数值）
    // 参数：progress - 当前进度百分比（0-100）
    void updateProgress(int progress);
    
    // 强制刷新图像显示（根据当前帧索引重新读取并显示图片）
    // 在窗口resize或帧切换时自动调用
    void updateImageDisplay();
    
    // 从多帧TIFF文件中读取指定帧（Qt原生方式，无需OpenCV转换）
    // 参数：
    //   filePath - TIFF文件路径
    //   frameIndex - 帧索引（从0开始）
    // 返回值：QImage对象（读取失败返回空QImage）
    QImage readTiffFrame(const QString& filePath, int frameIndex);

private:
    // 内部辅助函数：更新图像显示的核心实现
    // 根据当前帧索引从文件读取对应帧，等比缩放后显示到两个QLabel上
    void doUpdateImageDisplay();
};

#endif // ORANGEWIDGET_H
