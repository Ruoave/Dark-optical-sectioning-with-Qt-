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
#include "qtmaterialautocomplete.h"

using namespace cv;
using namespace std;
using namespace chrono;

// 函数声明
void separateHiLo(Mat image, Params params, double deg, double divide, Mat& Hi, Mat& Lo, Mat& lp, Mat& EL);
int confirm_block(Params params, Mat lp);
Mat dehaze_fast2(Mat image, double omega, int win_size, Mat EL, double dep, int thres);



// 辅助函数：安全地获取图像尺寸（自动处理单通道/三通道）
void getImageDimensions(const Mat& img, int& rows, int& cols, int& channels) {
    if (img.empty()) {
        rows = 0;
        cols = 0;
        channels = 0;
        return;
    }
    rows = img.rows;
    cols = img.cols;
    channels = img.channels();
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ui->textEdit->append("Starting Dark Sectioning...");
    darkSectioning();
    ui->textEdit->append("Dark Sectioning completed!");
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

void MainWindow::darkSectioning()
{

    // 计时开始
    auto start = high_resolution_clock::now();

    // 预处理：多通道图像栈读取和处理
    // 读取图像栈
    vector<Mat> imageStack;

    QString inputPathQt = ui->lineEdit_inputPath->text();
    if (inputPathQt.isEmpty()) {
        ui->textEdit->append("Error: 请先选择输入图片路径");
        return;
    }

    string inputPath = inputPathQt.toStdString();
    bool success = imreadmulti(inputPath, imageStack, IMREAD_UNCHANGED);


    // 检查图像读取结果
    if (!success || imageStack.empty()) {
        cout << "图片读取失败" << endl;
        ui->textEdit->append("Error: Could not read input image stack");
        ui->textEdit->append(QString::fromStdString("Please make sure the input file exists: " + inputPath));
        return;
    } else {
        // 获取第一帧的通道数以判断彩色/灰度
        int channels = imageStack[0].channels();
        int frameCount = imageStack.size();
        string colorType = (channels == 1) ? "grayscale" : "color";
        cout << "Image loaded successfully: " << frameCount << " frames, " << colorType << " image" << endl;
    }
    // 调试用：检查图像读取结果并输出详细信

    // 注意：已经在前面检查过success和imageStack，这里不需要重复检查

    // 获取图像栈信息
    int Nz = imageStack.size();
    int Nx0, Ny0, Nc;
    getImageDimensions(imageStack[0], Nx0, Ny0, Nc);

    // 检查图像尺寸是否有效
    if (Nx0 <= 0 || Ny0 <= 0 || Nc <= 0) {
        cout << "图像尺寸无效" << endl;
        ui->textEdit->append("Error: Invalid image dimensions");
        return;
    }

    int Nx = Nx0;
    int Ny = Ny0;

    ui->textEdit->append("Loaded image stack: " + QString::number(Nx0) + "x" +
                         QString::number(Ny0) + "x" + QString::number(Nz) +
                         " (channels: " + QString::number(Nc) + ")");

    // 数据类型转换和归一化（对每一帧和每一通道）- 通道独立归一化
    // 彩色图片分别对B、G、R通道取最小最大值，分别归一化
    vector<vector<Mat>> imageStack_processed(Nc, vector<Mat>(Nz));
    for (int z = 0; z < Nz; z++) {
        if (Nc == 1) {
            // 单通道图像，直接归一化
            Mat result;
            double minVal, maxVal;
            minMaxLoc(imageStack[z], &minVal, &maxVal);
            imageStack[z].convertTo(result, CV_64F);
            if (maxVal > minVal) {
                result = 255 * (result - minVal) / (maxVal - minVal);
            }
            imageStack_processed[0][z] = result;
        } else {
            // 多通道图像（B、G、R），分离通道并分别归一化
            vector<Mat> splitChannels;
            split(imageStack[z], splitChannels);

            for (int c = 0; c < Nc; c++) {
                Mat result;
                double minVal, maxVal;
                minMaxLoc(splitChannels[c], &minVal, &maxVal);
                splitChannels[c].convertTo(result, CV_64F);
                if (maxVal > minVal) {
                    result = 255 * (result - minVal) / (maxVal - minVal);
                }
                imageStack_processed[c][z] = result;
            }
        }
    }

    // 维度校准（补0对齐）- 对每一帧和每一通道进行处理
    vector<vector<Mat>> image0Stack = imageStack_processed;
    if (Ny > Nx) {
        Nx = Ny;
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                Mat temp = Mat::zeros(Ny, Ny, CV_64F);
                imageStack_processed[c][z].copyTo(temp(Rect(0, 0, Ny0, Nx0)));
                image0Stack[c][z] = temp;
            }
        }
    } else if (Ny < Nx) {
        Ny = Nx;
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                Mat temp = Mat::zeros(Nx, Nx, CV_64F);
                imageStack_processed[c][z].copyTo(temp(Rect(0, 0, Ny0, Nx0)));
                image0Stack[c][z] = temp;
            }
        }
    } else {
        // Ny == Nx，确保image0Stack中的矩阵不为空
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                if (image0Stack[c][z].empty()) {
                    image0Stack[c][z] = imageStack_processed[c][z].clone();
                }
            }
        }
    }

    //重建参数
    int background = 1; // 0-middle,1-severve
    int pad = 1;        //1-sysemtic对称填充,0-pad0零填充
    int denoise = 0;    // Guassion denoise，是否进行高斯去噪
    int thres = 70;     // Threshold to distinguish background and information，划分信息和背景的阈值
    double divide = 0.5; //划分高频/低频部分的边界

    // 预处理：通道级边缘填充
    int pad_size = 15; // 用于边缘渐变的填充大小
    vector<vector<Mat>> result_stack(Nc, vector<Mat>(Nz));      //用于存储最终高低频融合后的结果图像数据
    vector<vector<Mat>> Lo_process_stack(Nc, vector<Mat>(Nz));  //用于存储经过去雾处理中的低频部分图像数据
    vector<vector<Mat>> Hi_stack(Nc, vector<Mat>(Nz));          //用于存储分离出的高频部分图像数据
    vector<vector<Mat>> imageStack_padded(Nc, vector<Mat>(Nz)); //用于存储经过边缘填充（pad）处理后的图像数据


    // 对每个通道和每个帧进行边缘填充
    for (int c = 0; c < Nc; c++) {
        for (int z = 0; z < Nz; z++) {
            if (pad == 1) {
                // 对称填充
                int pad_rows = floor(Nx / pad_size) + 1;
                int pad_cols = floor(Ny / pad_size) + 1;
                copyMakeBorder(image0Stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, BORDER_REFLECT);
            } else {
                // 零填充
                int pad_rows = floor(Nx / pad_size) + 1;
                int pad_cols = floor(Ny / pad_size) + 1;
                copyMakeBorder(image0Stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, BORDER_CONSTANT, Scalar(0));
            }
        }
    }

    // 高低频分离：参数初始化
    Params params;
    if (!imageStack_padded.empty() && !imageStack_padded[0].empty()) {
        params.Nx = imageStack_padded[0][0].rows;
        params.Ny = imageStack_padded[0][0].cols;
    } else {
        params.Nx = Nx;
        params.Ny = Ny;
    }
    params.NA = 1.49;
    params.emwavelength = 610;
    params.pixelsize = 65;
    params.factor = 2;

    // 背景设置
    int maxtime;
    vector<double> deg_matrix, dep_matrix, hl_matrix;

    // 0-middle,1-severve
    if (background == 1) {
        maxtime = 2;
        deg_matrix = {6, 3, 1.2};
        dep_matrix = {3, 3, 2};
        hl_matrix = {1, 1, 1};
    } else if (background == 0) {
        maxtime = 1;
        deg_matrix = {6};
        dep_matrix = {3};
        hl_matrix = {1};
    }

    // Dark sectioning 主处理流程
    for (int time = 0; time < maxtime; time++) {
        // 参数设置
        double deg = deg_matrix[time];   // 3-10，极通滤波器的截止频率
        double dep = dep_matrix[time];   // 0.7-2，去雾阈值
        double hl = hl_matrix[maxtime - 1];    // 3-8，加权因子，用于低通滤波器的权重计算

        // 对每个通道和每一帧进行处理
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                // 高低频分离：分离频谱
                Mat Hi, Lo, lp, EL;
                separateHiLo(imageStack_padded[c][z], params, deg, divide, Hi, Lo, lp, EL);
                //imageStack_padded[c][z]是经过处理的单通道二维mat了,separateHiLo的输入输出后续都可以直接当灰白单帧图像处理

                //腐蚀核尺寸确定：暗通道通过形态学腐蚀得到，该步为形态学腐蚀核尺寸确定步骤
                int block_size = confirm_block(params, lp);

                // 暗通道去雾：对低频部分进行去雾处理
                Mat Lo_process = dehaze_fast2(Lo, 0.95, block_size, EL, dep, thres);

                // 高低频融合：加权叠加融合
                Mat result = Lo_process / hl + Hi;
                //其中hl是加权因子,用于低通滤波器的权重计算，在320行处定义

                // 后处理优化：边缘裁剪
                int crop_rows = floor(Nx / pad_size) + 1;
                int crop_cols = floor(Ny / pad_size) + 1;
                int start_row = crop_rows;
                int start_col = crop_cols;

                Lo_process = Lo_process(Rect(start_col, start_row, Ny, Nx)).clone();
                Lo = Lo(Rect(start_col, start_row, Ny, Nx)).clone();
                Hi = Hi(Rect(start_col, start_row, Ny, Nx)).clone();
                result = result(Rect(start_col, start_row, Ny, Nx)).clone();

                // 保存结果
                result_stack[c][z] = result.clone();
                Lo_process_stack[c][z] = Lo_process.clone();
                Hi_stack[c][z] = Hi.clone();
            }
        }

        // 更新图像并重新填充边缘
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                if (pad == 1) {
                    // 对称填充
                    int pad_rows = floor(Nx / pad_size) + 1;
                    int pad_cols = floor(Ny / pad_size) + 1;
                    copyMakeBorder(result_stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, BORDER_REFLECT);
                } else {
                    // 零填充
                    int pad_rows = floor(Nx / pad_size) + 1;
                    int pad_cols = floor(Ny / pad_size) + 1;
                    copyMakeBorder(result_stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, BORDER_CONSTANT, Scalar(0));
                }
            }
        } // 更新图像并重新填充边缘


    }// Dark sectioning 主处理流程
    // Dark sectioning 主处理流程


    ////////////////////////////////////////////////////检查以下步骤是否正确
    // 后处理优化：多通道遍历，用高斯去噪（以后或许可以换成引导滤波去噪？）
