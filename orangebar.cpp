// OrangeBar 橙区中间控制条模块实现
// 功能：管理处理前/后图片的帧滑块、进度条、帧号显示等控件
// 从OrangeWidget的horizontalLayout_2拆分而来
// 包含：sliderOriginal/sliderProcessed滑块控制、同步帧模式、帧切换逻辑、处理流程控制

#include "orangebar.h"
#include "ui_orangebar.h"
#include "qtmaterialslider.h"
#include "qtmaterialautocomplete.h"

// Qt标准库头文件引入
#include <QApplication>
#include <QTimer>
#include <QIntValidator>
#include <QRegularExpressionValidator>


// ============================================================================
// 【构造函数：初始化OrangeBar所有控件】
// ============================================================================
OrangeBar::OrangeBar(QWidget *parent) :
    QWidget(parent),                             // 调用QWidget基类构造函数
    ui(new Ui::OrangeBar),                       // 创建UI对象（由Qt Designer生成）
    m_totalOriginalFrames(0),                    // 处理前总帧数初始化为0（无数据状态）
    m_totalProcessedFrames(0),                   // 处理后总帧数初始化为0（无数据状态）
    m_isSyncMode(false)                          // 同步模式默认关闭（两侧独立切帧）
{
    // 第1步：调用Qt Designer生成的UI设置（加载orangebar.ui中的界面定义）
    ui->setupUi(this);

    // 第2步：初始化滑块组件（设置范围、初始值、颜色）
    initSliders();

    // 第3步：绑定OrangeBar内部信号槽连接（滑块值变化等交互逻辑）
    connectSignals();
}


// ============================================================================
// 【析构函数：释放OrangeBar资源】
// ============================================================================
OrangeBar::~OrangeBar()
{
    // 释放UI对象（自动清理所有子控件）
    delete ui;
}


// ============================================================================
// 【初始化滑块组件】
// 功能：设置两个帧滑块的初始范围和值
// 说明：初始状态下无图像数据，滑块范围设为0-0（无效状态）
//       使用blockSignals防止初始化时发射valueChanged信号
//       避免在OrangeWidget尚未连接信号时就触发槽函数
// ============================================================================
void OrangeBar::initSliders()
{
    // ---------- 初始化处理前图片帧滑块 ----------
    ui->sliderOriginal->blockSignals(true);      // 阻止信号发射（防止初始化时触发槽函数）
    ui->sliderOriginal->setRange(0, 0);          // 初始范围设为0-0（无数据时无效状态）
    ui->sliderOriginal->setValue(0);             // 初始值设为0（第一帧）
    ui->sliderOriginal->blockSignals(false);     // 恢复信号发射

    // ---------- 初始化处理后图片帧滑块 ----------
    ui->sliderProcessed->blockSignals(true);     // 阻止信号发射
    ui->sliderProcessed->setRange(0, 0);         // 初始范围设为0-0（无数据时无效状态）
    ui->sliderProcessed->setValue(0);            // 初始值设为0（第一帧）
    ui->sliderProcessed->blockSignals(false);    // 恢复信号发射

    // ---------- 设置滑块颜色 ----------
    // QtMaterialSlider颜色机制（见qtmaterialslider_internal.cpp的paintEvent）：
    //   - 已滚过轨道(fg)：使用thumbColor()绘制
    //   - 未滚过轨道(bg)：使用trackColor()绘制（由状态机控制fillColor属性）
    //   - 圆钮(thumb)：使用thumbColor()绘制
    // 未滚过轨道保留Material主题默认深灰色，不设置trackColor
    ui->sliderOriginal->setThumbColor(QColor("#5aafff"));    // 圆钮+已滚过轨道设为#5aafff
    ui->sliderProcessed->setThumbColor(QColor("#5aafff"));   // 圆钮+已滚过轨道设为#5aafff

    // ---------- 设置文本输入框颜色 ----------
    // QtMaterialAutoComplete继承自QtMaterialTextField
    // 底部色条颜色机制（见qtmaterialtextfield.cpp的paintEvent）：
    //   - 未选中时的底线：使用inputLineColor()绘制（默认深灰色）
    //   - 选中时的底部高亮条：使用inkColor()绘制（聚焦后progress>0时显示）
    //   - 选中时浮动标签颜色：也使用inkColor()（见状态机setupProperties）
    // 因此setInkColor同时控制选中高亮条和浮动标签的颜色
    // inputLineColor不设置，保留Material主题默认深灰色
    ui->lineEdit_original->setInkColor(QColor("#5aafff"));     // 选中后底部高亮条+浮动标签设为#5aafff
    ui->lineEdit_progressed->setInkColor(QColor("#5aafff"));   // 选中后底部高亮条+浮动标签设为#5aafff

    // ---------- 初始化帧号输入框 ----------
    // 设置只允许输入数字的验证器（QRegularExpressionValidator: 仅允许0-9数字和空字符串）
    // 注意：QIntValidator允许输入负号，不符合"只能输入数字"的需求，因此使用正则验证器
    QRegularExpressionValidator *digitOnlyValidator = new QRegularExpressionValidator(
        QRegularExpression("\\d*"), this);                     // 正则\d*：匹配0个或多个数字
    ui->lineEdit_original->setValidator(digitOnlyValidator);   // 设置处理前输入框的数字验证器
    ui->lineEdit_progressed->setValidator(                     // 设置处理后输入框的数字验证器
        new QRegularExpressionValidator(QRegularExpression("\\d*"), this));

    // 设置输入框居中对齐（帧号居中显示更美观）
    ui->lineEdit_original->setAlignment(Qt::AlignCenter);      // 处理前帧号居中
    ui->lineEdit_progressed->setAlignment(Qt::AlignCenter);    // 处理后帧号居中

    // 设置初始固定宽度为36px（最小宽度，后续根据帧号位数动态调整）
    // 使用setFixedWidth而非setMinimumWidth：精确控制宽度，防止lineEdit挤压其他控件
    ui->lineEdit_original->setFixedWidth(36);                  // 处理前输入框初始宽度36px
    ui->lineEdit_progressed->setFixedWidth(36);                // 处理后输入框初始宽度36px

    // 初始状态下无图像数据，lineEdit显示空文本（setEnabled(false)由startProcessing管理）
    ui->lineEdit_original->setText("");                        // 处理前输入框初始为空
    ui->lineEdit_progressed->setText("");                      // 处理后输入框初始为空

    // 修复滑块overlay控件的Z-order图层顺序
    // 必须在initSliders()中调用（滑块构造时overlay已创建完成）
    // 修复原因：QtMaterialSlider的thumb/track是overlay widget，track后创建导致覆盖thumb
    // 独立 OrangeBar 后子控件减少，Z-order 完全由创建顺序决定，问题暴露
    fixSliderOverlayZOrder();
}


