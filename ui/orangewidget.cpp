// OrangeWidget 橙区模块实
// 功能：双图像显示区域 + 两行底部控制条
// 包含：处理前/后图片对比显示、帧切换控制、进度显示、同步帧功能

#include "orangewidget.h"
#include "ui_orangewidget.h"
// qtmaterialslider.h 已移除：滑块已迁入OrangeBar，OrangeWidget不再直接操作滑块

// Qt标准库头文件引入
#include <QImageReader>
#include <QPixmap>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QStateMachine>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QColor>

using namespace std;


// ============================================================================
// 【SyncMaterialButton 子类】
// 继承 QtMaterialFlatButton，重写 paintEvent
// 目的：enabled 状态下始终显示 Halo 光晕呼吸动画，disabled 时停止
//
// 原理：基类状态机在按钮失去焦点时将 haloOpacity/haloSize 设为0，导致 Halo 消失。
// 但 haloScaleFactor 的呼吸动画（0.56↔0.63循环）一直在运行并触发 update()，
// 所以 paintEvent 会被持续调用。本子类在基类绘制后，若 enabled+haloVisible
// 且基类未画 Halo（haloOpacity≈0），则自行补画，复用 haloScaleFactor 实现呼吸。
// 悬停效果不受影响（由 overlayOpacity 独立控制）。
// ============================================================================
class SyncMaterialButton : public QtMaterialFlatButton
{
public:
    // 构造函数：与基类 QtMaterialFlatButton 兼容
    // 参数：text - 按钮文字，parent - 父控件指针
    explicit SyncMaterialButton(const QString &text, QWidget *parent = nullptr)
        : QtMaterialFlatButton(text, parent)
    {
    }

protected:
    // 重写绘制事件：在基类绘制完成后，补充绘制 checked 状态下的持续 Halo
    // 调用时机：每次按钮需要重绘时由 Qt 框架自动调用
    // 流程：基类 paintEvent（绘制背景+悬停overlay+焦点态Halo+前景文字）
    //       → 判断是否需要补充Halo → 绘制持续Halo
    void paintEvent(QPaintEvent *event) override
    {
        // 第1步：调用基类 paintEvent，完成所有原有绘制
        // 包括：背景、悬停overlay、焦点态Halo、前景文字等
        // 悬停效果由 overlayOpacity 控制，与 Halo 无关，完全保留
        QtMaterialFlatButton::paintEvent(event);

        // 第2步：判断是否需要补充绘制 Halo
        // 条件1：Halo 可见（setHaloVisible(true) 已设置）
        if (!isHaloVisible()) {
            return;
        }
        // 条件2：按钮启用（未禁用）——仅在disabled状态下不播Halo动画
        if (!isEnabled()) {
            return;
        }
        // 条件3：按钮未被按下（按下时基类也不绘制Halo，保持一致）
        if (isDown()) {
            return;
        }

        // 第3步：查找按钮的状态机子对象
        // 状态机是 QtMaterialFlatButton 内部创建的 QStateMachine 子对象，
        // 通过 findChildren 可以找到它
        QList<QStateMachine*> machines = findChildren<QStateMachine*>();
        if (machines.isEmpty()) {
            return;  // 找不到状态机，安全退出
        }

        // 第4步：读取状态机当前的 haloOpacity 属性
        // 如果基类已经绘制了可见的 Halo（haloOpacity > 0.01），
        // 说明按钮处于有焦点状态，不需要补充绘制，避免双重叠加
        qreal currentHaloOpacity = machines.first()->property("haloOpacity").toReal();
        if (currentHaloOpacity > 0.01) {
            return;  // 基类已绘制Halo，无需补充
        }

        // 第5步：读取状态机的 haloScaleFactor 属性
        // 这个属性一直在被呼吸动画驱动（0.56↔0.63循环），
        // 每次 setHaloScaleFactor 都会调用 m_button->update() 触发重绘，
        // 所以 paintEvent 会被持续调用，呼吸效果自然延续
        qreal scaleFactor = machines.first()->property("haloScaleFactor").toReal();

        // 第6步：计算 Halo 半径
        // 使用固定的 haloSize=0.7（与 neutralFocusedState 一致）
        // 乘以呼吸动画的 scaleFactor，再乘以按钮宽度，得到最终像素半径
        const qreal haloSize = 0.7;
        const qreal radius = static_cast<qreal>(width()) * scaleFactor * haloSize;

        // 第7步：绘制补充 Halo 光晕
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿，使圆形边缘平滑

        // 设置圆角裁剪区域（与基类 paintEvent 一致）
        // 防止 Halo 圆形超出按钮的圆角边界
        const qreal cr = cornerRadius();
        if (cr > 0) {
            QPainterPath path;
            path.addRoundedRect(rect(), cr, cr);
            painter.setClipPath(path);
            painter.setClipping(true);
        }

        // 绘制 Halo 圆形光晕
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(foregroundColor());  // 使用按钮前景色（与基类 paintHalo 一致）
        painter.setOpacity(baseOpacity());  // 使用按钮基础透明度（与基类 paintHalo 一致）
        painter.setBrush(brush);
        painter.setPen(Qt::NoPen);
        const QPointF center = rect().center();  // 以按钮中心为圆心
        painter.drawEllipse(center, radius, radius);  // 绘制圆形光晕

        painter.setClipping(false);  // 恢复裁剪状态
    }
};


// ============================================================================
// 【构造函数：初始化橙区所有控件】
// ============================================================================
OrangeWidget::OrangeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrangeWidget),
    m_inputFilePath(""),                        // 初始化输入文件路径为空字符串
    m_outputFilePath(""),                       // 初始化输出文件路径为空字符串
    currentOriginalFrame(0),                    // 当前处理前图片帧索引初始化为0（第一帧）
    currentProcessedFrame(0),                   // 当前处理后图片帧索引初始化为0（第一帧）
    // totalOriginalFrames/totalProcessedFrames 已迁入OrangeBar
    // isSyncMode 已迁入OrangeBar
    isOrangeWidgetSelected(false),              // OrangeWidget初始未选中
    isOriginalLabelSelected(true),              // 处理前图片label初始选中
    isProcessedLabelSelected(false)             // 处理后图片label初始未选中
{
    // 第1步：调用Qt Designer生成的UI设置（加载orangewidget.ui中的界面定义）
    ui->setupUi(this);

    // 设置焦点策略，使其能够接收键盘事件
    setFocusPolicy(Qt::StrongFocus);

    // 第2步：初始化所有Material组件（创建对象、配置属性、设置样式）
    initOrangeAreaComponents();

    // 第3步：将Material组件嵌入UI布局（替换原有Qt原生控件）
    setupOrangeAreaInLayout();

    // 第4步：绑定所有信号槽连接（箭头按钮、滑块、同步按钮的交互逻辑）
    connectOrangeAreaSignals();

    // 第5步：初始化边框样式
    updateLabelBorders();

    // 第6步：安装事件过滤器到父窗口（MainWindow）
    // 用于检测点击OrangeWidget外部区域时，自动取消选中状态
    // 注意：parent()在构造函数中可能为nullptr（如果还未被添加到父窗口布局）
    // 因此使用QTimer延迟安装，确保父窗口已就绪
    QTimer::singleShot(100, this, [this]() {
        if (parentWidget()) {
            parentWidget()->installEventFilter(this);
        }
    });
}


