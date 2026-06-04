#include "helpdialog.h"            // 帮助对话框类头文件

#include <QListWidget>              // 左侧目录列表控件
#include <QTextBrowser>             // 右侧帮助内容显示控件（支持 Markdown + GIF + 文本选中复制）
#include <QHBoxLayout>              // 主水平布局（左目录 + 右内容）
#include <QVBoxLayout>              // 右侧垂直布局（内容区 + 关闭按钮行）
#include <QPushButton>              // 关闭按钮
#include <QFile>                    // 检查文件是否存在（findHelpFilePath）
#include <QTextDocument>            // 设置文档属性（base URL 等）
#include <QTextCursor>              // 文本搜索和定位（用于锚点跳转）
#include <QFileInfo>                // 获取文件路径信息（提取目录路径）
#include <QDir>                     // 目录操作
#include <QCoreApplication>         // 应用程序路径（applicationDirPath）
#include <QRegularExpression>       // 正则表达式（提取 HTML 标题文字）
#include <QMap>                     // QMap 容器（锚点名→标题文字的映射表）

// ============================================================================
// 【构造函数】
// 功能：初始化帮助对话框的 UI 和内容
// anchor：初始跳转的锚点名称（如"quickstart"），为空则显示文档开头
// parent：父窗口指针
// ============================================================================
HelpDialog::HelpDialog(const QString &anchor, QWidget *parent) :
    QDialog(parent),
    m_directoryList(nullptr),        // 左侧目录列表（在 setupUI 中创建）
    m_contentBrowser(nullptr)        // 右侧内容浏览器（在 setupUI 中创建）
{
    // ========== 窗口基本属性 ==========
    setWindowTitle(QStringLiteral("使用帮助 - Dark Optical Sectioning"));  // 窗口标题
    resize(1500, 720);               // 默认窗口大小（宽屏，适合显示图文内容，用户可自由缩放）
    setMinimumSize(700, 500);        // 最小窗口限制（防止缩得太小导致内容被严重压缩）
    setModal(true);                  // 模态对话框：弹出后必须关闭才能操作主窗口

    // ========== 初始化 UI 布局和控件 ==========
    setupUI();

    // ========== 加载帮助文档内容 ==========
    loadHelpContent();

    // ========== 如果指定了锚点，跳转到对应位置 ==========
    if (!anchor.isEmpty()) {
        scrollToSection(anchor);
    }
}

// ============================================================================
// 【析构函数】
// 功能：释放对话框资源
// 说明：Qt 父子对象机制会自动回收所有子控件和布局，不需要手动 delete
// ============================================================================
HelpDialog::~HelpDialog()
{
}