// ============================================================================
// 【绑定OrangeBar内部信号槽连接】
// 功能：连接sliderOriginal和sliderProcessed的valueChanged信号到内部槽函数
// 说明：滑块值变化时，内部槽函数处理同步逻辑，并发射外部信号通知OrangeWidget
// ============================================================================
void OrangeBar::connectSignals()
{
    // ---------- 处理前图片滑块值变化信号槽连接 ----------
    // 信号源：ui->sliderOriginal的valueChanged信号
    // 槽函数：onSliderOriginalValueChanged（处理同步逻辑、发射外部信号）
    connect(ui->sliderOriginal, &QtMaterialSlider::valueChanged,
            this, &OrangeBar::onSliderOriginalValueChanged);

    // ---------- 处理后图片滑块值变化信号槽连接 ----------
    // 信号源：ui->sliderProcessed的valueChanged信号
    // 槽函数：onSliderProcessedValueChanged（处理同步逻辑、发射外部信号）
    connect(ui->sliderProcessed, &QtMaterialSlider::valueChanged,
            this, &OrangeBar::onSliderProcessedValueChanged);

    // ---------- 处理前lineEdit回车/离焦信号槽连接 ----------
    // 信号源：ui->lineEdit_original的editingFinished信号（回车键或失去焦点时发射）
    // 槽函数：onLineEditOriginalEditingFinished（验证输入、合法跳帧/非法回退）
    connect(ui->lineEdit_original, &QtMaterialAutoComplete::editingFinished,
            this, &OrangeBar::onLineEditOriginalEditingFinished);

    // ---------- 处理后lineEdit回车/离焦信号槽连接 ----------
    // 信号源：ui->lineEdit_progressed的editingFinished信号（回车键或失去焦点时发射）
    // 槽函数：onLineEditProgressedEditingFinished（验证输入、合法跳帧/非法回退）
    connect(ui->lineEdit_progressed, &QtMaterialAutoComplete::editingFinished,
            this, &OrangeBar::onLineEditProgressedEditingFinished);
}


// ============================================================================
// 【开始处理流程】
// 功能：禁用两个帧滑块（处理过程中不允许用户切换帧，避免干扰算法运行）
// 调用时机：OrangeWidget::startProcessing()中调用
// 说明：同步帧按钮的禁用由OrangeWidget自行处理（按钮不在OrangeBar内）
// ============================================================================
void OrangeBar::startProcessing()
{
    // 禁用两个帧滑块（禁用后滑块仍可见但不可交互，避免布局跳动）
    ui->sliderOriginal->setEnabled(false);         // 禁用处理前图片滑块
    ui->sliderProcessed->setEnabled(false);        // 禁用处理后图片滑块

    // 禁用两个帧号输入框（处理过程中不允许用户手动输入帧号）
    ui->lineEdit_original->setEnabled(false);      // 禁用处理前帧号输入框
    ui->lineEdit_progressed->setEnabled(false);    // 禁用处理后帧号输入框

    // 强制立即刷新UI界面（确保上述变更立刻显示给用户）
    QApplication::processEvents();
}