// ============================================================================
// 【析构函数：释放橙区资源】
// ============================================================================
OrangeWidget::~OrangeWidget()
{
    // 释放UI对象（自动清理所有子控件）
    delete ui;

    // 释放【橙区】Material组件内存（防止内存泄漏）
    delete m_syncButton;                        // 释放同步帧按钮
    // 滑块已迁入OrangeBar，由OrangeBar自行管理生命周期
    // OrangeBar作为OrangeWidget的子对象（通过.ui提升），Qt父子机制会自动回收
}


// ============================================================================
// 【橙区Material组件初始化】
// 功能：创建所有Material风格控件，设置初始属性、尺寸、样式
// 说明：此函数仅负责创建和配置控件对象，不涉及布局嵌入
// ============================================================================
void OrangeWidget::initOrangeAreaComponents()
{
    ///////////////////////////////////////////
    ////// ---------- 同步帧按钮 ----------//////
    ///////////////////////////////////////////

    // Material风格的扁平按钮（用于开启/关闭前后图片帧同步模式）
    // 使用 SyncMaterialButton 子类：重写paintEvent，使checked状态下Halo光晕在失去焦点时仍持续显示
    m_syncButton = new SyncMaterialButton("同步帧");  // 默认文字为"同步帧"
    // 设置大小策略：水平方向可拉伸填充空间，垂直方向固定高度
    m_syncButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_syncButton->setMinimumHeight(30);         // 最小高度30像素
    m_syncButton->setCornerRadius(4);             // 设置圆角半径为4像素


    // ---------- 初始状态设为不可用（程序完成一次运行前禁用）----------
    m_syncButton->setEnabled(false);           // 禁用按钮，用户无法点击

    // ---------- 字体设置 ----------
    m_syncButton->setFontSize(14);

    // ---------- 按钮颜色设置（使用 QtMaterialFlatButton 自身 API，不能用 QSS）----------
    // 重要：必须先禁用主题色，否则自定义颜色会被覆盖无效
    m_syncButton->setUseThemeColors(false);
    m_syncButton->setForegroundColor(QColor("#55aaff"));
    m_syncButton->setBackgroundColor(QColor("#c4dff"));
    m_syncButton->setBackgroundMode(Qt::TransparentMode);
    m_syncButton->setOverlayColor(QColor("#a1afc9"));
    m_syncButton->setBaseOpacity(0.2);  // 覆盖层透明度20%（和示例程序一致，悬停/选中时淡淡叠加）
    m_syncButton->setDisabledForegroundColor(QColor("#9e9e9e"));  // 灰色文字
    m_syncButton->setDisabledBackgroundColor(QColor("#e0e0e0")); // 浅灰背景


    // ---------- Material特性设置 ----------
    m_syncButton->setHaloVisible(true);             // 设置为有光晕动态效果
    m_syncButton->setOverlayStyle(Material::TintedOverlay);  // 设置为TintedOverlay色调覆盖层（悬停/选中时叠加overlayColor）
    m_syncButton->setRippleStyle(Material::CenteredRipple);   // 设置涟漪效果为CenteredRipple（点击时涟漪从按钮中心向外扩散，而非点击位置扩散）
    m_syncButton->setCheckable(true);             // 设置为可选中状态
    m_syncButton->setTextAlignment(Qt::AlignCenter);  // 文字居中对齐

    // 滑块初始化已迁入OrangeBar::initSliders()
    // OrangeBar构造函数中自动调用initSliders()，无需在此处初始化
}


// ============================================================================
// 【将Material组件嵌入UI布局】
// 功能：从UI布局中移除原有的Qt原生控件，替换为Material风格控件
// 说明：使用insertWidget在相同位置插入新控件，保持布局不变
// ============================================================================
void OrangeWidget::setupOrangeAreaInLayout()
{
    // ---------- 替换同步帧按钮（在底部控制条 horizontalLayout） ----------

    // 获取底部控制条的水平布局对象（pushButton_syncFrames所在的布局）
    QHBoxLayout *controlBarLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout);
    if (controlBarLayout) {
        // 替换同步帧按钮（pushButton_syncFrames -> m_syncButton）
        int syncIndex = controlBarLayout->indexOf(ui->pushButton_syncFrames);
        if (syncIndex >= 0) {
            controlBarLayout->removeWidget(ui->pushButton_syncFrames);   // 从布局中移除原按钮
            ui->pushButton_syncFrames->deleteLater();                    // 延迟删除原按钮（安全释放）
            controlBarLayout->insertWidget(syncIndex, m_syncButton);     // 在原位置插入Material按钮
            // 关键：insertWidget后stretch因子会被重置为0，必须手动重新设置
            // stretch=0表示左箭头组不分配额外空间，stretch=1表示m_syncButton独占所有额外空间，stretch=0表示右箭头组不分配额外空间
            controlBarLayout->setStretch(0, 0);   // 左箭头按钮组：不拉伸
            controlBarLayout->setStretch(1, 1);   // m_syncButton：独占所有额外空间
            controlBarLayout->setStretch(2, 0);   // 右箭头按钮组：不拉伸
        }
    }

    // ---------- 约束箭头按钮布局容器：防止窗口放大时箭头按钮区域膨胀 ----------

    // 设置左侧箭头按钮布局的sizeConstraint为SetFixedSize
    // 这样布局容器只会占据内部按钮所需的最小空间，不会随窗口放大而膨胀
    ui->horizontalLayout_leftArrows->setSizeConstraint(QLayout::SetFixedSize);
    // 设置右侧箭头按钮布局的sizeConstraint为SetFixedSize
    ui->horizontalLayout_rightArrows->setSizeConstraint(QLayout::SetFixedSize);
}


