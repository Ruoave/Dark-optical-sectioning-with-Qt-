// The main program of Dark section
// https://github.com/sjtrny/Dark-Channel-Haze-Removal
// This program is finished by Caoruijie and professor Xipeng in Peking
// University.
//
// For referrence:
// Single Image Haze Removal Using Dark Channel Prior
// Kaiming He, Jian Sun and Xiaoou Tang
// IEEE Transactions on Pattern Analysis and Machine Intelligence
// Volume 30, Number 12, Pages 2341-2353
//
// For any question, please contact: caoruijie@stu.pku.edu.cn or
// xipeng@pku.edu.cn
//
// We claim a Apache liscence for Dark sectioning.

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "params.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <windows.h>
#include <ViewMat.h>
#include <vector>
#include <QFileDialog>
#include <QString>
#include <QStandardPaths>
#include <QImage>
#include <QPixmap>

using namespace cv;
using namespace std;
using namespace chrono;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    darkSectioning(new DarkSectioning(ui)),
    currentFrame(0),
    isSyncMode(false)
{
    ui->setupUi(this);
    
    // 初始化Material组件
    // 1. 初始化开关控件
    m_toggle = new QtMaterialToggle(this);
    
    // 2. 初始化复选框控件
    m_checkbox = new QtMaterialCheckBox(this);
    
    // 3. 初始化文本输入框
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    m_textFields.append(new QtMaterialTextField(this));
    
    // 4. 初始化按钮
    m_runButton = new QtMaterialRaisedButton("Run Dark Sectioning", this);
    m_browseButton = new QtMaterialFlatButton("浏览", this);
    m_browseOutputButton = new QtMaterialFlatButton("浏览", this);
    
    // 5. 初始化图标按钮
    m_prevButton = new QtMaterialIconButton(QIcon(":/icons/prev.png"), this);
    m_nextButton = new QtMaterialIconButton(QIcon(":/icons/next.png"), this);
    m_prevButton2 = new QtMaterialIconButton(QIcon(":/icons/prev.png"), this);
    m_nextButton2 = new QtMaterialIconButton(QIcon(":/icons/next.png"), this);
    
    // 6. 初始化进度条
    m_progressBar = new QtMaterialProgress(this);
    m_progressBar->setRange(0, 100);
    
    // 7. 初始化滑块
    m_slider1 = new QtMaterialSlider(this);
    m_slider1->setRange(0, 0);
    m_slider2 = new QtMaterialSlider(this);
    m_slider2->setRange(0, 0);
    
    // 8. 初始化同步按钮
    m_syncButton = new QtMaterialRaisedButton("同步帧", this);
    
    // 信号槽绑定
    // 帧切换按钮
    connect(m_prevButton, &QtMaterialIconButton::clicked, this, &MainWindow::onFramePrev);
    connect(m_nextButton, &QtMaterialIconButton::clicked, this, &MainWindow::onFrameNext);
    connect(m_prevButton2, &QtMaterialIconButton::clicked, this, &MainWindow::onFramePrev2);
    connect(m_nextButton2, &QtMaterialIconButton::clicked, this, &MainWindow::onFrameNext2);
    
    // 同步按钮
    connect(m_syncButton, &QtMaterialRaisedButton::clicked, this, &MainWindow::onSyncFrames);
    
    // 滑块信号
    connect(m_slider1, &QtMaterialSlider::valueChanged, [this](int value) {
        currentFrame = value;
        if (isSyncMode) {
            m_slider2->setValue(value);
        }
        // 更新图像显示
        updateImageDisplay();
    });
    
    connect(m_slider2, &QtMaterialSlider::valueChanged, [this](int value) {
        currentFrame = value;
        if (isSyncMode) {
            m_slider1->setValue(value);
        }
        // 更新图像显示
        updateImageDisplay();
    });
}

MainWindow::~MainWindow()
{
    delete darkSectioning;
    delete ui;
    
    // 清理Material组件
    delete m_toggle;
    delete m_checkbox;
    qDeleteAll(m_textFields);
    delete m_runButton;
    delete m_browseButton;
    delete m_browseOutputButton;
    delete m_prevButton;
    delete m_nextButton;
    delete m_prevButton2;
    delete m_nextButton2;
    delete m_progressBar;
    delete m_slider1;
    delete m_slider2;
    delete m_syncButton;
}

