// OrangeWidget 橙区模块实现
// 功能：双图像显示区域 + 两行底部控制条
// 包含：处理前/后图片对比显示、帧切换控制、进度显示、同步帧功能

#include "orangewidget.h"
#include "ui_orangewidget.h"

// Qt标准库头文件引入
#include <QImageReader>
#include <QPixmap>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QTimer>

using namespace std;


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
    totalOriginalFrames(0),                     // 处理前总帧数初始化为0（无数据状态）
    totalProcessedFrames(0),                    // 处理后总帧数初始化为0（无数据状态）
    isSyncMode(false)                           // 同步模式默认关闭（两侧独立切帧）
{
    // 第1步：调用Qt Designer生成的UI设置（加载orangewidget.ui中的界面定义）
    ui->setupUi(this);

    // 第2步：初始化所有Material组件（创建对象、配置属性、设置样式）
    initOrangeAreaComponents();

    // 第3步：将Material组件嵌入UI布局（替换原有Qt原生控件）
    setupOrangeAreaInLayout();

    // 第4步：绑定所有信号槽连接（箭头按钮、滑块、同步按钮的交互逻辑）
    connectOrangeAreaSignals();

    // 第5步：应用Material主题颜色（统一设置为#55aaff天蓝色）
    applyMaterialTheme();
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
    delete m_progressBar;                       // 释放进度条控件
    delete m_sliderOriginal;                    // 释放处理前图片滑块
    delete m_sliderProcessed;                   // 释放处理后图片滑块
}


// ============================================================================
// 【橙区Material组件初始化】
// 功能：创建所有Material风格控件，设置初始属性、尺寸、样式
// 说明：此函数仅负责创建和配置控件对象，不涉及布局嵌入
// ============================================================================
void OrangeWidget::initOrangeAreaComponents()
{
    // ---------- 创建同步帧按钮 ----------

    // Material风格的扁平按钮（用于开启/关闭前后图片帧同步模式）
    m_syncButton = new QtMaterialFlatButton("同步帧");  // 默认文字为"同步帧"
    // 设置大小策略：水平方向可拉伸填充空间，垂直方向固定高度
    m_syncButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_syncButton->setMinimumHeight(30);         // 最小高度30像素

    // ---------- Material特性设置 ----------
    // 设置涟漪效果为CenteredRipple（点击时涟漪从按钮中心向外扩散，而非点击位置扩散）
    m_syncButton->setRippleStyle(Material::CenteredRipple);
    // 设置按钮为可选中模式（checkable）
    // 选中后按钮背景色自动变深（Material框架自带效果），再次点击取消选中恢复原色
    // 这与"同步帧/取消同步"的切换语义完美匹配：未选中=不同步，选中=已开启同步
    m_syncButton->setCheckable(true);
    // ---------- 创建处理进度条 ----------

    // Material风格的进度条控件（用于显示Dark Sectioning算法的处理进度）
    m_progressBar = new QtMaterialProgress();
    m_progressBar->setRange(0, 100);            // 设置进度范围：0%（开始）到100%（完成）
    m_progressBar->setValue(0);                  // 初始值设为0（未开始状态）

    // ---------- 创建进度数值显示标签 ----------

    // QLabel标签用于显示当前进度的百分比文本（如"100%"）
    m_progressValueLabel = new QLabel("100%");   // 默认显示"100%"（因为进度功能暂未完全实现）
    m_progressValueLabel->setAlignment(Qt::AlignCenter);  // 文字居中对齐
    // 设置样式表：#55aaff天蓝色、微软雅黑字体、加粗、12px字号
    m_progressValueLabel->setStyleSheet(
        "QLabel { color: #55aaff; font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 12px; }"
    );

    // ---------- 创建处理前图片帧滑块（初始隐藏） ----------

    // Material风格的滑块控件（拖动可快速跳转到指定帧）
    m_sliderOriginal = new QtMaterialSlider(ui->widget_sliderOriginal);  // 父容器为widget_sliderOriginal
    m_sliderOriginal->setRange(0, 0);           // 初始范围设为0-0（无数据时无效状态）
    m_sliderOriginal->setValue(0);              // 初始值设为0（第一帧）
    m_sliderOriginal->hide();                   // 初始隐藏（处理完成前不显示滑块）

    // ---------- 创建处理后图片帧滑块（初始隐藏） ----------

    m_sliderProcessed = new QtMaterialSlider(ui->widget_sliderProcessed);  // 父容器为widget_sliderProcessed
    m_sliderProcessed->setRange(0, 0);          // 初始范围设为0-0（无数据时无效状态）
    m_sliderProcessed->setValue(0);             // 初始值设为0（第一帧）
    m_sliderProcessed->hide();                  // 初始隐藏（处理完成前不显示滑块）
}