// ============================================================================
// 【绑定信号槽连接】
// 功能：连接所有用户交互控件的信号与对应的槽函数
// 说明：包括箭头按钮点击、滑块值改变、同步帧按钮点击等交互事件
// ============================================================================
void OrangeWidget::connectOrangeAreaSignals()
{
    // ---------- 左侧箭头按钮信号槽连接（控制处理前图片帧） ----------

    // 左箭头按钮点击 -> 先选中处理前label -> 再执行onPrevFrameLeft()槽函数
    connect(ui->pushButton_prevLeft, &QPushButton::clicked, [this]() {
        setOrangeWidgetSelected(true);
        setOriginalLabelSelected(true);
        onPrevFrameLeft();
    });

    // 右箭头按钮点击 -> 先选中处理前label -> 再执行onNextFrameLeft()槽函数
    connect(ui->pushButton_nextLeft, &QPushButton::clicked, [this]() {
        setOrangeWidgetSelected(true);
        setOriginalLabelSelected(true);
        onNextFrameLeft();
    });

    // ---------- 右侧箭头按钮信号槽连接（控制处理后图片帧） ----------

    // 左箭头按钮点击 -> 先选中处理后label -> 再执行onPrevFrameRight()槽函数
    connect(ui->pushButton_prevRight, &QPushButton::clicked, [this]() {
        setOrangeWidgetSelected(true);
        setProcessedLabelSelected(true);
        onPrevFrameRight();
    });

    // 右箭头按钮点击 -> 先选中处理后label -> 再执行onNextFrameRight()槽函数
    connect(ui->pushButton_nextRight, &QPushButton::clicked, [this]() {
        setOrangeWidgetSelected(true);
        setProcessedLabelSelected(true);
        onNextFrameRight();
    });

    // ---------- 同步帧按钮信号槽连接 ----------

    // 同步帧按钮点击 -> 执行onSyncFramesClicked()槽函数（切换同步模式）
    connect(m_syncButton, &QtMaterialFlatButton::clicked,
            this, &OrangeWidget::onSyncFramesClicked);

    // ---------- OrangeBar信号槽连接（接收OrangeBar发射的信号） ----------
    // 滑块内部信号连接已迁入OrangeBar::connectSignals()
    // 此处仅连接OrangeBar发射的对外信号到OrangeWidget的槽函数

    // 处理前滑块选中信号 -> 选中OrangeWidget和处理前图片label
    // 信号源：OrangeBar::originalSliderSelected()
    // 功能：用户拖动/点击sliderOriginal时，选中OrangeWidget并选中处理前图片label
    connect(ui->widget_orangeBarPlaceholder, &OrangeBar::originalSliderSelected, this, [this]() {
        setOrangeWidgetSelected(true);
        setOriginalLabelSelected(true);
    });

    // 处理后滑块选中信号 -> 选中OrangeWidget和处理后图片label
    // 信号源：OrangeBar::processedSliderSelected()
    // 功能：用户拖动/点击sliderProcessed时，选中OrangeWidget并选中处理后图片label
    connect(ui->widget_orangeBarPlaceholder, &OrangeBar::processedSliderSelected, this, [this]() {
        setOrangeWidgetSelected(true);
        setProcessedLabelSelected(true);
    });

    // 处理前帧变化信号 -> 更新帧索引并刷新图像显示
    // 信号源：OrangeBar::originalFrameChanged(int)
    // 功能：sliderOriginal值改变时，更新currentOriginalFrame并刷新图像
    //       同步模式下同时更新currentProcessedFrame
    connect(ui->widget_orangeBarPlaceholder, &OrangeBar::originalFrameChanged, this, [this](int frame) {
        currentOriginalFrame = frame;              // 更新处理前帧索引

        // 如果处于同步模式，同步更新处理后帧索引
        // （OrangeBar内部已通过syncSlider同步了滑块位置，
        //   但currentProcessedFrame需要OrangeWidget自行更新）
        if (ui->widget_orangeBarPlaceholder->isSyncMode()) {
            currentProcessedFrame = frame;          // 同步更新处理后帧索引
        }

        updateImageDisplay();                      // 刷新图像显示
    });

    // 处理后帧变化信号 -> 更新帧索引并刷新图像显示
    // 信号源：OrangeBar::processedFrameChanged(int)
    // 功能：sliderProcessed值改变时，更新currentProcessedFrame并刷新图像
    connect(ui->widget_orangeBarPlaceholder, &OrangeBar::processedFrameChanged, this, [this](int frame) {
        currentProcessedFrame = frame;             // 更新处理后帧索引

        // 如果处于同步模式，同步更新处理前帧索引
        if (ui->widget_orangeBarPlaceholder->isSyncMode()) {
            currentOriginalFrame = frame;           // 同步更新处理前帧索引
        }

        updateImageDisplay();                      // 刷新图像显示
    });

    // 同步模式变化信号 -> 更新按钮文字和label选中状态
    // 信号源：OrangeBar::syncModeChanged(bool)
    // 功能：同步模式开启/关闭时，更新同步按钮文字、label选中状态、帧索引
    connect(ui->widget_orangeBarPlaceholder, &OrangeBar::syncModeChanged, this, [this](bool sync) {
        if (sync) {
            // ========== 开启同步模式 ==========
            m_syncButton->setText("同步中");       // 按钮文字改为"同步中"

            // 同步帧索引（OrangeBar已将processedSlider同步到originalSlider的值）
            currentProcessedFrame = ui->widget_orangeBarPlaceholder->processedFrame();
            currentOriginalFrame = ui->widget_orangeBarPlaceholder->originalFrame();

            // 同步模式下，两个label都被选中
            if (isOrangeWidgetSelected) {
                isOriginalLabelSelected = true;
                isProcessedLabelSelected = true;
                updateLabelBorders();
            }
        } else {
            // ========== 关闭同步模式 ==========
            m_syncButton->setText("同步帧");       // 按钮文字恢复为"同步帧"

            // 关闭同步模式后，如果两个label都被选中，默认只保留处理后图片label的选中状态
            if (isOrangeWidgetSelected && isOriginalLabelSelected && isProcessedLabelSelected) {
                isOriginalLabelSelected = false;
                updateLabelBorders();
            }
        }

        // 无论是否开启同步模式，都强制刷新一次图像显示（确保界面状态一致）
        updateImageDisplay();
    });

    // 日志消息信号 -> 转发给主窗口紫区日志栏
    // 信号源：OrangeBar::logMessage(QString)
    // 功能：OrangeBar内部产生的日志消息，通过OrangeWidget转发给主窗口
    connect(ui->widget_orangeBarPlaceholder, &OrangeBar::logMessage,
            this, &OrangeWidget::logMessage);
}


// ============================================================================
// 【橙区全局风格】已移除
// 原applyMaterialTheme()函数中的全局主题色设定已删除
// 各组件样式由各自内部管理
// ============================================================================


// ==================== 【橙区槽函数实现：双图显示与帧控制】 ====================