vector<vector<Mat>> result_final(Nc, vector<Mat>(Nz));
for (int c = 0; c < Nc; c++) {
    for (int z = 0; z < Nz; z++) {
        if (denoise == 0) {
            // 不需要去噪，直接使用原始图像
            result_final[c][z] = result_stack[c][z].clone();
        } else {
            // 需要去噪，执行填充-去噪-裁剪流程
            Mat temp;
            if (pad == 1) {
                int pad_rows = floor(Nx / pad_size) + 1;
                int pad_cols = floor(Ny / pad_size) + 1;
                copyMakeBorder(result_stack[c][z], temp, pad_rows, pad_rows, pad_cols, pad_cols, BORDER_REFLECT_101);
            } else {
                int pad_rows = floor(Nx / pad_size) + 1;
                int pad_cols = floor(Ny / pad_size) + 1;
                copyMakeBorder(result_stack[c][z], temp, pad_rows, pad_rows, pad_cols, pad_cols, BORDER_CONSTANT, Scalar(0));
            }

            // 高斯去噪
            GaussianBlur(temp, temp, Size(2, 2), 1, 1, BORDER_REPLICATE);

            // 边缘裁剪
            int crop_rows = floor(Nx / pad_size) + 1;
            int crop_cols = floor(Ny / pad_size) + 1;
            int start_row = crop_rows;
            int start_col = crop_cols;

            result_final[c][z] = temp(Rect(start_col, start_row, Ny, Nx)).clone();
        }
    }
}

    // 后处理优化：尺寸校准
    if (Nx0 != Nx || Ny0 != Ny) {
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                if (Nx > Nx0) {
                    result_final[c][z] = result_final[c][z](Rect(0, 0, Ny0, Nx0)).clone();
                }
                if (Ny0 > Ny){
                    result_final[c][z] = result_final[c][z](Rect(0,0,Ny0,Nx0)).clone();
                }
            }
        }
    }
    //ViewMat(result_final[0][0], "result_final_after");

    // 后处理优化：动态范围归一化和最终结果输出
    // 保存结果（多页TIFF）
    vector<Mat> final_images;
    for (int z = 0; z < Nz; z++) {
        if (Nc == 1) {
            // 单通道灰度图像
            double minVal, maxVal;
            minMaxLoc(result_final[0][z], &minVal, &maxVal);
            Mat final_image;
            result_final[0][z].convertTo(final_image, CV_16U, 65535.0 / maxVal);
            final_images.push_back(final_image);
        } else {
            // 多通道彩色图像，合并通道
            vector<Mat> channels;
            for (int c = 0; c < Nc; c++) {
                double minVal, maxVal;
                minMaxLoc(result_final[c][z], &minVal, &maxVal);
                Mat channel;
                result_final[c][z].convertTo(channel, CV_16U, 65535.0 / maxVal);
                channels.push_back(channel);
            }
            Mat final_image;
            merge(channels, final_image);
            final_images.push_back(final_image);
        }
    }

    // 获取用户选择的输出目录
    QString outputPathQt = ui->lineEdit_outputPath->text();
    string outputPath = outputPathQt.isEmpty() ? "D:/QtWorkSpace/DarkQt_V_3_8_2/output" : outputPathQt.toStdString();

    // 确保路径以斜杠结尾
    if (!outputPath.empty() && outputPath.back() != '/' && outputPath.back() != '\\') {
        outputPath += '/';
    }

    // 保存多页TIFF
    if (Nz == 1) {
        // 单帧图像，使用imwrite
        string savePath = outputPath + "Dark.tif";
        imwrite(savePath, final_images[0]);
        //暂时先默认存为tif，imwrite还可以存png，jpg文件，后面再加功能
    } else {
        // 多帧图像，使用imwritemulti，多帧只能存tif或tiff图像
        string savePath = outputPath + "Dark.tif";
        bool success = imwritemulti(savePath, final_images);
        if (success) {
            ui->textEdit->append("Successfully saved multi-frame TIFF");
        } else {
            ui->textEdit->append("Failed to save multi-frame TIFF, saving as individual files");
            // 保存失败时的备用方案
            string savePathFirst = outputPath + "Dark.tif";
            imwrite(savePathFirst, final_images[0]);
            for (int z = 1; z < Nz; z++) {
                string filename = outputPath + "Dark_" + to_string(z) + ".tif";
                imwrite(filename, final_images[z]);
            }
        }
    }

    // 输出保存路径信息
    ui->textEdit->append("Output directory: " + QString::fromStdString(outputPath));

    // 计时结束
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    ui->textEdit->append("Processing time: " + QString::number(duration.count()) + " ms");
    ui->textEdit->append("Processed " + QString::number(Nz) + " frames");
}
//