// ============================================================================
// 【将Material组件嵌入UI布局】
// 功能：从UI布局中移除原有的Qt原生控件，替换为Material风格控件
// 说明：使用insertWidget在相同位置插入新控件，保持布局不变
// ============================================================================
void OrangeWidget::setupOrangeAreaInLayout()
{
    // ---------- 替换进度条（移到顶部控制条 horizontalLayout_topControlBar） ----------

    // 获取顶部控制条的水平布局对象
    QHBoxLayout *topControlBarLayout = qobject_cast<QHBoxLayout*>(ui->horizontalLayout_topControlBar);
    if (topControlBarLayout) {
        // 移除原始进度条控件（progressBar_processing是Qt原生QProgressBar）
        QLayoutItem *progressItem = topControlBarLayout->itemAt(1);  // 进度条在第2个位置（索引1）
        if (progressItem && progressItem->widget() == ui->progressBar_processing) {
            topControlBarLayout->removeWidget(ui->progressBar_processing);  // 从布局移除
            ui->progressBar_processing->deleteLater();                      // 延迟删除原进度条
            topControlBarLayout->insertWidget(1, m_progressBar);             // 在原位置插入Material进度条
        }

        // 添加进度数值显示标签到widget_progressValue容器中
        QLayoutItem *progressValueItem = topControlBarLayout->itemAt(2);  // 进度数值容器在第3个位置（索引2）
        if (progressValueItem && progressValueItem->widget() == ui->widget_progressValue) {
            // 为widget_progressValue容器创建垂直布局
            QVBoxLayout *progressValueLayout = new QVBoxLayout(ui->widget_progressValue);
            progressValueLayout->setContentsMargins(0, 0, 0, 0);       // 设置边距为0（紧凑显示）
            progressValueLayout->addWidget(m_progressValueLabel);          // 将标签添加到布局中
        }
    }

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

    // 左箭头按钮点击 -> 执行onPrevFrameLeft()槽函数（切换到上一帧）
    connect(ui->pushButton_prevLeft, &QPushButton::clicked,
            this, &OrangeWidget::onPrevFrameLeft);

    // 右箭头按钮点击 -> 执行onNextFrameLeft()槽函数（切换到下一帧）
    connect(ui->pushButton_nextLeft, &QPushButton::clicked,
            this, &OrangeWidget::onNextFrameLeft);

    // ---------- 右侧箭头按钮信号槽连接（控制处理后图片帧） ----------

    // 左箭头按钮点击 -> 执行onPrevFrameRight()槽函数（切换到上一帧）
    connect(ui->pushButton_prevRight, &QPushButton::clicked,
            this, &OrangeWidget::onPrevFrameRight);

    // 右箭头按钮点击 -> 执行onNextFrameRight()槽函数（切换到下一帧）
    connect(ui->pushButton_nextRight, &QPushButton::clicked,
            this, &OrangeWidget::onNextFrameRight);

    // ---------- 同步帧按钮信号槽连接 ----------

    // 同步帧按钮点击 -> 执行onSyncFramesClicked()槽函数（切换同步模式）
    connect(m_syncButton, &QtMaterialFlatButton::clicked,
            this, &OrangeWidget::onSyncFramesClicked);

    // ---------- 处理前图片滑块信号槽连接 ----------

    // 滑块值改变 -> 更新当前帧索引并刷新图像显示
    // 使用Lambda表达式捕获当前对象指针[this]，实现内联槽函数逻辑
    connect(m_sliderOriginal, &QtMaterialSlider::valueChanged, [this](int value) {
        currentOriginalFrame = value;              // 更新当前帧索引为滑块的当前值

        // 如果处于同步模式，需要同步更新右侧（处理后图片）的帧索引和滑块位置
        if (isSyncMode) {
            currentProcessedFrame = value;          // 同步更新处理后图片的帧索引
            // 阻止右侧滑块发射信号（避免循环触发导致无限递归）
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(value);     // 手动设置右侧滑块的位置
            m_sliderProcessed->blockSignals(false); // 恢复右侧滑块的信号发射
        }

        // 调用图像显示更新函数，重新读取并显示当前帧的图像
        updateImageDisplay();
    });

    // ---------- 处理后图片滑块信号槽连接 ----------

    // 滑块值改变 -> 更新当前帧索引并刷新图像显示
    connect(m_sliderProcessed, &QtMaterialSlider::valueChanged, [this](int value) {
        currentProcessedFrame = value;             // 更新当前帧索引

        // 如果处于同步模式，需要同步更新左侧（处理前图片）的帧索引和滑块位置
        if (isSyncMode) {
            currentOriginalFrame = value;           // 同步更新处理前图片的帧索引
            // 阻止左侧滑块发射信号（避免循环触发）
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(value);      // 手动设置左侧滑块的位置
            m_sliderOriginal->blockSignals(false);  // 恢复左侧滑块的信号发射
        }

        // 调用图像显示更新函数
        updateImageDisplay();
    });
}


