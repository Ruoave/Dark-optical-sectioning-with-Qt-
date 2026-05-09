#ifndef ORANGEWIDGET_H
#define ORANGEWIDGET_H

#include <QWidget>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QLabel>

// Material组件头文件引入（严格使用libs文件夹下的公共接口头文件）
// 仅包含橙区实际使用的Material组件，不包含其他区域组件
#include "qtmaterialflatbutton.h"

// 前向声明：SyncMaterialButton 子类（定义在orangewidget.cpp中）
// 继承自QtMaterialFlatButton，重写paintEvent使enabled状态下Halo光晕持续显示
class SyncMaterialButton;

// 引入OrangeBar自定义控件头文件（用于访问OrangeBar的公共接口）
#include "orangebar.h"

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
    
    // Qt原生事件重载：绘制事件
    // 功能：绘制OrangeWidget的边框（根据选中状态显示不同颜色）
    void paintEvent(QPaintEvent *event) override;
    
    // Qt原生事件重载：鼠标点击事件
    // 功能：处理OrangeWidget内部的点击，实现选中逻辑
    void mousePressEvent(QMouseEvent *event) override;
    
    // Qt原生事件重载：键盘按下事件
    // 功能：处理键盘方向键和字母键，控制帧切换
    void keyPressEvent(QKeyEvent *event) override;
    
    // Qt原生事件重载：滚轮事件
    // 功能：处理鼠标滚轮，控制帧切换
    void wheelEvent(QWheelEvent *event) override;

    // Qt事件过滤器：用于检测父窗口（MainWindow）上的点击事件
    // 功能：当点击OrangeWidget外部区域时，自动取消选中状态
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::OrangeWidget *ui;                        // UI界面指针（来自Qt Designer生成的ui_orangewidget.h）

    // ==================== 橙区Material组件声明 ====================
    
    // 同步帧按钮：控制两侧图片是否同步切帧
    // 使用 SyncMaterialButton 子类（继承自QtMaterialFlatButton），
    // 重写paintEvent使enabled状态下Halo光晕在失去焦点时仍持续显示（disabled时停止）
    SyncMaterialButton *m_syncButton;           // 同步帧扁平按钮（Material风格，带持续Halo）
    
    // 帧滑块已迁入OrangeBar（通过方法B占位容器封装法）
    // 可通过 ui->widget_orangeBarPlaceholder->originalFrame() 等接口访问滑块状态
    
    // ==================== 橙区数据成员 ====================
    
    QString m_inputFilePath;                      // 输入图片文件路径（从蓝区传入）
    QString m_outputFilePath;                     // 输出文件路径（处理后，从算法模块获取）
    int currentOriginalFrame;                     // 当前处理前图片帧索引（0开始）
    int currentProcessedFrame;                    // 当前处理后图片帧索引（0开始）
    // totalOriginalFrames/totalProcessedFrames 已迁入OrangeBar
    // 通过 ui->widget_orangeBarPlaceholder->totalOriginalFrames() 获取
    // isSyncMode 已迁入OrangeBar
    // 通过 ui->widget_orangeBarPlaceholder->isSyncMode() 获取
    
    // ==================== 选中状态数据成员 ====================
    
    bool isOrangeWidgetSelected;                  // OrangeWidget是否被选中
    bool isOriginalLabelSelected;                 // 处理前图片label是否被选中
    bool isProcessedLabelSelected;                // 处理后图片label是否被选中
    const QColor unselected_OrangeWidgetBorderColor = QColor("#9e9e9e");  // 未选中时边框颜色（灰色）
    const QColor selected_OrangeWidgetBorderColor = QColor("#c4dfff"); // OrangeWidget选中时边框颜色
    const QColor labelSelectedColor = QColor("#55aaff");       // label选中时边框颜色

    // ==================== 橙区内部辅助函数 ====================
    
    // 初始化所有Material组件（创建对象、设置属性、配置样式）
    void initOrangeAreaComponents();
    
    // 将Material组件嵌入UI布局（替换原有Qt原生控件）
    void setupOrangeAreaInLayout();
    
    // 绑定所有信号槽连接（箭头按钮、滑块、同步按钮等）
    void connectOrangeAreaSignals();
    
    // 更新OrangeWidget边框样式（根据选中状态）
    void updateOrangeWidgetBorder();
    
    // 更新label边框样式（根据选中状态）
    void updateLabelBorders();
    
    // 设置OrangeWidget选中状态
    void setOrangeWidgetSelected(bool selected);
    
    // 设置处理前图片label选中状态
    void setOriginalLabelSelected(bool selected);
    
    // 设置处理后图片label选中状态
    void setProcessedLabelSelected(bool selected);

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
    
    // 获取OrangeBar指针（供DarkSectioning直接调用setProgress等接口）
    // 返回值：OrangeBar控件指针
    OrangeBar* orangeBar() const;
    
    // 强制刷新图像显示（根据当前帧索引重新读取并显示图片）
    // 在窗口resize或帧切换时自动调用
    void updateImageDisplay();
    
    // 获取当前处理后图片的帧索引（供MainWindow另存图片时使用）
    // 返回值：当前帧索引（从0开始，与final_images数组索引一致）
    int getCurrentProcessedFrame() const { return currentProcessedFrame; }

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