// 项目自定义槽函数：左侧上一帧（处理前图片）
// 信号源：ui->pushButton_prevLeft clicked()信号
// 功能：切换到处理前图片的上一帧（如果当前不在第一帧）
// 说明：帧切换逻辑已迁入OrangeBar::onPrevFrameOriginal()
//       OrangeBar内部处理边界检查、滑块设置、同步模式联动
//       OrangeWidget通过OrangeBar的信号更新帧索引和刷新图像
void OrangeWidget::onPrevFrameLeft()
{
    // 委托OrangeBar执行帧切换（内部处理边界检查、滑块设置、同步联动）
    ui->widget_orangeBarPlaceholder->onPrevFrameOriginal();
}


// 项目自定义槽函数：左侧下一帧（处理前图片）
// 信号源：ui->pushButton_nextLeft clicked()信号
// 功能：切换到处理前图片的下一帧（如果当前不在最后一帧）
// 说明：帧切换逻辑已迁入OrangeBar::onNextFrameOriginal()
void OrangeWidget::onNextFrameLeft()
{
    // 委托OrangeBar执行帧切换
    ui->widget_orangeBarPlaceholder->onNextFrameOriginal();
}


// 项目自定义槽函数：右侧上一帧（处理后图片）
// 信号源：ui->pushButton_prevRight clicked()信号
// 功能：切换到处理后图片的上一帧（如果当前不在第一帧）
// 说明：帧切换逻辑已迁入OrangeBar::onPrevFrameProcessed()
void OrangeWidget::onPrevFrameRight()
{
    // 委托OrangeBar执行帧切换
    ui->widget_orangeBarPlaceholder->onPrevFrameProcessed();
}


// 项目自定义槽函数：右侧下一帧（处理后图片）
// 信号源：ui->pushButton_nextRight clicked()信号
// 功能：切换到处理后图片的下一帧（如果当前不在最后一帧）
// 说明：帧切换逻辑已迁入OrangeBar::onNextFrameProcessed()
void OrangeWidget::onNextFrameRight()
{
    // 委托OrangeBar执行帧切换
    ui->widget_orangeBarPlaceholder->onNextFrameProcessed();
}


// 项目自定义槽函数：同步帧按钮点击处理
// 信号源：m_syncButton clicked()信号
// 功能：开启或关闭前后图片帧的同步模式
// 说明：滑块同步逻辑已迁入OrangeBar::toggleSyncMode()
//       此处仅负责调用OrangeBar接口，UI更新由syncModeChanged信号处理器完成
void OrangeWidget::onSyncFramesClicked()
{
    // 委托OrangeBar切换同步模式
    // OrangeBar内部处理：切换m_isSyncMode、同步滑块位置、发射syncModeChanged信号
    // OrangeWidget通过syncModeChanged信号处理器更新按钮文字、label选中状态、帧索引
    ui->widget_orangeBarPlaceholder->toggleSyncMode();
}


// ==================== 【橙区公共接口函数实现】 ====================


// 设置输入文件路径（由主窗口调用）
// 参数：filePath - 用户选择的输入图片文件的完整路径（如"D:/Input/image.tif"）
// 使用场景：主窗口在用户通过蓝区浏览按钮选择文件后调用此函数传递路径
void OrangeWidget::setInputFilePath(const QString &filePath)
{
    // 保存输入文件路径到成员变量（后续读取图像时使用）
    m_inputFilePath = filePath;
}


// 获取输入文件路径（供其他模块查询）
// 返回值：当前设置的输入文件路径字符串（如果未设置则返回空字符串""）
QString OrangeWidget::getInputFilePath() const
{
    return m_inputFilePath;
}


// 设置输出文件路径（由主窗口在算法处理完成后调用）
// 参数：filePath - 处理后输出图片文件的完整路径（如"D:/Output/image_Darked.tif"）
// 使用场景：主窗口在darkSectioning->process()完成后构建输出路径并调用此函数
void OrangeWidget::setOutputFilePath(const QString &filePath)
{
    // 保存输出文件路径到成员变量（后续显示处理后图像时使用）
    m_outputFilePath = filePath;
}


// 获取输出文件路径（供其他模块查询）
// 返回值：当前设置的输出文件路径字符串
QString OrangeWidget::getOutputFilePath() const
{
    return m_outputFilePath;
}


// 预加载图像预览信息（仅获取帧数信息，不读取实际像素数据）
// 功能：快速获取多帧TIFF文件的帧数总数，用于初始化滑块的范围（0 到 总帧数-1）
//       此函数速度极快（毫秒级），因为它只读取文件头信息，不读取像素数据
// 参数：filePath - 图像文件路径（支持.tif/.tiff/.png/.jpg等格式）
// 使用场景：主窗口在用户选择输入文件后立即调用此函数，让用户知道该文件有多少帧
void OrangeWidget::preloadImagePreview(const QString &filePath)
{
    // 使用Qt的QImageReader类获取图像基本信息（轻量级操作，不读取像素）
    QImageReader reader(filePath);

    // 检查文件是否可以正常读取（文件是否存在、格式是否支持、权限是否足够等）
    if (!reader.canRead()) {
        // 无法读取时发射错误日志消息（主窗口会在紫区日志栏显示）
        emit logMessage("警告: 无法读取文件 " + filePath);
        return;  // 提前返回，不继续后续操作
    }

    // 获取图像的总帧数（对于单帧图像返回1，对于多帧TIFF返回实际的帧数量）
    int frameCount = reader.imageCount();

    // 处理某些特殊情况：某些图像格式可能返回-1或0（无法确定帧数）
    // 这种情况下统一视为单帧图像
    if (frameCount <= 0) {
        frameCount = 1;
    }

    // 更新处理前图片的总帧数（用于后续设置滑块的范围）
    // 已迁入OrangeBar，通过setTotalOriginalFrames设置
    ui->widget_orangeBarPlaceholder->setTotalOriginalFrames(frameCount);

    // 保存输入文件路径（供后续updateImageDisplay()读取图像时使用）
    m_inputFilePath = filePath;

    // 重置当前帧索引到第一帧（索引0表示第一帧）
    currentOriginalFrame = 0;

    // 立即显示第一帧图像（此时m_inputFilePath已设置，m_outputFilePath为空，
    // 所以只有左侧label_originalImage会显示处理前图片，右侧不受影响）
    updateImageDisplay();

    // 发射日志消息通知用户预加载结果
    emit logMessage(QString("预加载完成: %1 帧图像").arg(frameCount));
}


