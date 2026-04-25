#include "darkSectioning.h"
#include <QStandardPaths>
#include <QApplication>  // 添加：用于保持UI响应
#include <QFileInfo>     // 添加：用于文件路径操作

// 外部函数声明
void separateHiLo(cv::Mat image, Params params, double deg, double divide, cv::Mat& Hi, cv::Mat& Lo, cv::Mat& lp, cv::Mat& EL);
int confirm_block(Params params, cv::Mat lp);
cv::Mat dehaze_fast2(cv::Mat image, double omega, int win_size, cv::Mat EL, double dep, int thres);

DarkSectioning::DarkSectioning(Ui::MainWindow *ui)
    : ui(ui)
{}

DarkSectioning::~DarkSectioning()
{}

void DarkSectioning::getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc)
{
    if (image.empty()) {
        Nx = 0;
        Ny = 0;
        Nc = 0;
        return;
    }
    Nx = image.rows;
    Ny = image.cols;
    Nc = image.channels();
}



void DarkSectioning::process()
{
    // 计时开始
    auto start = std::chrono::high_resolution_clock::now();

    // 预处理：多通道图像栈读取和处理
    // 清空之前的图像数据
    imageStack.clear();
    final_images.clear();
    
    // 读取图像栈（直接使用成员变量，供MainWindow访问）
    QString inputPathQt = ui->lineEdit_inputPath->text();
    if (inputPathQt.isEmpty()) {
        ui->textEdit_log->append("Error: 请先选择输入图片路径");
        return;
    }
    
    // 提取输入文件名（不含路径）
    QString image_name = QFileInfo(inputPathQt).fileName();
    
    // 检查输出目录
    QString outputPathQt = ui->lineEdit_outputPath->text();
    if (outputPathQt.isEmpty()) {
        // 设置默认输出目录为桌面
        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        ui->lineEdit_outputPath->setText(desktopPath);
        outputPathQt = desktopPath;  // 同时更新变量，避免后续使用空值
        ui->textEdit_log->append("没有选择输出目录，图片将输出到桌面");
    }

    std::string inputPath = inputPathQt.toStdString();
    bool success = cv::imreadmulti(inputPath, imageStack, cv::IMREAD_UNCHANGED);

    // 检查图像读取结果
    if (!success || imageStack.empty()) {
        std::cout << "图片读取失败" << std::endl;
        ui->textEdit_log->append("Error: Could not read input image stack");
        ui->textEdit_log->append(QString::fromStdString("Please make sure the input file exists: " + inputPath));
        return;
    } else {
        // 获取第一帧的通道数以判断彩色/灰度
        int channels = imageStack[0].channels();
        int frameCount = imageStack.size();
        std::string colorType = (channels == 1) ? "grayscale" : "color";
        std::cout << "Image loaded successfully: " << frameCount << " frames, " << colorType << " image" << std::endl;
    }

    // 获取图像栈信息
    int Nz = imageStack.size();
    int Nx0, Ny0, Nc;
    getImageDimensions(imageStack[0], Nx0, Ny0, Nc);

    // 检查图像尺寸是否有效
    if (Nx0 <= 0 || Ny0 <= 0 || Nc <= 0) {
        std::cout << "图像尺寸无效" << std::endl;
        ui->textEdit_log->append("Error: Invalid image dimensions");
        return;
    }

    int Nx = Nx0;
    int Ny = Ny0;

    ui->textEdit_log->append("Loaded image stack: " + QString::number(Nx0) + "x" +
                         QString::number(Ny0) + "x" + QString::number(Nz) +
                         " (channels: " + QString::number(Nc) + ")");

    // 保持UI响应：处理事件循环
    QApplication::processEvents();

    // 数据类型转换和归一化（对每一帧和每一通道）- 通道独立归一化
    // 彩色图片分别对B、G、R通道取最小最大值，分别归一化
    std::vector<std::vector<cv::Mat>> imageStack_processed(Nc, std::vector<cv::Mat>(Nz));
    for (int z = 0; z < Nz; z++) {
        if (Nc == 1) {
            // 单通道图像，直接归一化
            cv::Mat result;
            double minVal, maxVal;
            cv::minMaxLoc(imageStack[z], &minVal, &maxVal);
            imageStack[z].convertTo(result, CV_64F);
            if (maxVal > minVal) {
                result = 255 * (result - minVal) / (maxVal - minVal);
            }
            imageStack_processed[0][z] = result;
        } else {
            // 多通道图像（B、G、R），分离通道并分别归一化
            std::vector<cv::Mat> splitChannels;
            cv::split(imageStack[z], splitChannels);

            for (int c = 0; c < Nc; c++) {
                cv::Mat result;
                double minVal, maxVal;
                cv::minMaxLoc(splitChannels[c], &minVal, &maxVal);
                splitChannels[c].convertTo(result, CV_64F);
                if (maxVal > minVal) {
                    result = 255 * (result - minVal) / (maxVal - minVal);
                }
                imageStack_processed[c][z] = result;
            }
        }
    }

    // 保持UI响应：归一化完成后刷新界面
    QApplication::processEvents();

    // 维度校准（补0对齐）- 对每一帧和每一通道进行处理
    std::vector<std::vector<cv::Mat>> image0Stack = imageStack_processed;
    if (Ny > Nx) {
        Nx = Ny;
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                cv::Mat temp = cv::Mat::zeros(Ny, Ny, CV_64F);
                imageStack_processed[c][z].copyTo(temp(cv::Rect(0, 0, Ny0, Nx0)));
                image0Stack[c][z] = temp;
            }
        }
    } else if (Ny < Nx) {
        Ny = Nx;
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                cv::Mat temp = cv::Mat::zeros(Nx, Nx, CV_64F);
                imageStack_processed[c][z].copyTo(temp(cv::Rect(0, 0, Ny0, Nx0)));
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
    std::vector<std::vector<cv::Mat>> result_stack(Nc, std::vector<cv::Mat>(Nz));      //用于存储最终高低频融合后的结果图像数据
    std::vector<std::vector<cv::Mat>> Lo_process_stack(Nc, std::vector<cv::Mat>(Nz));  //用于存储经过去雾处理中的低频部分图像数据
    std::vector<std::vector<cv::Mat>> Hi_stack(Nc, std::vector<cv::Mat>(Nz));          //用于存储分离出的高频部分图像数据
    std::vector<std::vector<cv::Mat>> imageStack_padded(Nc, std::vector<cv::Mat>(Nz)); //用于存储经过边缘填充（pad）处理后的图像数据

    // 对每个通道和每个帧进行边缘填充
    for (int c = 0; c < Nc; c++) {
        for (int z = 0; z < Nz; z++) {
            if (pad == 1) {
                // 对称填充
                int pad_rows = std::floor(Nx / pad_size) + 1;
                int pad_cols = std::floor(Ny / pad_size) + 1;
                cv::copyMakeBorder(image0Stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, cv::BORDER_REFLECT);
            } else {
                // 零填充
                int pad_rows = std::floor(Nx / pad_size) + 1;
                int pad_cols = std::floor(Ny / pad_size) + 1;
                cv::copyMakeBorder(image0Stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, cv::BORDER_CONSTANT, cv::Scalar(0));
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
    std::vector<double> deg_matrix, dep_matrix, hl_matrix;

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
                cv::Mat Hi, Lo, lp, EL;
                separateHiLo(imageStack_padded[c][z], params, deg, divide, Hi, Lo, lp, EL);
                //imageStack_padded[c][z]是经过处理的单通道二维mat了,separateHiLo的输入输出后续都可以直接当灰白单帧图像处理

                //腐蚀核尺寸确定：暗通道通过形态学腐蚀得到，该步为形态学腐蚀核尺寸确定步骤
                int block_size = confirm_block(params, lp);

                // 暗通道去雾：对低频部分进行去雾处理
                cv::Mat Lo_process = dehaze_fast2(Lo, 0.95, block_size, EL, dep, thres);

                // 高低频融合：加权叠加融合
                cv::Mat result = Lo_process / hl + Hi;
                //其中hl是加权因子,用于低通滤波器的权重计算，在320行处定义

                // 后处理优化：边缘裁剪
                int crop_rows = std::floor(Nx / pad_size) + 1;
                int crop_cols = std::floor(Ny / pad_size) + 1;
                int start_row = crop_rows;
                int start_col = crop_cols;

                Lo_process = Lo_process(cv::Rect(start_col, start_row, Ny, Nx)).clone();
                Lo = Lo(cv::Rect(start_col, start_row, Ny, Nx)).clone();
                Hi = Hi(cv::Rect(start_col, start_row, Ny, Nx)).clone();
                result = result(cv::Rect(start_col, start_row, Ny, Nx)).clone();

                // 保存结果
                result_stack[c][z] = result.clone();
                Lo_process_stack[c][z] = Lo_process.clone();
                Hi_stack[c][z] = Hi.clone();
                
                // 保持UI响应：每处理完一帧刷新一次界面
                QApplication::processEvents();
            }
        }

        // 更新图像并重新填充边缘
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                if (pad == 1) {
                    // 对称填充
                    int pad_rows = std::floor(Nx / pad_size) + 1;
                    int pad_cols = std::floor(Ny / pad_size) + 1;
                    cv::copyMakeBorder(result_stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, cv::BORDER_REFLECT);
                } else {
                    // 零填充
                    int pad_rows = std::floor(Nx / pad_size) + 1;
                    int pad_cols = std::floor(Ny / pad_size) + 1;
                    cv::copyMakeBorder(result_stack[c][z], imageStack_padded[c][z], pad_rows, pad_rows, pad_cols, pad_cols, cv::BORDER_CONSTANT, cv::Scalar(0));
                }
            }
        } // 更新图像并重新填充边缘

    }// Dark sectioning 主处理流程

    // 后处理优化：多通道遍历，用高斯去噪（以后或许可以换成引导滤波去噪？）
    std::vector<std::vector<cv::Mat>> result_final(Nc, std::vector<cv::Mat>(Nz));
    for (int c = 0; c < Nc; c++) {
        for (int z = 0; z < Nz; z++) {
            if (denoise == 0) {
                // 不需要去噪，直接使用原始图像
                result_final[c][z] = result_stack[c][z].clone();
            } else {
                // 需要去噪，执行填充-去噪-裁剪流程
                cv::Mat temp;
                if (pad == 1) {
                    int pad_rows = std::floor(Nx / pad_size) + 1;
                    int pad_cols = std::floor(Ny / pad_size) + 1;
                    cv::copyMakeBorder(result_stack[c][z], temp, pad_rows, pad_rows, pad_cols, pad_cols, cv::BORDER_REFLECT_101);
                } else {
                    int pad_rows = std::floor(Nx / pad_size) + 1;
                    int pad_cols = std::floor(Ny / pad_size) + 1;
                    cv::copyMakeBorder(result_stack[c][z], temp, pad_rows, pad_rows, pad_cols, pad_cols, cv::BORDER_CONSTANT, cv::Scalar(0));
                }

                // 高斯去噪
                cv::GaussianBlur(temp, temp, cv::Size(2, 2), 1, 1, cv::BORDER_REPLICATE);

                // 边缘裁剪
                int crop_rows = std::floor(Nx / pad_size) + 1;
                int crop_cols = std::floor(Ny / pad_size) + 1;
                int start_row = crop_rows;
                int start_col = crop_cols;

                result_final[c][z] = temp(cv::Rect(start_col, start_row, Ny, Nx)).clone();
            }
        }
    }

    // 后处理优化：尺寸校准
    if (Nx0 != Nx || Ny0 != Ny) {
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                if (Nx > Nx0) {
                    result_final[c][z] = result_final[c][z](cv::Rect(0, 0, Ny0, Nx0)).clone();
                }
                if (Ny0 > Ny){
                    result_final[c][z] = result_final[c][z](cv::Rect(0,0,Ny0,Nx0)).clone();
                }
            }
        }
    }

    // 后处理优化：动态范围归一化和最终结果输出
    // 清空成员变量并保存结果到成员变量（供MainWindow访问）
    this->final_images.clear();
    
    for (int z = 0; z < Nz; z++) {
        if (Nc == 1) {
            // 单通道灰度图像
            double minVal, maxVal;
            cv::minMaxLoc(result_final[0][z], &minVal, &maxVal);
            cv::Mat final_image;
            result_final[0][z].convertTo(final_image, CV_16U, 65535.0 / maxVal);
            final_images.push_back(final_image);
        } else {
            // 多通道彩色图像，合并通道
            std::vector<cv::Mat> channels;
            for (int c = 0; c < Nc; c++) {
                double minVal, maxVal;
                cv::minMaxLoc(result_final[c][z], &minVal, &maxVal);
                cv::Mat channel;
                result_final[c][z].convertTo(channel, CV_16U, 65535.0 / maxVal);
                channels.push_back(channel);
            }
            cv::Mat final_image;
            cv::merge(channels, final_image);
            final_images.push_back(final_image);
        }
    }

    // 获取用户选择的输出目录
    std::string outputPath = outputPathQt.isEmpty() ? "D:/QtWorkSpace/DarkQt_V_3_8_2/output" : outputPathQt.toStdString();

    // 确保路径以斜杠结尾
    if (!outputPath.empty() && outputPath.back() != '/' && outputPath.back() != '\\') {
        outputPath += '/';
    }

    // 构建输出文件名：原始文件名 + _Darked.tif
    QString baseName = QFileInfo(image_name).baseName();  // 去除扩展名的文件名
    QString outputFileName = baseName + "_Darked.tif";
    std::string outputFileNameStd = outputFileName.toStdString();
    
    // 保存多页TIFF
    if (Nz == 1) {
        // 单帧图像，使用imwrite
        std::string savePath = outputPath + outputFileNameStd;
        cv::imwrite(savePath, final_images[0]);
        //暂时先默认存为tif，imwrite还可以存png，jpg文件，后面再加功能
    } else {
        // 多帧图像，使用imwritemulti，多帧只能存tif或tiff图像
        std::string savePath = outputPath + outputFileNameStd;
        bool success = cv::imwritemulti(savePath, final_images);
        if (success) {
            ui->textEdit_log->append("Successfully saved multi-frame TIFF");
        } else {
            ui->textEdit_log->append("Failed to save multi-frame TIFF, saving as individual files");
            // 保存失败时的备用方案
            std::string savePathFirst = outputPath + outputFileNameStd;
            cv::imwrite(savePathFirst, final_images[0]);
            for (int z = 1; z < Nz; z++) {
                std::string filename = outputPath + baseName.toStdString() + "_Darked_" + std::to_string(z) + ".tif";
                cv::imwrite(filename, final_images[z]);
            }
        }
    }

    // 输出保存路径信息
    ui->textEdit_log->append("Output directory: " + QString::fromStdString(outputPath));

    // 计时结束
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    ui->textEdit_log->append("Processing time: " + QString::number(duration.count()) + " ms");
    ui->textEdit_log->append("Processed " + QString::number(Nz) + " frames");
}