// ============================================================================
// 【完成处理流程】
// 功能：配置滑块的有效范围、显示滑块、重置帧索引到第一帧
// 调用时机：OrangeWidget::finishProcessing()中调用
// 参数：
//   originalFrames - 处理前图片的总帧数（从darkSectioning->imageStack.size()获取）
//   processedFrames - 处理后图片的总帧数（从darkSectioning->final_images.size()获取）
// 说明：
//   - 使用blockSignals防止setRange/setValue时触发valueChanged信号
//   - 配置完成后手动发射originalFrameChanged(0)和processedFrameChanged(0)
//   - OrangeWidget接收信号后更新currentOriginalFrame/currentProcessedFrame并刷新图像
// ============================================================================
void OrangeBar::finishProcessing(int originalFrames, int processedFrames)
{
    // 保存总帧数到成员变量（用于边界检查和滑块范围设置）
    m_totalOriginalFrames = originalFrames;
    m_totalProcessedFrames = processedFrames;

    // 阻止两个滑块发射信号（防止setRange/setValue时触发valueChanged，
    // 避免在配置过程中发射中间状态的帧变化信号）
    ui->sliderOriginal->blockSignals(true);
    ui->sliderProcessed->blockSignals(true);

    // ---------- 配置处理前图片滑块 ----------
    if (m_totalOriginalFrames > 0) {
        // 只有当存在有效帧数时才启用滑块（避免除零错误或无效范围）
        ui->sliderOriginal->setRange(0, m_totalOriginalFrames - 1);  // 设置范围：0 到 总帧数-1
        ui->sliderOriginal->setValue(0);          // 重置滑块位置到第一帧（索引0）
        ui->sliderOriginal->setEnabled(true);     // 启用滑块（之前被startProcessing()禁用了）
        ui->lineEdit_original->setEnabled(true);  // 启用处理前帧号输入框
    }

    // ---------- 配置处理后图片滑块 ----------
    if (m_totalProcessedFrames > 0) {
        // 只有当存在有效帧数时才启用滑块
        ui->sliderProcessed->setRange(0, m_totalProcessedFrames - 1);  // 设置范围
        ui->sliderProcessed->setValue(0);         // 重置到第一帧
        ui->sliderProcessed->setEnabled(true);    // 启用滑块（之前被startProcessing()禁用了）
        ui->lineEdit_progressed->setEnabled(true); // 启用处理后帧号输入框
    }

    // 恢复两个滑块的信号发射
    ui->sliderOriginal->blockSignals(false);
    ui->sliderProcessed->blockSignals(false);

    // 更新帧号标签显示（如"/10"）和lineEdit显示（如"1"）
    updateFrameLabels();
    updateLineEditFromSlider(ui->sliderOriginal, ui->lineEdit_original);    // 处理前：slider值0→显示"1"
    updateLineEditFromSlider(ui->sliderProcessed, ui->lineEdit_progressed); // 处理后：slider值0→显示"1"

    // 手动发射帧变化信号，通知OrangeWidget更新帧索引和刷新图像显示
    // 参数0表示重置到第一帧
    emit originalFrameChanged(0);                 // 通知OrangeWidget：处理前帧已重置到0
    emit processedFrameChanged(0);                // 通知OrangeWidget：处理后帧已重置到0
}


// ============================================================================
// 【获取当前处理前帧索引】
// 返回值：当前处理前图片的帧索引（从sliderOriginal读取，0开始）
// ============================================================================
int OrangeBar::originalFrame() const
{
    return ui->sliderOriginal->value();           // 直接从滑块读取当前值
}


// ============================================================================
// 【获取当前处理后帧索引】
// 返回值：当前处理后图片的帧索引（从sliderProcessed读取，0开始）
// ============================================================================
int OrangeBar::processedFrame() const
{
    return ui->sliderProcessed->value();          // 直接从滑块读取当前值
}


// ============================================================================
// 【获取处理前总帧数】
// 返回值：处理前图片的总帧数
// ============================================================================
int OrangeBar::totalOriginalFrames() const
{
    return m_totalOriginalFrames;
}


// ============================================================================
// 【设置处理前总帧数】
// 参数：frames - 处理前图片的总帧数
// 调用时机：OrangeWidget::preloadImagePreview()中调用
// 功能：更新内部数据成员m_totalOriginalFrames，同时配置处理前滑块范围并启用滑块
//       使预加载后用户即可通过箭头按钮/滑块切换处理前图片帧
// 说明：处理后滑块不在此处配置（需等finishProcessing），同步帧按钮由OrangeWidget管理
// ============================================================================
void OrangeBar::setTotalOriginalFrames(int frames)
{
    m_totalOriginalFrames = frames;

    // 配置处理前滑块范围并启用（使预加载后即可切帧）
    if (frames > 0) {
        ui->sliderOriginal->blockSignals(true);    // 阻止信号（防止setRange/setValue触发valueChanged）
        ui->sliderOriginal->setRange(0, frames - 1);  // 设置范围：0 到 总帧数-1
        ui->sliderOriginal->setValue(0);           // 重置到第一帧
        ui->sliderOriginal->blockSignals(false);   // 恢复信号发射
        ui->sliderOriginal->setEnabled(true);      // 启用滑块
        ui->lineEdit_original->setEnabled(true);   // 启用处理前帧号输入框
    }

    // 更新帧号标签显示（如"/10"）和lineEdit显示
    updateFrameLabels();
    updateLineEditFromSlider(ui->sliderOriginal, ui->lineEdit_original);
}


