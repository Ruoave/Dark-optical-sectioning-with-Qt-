#ifndef ORANGEBAR_H
#define ORANGEBAR_H

#include <QWidget>

#include "qtmaterialslider.h"
#include "qtmaterialautocomplete.h"

namespace Ui {
class OrangeBar;
}

// ============================================================================
// OrangeBar类：橙区中间控制条（从OrangeWidget的horizontalLayout_2拆分而来）
// 功能：管理处理前/后图片的帧滑块、进度条、帧号显示等控件
// 包含组件：sliderOriginal、sliderProcessed、progressBar、
//           lineEdit_original、lineEdit_progressed、label_original、label_progressed
// 设计说明：
//   - OrangeBar通过方法B（占位容器封装法）嵌入OrangeWidget
//   - 在orangewidget.ui中，widget_orangeBarPlaceholder被提升为OrangeBar
//   - OrangeBar内部的UI由其自身的.ui文件和.cpp代码管理
//   - 与OrangeWidget的交互通过信号/槽机制实现解耦
// ============================================================================
class OrangeBar : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：初始化OrangeBar控件
    // 参数：parent - 父控件指针（由uic在setupUi时自动传入）
    explicit OrangeBar(QWidget *parent = nullptr);

    // 析构函数：释放OrangeBar资源
    ~OrangeBar();

protected:
    // 重写resizeEvent（修复窗口缩放时slider圆钮位置不更新的bug）
    // 根因分析：
    //   QtMaterialSlider没有重写resizeEvent()，其thumb偏移量(offset)通过updateThumbOffset()计算
    //   updateThumbOffset()仅在sliderChange()中被调用，而sliderChange仅在value/range改变时触发
    //   窗口缩放时slider宽度改变但value和range都不变 → updateThumbOffset()永远不会被调用
    //   导致圆钮停留在旧位置，与轨道不匹配
    // 解决方案：
    //   在OrangeBar的resizeEvent中用QTimer::singleShot(0)延迟执行updateSliderThumbs()
    //   延迟原因：resize时layout尚未完成对子控件(slider)的几何调整，
    //             需要等事件循环回到主事件循环后slider才有正确的新尺寸
    //   updateSliderThumbs()通过临时改变range触发sliderChange(SliderRangeChange)
    //   sliderChange内部无条件调用updateThumbOffset()重新计算thumb位置
    void resizeEvent(QResizeEvent *event) override;

private:
    // 强制更新两个滑块的thumb位置（供resizeEvent调用）
    // 实现原理：
    //   QtMaterialSlider::setRange()在max不变时直接return不触发sliderChange
    //   因此临时将maximum+1再恢复，强制触发SliderRangeChange
    //   SliderRangeChange → sliderChange() → updateThumbOffset() 重新计算offset
    // 使用blockSignals防止发射rangeChanged/valueChanged等信号
    void updateSliderThumbs();

public:
    // ==================== 公共接口函数（供OrangeWidget调用） ====================

    // 初始化滑块（设置初始范围0-0和初始值0）
    // 调用时机：OrangeWidget构造函数中，setupUi之后
    void initSliders();

    // 开始处理流程（隐藏滑块、禁用同步模式）
    // 调用时机：OrangeWidget::startProcessing()中
    // 功能：处理过程中不允许用户切换帧，避免干扰算法运行
    void startProcessing();

    // 完成处理流程（配置滑块范围、显示滑块、重置帧索引）
    // 调用时机：OrangeWidget::finishProcessing()中
    // 参数：
    //   originalFrames - 处理前图片的总帧数
    //   processedFrames - 处理后图片的总帧数
    void finishProcessing(int originalFrames, int processedFrames);

    // 获取当前处理前帧索引（从sliderOriginal读取）
    // 返回值：当前处理前图片的帧索引（0开始）
    int originalFrame() const;

    // 获取当前处理后帧索引（从sliderProcessed读取）
    // 返回值：当前处理后图片的帧索引（0开始）
    int processedFrame() const;

    // 获取处理前总帧数
    // 返回值：处理前图片的总帧数
    int totalOriginalFrames() const;

    // 设置处理前总帧数（供OrangeWidget::preloadImagePreview调用）
    // 参数：frames - 处理前图片的总帧数
    // 说明：仅更新内部数据，不配置滑块范围（滑块范围在finishProcessing中配置）
    void setTotalOriginalFrames(int frames);

    // 获取处理后总帧数
    // 返回值：处理后图片的总帧数
    int totalProcessedFrames() const;

    // 获取同步模式状态
    // 返回值：true=同步模式开启，false=独立模式
    bool isSyncMode() const;

    // 设置同步模式
    // 参数：sync - true开启同步，false关闭同步
    // 调用时机：OrangeWidget的同步帧按钮点击时调用
    // 功能：开启同步时会自动将处理后滑块同步到处理前滑块的值
    void setSyncMode(bool sync);

    // 切换同步模式（取反当前状态）
    // 返回值：切换后的新同步模式状态（true=同步，false=独立）
    // 调用时机：OrangeWidget的同步帧按钮点击时调用
    bool toggleSyncMode();

    // 设置进度条值
    // 参数：value - 进度百分比（0-100）
    // 调用时机：DarkSectioning在各进度更新点直接调用
    void setProgress(int value);