// 开始处理流程（由主窗口在点击"Run"按钮时调用）
// 功能：显示进度条、禁用滑块、重置进度值为0%，准备进入处理状态
// 使用场景：用户点击"Run Dark Sectioning"按钮后，主窗口调用此函数进入处理前的UI准备阶段
void OrangeWidget::startProcessing()
{
    // 委托OrangeBar禁用滑块（处理过程中不允许用户切换帧，避免干扰算法运行）
    // OrangeBar::startProcessing()内部禁用两个滑块并调用QApplication::processEvents()
    ui->widget_orangeBarPlaceholder->startProcessing();

    // 禁用同步帧按钮（处理过程中不允许使用同步功能）
    m_syncButton->setEnabled(false);             // 禁用按钮，防止处理中误操作

    // 强制立即刷新UI界面（确保上述变更立刻显示给用户，而不是等到事件循环空闲时才绘制）
    QApplication::processEvents();
}


// 完成处理流程（由主窗口在darkSectioning->process()结束后调用）
// 功能：配置滑块的有效范围、显示滑块、更新进度到100%、显示第一帧图像
// 参数：
//   originalFrames - 处理前图片的总帧数（从darkSectioning->imageStack.size()获取）
//   processedFrames - 处理后图片的总帧数（从darkSectioning->final_images.size()获取）
// 使用场景：算法处理全部完成后，主窗口调用此函数恢复橙区的交互功能
void OrangeWidget::finishProcessing(int originalFrames, int processedFrames)
{
    // 阻止OrangeBar发射信号（避免finishProcessing内部发射的帧变化信号触发多次图像刷新）
    // OrangeBar::finishProcessing()配置完滑块后会发射originalFrameChanged(0)和processedFrameChanged(0)
    // 但此处我们需要手动控制帧索引更新和图像刷新的时序，所以暂时屏蔽信号
    ui->widget_orangeBarPlaceholder->blockSignals(true);

    // 委托OrangeBar配置滑块（设置范围、重置到第一帧、显示滑块）
    // OrangeBar内部使用blockSignals防止配置过程中发射中间状态信号
    ui->widget_orangeBarPlaceholder->finishProcessing(originalFrames, processedFrames);

    // 恢复OrangeBar信号发射
    ui->widget_orangeBarPlaceholder->blockSignals(false);

    // ---------- 重置帧索引并更新进度 ----------
    currentOriginalFrame = 0;      // 重置处理前图片帧索引到第一帧
    currentProcessedFrame = 0;     // 重置处理后图片帧索引到第一帧

    // 刷新图像显示（调用辅助函数显示第一帧的处理前/后对比图像）
    updateImageDisplay();

    // 启用同步帧按钮（程序已完成一次运行，用户现在可以使用同步功能）
    m_syncButton->setEnabled(true);

    // 强制最终UI刷新（确保所有变更都立刻呈现给用户）
    QApplication::processEvents();
}


// ============================================================================
// 【获取OrangeBar指针】
// 公共接口函数：供DarkSectioning直接调用setProgress等接口
// 返回值：OrangeBar控件指针（即ui->widget_orangeBarPlaceholder提升后的对象）
// ============================================================================
OrangeBar* OrangeWidget::orangeBar() const
{
    return ui->widget_orangeBarPlaceholder;
}


// 强制刷新图像显示（对外公开接口，内部调用doUpdateImageDisplay核心实现）
// 功能：根据当前的帧索引（currentOriginalFrame和currentProcessedFrame），
//       从对应的文件中读取指定帧的图像数据，等比缩放后显示到两个QLabel上
// 使用场景：
//   1. 窗口大小改变时（resizeEvent自动调用）
//   2. 帧切换时（箭头按钮或滑块的槽函数中调用）
//   3. 处理完成时（finishProcessing中调用）
void OrangeWidget::updateImageDisplay()
{
    // 调用内部核心实现函数（实际执行图像读取、缩放、显示的逻辑）
    doUpdateImageDisplay();
}


// 内部辅助函数：更新图像显示的核心实现（私有函数，不对外暴露）
// 详细流程：
//   1. 检查是否有有效的文件路径
//   2. 从TIFF文件中读取当前帧（调用readTiffFrame）
//   3. 获取QLabel的实际显示尺寸
//   4. 计算等比缩放比例（保持宽高比，适应显示区域）
//   5. 将缩放后的QPixmap设置到QLabel上进行显示
void OrangeWidget::doUpdateImageDisplay()
{
    // ========== 第一部分：显示处理前图片（左侧 label_originalImage） ==========

    // 检查输入文件路径是否有效（不为空字符串）
    if (!m_inputFilePath.isEmpty()) {
        // 调用readTiffFrame函数从多帧TIFF文件中读取指定帧
        // 参数1：文件路径（m_inputFilePath）
        // 参数2：帧索引（currentOriginalFrame，0表示第一帧）
        QImage qOriginalImg = readTiffFrame(m_inputFilePath, currentOriginalFrame);

        // 检查图像读取是否成功（QImage不为null表示成功）
        if (!qOriginalImg.isNull()) {
            // 获取左侧显示区域（label_originalImage）的当前实际像素尺寸
            QSize labelSize = ui->label_originalImage->size();

            // 将QImage转换为QPixmap（便于进行高质量缩放操作）
            QPixmap pixmapOriginal = QPixmap::fromImage(qOriginalImg);

            // 对图像进行等比缩放以适应QLabel的大小
            // 参数说明：
            //   labelSize - 目标尺寸（QLabel的宽度和高度）
            //   Qt::KeepAspectRatio - 保持宽高比（不会拉伸变形）
            //   Qt::SmoothTransformation - 使用平滑转换算法（高质量双线性插值）
            QPixmap scaledPixmapOriginal = pixmapOriginal.scaled(
                labelSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );

            // 将缩放后的图像设置到左侧QLabel上进行显示
            ui->label_originalImage->setPixmap(scaledPixmapOriginal);
        } else {
            // 图像读取失败时的降级处理：显示占位文字（包含当前帧数信息）
            // 格式示例："处理前图片\n(帧 3/10)" 表示共10帧，当前显示第3帧
            ui->label_originalImage->setText(
                "处理前图片\n(帧 " + QString::number(currentOriginalFrame + 1) + "/" +
                QString::number(ui->widget_orangeBarPlaceholder->totalOriginalFrames()) + ")"
            );
        }
    }


    // ========== 第二部分：显示处理后图片（右侧 label_processedImage） ==========

    // 检查输出文件路径是否有效（处理完成后才会设置此路径）
    if (!m_outputFilePath.isEmpty()) {
        // 从输出文件中读取当前帧（处理后图片）
        QImage qProcessedImage = readTiffFrame(m_outputFilePath, currentProcessedFrame);

        // 检查图像读取是否成功
        if (!qProcessedImage.isNull()) {
            // 获取右侧显示区域的实际尺寸
            QSize labelSize = ui->label_processedImage->size();

            // 转换为QPixmap并进行等比缩放
            QPixmap pixmapProcessed = QPixmap::fromImage(qProcessedImage);
            QPixmap scaledPixmapProcessed = pixmapProcessed.scaled(
                labelSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );

            // 显示缩放后的处理后图像
            ui->label_processedImage->setPixmap(scaledPixmapProcessed);
        } else {
            // 读取失败时显示占位文字
            ui->label_processedImage->setText(
                "处理后图片\n(帧 " + QString::number(currentProcessedFrame + 1) + "/" +
                QString::number(ui->widget_orangeBarPlaceholder->totalProcessedFrames()) + ")"
            );
        }
    }


    // ========== 第三部分：可选的状态反馈（未来扩展用） ==========

    // 可以在此处添加帧信息标签更新、坐标显示等功能
    // 例如：ui->label_frameInfo->setText(QString("帧 %1/%2").arg(...))
}