// ============================================================================
// 【应用Material主题颜色】
// 功能：统一设置所有Material组件的主题色为#55aaff（天蓝色RGB: 85, 170, 255）
// 说明：确保橙区所有控件的颜色风格与蓝区、绿区保持一致
// ============================================================================
void OrangeWidget::applyMaterialTheme()
{
    // 定义主题主色调常量（#55aaff天蓝色）
    QColor themeColor(85, 170, 255);

    // ---------- 为进度条应用主题色 ----------
    m_progressBar->setProgressColor(themeColor);   // 设置进度条的填充颜色

    // ---------- 为滑块组应用主题色 ----------
    m_sliderOriginal->setTrackColor(themeColor);   // 设置滑块轨道颜色
    m_sliderProcessed->setTrackColor(themeColor);
    m_sliderOriginal->setThumbColor(themeColor);   // 设置滑块滑块头（圆形把手）颜色
    m_sliderProcessed->setThumbColor(themeColor);

    // ---------- 为同步帧按钮应用主题色 ----------
    m_syncButton->setForegroundColor(themeColor);  // 设置按钮文字颜色为天蓝色
}


// ==================== 【橙区槽函数实现：双图显示与帧控制】 ====================


// 项目自定义槽函数：左侧上一帧（处理前图片）
// 信号源：ui->pushButton_prevLeft clicked()信号
// 流程：检查边界条件 -> 帧索引减1 -> 同步模式下更新右侧 -> 更新滑块 -> 刷新显示
// 功能：切换到处理前图片的上一帧（如果当前不在第一帧）
void OrangeWidget::onPrevFrameLeft()
{
    // 边界检查：确保不会超出第一帧（帧索引不能小于0）
    if (currentOriginalFrame > 0) {
        // 帧索引减1（切换到上一帧，例如从第5帧切换到第4帧）
        currentOriginalFrame--;

        // 同步模式下的特殊处理：如果开启了同步模式（isSyncMode == true）
        // 则处理后图片也需要跟着切换到相同的帧
        if (isSyncMode) {
            currentProcessedFrame = currentOriginalFrame;  // 同步更新右侧帧索引
            // 阻止右侧滑块发射valueChanged信号（避免触发右侧滑块的槽函数导致无限循环）
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(currentProcessedFrame);  // 手动设置右侧滑块位置
            m_sliderProcessed->blockSignals(false);  // 恢复右侧滑块的正常信号发射
        }

        // 更新左侧滑块的位置（这会触发valueChanged信号，进而调用updateImageDisplay刷新图像）
        m_sliderOriginal->setValue(currentOriginalFrame);
    }
    // 如果已经在第一帧（currentOriginalFrame == 0），则不做任何操作（静默忽略）
}


