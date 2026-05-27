#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>          // 模态对话框基类
#include <QFrame>           // 分隔线（QFrame::HLine）

// ============================================================================
// AboutDialog：关于对话框
// 功能：显示软件名称、版本、作者声明、原项目信息、仓库链接（可点击跳转+复制）
// 构建方式：纯 C++ 代码构建界面（无 .ui 文件），所有控件在构造函数中创建
// ============================================================================
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：创建所有 UI 控件并设置布局
    // parent：父窗口指针，默认为 nullptr
    explicit AboutDialog(QWidget *parent = nullptr);

    // 析构函数：释放对话框资源
    ~AboutDialog();

private:
    // ========== 布局构建辅助函数 ==========
    // 功能：创建顶部分隔线（QFrame + HLine）
    // 返回：已设置样式的水平分隔线 QFrame 指针
    QFrame* createSeparator();

    // ========== 复制链接槽函数 ==========
    // 功能：将原项目仓库链接复制到系统剪贴板
    void copyOriginalRepoUrl();

    // 功能：将本仓库链接复制到系统剪贴板
    void copyThisRepoUrl();

    // ========== 仓库链接常量 ==========
    // 原项目 GitHub 仓库地址（MATLAB 版）
    static const QString ORIGINAL_REPO_URL;
    // 本移植项目 GitHub 仓库地址（C++/Qt 版）
    static const QString THIS_REPO_URL;
};

#endif // ABOUTDIALOG_H