// ============================================================================
// 窗口大小改变事件处理函数（重载QWidget的虚函数）
// 功能：当橙区部件的大小发生改变时（例如主窗口最大化、用户拖拽调整大小等），
//       自动重新计算图片的缩放比例并更新显示，确保图片始终完整显示且不失真
// 参数：event - Qt框架传入的resize事件对象（包含新旧尺寸等信息）
// ============================================================================
void OrangeWidget::resizeEvent(QResizeEvent *event)
{
    // 首先调用父类的resizeEvent处理（这一步必须要有！）
    // 确保Qt框架正常的布局更新、子控件位置调整等基础功能正常工作
    QWidget::resizeEvent(event);

    // 使用单次定时器延迟50毫秒后再更新图像显示
    // 原因：resize事件发生后，布局系统需要一定时间重新计算所有子控件的新尺寸
    //       如果立即调用updateImageDisplay()，此时QLabel可能还没有更新到最终大小，
    //       导致图片缩放尺寸不准确。延迟50ms可以确保布局已经稳定。
    QTimer::singleShot(50, this, [this]() {
        // 定时器触发后调用图像显示更新函数
        updateImageDisplay();
    });
}

// ============================================================================
// 绘制事件处理函数（重载QWidget的虚函数）
// 功能：绘制OrangeWidget的边框，根据选中状态显示不同颜色
// 参数：event - Qt框架传入的paint事件对象
// ============================================================================
void OrangeWidget::paintEvent(QPaintEvent *event)
{
    // 首先调用父类的paintEvent处理（这一步必须要有！）
    // 确保子控件能够正常绘制
    QWidget::paintEvent(event);

    // 创建QPainter对象用于绘制边框
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 启用抗锯齿，使边框更平滑

    // 根据选中状态选择边框颜色
    QColor borderColor = isOrangeWidgetSelected ? selected_OrangeWidgetBorderColor : unselected_OrangeWidgetBorderColor;
    painter.setPen(QPen(borderColor, 2));  // 设置画笔颜色和宽度（2像素）

    // 绘制圆角矩形边框（圆角半径4像素）
    // rect().adjust(1, 1, -1, -1) 用于向内缩进1像素，避免边框超出widget边界
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
}

// ============================================================================
// 鼠标点击事件处理函数（重载QWidget的虚函数）
// 功能：处理OrangeWidget内部的点击事件，实现选中逻辑
// 参数：event - Qt框架传入的mouse事件对象
// ============================================================================
void OrangeWidget::mousePressEvent(QMouseEvent *event)
{
    // 首先调用父类的mousePressEvent处理（这一步必须要有！）
    QWidget::mousePressEvent(event);

    // 如果点击的是OrangeWidget内部空间（不是子控件），选中OrangeWidget
    // 检查点击位置是否在布局区域内但不在任何子控件上
    QPoint pos = event->pos();
    
    // 判断是否点击在label_originalImage上
    if (ui->label_originalImage->geometry().contains(pos)) {
        // 如果OrangeWidget未被选中，先选中它
        if (!isOrangeWidgetSelected) {
            setOrangeWidgetSelected(true);
        }
        // 选中处理前图片label
        setOriginalLabelSelected(true);
        // 如果不在同步模式，取消处理后图片label的选中状态
        if (!ui->widget_orangeBarPlaceholder->isSyncMode()) {
            setProcessedLabelSelected(false);
        }
        return;
    }
    
    // 判断是否点击在label_processedImage上
    if (ui->label_processedImage->geometry().contains(pos)) {
        // 如果OrangeWidget未被选中，先选中它
        if (!isOrangeWidgetSelected) {
            setOrangeWidgetSelected(true);
        }
        // 选中处理后图片label
        setProcessedLabelSelected(true);
        // 如果不在同步模式，取消处理前图片label的选中状态
        if (!ui->widget_orangeBarPlaceholder->isSyncMode()) {
            setOriginalLabelSelected(false);
        }
        return;
    }
    
    // 判断是否点击在左侧箭头按钮上（属于处理前图片区域）
    if (ui->pushButton_prevLeft->geometry().contains(pos) || 
        ui->pushButton_nextLeft->geometry().contains(pos)) {
        // 如果OrangeWidget未被选中，先选中它
        if (!isOrangeWidgetSelected) {
            setOrangeWidgetSelected(true);
        }
        // 选中处理前图片label
        setOriginalLabelSelected(true);
        if (!ui->widget_orangeBarPlaceholder->isSyncMode()) {
            setProcessedLabelSelected(false);
        }
        return;
    }
    
    // 判断是否点击在右侧箭头按钮上（属于处理后图片区域）
    if (ui->pushButton_prevRight->geometry().contains(pos) || 
        ui->pushButton_nextRight->geometry().contains(pos)) {
        // 如果OrangeWidget未被选中，先选中它
        if (!isOrangeWidgetSelected) {
            setOrangeWidgetSelected(true);
        }
        // 选中处理后图片label
        setProcessedLabelSelected(true);
        if (!ui->widget_orangeBarPlaceholder->isSyncMode()) {
            setOriginalLabelSelected(false);
        }
        return;
    }
    
    // 判断是否点击在OrangeBar占位区域（滑块已迁入OrangeBar内部，不再直接检查滑块几何）
    // OrangeBar内部的滑块选中由OrangeBar::originalSliderSelected/processedSliderSelected信号处理
    if (ui->widget_orangeBarPlaceholder->geometry().contains(pos)) {
        // 点击OrangeBar区域时，选中OrangeWidget
        // 具体选中哪个label由OrangeBar发射的信号决定
        if (!isOrangeWidgetSelected) {
            setOrangeWidgetSelected(true);
        }
        return;
    }
    
    // 判断是否点击在同步帧按钮上（不改变label选中状态，只切换同步模式）
    // 同步帧按钮的点击处理已经在connectOrangeAreaSignals中绑定
    // 这里不需要额外处理
    
    // 如果点击的是OrangeWidget内部空白区域，选中OrangeWidget但不改变label选中状态
    // 确保至少有一个label被选中
    if (!isOrangeWidgetSelected) {
        setOrangeWidgetSelected(true);
        // 如果两个label都未选中，默认选中处理前图片label
        if (!isOriginalLabelSelected && !isProcessedLabelSelected) {
            setOriginalLabelSelected(true);
        }
    }
}

