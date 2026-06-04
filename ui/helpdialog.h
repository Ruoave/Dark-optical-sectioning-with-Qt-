#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

class QListWidget;      // 左侧目录列表控件（前向声明，避免头文件依赖）
class QTextBrowser;     // 右侧帮助内容显示控件（前向声明）

// ============================================================================
// 【帮助对话框类】
// 功能：显示 Markdown 格式的帮助文档，支持左侧目录导航和锚点跳转
// 布局：左侧 QListWidget（目录）+ 右侧 QTextBrowser（内容）
// 图片：支持 PNG/JPG 静态图和 GIF 动图（QTextBrowser 原生支持）
// 文本：支持选中复制（QTextBrowser 默认支持右键菜单复制）
// ============================================================================
class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数
    // anchor：初始跳转的锚点名称（如"quickstart"），为空则显示文档开头
    // parent：父窗口指针
    explicit HelpDialog(const QString &anchor = QString(), QWidget *parent = nullptr);

    // 析构函数
    ~HelpDialog();

    // 滚动到指定锚点位置
    // anchor：锚点名称（对应 Markdown 中的 {#anchor} 语法）
    void scrollToSection(const QString &anchor);

private slots:
    // 目录列表项被点击时的响应
    // row：被点击的项的行号（0=开始使用, 1=参数说明, 2=批量处理, 3=代码说明）
    void onDirectoryItemClicked(int row);

private:
    QListWidget *m_directoryList;    // 左侧目录列表
    QTextBrowser *m_contentBrowser;  // 右侧帮助内容浏览器

    // 初始化 UI 布局和控件
    void setupUI();

    // 从文件系统加载 help.md 内容并显示
    void loadHelpContent();

    // 查找 help.md 文件的路径（支持多个搜索位置）
    QString findHelpFilePath();
};

#endif // HELPDIALOG_H