signals:
    // ==================== 信号（OrangeBar发射，OrangeWidget接收） ====================

    // 处理前帧变化信号
    // 发射时机：sliderOriginal值改变时（包括用户拖动和程序设置）
    // 参数：frame - 新的帧索引值
    // OrangeWidget接收后：更新currentOriginalFrame，刷新图像显示
    void originalFrameChanged(int frame);

    // 处理后帧变化信号
    // 发射时机：sliderProcessed值改变时（包括用户拖动和程序设置）
    // 参数：frame - 新的帧索引值
    // OrangeWidget接收后：更新currentProcessedFrame，刷新图像显示
    void processedFrameChanged(int frame);

    // 处理前滑块选中信号
    // 发射时机：sliderOriginal被用户交互操作时（拖动、点击）
    // OrangeWidget接收后：选中OrangeWidget，选中处理前图片label
    void originalSliderSelected();

    // 处理后滑块选中信号
    // 发射时机：sliderProcessed被用户交互操作时（拖动、点击）
    // OrangeWidget接收后：选中OrangeWidget，选中处理后图片label
    void processedSliderSelected();

    // 同步模式变化信号
    // 发射时机：同步模式被开启或关闭时
    // 参数：sync - 新的同步模式状态（true=同步，false=独立）
    // OrangeWidget接收后：更新同步按钮文字、更新label选中状态
    void syncModeChanged(bool sync);

    // 日志消息信号
    // 发射时机：OrangeBar内部产生需要记录的日志信息时
    // 参数：message - 日志文本内容
    // OrangeWidget接收后：转发给主窗口的紫区日志栏显示
    void logMessage(const QString &message);

public slots:
    // ==================== 公共槽函数（供OrangeWidget调用） ====================

    // 上一帧（处理前图片）
    // 调用时机：OrangeWidget的左箭头按钮pushButton_prevLeft点击时
    // 功能：将sliderOriginal值减1（如果不在第一帧）
    // 边界检查：帧索引不能小于0
    void onPrevFrameOriginal();

    // 下一帧（处理前图片）
    // 调用时机：OrangeWidget的右箭头按钮pushButton_nextLeft点击时
    // 功能：将sliderOriginal值加1（如果不在最后一帧）
    // 边界检查：帧索引不能大于等于总帧数
    void onNextFrameOriginal();

    // 上一帧（处理后图片）
    // 调用时机：OrangeWidget的左箭头按钮pushButton_prevRight点击时
    // 功能：将sliderProcessed值减1（如果不在第一帧）
    void onPrevFrameProcessed();

    // 下一帧（处理后图片）
    // 调用时机：OrangeWidget的右箭头按钮pushButton_nextRight点击时
    // 功能：将sliderProcessed值加1（如果不在最后一帧）
    void onNextFrameProcessed();