// ============================================================================
// 事件过滤器处理函数（重载QObject的虚函数）
// 功能：监听父窗口（MainWindow）的鼠标点击事件
//       当检测到点击在OrangeWidget外部区域时，自动取消选中状态
// 参数：
//   watched - 被监听的对象（父窗口MainWindow）
//   event - 发生的事件对象
// 返回值：true表示事件已处理（不传递给目标），false表示继续正常传递
// ============================================================================
bool OrangeWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 只处理鼠标按下事件
    if (event->type() == QEvent::MouseButtonPress) {
        // 将事件转换为鼠标事件
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        
        // 获取鼠标点击位置相对于OrangeWidget的坐标
        QPoint globalPos = mouseEvent->globalPos();
        QPoint localPos = mapFromGlobal(globalPos);
        
        // 检查点击位置是否在OrangeWidget外部
        // 如果点击不在OrangeWidget的矩形区域内，说明点击了外部区域
        if (!rect().contains(localPos) && isOrangeWidgetSelected) {
            // 取消OrangeWidget及其所有子label的选中状态
            setOrangeWidgetSelected(false);
        }
    }
    
    // 继续将事件传递给其他处理器（不拦截）
    return QWidget::eventFilter(watched, event);
}

// ============================================================================
// 键盘按下事件处理函数（重载QWidget的虚函数）
// 功能：处理键盘方向键和字母键，控制帧切换
// 参数：event - Qt框架传入的key事件对象
// ============================================================================
void OrangeWidget::keyPressEvent(QKeyEvent *event)
{
    // 如果OrangeWidget未被选中，不处理键盘事件
    if (!isOrangeWidgetSelected) {
        QWidget::keyPressEvent(event);
        return;
    }

    // 判断是否为向上切帧的按键：左箭头、上箭头、A、W
    bool isUpKey = (event->key() == Qt::Key_Left || 
                    event->key() == Qt::Key_Up || 
                    event->key() == Qt::Key_A || 
                    event->key() == Qt::Key_W);
    
    // 判断是否为向下切帧的按键：右箭头、下箭头、D、S
    bool isDownKey = (event->key() == Qt::Key_Right || 
                      event->key() == Qt::Key_Down || 
                      event->key() == Qt::Key_D || 
                      event->key() == Qt::Key_S);

    // 如果是向上切帧按键
    if (isUpKey) {
        // 如果两个label都被选中（同步模式），同时控制两侧
        if (isOriginalLabelSelected && isProcessedLabelSelected) {
            onPrevFrameLeft();
        } else if (isOriginalLabelSelected) {
            // 只有处理前图片label被选中
            onPrevFrameLeft();
        } else if (isProcessedLabelSelected) {
            // 只有处理后图片label被选中
            onPrevFrameRight();
        }
        event->accept();  // 标记事件已处理
        return;
    }

    // 如果是向下切帧按键
    if (isDownKey) {
        // 如果两个label都被选中（同步模式），同时控制两侧
        if (isOriginalLabelSelected && isProcessedLabelSelected) {
            onNextFrameLeft();
        } else if (isOriginalLabelSelected) {
            // 只有处理前图片label被选中
            onNextFrameLeft();
        } else if (isProcessedLabelSelected) {
            // 只有处理后图片label被选中
            onNextFrameRight();
        }
        event->accept();  // 标记事件已处理
        return;
    }

    // 如果不是切帧按键，交给父类处理
    QWidget::keyPressEvent(event);
}

// ============================================================================
// 滚轮事件处理函数（重载QWidget的虚函数）
// 功能：处理鼠标滚轮，控制帧切换
// 参数：event - Qt框架传入的wheel事件对象
// ============================================================================
void OrangeWidget::wheelEvent(QWheelEvent *event)
{
    // 如果OrangeWidget未被选中，不处理滚轮事件
    if (!isOrangeWidgetSelected) {
        QWidget::wheelEvent(event);
        return;
    }

    // 判断滚轮方向：正值表示向上滚动，负值表示向下滚动
    int delta = event->delta();
    
    // 如果两个label都被选中（同步模式），同时控制两侧
    if (isOriginalLabelSelected && isProcessedLabelSelected) {
        if (delta > 0) {
            // 滚轮上滑：向上切帧
            onPrevFrameLeft();
        } else {
            // 滚轮下滑：向下切帧
            onNextFrameLeft();
        }
        event->accept();
        return;
    }
    
    // 如果只有处理前图片label被选中
    if (isOriginalLabelSelected) {
        if (delta > 0) {
            onPrevFrameLeft();
        } else {
            onNextFrameLeft();
        }
        event->accept();
        return;
    }
    
    // 如果只有处理后图片label被选中
    if (isProcessedLabelSelected) {
        if (delta > 0) {
            onPrevFrameRight();
        } else {
            onNextFrameRight();
        }
        event->accept();
        return;
    }

    // 如果没有label被选中，交给父类处理
    QWidget::wheelEvent(event);
}

// ============================================================================
// 更新OrangeWidget边框样式
// 功能：根据选中状态更新边框颜色
// ============================================================================
void OrangeWidget::updateOrangeWidgetBorder()
{
    // 触发重绘事件，调用paintEvent绘制边框
    update();
}

// ============================================================================
// 更新label边框样式
// 功能：根据选中状态更新两个label的边框颜色
// ============================================================================
void OrangeWidget::updateLabelBorders()
{
    // 更新处理前图片label的边框样式
    QString originalStyleSheet = QString("QLabel {"
                                         "border: 2px solid %1;"
                                         "border-radius: 8px;"
                                         "background-color: #f0f0f0;"
                                         "}").arg(isOriginalLabelSelected ? labelSelectedColor.name() : unselected_OrangeWidgetBorderColor.name());
    ui->label_originalImage->setStyleSheet(originalStyleSheet);

    // 更新处理后图片label的边框样式
    QString processedStyleSheet = QString("QLabel {"
                                          "border: 2px solid %1;"
                                          "border-radius: 8px;"
                                          "background-color: #f0f0f0;"
                                          "}").arg(isProcessedLabelSelected ? labelSelectedColor.name() : unselected_OrangeWidgetBorderColor.name());
    ui->label_processedImage->setStyleSheet(processedStyleSheet);
}