// ============================================================================
// 【获取处理后总帧数】
// 返回值：处理后图片的总帧数
// ============================================================================
int OrangeBar::totalProcessedFrames() const
{
    return m_totalProcessedFrames;
}


// ============================================================================
// 【获取同步模式状态】
// 返回值：true=同步模式开启（前后图片帧联动），false=独立模式
// ============================================================================
bool OrangeBar::isSyncMode() const
{
    return m_isSyncMode;
}


// ============================================================================
// 【设置同步模式】
// 参数：sync - true开启同步模式，false关闭同步模式
// 调用时机：OrangeWidget的同步帧按钮点击时调用
// 功能：
//   开启同步时：将处理后滑块同步到处理前滑块的当前值，发射syncModeChanged信号
//   关闭同步时：仅发射syncModeChanged信号
// 说明：
//   - 开启同步时，processedSlider使用blockSignals防止循环触发
//   - OrangeWidget接收syncModeChanged信号后更新按钮文字、label选中状态、帧索引、图像显示
//   - 如果新状态与当前状态相同，直接返回不做任何操作（防止重复触发）
// ============================================================================
void OrangeBar::setSyncMode(bool sync)
{
    // 如果新状态与当前状态相同，直接返回（防止重复触发）
    if (m_isSyncMode == sync) {
        return;
    }

    // 更新同步模式标志位
    m_isSyncMode = sync;

    if (m_isSyncMode) {
        // ========== 开启同步模式 ==========
        // 立即将处理后滑块同步到处理前滑块的当前值
        // 阻止processedSlider发射valueChanged信号（避免触发onSliderProcessedValueChanged导致循环）
        ui->sliderProcessed->blockSignals(true);
        ui->sliderProcessed->setValue(ui->sliderOriginal->value());  // 同步到处理前滑块的值
        ui->sliderProcessed->blockSignals(false);  // 恢复信号发射

        // 发射日志消息信号（OrangeWidget会转发给主窗口紫区日志栏）
        emit logMessage("同步模式已开启: 前后图片帧数将联动");
    } else {
        // ========== 关闭同步模式 ==========
        // 发射日志消息信号
        emit logMessage("同步模式已关闭: 前后图片帧数独立控制");
    }

    // 发射同步模式变化信号（OrangeWidget接收后更新按钮文字、label选中状态、帧索引、图像显示）
    emit syncModeChanged(m_isSyncMode);
}


// ============================================================================
// 【切换同步模式】
// 返回值：切换后的新同步模式状态（true=同步，false=独立）
// 调用时机：OrangeWidget的同步帧按钮点击时调用
// 功能：取反当前同步模式状态（true变false，false变true）
// ============================================================================
bool OrangeBar::toggleSyncMode()
{
    // 调用setSyncMode取反当前状态
    setSyncMode(!m_isSyncMode);
    // 返回切换后的新状态
    return m_isSyncMode;
}


// ============================================================================
// 【上一帧（处理前图片）】
// 公共槽函数：供OrangeWidget的左箭头按钮pushButton_prevLeft调用
// 功能：将sliderOriginal值减1（如果不在第一帧）
// 边界检查：帧索引不能小于0
// 流程：检查边界 -> 设置滑块值 -> 滑块valueChanged信号触发onSliderOriginalValueChanged
//       -> onSliderOriginalValueChanged内部处理同步逻辑并发射originalFrameChanged信号
// ============================================================================
void OrangeBar::onPrevFrameOriginal()
{
    // 从滑块读取当前帧索引
    int frame = ui->sliderOriginal->value();

    // 边界检查：确保不会超出第一帧（帧索引不能小于0）
    if (frame > 0) {
        // 帧索引减1，设置滑块值（会触发valueChanged信号，进而触发onSliderOriginalValueChanged）
        ui->sliderOriginal->setValue(frame - 1);
    }
    // 如果已经在第一帧（frame == 0），则不做任何操作（静默忽略）
}


// ============================================================================
// 【下一帧（处理前图片）】
// 公共槽函数：供OrangeWidget的右箭头按钮pushButton_nextLeft调用
// 功能：将sliderOriginal值加1（如果不在最后一帧）
// 边界检查：帧索引不能大于等于总帧数
// ============================================================================
void OrangeBar::onNextFrameOriginal()
{
    // 从滑块读取当前帧索引
    int frame = ui->sliderOriginal->value();

    // 边界检查：确保总帧数有效且不会超出最后一帧
    if (m_totalOriginalFrames > 0 && frame < m_totalOriginalFrames - 1) {
        // 帧索引加1，设置滑块值（会触发valueChanged信号）
        ui->sliderOriginal->setValue(frame + 1);
    }
}