private slots:
    // ==================== 私有槽函数（内部信号处理） ====================

    // 处理前滑块值变化内部处理
    // 信号源：ui->sliderOriginal的valueChanged信号
    // 功能：处理同步模式下的滑块联动，发射originalFrameChanged和originalSliderSelected信号
    void onSliderOriginalValueChanged(int value);

    // 处理后滑块值变化内部处理
    // 信号源：ui->sliderProcessed的valueChanged信号
    // 功能：处理同步模式下的滑块联动，发射processedFrameChanged和processedSliderSelected信号
    void onSliderProcessedValueChanged(int value);

    // 处理前lineEdit回车/离焦验证槽函数
    // 信号源：ui->lineEdit_original的editingFinished信号
    // 功能：验证输入是否为1~总帧数之间的整数，合法则跳帧，不合法则回退到当前帧显示
    void onLineEditOriginalEditingFinished();

    // 处理后lineEdit回车/离焦验证槽函数
    // 信号源：ui->lineEdit_progressed的editingFinished信号
    // 功能：验证输入是否为1~总帧数之间的整数，合法则跳帧，不合法则回退到当前帧显示
    void onLineEditProgressedEditingFinished();

private:
    Ui::OrangeBar *ui;                          // UI界面指针（来自Qt Designer生成的ui_orangebar.h）

    // ==================== 数据成员 ====================

    int m_totalOriginalFrames;                  // 处理前图片总帧数（用于滑块范围设置和边界检查）
    int m_totalProcessedFrames;                 // 处理后图片总帧数（用于滑块范围设置和边界检查）
    bool m_isSyncMode;                          // 同步模式标志位（true=同步，false=独立）

    // ==================== 私有辅助函数 ====================

    // 绑定OrangeBar内部信号槽连接
    // 功能：连接sliderOriginal和sliderProcessed的valueChanged信号到内部槽函数
    void connectSignals();

    // 同步滑块（在同步模式下，将另一个滑块同步到源滑块的值）
    // 参数：sourceSlider - 触发同步的源滑块指针
    // 功能：当同步模式开启时，修改一个滑块会自动同步另一个滑块
    //       使用blockSignals防止循环触发
    void syncSlider(QtMaterialSlider *sourceSlider);

    // 修复滑块overlay控件的Z-order图层顺序
    // 根因：QtMaterialSlider的thumb和track都是overlay widget（父对象是OrangeBar而非slider自身）
    //       track在thumb之后创建，导致灰色未选中轨道绘制在thumb圆钮之上
    // 解决：将thumb overlay提升到track overlay之上，确保thumb始终可见
    void fixSliderOverlayZOrder();

    // 从slider值同步到lineEdit显示文本（0-based→1-based转换）
    // 参数：slider - 源滑块指针，lineEdit - 目标输入框指针
    // 功能：将slider的0-based值转为1-based文本显示在lineEdit中，并调整宽度
    // 调用时机：slider值变化时、输入无效回退时、finishProcessing初始化时
    void updateLineEditFromSlider(QtMaterialSlider *slider, QtMaterialAutoComplete *lineEdit);

    // 根据lineEdit文本内容自动调整宽度
    // 参数：lineEdit - 需要调整宽度的输入框指针
    // 功能：计算文本像素宽度+内边距，与最小宽度36取较大值，调用setFixedWidth精确控制
    // 调用时机：确认输入合法后（editingFinished验证通过时）
    void adjustLineEditWidth(QtMaterialAutoComplete *lineEdit);

    // 验证lineEdit输入并返回合法的0-based帧索引
    // 参数：lineEdit - 输入框指针，totalFrames - 总帧数（1-based上限）
    // 返回值：合法返回0-based帧索引，不合法（非数字/空）返回-1
    // 规则：<1自动修正为1，>totalFrames自动修正为totalFrames
    int validateLineEditInput(QtMaterialAutoComplete *lineEdit, int totalFrames);

    // 更新label_original和label_progressed显示的总帧数文本（如"/10"）
    // 功能：根据m_totalOriginalFrames和m_totalProcessedFrames更新QLabel文本
    // 调用时机：finishProcessing、setTotalOriginalFrames中
    void updateFrameLabels();
};

#endif // ORANGEBAR_H