// ============================================================================
// 设置OrangeWidget选中状态
// 参数：selected - 是否选中
// ============================================================================
void OrangeWidget::setOrangeWidgetSelected(bool selected)
{
    isOrangeWidgetSelected = selected;
    updateOrangeWidgetBorder();
    
    // 如果取消选中，同时取消所有label的选中状态
    if (!selected) {
        isOriginalLabelSelected = false;
        isProcessedLabelSelected = false;
        updateLabelBorders();
    }
}

// ============================================================================
// 设置处理前图片label选中状态
// 参数：selected - 是否选中
// ============================================================================
void OrangeWidget::setOriginalLabelSelected(bool selected)
{
    // 如果OrangeWidget未被选中，不允许选中label
    if (!isOrangeWidgetSelected) {
        return;
    }
    
    isOriginalLabelSelected = selected;
    
    // 如果不在同步模式，且选中了处理前图片label，需要取消处理后图片label的选中
    if (selected && !ui->widget_orangeBarPlaceholder->isSyncMode()) {
        isProcessedLabelSelected = false;
    }
    
    // 如果在同步模式，选中一个label时同时选中另一个
    if (selected && ui->widget_orangeBarPlaceholder->isSyncMode()) {
        isProcessedLabelSelected = true;
    }
    
    updateLabelBorders();
}

// ============================================================================
// 设置处理后图片label选中状态
// 参数：selected - 是否选中
// ============================================================================
void OrangeWidget::setProcessedLabelSelected(bool selected)
{
    // 如果OrangeWidget未被选中，不允许选中label
    if (!isOrangeWidgetSelected) {
        return;
    }
    
    isProcessedLabelSelected = selected;
    
    // 如果不在同步模式，且选中了处理后图片label，需要取消处理前图片label的选中
    if (selected && !ui->widget_orangeBarPlaceholder->isSyncMode()) {
        isOriginalLabelSelected = false;
    }
    
    // 如果在同步模式，选中一个label时同时选中另一个
    if (selected && ui->widget_orangeBarPlaceholder->isSyncMode()) {
        isOriginalLabelSelected = true;
    }
    
    updateLabelBorders();
}

// ============================================================================
// 核心函数：从多帧TIFF文件中读取指定帧（Qt原生方式）
// 功能：使用Qt原生的QImageReader类直接读取TIFF文件的特定帧，
//       直接返回QImage对象，显示效果与Windows照片查看器完全一致（无色调偏差）
// 参数：
//   filePath - TIFF文件的完整路径（支持.tif/.tiff/.png/.jpg等多种格式）
//   frameIndex - 要读取的帧索引（从0开始，0=第一帧，1=第二帧，以此类推）
// 返回值：
//   成功时返回包含该帧像素数据的QImage对象
//   失败时返回空的QImage对象（调用方可通过isNull()判断是否成功）
// ============================================================================
QImage OrangeWidget::readTiffFrame(const QString& filePath, int frameIndex)
{
    // 创建QImageReader对象（Qt提供的图像读取器，支持多帧TIFF等高级特性）
    QImageReader reader(filePath);

    // 第一步：检查文件是否可以正常读取
    // canRead()会验证：文件是否存在、文件格式是否支持、是否有读取权限、文件是否损坏等
    if (!reader.canRead()) {
        // 无法读取时发射错误日志消息（主窗口接收后在紫区显示）
        emit logMessage("错误: 无法读取文件 " + filePath);
        return QImage();  // 返回空的QImage对象表示失败
    }

    // 第二步：获取图像的总帧数（用于后续的边界检查）
    // imageCount()对于单帧图像返回1，对于多帧TIFF返回实际帧数（如10、50、100等）
    int totalFrames = reader.imageCount();

    // 第三步：处理特殊情况（某些TIFF格式可能返回-1或0表示无法确定帧数）
    if (totalFrames <= 0) {
        // 视为单帧图像，直接读取并返回
        QImage image = reader.read();

        // 如果请求的是第0帧（第一帧）或者frameIndex为负数（异常情况），直接返回
        if (frameIndex == 0 || frameIndex < 0) {
            return image;
        } else {
            // 如果请求的帧索引大于0但图像只有1帧，记录警告日志并返回唯一的一帧
            emit logMessage(QString("警告: 请求帧 %1 但图像为单帧").arg(frameIndex));
            return image;
        }
    }

    // 第四步：边界检查（确保请求的帧索引在有效范围内）
    // 有效范围：0 <= frameIndex < totalFrames
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        // 帧索引越界时记录错误日志
        emit logMessage(QString("错误: 帧索引 %1 超出范围 (总帧数: %2)")
                         .arg(frameIndex)
                         .arg(totalFrames));
        return QImage();  // 返回空QImage表示失败
    }

    // 第五步：跳转到指定的帧位置（随机访问方式，效率高）
    // jumpToImage()会将读取器的内部指针移动到指定帧，下次read()就会读取该帧
    bool jumpSuccess = reader.jumpToImage(frameIndex);

    if (!jumpSuccess) {
        // 某些老旧或不标准的TIFF格式可能不支持随机访问（jumpToImage失败）
        // 此时采用备用方案：顺序遍历方式逐帧跳转（虽然慢一些但兼容性更好）
        reader.setFileName(filePath);  // 重置读取器（回到文件开头）

        // 循环调用jumpToNextImage逐帧向前跳转，直到到达目标帧
        for (int i = 0; i < frameIndex; i++) {
            if (!reader.jumpToNextImage()) {  // 如果某次跳转失败
                // 记录详细的错误信息（包括目标帧和失败时的当前位置）
                emit logMessage(QString("错误: 无法跳转到帧 %1 (在第 %2 帧失败)")
                                 .arg(frameIndex).arg(i));
                return QImage();  // 返回空QImage表示失败
            }
        }
    }

    // 第六步：读取指定帧的像素数据并返回
    // read()会读取当前帧的全部像素数据，并自动处理颜色空间转换（如CMYK->RGB）
    QImage image = reader.read();

    // 第七步：检查读取结果是否有效
    if (image.isNull()) {
        // 读取失败（可能是文件损坏、帧数据缺失等原因）
        emit logMessage(QString("错误: 读取帧 %1 失败 (可能是TIFF格式问题)")
                         .arg(frameIndex));
    }

    // 返回读取到的QImage对象（无论成功还是失败都要返回，让调用方判断）
    return image;
}