// ============================================================================
// 【上一帧（处理后图片）】
// 公共槽函数：供OrangeWidget的左箭头按钮pushButton_prevRight调用
// 功能：将sliderProcessed值减1（如果不在第一帧）
// 边界检查：帧索引不能小于0
// ============================================================================
void OrangeBar::onPrevFrameProcessed()
{
    // 从滑块读取当前帧索引
    int frame = ui->sliderProcessed->value();

    // 边界检查：确保不会超出第一帧
    if (frame > 0) {
        // 帧索引减1，设置滑块值（会触发valueChanged信号）
        ui->sliderProcessed->setValue(frame - 1);
    }
}


// ============================================================================
// 【下一帧（处理后图片）】
// 公共槽函数：供OrangeWidget的右箭头按钮pushButton_nextRight调用
// 功能：将sliderProcessed值加1（如果不在最后一帧）
// 边界检查：帧索引不能大于等于总帧数
// ============================================================================
void OrangeBar::onNextFrameProcessed()
{
    // 从滑块读取当前帧索引
    int frame = ui->sliderProcessed->value();

    // 边界检查：确保总帧数有效且不会超出最后一帧
    if (m_totalProcessedFrames > 0 && frame < m_totalProcessedFrames - 1) {
        // 帧索引加1，设置滑块值（会触发valueChanged信号）
        ui->sliderProcessed->setValue(frame + 1);
    }
}


// ============================================================================
// 【处理前滑块值变化内部处理】
// 私有槽函数：响应ui->sliderOriginal的valueChanged信号
// 参数：value - 滑块的新值（即新的帧索引）
// 功能：
//   1. 发射originalSliderSelected信号（通知OrangeWidget选中处理前图片label）
//   2. 如果处于同步模式，调用syncSlider同步处理后滑块
//   3. 发射originalFrameChanged信号（通知OrangeWidget更新帧索引并刷新图像）
// 说明：
//   - 同步模式下syncSlider会静默设置另一个滑块（blockSignals防止循环触发）
//   - OrangeWidget接收originalFrameChanged后应同时更新currentOriginalFrame
//     如果同步模式还应同步更新currentProcessedFrame（由OrangeWidget自行判断）
// ============================================================================
void OrangeBar::onSliderOriginalValueChanged(int value)
{
    // 第1步：发射滑块选中信号（通知OrangeWidget选中OrangeWidget和处理前图片label）
    emit originalSliderSelected();

    // 第2步：如果处于同步模式，同步处理后滑块到当前值
    if (m_isSyncMode) {
        syncSlider(ui->sliderOriginal);           // 同步processedSlider到originalSlider的值
    }

    // 第3步：更新处理前帧号输入框显示（slider 0-based → lineEdit 1-based）
    updateLineEditFromSlider(ui->sliderOriginal, ui->lineEdit_original);

    // 第4步：发射帧变化信号（通知OrangeWidget更新帧索引并刷新图像显示）
    emit originalFrameChanged(value);
}


// ============================================================================
// 【处理后滑块值变化内部处理】
// 私有槽函数：响应ui->sliderProcessed的valueChanged信号
// 参数：value - 滑块的新值（即新的帧索引）
// 功能：
//   1. 发射processedSliderSelected信号（通知OrangeWidget选中处理后图片label）
//   2. 如果处于同步模式，调用syncSlider同步处理前滑块
//   3. 发射processedFrameChanged信号（通知OrangeWidget更新帧索引并刷新图像）
// ============================================================================
void OrangeBar::onSliderProcessedValueChanged(int value)
{
    // 第1步：发射滑块选中信号（通知OrangeWidget选中OrangeWidget和处理后图片label）
    emit processedSliderSelected();

    // 第2步：如果处于同步模式，同步处理前滑块到当前值
    if (m_isSyncMode) {
        syncSlider(ui->sliderProcessed);          // 同步originalSlider到processedSlider的值
    }

    // 第3步：更新处理后帧号输入框显示（slider 0-based → lineEdit 1-based）
    updateLineEditFromSlider(ui->sliderProcessed, ui->lineEdit_progressed);

    // 第4步：发射帧变化信号（通知OrangeWidget更新帧索引并刷新图像显示）
    emit processedFrameChanged(value);
}


