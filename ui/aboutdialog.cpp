#include "aboutdialog.h"             // 关于对话框类头文件

#include <QLabel>                    // 显示文本和可点击链接
#include <QPushButton>               // "复制"按钮和"确定"按钮
#include <QVBoxLayout>               // 垂直布局：从上到下排列所有内容
#include <QHBoxLayout>               // 水平布局：链接文字 + 复制按钮 并排
#include <QFrame>                    // 分隔线（Horizontal Line）
#include <QApplication>              // 剪贴板访问（QApplication::clipboard()）
#include <QClipboard>                // 系统剪贴板对象

// ============================================================================
// 静态常量定义（仓库链接）
// ============================================================================
const QString AboutDialog::ORIGINAL_REPO_URL = QStringLiteral("https://github.com/Cao-ruijie/Dark-sectioning");
const QString AboutDialog::THIS_REPO_URL    = QStringLiteral("https://github.com/Ruoave/Dark-optical-sectioning-with-Qt-");

// ============================================================================
// 【构造函数】
// 功能：创建所有 UI 控件、设置布局、配置信号槽、应用样式
// parent：父窗口指针，默认为 nullptr
// ============================================================================
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent)                  // 调用基类 QDialog 构造函数
{
    // ========== 窗口基本属性设置 ==========
    setWindowTitle(QStringLiteral("关于 Dark Sectioning"));  // 窗口标题栏文字
    setFixedSize(620, 620);          // 固定窗口大小（禁止缩放，保持布局整洁）
    setModal(true);                  // 模态对话框：弹出后必须关闭才能操作主窗口

    // ========== 创建顶层垂直布局 ==========
    QVBoxLayout *mainLayout = new QVBoxLayout(this);   // 对话框的主布局
    mainLayout->setContentsMargins(32, 28, 32, 24);    // 四周留白：左32 上28 右32 下24
    mainLayout->setSpacing(14);                        // 控件之间的垂直间距（20号字加大间距）

    // ========== 1. 软件名称标题（大号加粗） ==========
    QLabel *titleLabel = new QLabel(this);
    titleLabel->setText(QStringLiteral("Dark Sectioning"));    // 软件名称
    titleLabel->setStyleSheet(
        "font-size: 22px;"           // 大号字体
        "font-weight: bold;"         // 加粗
        "color: #212121;"            // 深灰色（接近黑色）
    );
    titleLabel->setAlignment(Qt::AlignCenter);    // 居中显示
    mainLayout->addWidget(titleLabel);

    // ========== 2. 版本号 ==========
    QLabel *versionLabel = new QLabel(this);
    versionLabel->setText(QStringLiteral("版本 1.0.0"));                // 版本号
    versionLabel->setStyleSheet(
        "font-size: 14px;"           // 中等字体
        "color: #757575;"            // 灰色
    );
    versionLabel->setAlignment(Qt::AlignCenter);    // 居中显示
    mainLayout->addWidget(versionLabel);

    // ========== 3. 分隔线 ==========
    mainLayout->addWidget(createSeparator());

    // ========== 4. 项目描述文本 ==========
    QLabel *descLabel = new QLabel(this);
    // 使用富文本（Rich Text）排版描述内容
    descLabel->setText(
        QStringLiteral(
            "<p style='font-size:18px; color:#424242; line-height:1.6;'>"
            "本软件是本科毕业设计作品。</p>"
            "<p style='font-size:18px; color:#424242; line-height:1.6;'>"
            "本项目是 Cao-ruijie/Dark-sectioning（Apache License）"
            "中 MATLAB 代码的 C++/Qt 移植版本，"
            "并基于 Material Design 风格增加了图形用户界面。</p>"
        )
    );
    descLabel->setWordWrap(true);                // 自动换行（避免文字被截断）
    mainLayout->addWidget(descLabel);

    // ========== 5. 原项目作者信息 ==========
    QLabel *authorLabel = new QLabel(this);
    authorLabel->setText(
        QStringLiteral(
            "<p style='font-size:18px; color:#424242; line-height:1.6;'>"
            "<b>原项目作者</b>：曹睿杰等<br/>"
            "<b>原论文</b>：Dark-based Optical Sectioning assists Background "
            "Removal in Fluorescence Microscopy "
            "(Nature Methods, 2025)</p>"
        )
    );
    authorLabel->setWordWrap(true);              // 自动换行
    mainLayout->addWidget(authorLabel);

    // ========== 6. 分隔线 ==========
    mainLayout->addWidget(createSeparator());

    // ========== 7. 原项目仓库链接行（点击跳转 + 复制按钮） ==========
    QHBoxLayout *originalRepoRow = new QHBoxLayout();   // 水平布局：链接 + 按钮 并排
    originalRepoRow->setSpacing(8);                      // 链接与按钮之间的间距

    // 7a. 原项目链接标签（可点击，setOpenExternalLinks(true) 实现浏览器跳转）
    QLabel *originalRepoLabel = new QLabel(this);
    // HTML 格式中的 <a href="..."> 标签：Qt 识别后生成可点击链接
    // style 内联样式：蓝色文字、去掉下划线（text-decoration: none）
    originalRepoLabel->setText(
        QStringLiteral(
            "<span style='font-size:18px; color:#424242;'>原项目：</span>"
            "<a href='%1' style='font-size:16px; color:#1976D2; text-decoration:none;'>"
            "github.com/Cao-ruijie/Dark-sectioning</a>"
        ).arg(ORIGINAL_REPO_URL)
    );
    originalRepoLabel->setOpenExternalLinks(true);       // ★ 关键设置：点击链接自动用默认浏览器打开
    originalRepoLabel->setMinimumHeight(32);             // 保证和按钮高度对齐

    // 7b. 原项目链接的复制按钮
    QPushButton *btnCopyOriginal = new QPushButton(this);
    btnCopyOriginal->setText(QStringLiteral("复制"));    // 按钮文字
    btnCopyOriginal->setFixedSize(56, 28);               // 固定大小：宽56px 高28px（小巧不占空间）
    btnCopyOriginal->setCursor(Qt::PointingHandCursor);  // 鼠标悬停时显示手型指针
    btnCopyOriginal->setStyleSheet(
        "QPushButton {"
        "  background-color: #E3F2FD;"     // 浅蓝色背景
        "  border: 1px solid #90CAF9;"     // 蓝色边框
        "  border-radius: 4px;"            // 圆角
        "  font-size: 12px;"
        "  color: #1976D2;"                // 蓝色文字
        "}"
        "QPushButton:hover {"
        "  background-color: #BBDEFB;"     // 悬停时加深背景
        "}"
        "QPushButton:pressed {"
        "  background-color: #90CAF9;"     // 按下时再加深
        "}"
    );
    // 点击"复制"按钮 → 调用 copyOriginalRepoUrl() 槽函数
    connect(btnCopyOriginal, &QPushButton::clicked, this, &AboutDialog::copyOriginalRepoUrl);

    // 7c. 将链接标签和复制按钮添加到水平布局
    originalRepoRow->addWidget(originalRepoLabel, 1);   // stretch=1：链接文字区域占满剩余空间
    originalRepoRow->addWidget(btnCopyOriginal);         // 复制按钮靠右对齐
    mainLayout->addLayout(originalRepoRow);              // 将这一行添加到主布局

    // ========== 8. 本仓库链接行（点击跳转 + 复制按钮） ==========
    QHBoxLayout *thisRepoRow = new QHBoxLayout();        // 水平布局：链接 + 按钮 并排
    thisRepoRow->setSpacing(8);

    // 8a. 本仓库链接标签（可点击）
    QLabel *thisRepoLabel = new QLabel(this);
    thisRepoLabel->setText(
        QStringLiteral(
            "<span style='font-size:18px; color:#424242;'>本仓库：</span>"
            "<a href='%1' style='font-size:16px; color:#1976D2; text-decoration:none;'>"
            "github.com/Ruoave/Dark-optical-sectioning-with-Qt-</a>"
        ).arg(THIS_REPO_URL)
    );
    thisRepoLabel->setOpenExternalLinks(true);           // ★ 关键设置：可点击跳转浏览器
    thisRepoLabel->setMinimumHeight(32);

    // 8b. 本仓库链接的复制按钮
    QPushButton *btnCopyThis = new QPushButton(this);
    btnCopyThis->setText(QStringLiteral("复制"));
    btnCopyThis->setFixedSize(56, 28);
    btnCopyThis->setCursor(Qt::PointingHandCursor);
    btnCopyThis->setStyleSheet(
        "QPushButton {"
        "  background-color: #E3F2FD;"
        "  border: 1px solid #90CAF9;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "  color: #1976D2;"
        "}"
        "QPushButton:hover {"
        "  background-color: #BBDEFB;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #90CAF9;"
        "}"
    );
    connect(btnCopyThis, &QPushButton::clicked, this, &AboutDialog::copyThisRepoUrl);

    // 8c. 添加这一行到布局
    thisRepoRow->addWidget(thisRepoLabel, 1);
    thisRepoRow->addWidget(btnCopyThis);
    mainLayout->addLayout(thisRepoRow);

    // ========== 9. 分隔线 ==========
    mainLayout->addWidget(createSeparator());

    // ========== 10. 开源声明 ==========
    QLabel *licenseLabel = new QLabel(this);
    licenseLabel->setText(
        QStringLiteral(
            "<p style='font-size:14px; color:#757575; line-height:1.5;'>"
            "开源声明：本软件遵循 Apache License 2.0 发布。<br/>"
            "基于原 MATLAB 代码移植修改，特此感谢原作者的贡献。</p>"
        )
    );
    licenseLabel->setWordWrap(true);
    licenseLabel->setAlignment(Qt::AlignCenter);         // 居中显示声明文字
    mainLayout->addWidget(licenseLabel);

    // ========== 11. "确定"按钮（关闭对话框） ==========
    QHBoxLayout *buttonRow = new QHBoxLayout();          // 水平布局：让按钮居中
    buttonRow->setContentsMargins(0, 8, 0, 0);           // 上方留一点间距

    QPushButton *btnClose = new QPushButton(this);
    btnClose->setText(QStringLiteral("确定"));            // 按钮文字
    btnClose->setFixedSize(100, 34);                     // 固定大小
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setStyleSheet(
        "QPushButton {"
        "  background-color: #1976D2;"     // Material Design 蓝色主色调
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 14px;"
        "  color: white;"                  // 白色文字
        "}"
        "QPushButton:hover {"
        "  background-color: #1565C0;"     // 悬停加深
        "}"
        "QPushButton:pressed {"
        "  background-color: #0D47A1;"     // 按下再加深
        "}"
    );
    // 点击"确定"按钮 → 关闭对话框（QDialog::accept() 关闭并返回 Accepted）
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    buttonRow->addStretch();               // 弹性空间，把按钮推到中间
    buttonRow->addWidget(btnClose);
    buttonRow->addStretch();               // 弹性空间，让按钮居中
    mainLayout->addLayout(buttonRow);

    // ========== 整体布局添加弹性伸缩 ==========
    // 在所有内容下方加一个弹性空间，防止控件被拉伸分散，
    // 保证内容从顶部开始紧凑排列
    mainLayout->addStretch(1);
}