// ============================================================================
// 【setupUI】
// 功能：创建并布局所有 UI 控件
// 布局结构：
//   ┌──────────────┬──────────────────────────────┐
//   │  目录列表     │  QTextBrowser（帮助内容）      │
//   │  QListWidget  │  支持 Markdown / GIF / 复制   │
//   │              │                              │
//   ├──────────────┴──────────────────────────────┤
//   │                              [关闭]          │
//   └──────────────────────────────────────────────┘
// ============================================================================
void HelpDialog::setupUI()
{
    // ========== 1. 创建主水平布局（左目录 + 右内容） ==========
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);    // 无边距（目录和内容紧贴）
    mainLayout->setSpacing(0);                       // 无间距

    // ========== 2. 左侧目录列表 ==========
    m_directoryList = new QListWidget(this);
    m_directoryList->setFixedWidth(160);             // 固定宽度（不随窗口拉伸）

    // 添加四个目录项（与 help.html 中的四个章节锚点对应）
    m_directoryList->addItem(QStringLiteral("开始使用"));     // 第0项 → {#quickstart}
    m_directoryList->addItem(QStringLiteral("参数说明"));     // 第1项 → {#paramsexplain}
    m_directoryList->addItem(QStringLiteral("批量处理"));     // 第2项 → {#batchuse}
    m_directoryList->addItem(QStringLiteral("代码说明"));     // 第3项 → 搜索"项目源码说明"
    m_directoryList->setCurrentRow(0);               // 默认选中第一项

    // 目录列表样式（Material Design 风格）
    m_directoryList->setStyleSheet(
        "QListWidget {"
        "  background-color: #F5F5F5;"               // 浅灰背景
        "  border: none;"                             // 无边框
        "  border-right: 1px solid #E0E0E0;"          // 右侧分隔线
        "  font-size: 14px;"                          // 字号
        "  padding: 8px;"                             // 内边距
        "}"
        "QListWidget::item {"
        "  padding: 12px 16px;"                       // 每项的内边距
        "  border-radius: 4px;"                       // 圆角
        "  color: #424242;"                           // 深灰色文字
        "}"
        "QListWidget::item:selected {"
        "  background-color: #E3F2FD;"                // 选中项浅蓝背景
        "  color: #1976D2;"                           // 选中项蓝色文字
        "}"
        "QListWidget::item:hover {"
        "  background-color: #EEEEEE;"                // 悬停项灰色背景
        "}"
    );

    // 目录项点击 → 跳转到对应章节
    connect(m_directoryList, &QListWidget::currentRowChanged,
            this, &HelpDialog::onDirectoryItemClicked);

    // ========== 3. 右侧内容区域 ==========
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(16, 16, 16, 8);  // 四周留白
    rightLayout->setSpacing(8);                       // 控件间距

    // 3a. 帮助内容浏览器
    m_contentBrowser = new QTextBrowser(this);
    m_contentBrowser->setOpenExternalLinks(true);     // 允许点击链接跳转浏览器
    // 注意：内容通过 setSource() 加载 help.html，支持表格/GIF动图/文本选中复制
    m_contentBrowser->setStyleSheet(
        "QTextBrowser {"
        "  border: none;"                             // 无边框
        "  background-color: white;"                  // 白色背景
        "  font-size: 14px;"                          // 正文字号
        "  padding: 8px;"                             // 内边距
        "}"
    );

    // 3b. 关闭按钮行
    QHBoxLayout *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();                          // 弹性空间，把按钮推到右侧

    QPushButton *btnClose = new QPushButton(this);
    btnClose->setText(QStringLiteral("关闭"));         // 按钮文字
    btnClose->setFixedSize(100, 34);                  // 固定大小
    btnClose->setCursor(Qt::PointingHandCursor);      // 鼠标悬停手型
    btnClose->setStyleSheet(
        "QPushButton {"
        "  background-color: #1976D2;"                // Material Design 蓝色
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 14px;"
        "  color: white;"                             // 白色文字
        "}"
        "QPushButton:hover {"
        "  background-color: #1565C0;"                // 悬停加深
        "}"
        "QPushButton:pressed {"
        "  background-color: #0D47A1;"                // 按下再加深
        "}"
    );
    // 点击"关闭"按钮 → 关闭对话框
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    buttonRow->addWidget(btnClose);

    // 将内容浏览器和关闭按钮添加到右侧布局
    rightLayout->addWidget(m_contentBrowser, 1);      // stretch=1：内容区占满剩余空间
    rightLayout->addLayout(buttonRow);

    // ========== 4. 组装主布局 ==========
    mainLayout->addWidget(m_directoryList);            // 左侧目录
    mainLayout->addLayout(rightLayout, 1);             // 右侧内容（stretch=1 占满）
}

// ============================================================================
// 【loadHelpContent】
// 功能：加载 help.html 并显示在 QTextBrowser 中
// 使用 QTextBrowser::setSource() 加载 HTML 文件，优点：
//   - 自动设置文档 base URL（图片相对路径自动解析）
//   - 原生支持 HTML 锚点跳转（scrollToAnchor）
//   - 原生支持 GIF 动图播放
//   - 完美渲染表格
//   - 文本默认支持选中复制
// 注意：需要在 .pro 中定义 PROJECT_SOURCE_DIR 宏，
//       编译时注入源码目录路径，运行时定位 help.html 文件
// ============================================================================
void HelpDialog::loadHelpContent()
{
    // ========== 第1步：查找 help.html 文件路径 ==========
    QString helpFilePath = findHelpFilePath();

    if (helpFilePath.isEmpty()) {
        // 文件未找到 → 显示错误提示
        m_contentBrowser->setHtml(
            QStringLiteral(
                "<p style='color: #D32F2F; font-size: 16px;'>无法找到帮助文档。</p>"
                "<p>请确保 help/help.html 文件存在于项目目录下。</p>"
                "<p>搜索路径：</p>"
                "<ul>"
                "<li>项目源码目录下的 help/help.html</li>"
                "<li>可执行文件目录下的 help/help.html</li>"
                "</ul>"
            )
        );
        return;
    }

    // ========== 第2步：通过 QTextBrowser::setSource() 加载 HTML 文件 ==========
    // setSource() 会自动：
    //   - 读取文件内容
    //   - 根据 .html 后缀识别为 HTML 格式
    //   - 设置文档 base URL 为文件所在目录（使相对路径图片生效）
    //   - 注册 HTML 中的命名锚点（<a name="...">）供 scrollToAnchor 使用
    m_contentBrowser->setSource(QUrl::fromLocalFile(helpFilePath));
}