// ============================================================================
// 【同步滑块】
// 私有辅助函数：在同步模式下，将另一个滑块同步到源滑块的值
// 参数：sourceSlider - 触发同步的源滑块指针（sliderOriginal或sliderProcessed）
// 功能：
//   - 在同步模式下，将另一个滑块同步到源滑块的值
//   - 使用blockSignals防止被同步的滑块发射valueChanged信号（避免循环触发导致无限递归）
// 说明：
//   此函数仅在m_isSyncMode为true时被调用（由onSliderXxxValueChanged内部判断）
//   调用前已确认同步模式开启，无需内置检查
// ============================================================================
void OrangeBar::syncSlider(QtMaterialSlider *sourceSlider)
{
    // 获取源滑块的当前值
    int value = sourceSlider->value();

    // 判断源滑块是哪一个，同步另一个滑块
    if (sourceSlider == ui->sliderOriginal) {
        // 源是处理前滑块 -> 同步处理后滑块
        ui->sliderProcessed->blockSignals(true);  // 阻止信号（防止触发onSliderProcessedValueChanged导致循环）
        ui->sliderProcessed->setValue(value);     // 设置处理后滑块为相同值
        ui->sliderProcessed->blockSignals(false); // 恢复信号发射
        // 同步处理后帧号输入框显示（blockSignals阻止了valueChanged，需手动更新lineEdit）
        updateLineEditFromSlider(ui->sliderProcessed, ui->lineEdit_progressed);
    } else if (sourceSlider == ui->sliderProcessed) {
        // 源是处理后滑块 -> 同步处理前滑块
        ui->sliderOriginal->blockSignals(true);   // 阻止信号（防止触发onSliderOriginalValueChanged导致循环）
        ui->sliderOriginal->setValue(value);      // 设置处理前滑块为相同值
        ui->sliderOriginal->blockSignals(false);  // 恢复信号发射
        // 同步处理前帧号输入框显示（blockSignals阻止了valueChanged，需手动更新lineEdit）
        updateLineEditFromSlider(ui->sliderOriginal, ui->lineEdit_original);
    }
}


// ============================================================================
// 【重写resizeEvent：窗口缩放时强制更新slider圆钮位置】
// protected重写：当OrangeBar尺寸改变时（窗口缩放/布局调整），强制更新两个滑块的thumb位置
// 根因分析（旧方案为何无效）：
//   旧方案使用eventFilter拦截slider的Resize事件，然后调用setValue(sameValue)
//   但QAbstractSlider::setValue()在值相同时直接return，不触发sliderChange()
//   因此updateThumbOffset()从未被调用，thumb位置不会更新
// 新方案原理：
//   通过临时修改slider的maximum（+1再恢复），强制触发SliderRangeChange
//   QtMaterialSlider::sliderChange()内部无条件调用updateThumbOffset()
//   updateThumbOffset()根据当前value、min、max和新的width重新计算offset
// 使用QTimer::singleShot(0)延迟执行的原因：
//   resizeEvent触发时，layout正在调整子控件几何，slider的新尺寸尚未确定
//   延迟到当前事件循环处理完毕后，layout已完成调整，slider有正确的新尺寸
//   此时再触发updateThumbOffset()才能基于正确的width计算正确的offset
// ============================================================================
void OrangeBar::resizeEvent(QResizeEvent *event)
{
    // 先调用基类resizeEvent（完成默认的布局调整）
    QWidget::resizeEvent(event);

    // 使用QTimer::singleShot(0)延迟更新thumb位置
    // 延迟到当前事件循环中所有待处理事件完成后执行
    // 此时layout已完成对slider几何的调整，slider->width()已是新尺寸
    QTimer::singleShot(0, this, &OrangeBar::updateSliderThumbs);
}


// ============================================================================
// 【强制更新两个滑块的thumb位置】
// 私有辅助函数：通过临时改变range触发sliderChange → updateThumbOffset()
// 实现原理：
//   QAbstractSlider::setRange(min, max)在min/max与当前相同时直接return，不触发任何change
//   因此需要临时将maximum改为max+1再改回max，制造一次真正的range变化
//   setRange(max+1) 触发 SliderRangeChange
//     → QtMaterialSlider::sliderChange(SliderRangeChange)
//       → updateThumbOffset()  [sliderChange内无条件调用]
//         → 根据 value/(max-min) * width 重新计算offset
//   setRange(max) 恢复原始范围
//     → 再次触发 sliderChange → 再次 updateThumbOffset（此时用正确max计算）
// 使用blockSignals防止发射rangeChanged/valueChanged信号（外部不需要知道这次临时变化）
// 安全性说明：
//   临时设为max+1时，如果value==max，value会被clamp到max+1（不超范围）
//   恢复到max时，如果value曾被clamp到max+1，会被clamp回max
//   最终value不变，且blockSignals阻止了信号发射，对外完全无副作用
// ============================================================================
void OrangeBar::updateSliderThumbs()
{
    // 对两个滑块分别执行range trick
    const QList<QtMaterialSlider *> sliders = { ui->sliderOriginal, ui->sliderProcessed };

    for (QtMaterialSlider *slider : sliders) {
        // 阻止所有信号发射（临时range变化不应通知外部）
        slider->blockSignals(true);

        // 记录原始范围
        int min = slider->minimum();
        int max = slider->maximum();

        // 第一步：临时将maximum+1，触发SliderRangeChange → sliderChange → updateThumbOffset()
        slider->setRange(min, max + 1);

        // 第二步：恢复原始范围，再次触发sliderChange → updateThumbOffset()（用正确max）
        slider->setRange(min, max);

        // 恢复信号发射
        slider->blockSignals(false);
    }
}