// ============================================================================
// 【析构函数】
// 功能：释放对话框资源
// 说明：Qt 父子对象机制会自动回收所有子控件（QLabel/QPushButton/QFrame等），
//       布局（QLayout）也会自动释放，因此不需要手动 delete
// ============================================================================
AboutDialog::~AboutDialog()
{
    // Qt 父子机制自动回收，无需手动释放
}

// ============================================================================
// 【辅助函数：创建分隔线】
// 功能：创建一条水平分隔线（QFrame + HLine），分隔不同内容区块
// 返回：已设置样式的 QFrame 指针
// ============================================================================
QFrame* AboutDialog::createSeparator()
{
    QFrame *line = new QFrame(this);       // 创建分隔线框架
    line->setFrameShape(QFrame::HLine);    // 形状：水平线
    line->setFrameShadow(QFrame::Sunken);  // 样式：轻微下沉效果
    line->setStyleSheet(
        "QFrame {"
        "  color: #E0E0E0;"               // 浅灰色线条
        "  max-height: 1px;"               // 线条高度仅1像素
        "}"
    );
    return line;
}

// ============================================================================
// 【槽函数：复制原项目仓库链接】
// 信号源：btnCopyOriginal 按钮的 clicked() 信号
// 功能：将 ORIGINAL_REPO_URL 写入系统剪贴板，并临时改变按钮文字提示用户
// ============================================================================
void AboutDialog::copyOriginalRepoUrl()
{
    // 获取系统剪贴板对象 → 设置文本内容为仓库链接
    QApplication::clipboard()->setText(ORIGINAL_REPO_URL);
}

// ============================================================================
// 【槽函数：复制本仓库链接】
// 信号源：btnCopyThis 按钮的 clicked() 信号
// 功能：将 THIS_REPO_URL 写入系统剪贴板
// ============================================================================
void AboutDialog::copyThisRepoUrl()
{
    QApplication::clipboard()->setText(THIS_REPO_URL);
}