void MainWindow::on_pushButton_clicked()
{
    ui->textEdit->append("开始图像处理...");
    darkSectioning->process();
    ui->textEdit->append("图像处理完成!");
    
    // 处理完成后显示滑块，隐藏进度条
    m_progressBar->hide();
    m_slider1->show();
    m_slider2->show();
    
    // 更新滑块范围
    if (!imageStack.empty()) {
        m_slider1->setRange(0, imageStack.size() - 1);
        m_slider2->setRange(0, final_images.size() - 1);
        m_slider1->setValue(0);
        m_slider2->setValue(0);
        currentFrame = 0;
        updateImageDisplay();
    }
}

void MainWindow::on_pushButton_browse_clicked()
{
    // 打开文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择输入图片",
        "",
        "图片文件 (*.tif *.tiff *.png *.jpg *.jpeg *.bmp);;所有文件 (*.*)"
    );

    if (!filePath.isEmpty()) {
        // 检查路径是否包含中文字符
        bool hasChinese = false;
        for (int i = 0; i < filePath.length(); i++) {
            QChar ch = filePath.at(i);
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                hasChinese = true;
                break;
            }
        }

        if (hasChinese) {
            ui->textEdit->append("警告: 路径包含中文字符，可能导致处理失败");
        }

        ui->lineEdit_inputPath->setText(filePath);
        ui->textEdit->append("已选择文件: " + filePath);
    }
}

void MainWindow::on_pushButton_browseOutput_clicked()
{
    // 打开文件夹选择对话框
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        "选择输出目录",
        ""
    );

    if (!folderPath.isEmpty()) {
        // 检查路径是否包含中文字符
        bool hasChinese = false;
        for (int i = 0; i < folderPath.length(); i++) {
            QChar ch = folderPath.at(i);
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                hasChinese = true;
                break;
            }
        }

        if (hasChinese) {
            ui->textEdit->append("警告: 路径包含中文字符，可能导致处理失败");
        }

        ui->lineEdit_outputPath->setText(folderPath);
        ui->textEdit->append("已选择输出目录: " + folderPath);
    }
}

// 项目自定义槽函数：处理帧切换
void MainWindow::onFramePrev()
{
    if (currentFrame > 0) {
        currentFrame--;
        m_slider1->setValue(currentFrame);
        if (isSyncMode) {
            m_slider2->setValue(currentFrame);
        }
        updateImageDisplay();
    }
}

void MainWindow::onFrameNext()
{
    if (currentFrame < imageStack.size() - 1) {
        currentFrame++;
        m_slider1->setValue(currentFrame);
        if (isSyncMode) {
            m_slider2->setValue(currentFrame);
        }
        updateImageDisplay();
    }
}

void MainWindow::onFramePrev2()
{
    if (currentFrame > 0) {
        currentFrame--;
        m_slider2->setValue(currentFrame);
        if (isSyncMode) {
            m_slider1->setValue(currentFrame);
        }
        updateImageDisplay();
    }
}

void MainWindow::onFrameNext2()
{
    if (currentFrame < final_images.size() - 1) {
        currentFrame++;
        m_slider2->setValue(currentFrame);
        if (isSyncMode) {
            m_slider1->setValue(currentFrame);
        }
        updateImageDisplay();
    }
}

// 项目自定义槽函数：处理同步按钮点击
void MainWindow::onSyncFrames()
{
    isSyncMode = !isSyncMode;
    if (isSyncMode) {
        // 同步处理前后图片的显示帧数
        m_slider1->setValue(currentFrame);
        m_slider2->setValue(currentFrame);
        m_syncButton->setText("同步已开启");
    } else {
        m_syncButton->setText("同步帧");
    }
}

// 辅助函数：更新图像显示
void MainWindow::updateImageDisplay()
{
    if (currentFrame >= 0 && currentFrame < imageStack.size()) {
        // 显示处理前图片
        Mat img = imageStack[currentFrame];
        QImage qimg(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
        QPixmap pixmap = QPixmap::fromImage(qimg);
        ui->label_original->setPixmap(pixmap.scaled(ui->label_original->size(), Qt::KeepAspectRatio));
    }
    
    if (currentFrame >= 0 && currentFrame < final_images.size()) {
        // 显示处理后图片
        Mat img = final_images[currentFrame];
        QImage qimg(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
        QPixmap pixmap = QPixmap::fromImage(qimg);
        ui->label_processed->setPixmap(pixmap.scaled(ui->label_processed->size(), Qt::KeepAspectRatio));
    }
}

//

