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
#include "qtmaterialautocomplete.h"

using namespace cv;
using namespace std;
using namespace chrono;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    darkSectioning(new DarkSectioning(ui))
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete darkSectioning;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ui->textEdit->append("开始图像处理...");
    darkSectioning->process();
    ui->textEdit->append("图像处理完成!");
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


//