// 项目自定义槽函数：左侧下一帧（处理前图片）
// 信号源：ui->pushButton_nextLeft clicked()信号
// 流程：检查边界条件 -> 帧索引加1 -> 同步模式下更新右侧 -> 更新滑块 -> 刷新显示
// 功能：切换到处理前图片的下一帧（如果当前不在最后一帧）
void OrangeWidget::onNextFrameLeft()
{
    // 边界检查：确保不会超出最后一帧（帧索引不能大于等于总帧数）
    if (currentOriginalFrame < totalOriginalFrames - 1) {
        // 帧索引加1（切换到下一帧，例如从第4帧切换到第5帧）
        currentOriginalFrame++;

        // 同步模式下的特殊处理
        if (isSyncMode) {
            currentProcessedFrame = currentOriginalFrame;
            m_sliderProcessed->blockSignals(true);
            m_sliderProcessed->setValue(currentProcessedFrame);
            m_sliderProcessed->blockSignals(false);
        }

        // 更新左侧滑块位置
        m_sliderOriginal->setValue(currentOriginalFrame);
    }
}


// 项目自定义槽函数：右侧上一帧（处理后图片）
// 信号源：ui->pushButton_prevRight clicked()信号
// 功能：切换到处理后图片的上一帧（如果当前不在第一帧）
void OrangeWidget::onPrevFrameRight()
{
    // 边界检查：确保不会超出第一帧
    if (currentProcessedFrame > 0) {
        // 帧索引减1
        currentProcessedFrame--;

        // 同步模式下的特殊处理
        if (isSyncMode) {
            currentOriginalFrame = currentProcessedFrame;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(currentOriginalFrame);
            m_sliderOriginal->blockSignals(false);
        }

        // 更新右侧滑块位置
        m_sliderProcessed->setValue(currentProcessedFrame);
    }
}


// 项目自定义槽函数：右侧下一帧（处理后图片）
// 信号源：ui->pushButton_nextRight clicked()信号
// 功能：切换到处理后图片的下一帧（如果当前不在最后一帧）
void OrangeWidget::onNextFrameRight()
{
    // 边界检查：确保不会超出最后一帧
    if (currentProcessedFrame < totalProcessedFrames - 1) {
        // 帧索引加1
        currentProcessedFrame++;

        // 同步模式下的特殊处理
        if (isSyncMode) {
            currentOriginalFrame = currentProcessedFrame;
            m_sliderOriginal->blockSignals(true);
            m_sliderOriginal->setValue(currentOriginalFrame);
            m_sliderOriginal->blockSignals(false);
        }

        // 更新右侧滑块位置
        m_sliderProcessed->setValue(currentProcessedFrame);
    }
}


// 项目自定义槽函数：同步帧按钮点击处理
// 信号源：m_syncButton clicked()信号
// 流程：切换同步模式标志位 -> 更新按钮文字提示 -> 发射日志信号 -> 可选地同步两侧帧
// 功能：开启或关闭前后图片帧的同步模式
//       开启同步模式后：操作一侧（如点击左箭头），另一侧会自动跟随切换到相同帧
//       关闭同步模式后：两侧可以独立进行帧切换操作，互不影响
void OrangeWidget::onSyncFramesClicked()
{
    // 切换同步模式标志位（使用逻辑非运算符!取反：true变false，false变true）
    isSyncMode = !isSyncMode;

    // 根据新的模式状态执行不同的UI更新逻辑
    if (isSyncMode) {
        // ========== 开启同步模式 ==========
        m_syncButton->setText("取消同步");           // 按钮文字改为"取消同步"（提示用户再次点击可取消）

        // 发射日志消息信号（主窗口会接收到并在紫区日志栏显示）
        emit logMessage("同步模式已开启: 前后图片帧数将联动");

        // 立即同步两侧到同一帧（以左侧处理前图片的当前帧为准）
        currentProcessedFrame = currentOriginalFrame;
        // 阻止右侧滑块信号，手动设置其位置
        m_sliderProcessed->blockSignals(true);
        m_sliderProcessed->setValue(currentProcessedFrame);
        m_sliderProcessed->blockSignals(false);
    } else {
        // ========== 关闭同步模式 ==========
        m_syncButton->setText("同步帧");              // 按钮文字恢复为"同步帧"（初始状态）

        // 发射日志消息信号
        emit logMessage("同步模式已关闭: 前后图片帧数独立控制");
    }

    // 无论是否开启同步模式，都强制刷新一次图像显示（确保界面状态一致）
    updateImageDisplay();
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
    totalOriginalFrames = frameCount;

    // 保存输入文件路径（供后续updateImageDisplay()读取图像时使用）
    m_inputFilePath = filePath;

    // 重置当前帧索引到第一帧（索引0表示第一帧）
    currentOriginalFrame = 0;

    // 发射日志消息通知用户预加载结果
    emit logMessage(QString("预加载完成: %1 帧图像").arg(frameCount));

    // 注意：此处不调用updateImageDisplay()，因为只是预加载了帧数信息
    // 实际的图像像素数据显示将在用户点击"Run Dark Sectioning"按钮后进行
}


