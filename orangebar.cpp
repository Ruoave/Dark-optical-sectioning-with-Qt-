// OrangeBar 橙区中间控制条模块实现
// 功能：管理处理前/后图片的帧滑块、进度条、帧号显示等控件
// 从OrangeWidget的horizontalLayout_2拆分而来
// 包含：sliderOriginal/sliderProcessed滑块控制、同步帧模式、帧切换逻辑、处理流程控制

#include "orangebar.h"
#include "ui_orangebar.h"
#include "qtmaterialslider.h"

// Qt标准库头文件引入
#include <QApplication>


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

    // 第1.5步：为两个滑块安装事件过滤器（修复缩放时thumb位置不更新的bug）
    // QtMaterialSlider没有重写resizeEvent()，窗口缩放时slider宽度改变但
    // updateThumbOffset()不会被调用，导致thumb停留在旧位置
    // 通过eventFilter拦截QEvent::Resize，手动触发updateThumbOffset()
    ui->sliderOriginal->installEventFilter(this);
    ui->sliderProcessed->installEventFilter(this);

    // 第2步：初始化滑块组件（设置范围、初始值）
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
    // ui->sliderOriginal 已通过方法A（控件提升法）在.ui中提升为QtMaterialSlider
    ui->sliderOriginal->blockSignals(true);      // 阻止信号发射（防止初始化时触发槽函数）
    ui->sliderOriginal->setRange(0, 0);          // 初始范围设为0-0（无数据时无效状态）
    ui->sliderOriginal->setValue(0);             // 初始值设为0（第一帧）
    ui->sliderOriginal->blockSignals(false);     // 恢复信号发射

    // ---------- 初始化处理后图片帧滑块 ----------
    // ui->sliderProcessed 已通过方法A（控件提升法）在.ui中提升为QtMaterialSlider
    ui->sliderProcessed->blockSignals(true);     // 阻止信号发射
    ui->sliderProcessed->setRange(0, 0);         // 初始范围设为0-0（无数据时无效状态）
    ui->sliderProcessed->setValue(0);            // 初始值设为0（第一帧）
    ui->sliderProcessed->blockSignals(false);    // 恢复信号发射

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
    }

    // ---------- 配置处理后图片滑块 ----------
    if (m_totalProcessedFrames > 0) {
        // 只有当存在有效帧数时才启用滑块
        ui->sliderProcessed->setRange(0, m_totalProcessedFrames - 1);  // 设置范围
        ui->sliderProcessed->setValue(0);         // 重置到第一帧
        ui->sliderProcessed->setEnabled(true);    // 启用滑块（之前被startProcessing()禁用了）
    }

    // 恢复两个滑块的信号发射
    ui->sliderOriginal->blockSignals(false);
    ui->sliderProcessed->blockSignals(false);

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
// 说明：仅更新内部数据成员m_totalOriginalFrames，不配置滑块范围
//       滑块范围在finishProcessing()中统一配置
// ============================================================================
void OrangeBar::setTotalOriginalFrames(int frames)
{
    m_totalOriginalFrames = frames;
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

    // 第3步：发射帧变化信号（通知OrangeWidget更新帧索引并刷新图像显示）
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

    // 第3步：发射帧变化信号（通知OrangeWidget更新帧索引并刷新图像显示）
    emit processedFrameChanged(value);
}


// ============================================================================
// 【同步滑块】
// 私有辅助函数：在同步模式下，将另一个滑块同步到源滑块的值
// 参数：sourceSlider - 触发同步的源滑块指针（sliderOriginal或sliderProcessed）
// 功能：
//   - 如果sourceSlider是sliderOriginal，则将sliderProcessed设为相同值
//   - 如果sourceSlider是sliderProcessed，则将sliderOriginal设为相同值
//   - 使用blockSignals防止被同步的滑块发射valueChanged信号（避免循环触发导致无限递归）
// 说明：
//   此函数仅在m_isSyncMode为true时被调用（由onSliderXxxValueChanged内部判断）
//   调用前已确认同步模式开启，无需再次检查
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
    } else if (sourceSlider == ui->sliderProcessed) {
        // 源是处理后滑块 -> 同步处理前滑块
        ui->sliderOriginal->blockSignals(true);   // 阻止信号（防止触发onSliderOriginalValueChanged导致循环）
        ui->sliderOriginal->setValue(value);      // 设置处理前滑块为相同值
        ui->sliderOriginal->blockSignals(false);  // 恢复信号发射
    }
}