// ============================================================================
// 【修复滑块overlay控件的Z-order图层顺序】
// 根因：QtMaterialSlider的thumb和track都是OrangeBar的overlay子控件，
//       track在thumb之后创建，Qt按创建顺序绘制，导致track遮挡thumb。
// 解决：QWidget::raise()将thumb overlay提升到track overlay之上
// ============================================================================
void OrangeBar::fixSliderOverlayZOrder()
{
    const QList<QWidget *> children = this->findChildren<QWidget *>();  // 获取所有子控件
    QMap<QWidget *, QWidget *> sliderThumbMap;  // key:slider指针, value:thumb overlay指针

    for (QWidget *child : children) {
        QString className = child->metaObject()->className();  // 获取类名
        if (className == QLatin1String("QtMaterialSliderThumb")) {
            // 按发现顺序配对：第一个thumb归sliderOriginal，第二个归sliderProcessed
            if (!sliderThumbMap.contains(ui->sliderOriginal)) {
                sliderThumbMap.insert(ui->sliderOriginal, child);
            } else if (!sliderThumbMap.contains(ui->sliderProcessed)) {
                sliderThumbMap.insert(ui->sliderProcessed, child);
            }
        }
    }

    for (auto it = sliderThumbMap.constBegin(); it != sliderThumbMap.constEnd(); ++it) {
        if (it.value()) {
            it.value()->raise();  // 将thumb提升到track之上
        }
    }
}


// ============================================================================
// 【从slider值同步到lineEdit显示文本】
// 私有辅助函数：将slider的0-based值转为1-based文本显示在lineEdit中
// 参数：slider - 源滑块指针，lineEdit - 目标输入框指针
// 功能：
//   1. 将slider的0-based值+1转为1-based帧号文本
//   2. 设置到lineEdit中显示
//   3. 调用adjustLineEditWidth根据文本长度调整输入框宽度
// 调用时机：
//   - slider值变化时（onSliderXxxValueChanged中调用）
//   - 输入无效回退时（onLineEditXxxEditingFinished中调用）
//   - finishProcessing/setTotalOriginalFrames初始化时
//   - syncSlider同步时（blockSignals阻止了valueChanged，需手动调用）
// ============================================================================
void OrangeBar::updateLineEditFromSlider(QtMaterialSlider *slider, QtMaterialAutoComplete *lineEdit)
{
    // slider值是0-based（0=第一帧），lineEdit显示1-based（1=第一帧）
    int frame1Based = slider->value() + 1;                     // 0-based转1-based
    lineEdit->setText(QString::number(frame1Based));           // 设置lineEdit显示文本
    adjustLineEditWidth(lineEdit);                             // 根据文本长度调整宽度
}


// ============================================================================
// 【根据lineEdit文本内容自动调整宽度】
// 私有辅助函数：计算文本像素宽度+内边距，与最小宽度36取较大值
// 参数：lineEdit - 需要调整宽度的输入框指针
// 功能：
//   1. 使用fontMetrics计算当前文本的像素宽度
//   2. 加上左右内边距（QtMaterialTextField内部有padding）
//   3. 与最小宽度36px取较大值
//   4. 调用setFixedWidth精确控制宽度（同时设置min和max，防止挤压其他控件）
// 设计说明：
//   - 使用setFixedWidth而非setMinimumWidth：确保宽度精确等于计算值
//   - setFixedWidth内部同时调用setMinimumWidth和setMaximumWidth
//   - 布局必须给控件恰好这个宽度，不会挤压其他控件也不会被拉伸
// ============================================================================
void OrangeBar::adjustLineEditWidth(QtMaterialAutoComplete *lineEdit)
{
    // 获取文本的像素宽度（horizontalAdvance在Qt5.11+可用，替代已废弃的width()）
    int textWidth = lineEdit->fontMetrics().horizontalAdvance(lineEdit->text());
    // 内边距：QtMaterialTextField内部左右各有约8px的padding，加上边框余量
    const int padding = 20;
    // 最终宽度 = max(最小宽度36px, 文本像素宽度+内边距)
    int width = qMax(36, textWidth + padding);
    lineEdit->setFixedWidth(width);                            // 精确设置宽度
}