// 开始处理流程（由主窗口在点击"Run"按钮时调用）
// 功能：显示进度条、隐藏滑块、重置进度值为0%，准备进入处理状态
// 使用场景：用户点击"Run Dark Sectioning"按钮后，主窗口调用此函数进入处理前的UI准备阶段
void OrangeWidget::startProcessing()
{
    // 显示进度条（让用户看到处理正在进行中）
    m_progressBar->show();

    // 重置进度值到0%（表示刚开始处理）
    m_progressBar->setValue(0);

    // 隐藏两个帧滑块（处理过程中不允许用户切换帧，避免干扰算法运行）
    m_sliderOriginal->hide();     // 隐藏处理前图片滑块
    m_sliderProcessed->hide();    // 隐藏处理后图片滑块

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
    // 保存总帧数到成员变量（用于边界检查和滑块范围设置）
    totalOriginalFrames = originalFrames;
    totalProcessedFrames = processedFrames;

    // ---------- 配置处理前图片滑块 ----------
    if (totalOriginalFrames > 0) {
        // 只有当存在有效帧数时才启用滑块（避免除零错误或无效范围）
        m_sliderOriginal->setRange(0, totalOriginalFrames - 1);  // 设置范围：0 到 总帧数-1
        m_sliderOriginal->setValue(0);              // 重置滑块位置到第一帧（索引0）
        m_sliderOriginal->show();                  // 显示滑块（之前被startProcessing()隐藏了）
    }

    // ---------- 配置处理后图片滑块 ----------
    if (totalProcessedFrames > 0) {
        // 只有当存在有效帧数时才启用滑块
        m_sliderProcessed->setRange(0, totalProcessedFrames - 1);  // 设置范围
        m_sliderProcessed->setValue(0);             // 重置到第一帧
        m_sliderProcessed->show();                  // 显示滑块
    }

    // ---------- 重置帧索引并更新进度 ----------
    currentOriginalFrame = 0;      // 重置处理前图片帧索引到第一帧
    currentProcessedFrame = 0;     // 重置处理后图片帧索引到第一帧

    // 更新进度条到100%（表示处理已全部完成）
    m_progressBar->setValue(100);

    // 更新进度数值显示标签的文字为"100%"
    m_progressValueLabel->setText("100%");

    // 刷新图像显示（调用辅助函数显示第一帧的处理前/后对比图像）
    updateImageDisplay();

    // 强制最终UI刷新（确保所有变更都立刻呈现给用户）
    QApplication::processEvents();
}


// 更新处理进度（实时刷新进度条和进度数值）
// 参数：progress - 当前进度百分比（整数，范围0-100）
// 使用场景：主窗口在darkSectioning->process()执行期间定期调用此函数更新UI
//           例如：开始时传10，中间传50，快结束时传90，最后finishProcessing()自动设为100
void OrangeWidget::updateProgress(int progress)
{
    // 更新Material进度条的当前值（进度条会自动根据新值重绘填充部分）
    m_progressBar->setValue(progress);

    // 更新进度数值显示标签的文字（转换为字符串格式，如"45%"）
    m_progressValueLabel->setText(QString("%1%").arg(progress));

    // 可选：强制刷新UI（如果调用频率较低建议加上这句；如果调用频率很高则不需要）
    // QApplication::processEvents();
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
                QString::number(totalOriginalFrames) + ")"
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
                QString::number(totalProcessedFrames) + ")"
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
// 核心函数：从多帧TIFF文件中读取指定帧（Qt原生方式，无需OpenCV转换）
// 功能：使用Qt原生的QImageReader类直接读取TIFF文件的特定帧，
//       直接返回QImage对象，无需经过OpenCV的Mat中间转换，
//       因此显示效果与Windows照片查看器完全一致（无色调偏差）
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