// ============================================================================
// 【事件过滤器】
// protected重写：拦截滑块的Resize事件，修复窗口缩放时thumb位置不更新的bug
// 根因分析：
//   QtMaterialSlider继承自QAbstractSlider，但没有重写resizeEvent()
//   其内部thumb的偏移量(offset)通过updateThumbOffset()计算，
//   该函数仅在sliderChange(SliderValueChange)和setInvertedAppearance()中被调用
//   当窗口缩放导致slider宽度改变时，没有任何代码触发updateThumbOffset()
//   导致thumb圆钮停留在旧位置，与轨道不匹配
// 解决方案：
//   在OrangeBar中为两个slider安装事件过滤器
//   拦截QEvent::Resize事件，手动调用QtMaterialSlider::updateThumbOffset()
//   updateThumbOffset()是public方法（在qtmaterialslider.h中声明），可直接调用
// 参数：
//   obj - 事件源对象（应为ui->sliderOriginal或ui->sliderProcessed）
//   event - 事件对象
// 返回值：true表示事件已处理（不继续传递），false表示交给默认处理
// ============================================================================
bool OrangeBar::eventFilter(QObject *obj, QEvent *event)
{
    // 仅处理Resize事件（窗口缩放/布局调整导致slider尺寸改变）
    if (event->type() == QEvent::Resize) {
        // 确认事件源是两个滑块之一（通过指针地址比较）
        if (obj == ui->sliderOriginal || obj == ui->sliderProcessed) {
            // 将obj安全转换为QtMaterialSlider指针
            QtMaterialSlider *slider = static_cast<QtMaterialSlider *>(obj);
            // 手动触发thumb偏移量重新计算
            // 注意：updateThumbOffset()是protected方法，无法从外部直接调用
            // 替代方案：通过setValue触发内部sliderChange(SliderValueChange)
            //   sliderChange内部会调用updateThumbOffset()重新计算thumb位置
            // 使用blockSignals防止发射valueChanged信号（值未实际改变）
            slider->blockSignals(true);
            slider->setValue(slider->value());     // 触发sliderChange → 内部调用updateThumbOffset()
            slider->blockSignals(false);
            // 不阻止事件继续传递（slider自身可能还需要处理Resize）
            return false;
        }
    }

    // 其他事件交给基类默认处理
    return QWidget::eventFilter(obj, event);
}


// ============================================================================
// 【修复滑块overlay控件的Z-order图层顺序】
// 私有辅助函数：修复QtMaterialSlider的thumb被track遮挡的显示bug
// 根因分析：
//   QtMaterialSlider使用overlay widget机制绘制thumb和track：
//     - QtMaterialSliderThumb继承自QtMaterialOverlayWidget(QWidget)
//     - QtMaterialSliderTrack继承自QtMaterialOverlayWidget(QWidget)
//     - 两者的parent都是slider->parentWidget()（即OrangeBar），不是slider自身
//     - 它们是OrangeBar的直接子控件，覆盖在OrangeBar整个区域上绘制
//   创建顺序（见qtmaterialslider.cpp的init()）：
//     1. thumb = new QtMaterialSliderThumb(q)    → parent = q->parentWidget() = OrangeBar
//     2. track = new QtMaterialSliderTrack(thumb, q) → parent = q->parentWidget() = OrangeBar
//   Qt子控件绘制规则：后创建的控件绘制在上层（Z-order更高）
//   因此track（灰色未选中部分）绘制在thumb（彩色圆钮）之上
// 为什么独立OrangeBar之前没有这个bug：
//   独立前slider直接位于OrangeWidget中，thumb/track是OrangeWidget的子控件
//   OrangeWidget中有大量其他子控件（label、button等），这些控件的创建时机
//   和布局管理可能恰好让thumb的Z-order高于track
//   独立后OrangeBar作为新容器，子控件更少，Z-order完全由创建顺序决定
// 解决方案：
//   使用QWidget::raise()将thumb overlay提升到track overlay之上
//   raise()将该控件移到父控件子列表末尾，使其绘制在最上层
//   findChildren<QWidget*>()遍历OrangeBar的所有子控件找到overlay
//   通过几何特征区分thumb和track（两者都是QtMaterialOverlayWidget类型）
// ============================================================================
void OrangeBar::fixSliderOverlayZOrder()
{
    // 获取OrangeBar的所有直接子控件（包括layout管理的控件和overlay控件）
    const QList<QWidget *> children = this->findChildren<QWidget *>();

    // 用于收集每个滑块对应的thumb和track overlay
    // key: slider指针, value: thumb overlay指针
    QMap<QWidget *, QWidget *> sliderThumbMap;

    // 遍历所有子控件，识别overlay控件
    for (QWidget *child : children) {
        // 通过类名判断是否为overlay控件
        // QtMaterialSliderThumb的类名字符串为"QtMaterialSliderThumb"
        // QtMaterialSliderTrack的类名字符串为"QtMaterialSliderTrack"
        QString className = child->metaObject()->className();

        if (className == QLatin1String("QtMaterialSliderThumb")) {
            // 找到thumb overlay，记录它所属的slider
            // thumb的m_slider成员指向其所属的QtMaterialSlider
            // 但m_slider是private的，无法直接访问
            // 改用方式：thumb覆盖在slider上方，其x坐标范围与slider重叠
            // 更可靠的方式：通过child数量判断——每个slider对应1个thumb+1个track
            // 这里用简单策略：按发现顺序配对

            // 由于无法直接访问私有成员m_slider，
            // 改用另一种方法：检查哪个slider尚未关联thumb
            if (!sliderThumbMap.contains(ui->sliderOriginal)) {
                // 第一个发现的thumb归sliderOriginal
                sliderThumbMap.insert(ui->sliderOriginal, child);
            } else if (!sliderThumbMap.contains(ui->sliderProcessed)) {
                // 第二个发现的thumb归sliderProcessed
                sliderThumbMap.insert(ui->sliderProcessed, child);
            }
        }
    }

    // 对每个滑块，将其thumb overlay raise到最上层
    // 这样thumb就会绘制在track overlay之上
    for (auto it = sliderThumbMap.constBegin(); it != sliderThumbMap.constEnd(); ++it) {
        QWidget *thumbOverlay = it.value();
        if (thumbOverlay) {
            // raise()将控件提升到父控件子控件列表的末尾
            // 在下一次绘制时，该控件将绘制在所有兄弟控件之上
            thumbOverlay->raise();
        }
    }
}