// ============================================================================
// 【验证lineEdit输入并返回合法的0-based帧索引】
// 私有辅助函数：验证用户输入是否为合法帧号
// 参数：lineEdit - 输入框指针，totalFrames - 总帧数（1-based上限）
// 返回值：
//   - 合法：返回0-based帧索引（0 ~ totalFrames-1）
//   - 不合法（空字符串/非数字）：返回-1
// 验证规则：
//   1. 空字符串 → 不合法（返回-1，回退到当前帧）
//   2. 非数字文本 → 不合法（返回-1，回退到当前帧）
//      注意：由于设置了QRegularExpressionValidator("\\d*")，实际不会出现非数字输入
//      但此处仍做防御性检查，以防验证器被移除或绕过
//   3. 值 < 1 → 自动修正为1（返回0-based的0）
//   4. 值 > totalFrames → 自动修正为totalFrames（返回0-based的totalFrames-1）
//   5. 值在1~totalFrames之间 → 合法，返回0-based的value-1
// ============================================================================
int OrangeBar::validateLineEditInput(QtMaterialAutoComplete *lineEdit, int totalFrames)
{
    QString text = lineEdit->text().trimmed();                  // 去除首尾空白字符

    // 规则1：空字符串 → 不合法
    if (text.isEmpty()) {
        return -1;
    }

    // 规则2：尝试转换为整数，失败则不合法
    bool ok;
    int value = text.toInt(&ok);
    if (!ok) {
        return -1;                                              // 非数字输入（防御性检查）
    }

    // 规则3：值 < 1 → 自动修正为1
    if (value < 1) {
        value = 1;
    }

    // 规则4：值 > totalFrames → 自动修正为totalFrames
    // 仅在totalFrames > 0时执行（避免无数据时除零或无效比较）
    if (totalFrames > 0 && value > totalFrames) {
        value = totalFrames;
    }

    // 规则5：合法值，转换为0-based返回
    return value - 1;
}


// ============================================================================
// 【处理前lineEdit回车/离焦验证槽函数】
// 私有槽函数：响应ui->lineEdit_original的editingFinished信号
// 信号源：ui->lineEdit_original的editingFinished信号（回车键或失去焦点时发射）
// 功能：
//   1. 验证输入是否为1~m_totalOriginalFrames之间的整数
//   2. 合法：设置sliderOriginal值 → 触发valueChanged → 跳帧+更新显示
//   3. 不合法：回退lineEdit显示到当前slider值（不发生跳帧事件）
// 信号流：
//   合法输入 → sliderOriginal.setValue(validFrame)
//     → valueChanged → onSliderOriginalValueChanged
//       → syncSlider(同步模式) → updateLineEditFromSlider
//       → emit originalFrameChanged → OrangeWidget.updateImageDisplay → readTiffFrame
//   不合法输入 → updateLineEditFromSlider回退显示 → 不发射任何信号
// ============================================================================
void OrangeBar::onLineEditOriginalEditingFinished()
{
    // 验证输入，返回0-based帧索引（-1表示不合法）
    int frame = validateLineEditInput(ui->lineEdit_original, m_totalOriginalFrames);

    if (frame >= 0) {
        // 输入合法：检查是否与当前帧相同
        if (frame == ui->sliderOriginal->value()) {
            // 帧号未变，仅调整宽度（setValue相同值不会触发valueChanged）
            adjustLineEditWidth(ui->lineEdit_original);
        } else {
            // 帧号改变，设置slider值（会触发valueChanged → 跳帧+更新显示）
            ui->sliderOriginal->setValue(frame);
        }
    } else {
        // 输入不合法（空/非数字）：回退到当前slider值显示，不发生跳帧
        updateLineEditFromSlider(ui->sliderOriginal, ui->lineEdit_original);
    }
}


// ============================================================================
// 【处理后lineEdit回车/离焦验证槽函数】
// 私有槽函数：响应ui->lineEdit_progressed的editingFinished信号
// 信号源：ui->lineEdit_progressed的editingFinished信号（回车键或失去焦点时发射）
// 功能：与onLineEditOriginalEditingFinished逻辑完全对称，作用于处理后控件
// ============================================================================
void OrangeBar::onLineEditProgressedEditingFinished()
{
    // 验证输入，返回0-based帧索引（-1表示不合法）
    int frame = validateLineEditInput(ui->lineEdit_progressed, m_totalProcessedFrames);

    if (frame >= 0) {
        // 输入合法：检查是否与当前帧相同
        if (frame == ui->sliderProcessed->value()) {
            // 帧号未变，仅调整宽度
            adjustLineEditWidth(ui->lineEdit_progressed);
        } else {
            // 帧号改变，设置slider值（会触发valueChanged → 跳帧+更新显示）
            ui->sliderProcessed->setValue(frame);
        }
    } else {
        // 输入不合法：回退到当前slider值显示，不发生跳帧
        updateLineEditFromSlider(ui->sliderProcessed, ui->lineEdit_progressed);
    }
}


// ============================================================================
// 【更新帧号标签显示】
// 私有辅助函数：根据m_totalOriginalFrames和m_totalProcessedFrames更新QLabel文本
// 功能：
//   - label_original显示处理前总帧数（如"/10"）
//   - label_progressed显示处理后总帧数（如"/8"）
// 调用时机：finishProcessing、setTotalOriginalFrames中
// 说明：
//   - 总帧数为0时显示"/0"（无数据状态）
//   - QLabel的sizePolicy默认为Preferred，会根据文本长度自动调整宽度
//     无需手动设置宽度，文本从"/0"变为"/100"时QLabel自动变宽
// ============================================================================
void OrangeBar::updateFrameLabels()
{
    ui->label_original->setText(QString("/%1").arg(m_totalOriginalFrames));      // 处理前标签："/N"
    ui->label_progressed->setText(QString("/%1").arg(m_totalProcessedFrames));   // 处理后标签："/N"
}