// ============================================================================
// 【findHelpFilePath】
// 功能：在多个可能的位置搜索 help.html 文件
// 返回：找到的文件绝对路径，未找到则返回空字符串
// 搜索顺序：
//   1. 编译时指定的源码目录（PROJECT_SOURCE_DIR 宏，在 .pro 中定义）
//   2. 可执行文件同级目录（适用于发布版本，help 文件夹与 exe 在同一目录）
//   3. 可执行文件上级目录（适用于 Qt Creator 构建目录结构）
// ============================================================================
QString HelpDialog::findHelpFilePath()
{
    QStringList searchPaths;

    // 1. 编译时指定的源码目录（通过 .pro 中 DEFINES += PROJECT_SOURCE_DIR 宏注入）
    searchPaths << QString(PROJECT_SOURCE_DIR) + "/help/help.html";

    // 2. 可执行文件同级目录（发布时 help 文件夹与 exe 放在一起）
    searchPaths << QCoreApplication::applicationDirPath() + "/help/help.html";

    // 3. 可执行文件上级目录（开发时 exe 在 build 子目录中，help 在源码目录中）
    searchPaths << QCoreApplication::applicationDirPath() + "/../help/help.html";

    // 按顺序搜索，返回第一个找到的路径
    for (const QString &path : searchPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return QString();   // 所有路径都未找到，返回空字符串
}

// ============================================================================
// 【scrollToSection】
// 功能：滚动到指定锚点对应的章节位置
// anchor：锚点名称（如"quickstart"、"paramsexplain"、"batchuse"）
//
// Typora 导出的 HTML 中，Markdown 的 {#anchor} 语法被转为如下格式：
//   HTML: <a name="中文标题+锚点名">  （如 <a name="一）开始使用quickstart">）
// 因此 scrollToAnchor() 的参数需要匹配这个完整名称
//
// 实现方式：
//   1. 维护 anchor → Typora 生成的完整锚点名 的映射表
//   2. 调用 QTextBrowser::scrollToAnchor(完整锚点名) 进行跳转
//   3. 如果锚点跳转无效，降级为用 QTextDocument::find() 搜索标题文字定位
// ============================================================================
void HelpDialog::scrollToSection(const QString &anchor)
{
    // 锚点名称（用户友好名）→ Typora 导出 HTML 中的实际锚点名 的映射表
    // 这些名称来自 help.html 中 <a name="..."> 的值
    QMap<QString, QString> anchorMap;
    anchorMap["quickstart"]     = QStringLiteral("一）开始使用quickstart");
    anchorMap["paramsexplain"]  = QStringLiteral("二）参数说明paramsexplain");
    anchorMap["batchuse"]       = QStringLiteral("三）使用批量处理batchuse");
    anchorMap["code"]           = QStringLiteral("四）项目源码说明");

    // 获取 Typora 生成的完整锚点名
    QString actualAnchor = anchorMap.value(anchor, anchor);

    // 方式1：通过 scrollToAnchor() 跳转到 HTML 命名锚点
    // QTextBrowser 会搜索文档中 <a name="实际锚点名"> 的位置并滚动到那里
    m_contentBrowser->scrollToAnchor(actualAnchor);

    // 方式2：降级方案 — 如果锚点跳转未生效，通过搜索标题文字定位
    // 检查当前滚动位置是否还在文档顶部来判断锚点跳转是否成功
    // 这里直接搜索章节标题文字（如"开始使用"）作为备选定位方式
    // 注意：不用 anchorMap 做 title 映射，而是从 HTML 标题文字中直接搜索
    QMap<QString, QString> anchorToTitle;
    anchorToTitle["quickstart"]     = QStringLiteral("开始使用");
    anchorToTitle["paramsexplain"]  = QStringLiteral("参数说明");
    anchorToTitle["batchuse"]       = QStringLiteral("批量处理");
    anchorToTitle["code"]           = QStringLiteral("项目源码说明");

    QString titleText = anchorToTitle.value(anchor, anchor);
    QTextCursor cursor = m_contentBrowser->document()->find(titleText);
    if (!cursor.isNull()) {
        // 将光标移到找到的标题位置，并确保该位置在视口内可见
        m_contentBrowser->setTextCursor(cursor);
        m_contentBrowser->ensureCursorVisible();
    }
}

// ============================================================================
// 【onDirectoryItemClicked】
// 功能：目录列表项被点击时，跳转到对应章节
// row：被点击的项的行号
//   0 → 开始使用（quickstart）
//   1 → 参数说明（paramsexplain）
//   2 → 批量处理（batchuse）
//   3 → 代码说明（code）
// ============================================================================
void HelpDialog::onDirectoryItemClicked(int row)
{
    // 行号 → 锚点名称 的映射
    QStringList anchors;
    anchors << "quickstart"          // 第0项：开始使用
            << "paramsexplain"       // 第1项：参数说明
            << "batchuse"            // 第2项：批量处理
            << "code";               // 第3项：代码说明

    if (row >= 0 && row < anchors.size()) {
        scrollToSection(anchors[row]);
    }
}
