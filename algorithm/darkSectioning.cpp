#include "darkSectioning.h"
#include <QStandardPaths>
#include <QApplication>  // 添加：用于保持UI响应
#include <QFileInfo>     // 添加：用于文件路径操作
#include <QDebug>        // 添加：用于调试输出

#include "ViewMat.h"

//// 外部函数声明
void separateHiLo(cv::Mat image, ParamsBasic paramsBasicSet, double deg, double divide, cv::Mat& Hi, cv::Mat& Lo, cv::Mat& lp, cv::Mat& EL);
int confirm_block(ParamsBasic paramsBasicSet, cv::Mat lp);
cv::Mat dehaze_fast2(cv::Mat image, double omega, int win_size, cv::Mat EL, double dep, int thres);

DarkSectioning::DarkSectioning(Ui::MainWindow *ui)
    : ui(ui), m_orangeBar(nullptr), progressValue_calcu(0), progressValue_step(0)
{}

DarkSectioning::~DarkSectioning()
{}

void DarkSectioning::setOrangeBar(OrangeBar *bar)
{
    m_orangeBar = bar;
}

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


        //重建参数：从 paramsBasicSet 成员变量读取（由 MainWindow 在运行前从 GreenWidget 控件设置）
    int background = paramsBasicSet.background; // 0-离焦不严重, 1-离焦严重
    int pad = paramsBasicSet.pad;               // 1-对称填充, 0-零填充
    int denoise = paramsBasicSet.denoise;       // 0-不去噪, 1-高斯平滑, 2-中值滤波

    int thres = paramsExpertSet.thres;   // 划分信息和背景的阈值;荧光信号越强，阈值要越高
    double divide = paramsExpertSet.divide; //划分高频/低频部分的边界;基本不用调
    int pad_size = paramsExpertSet.padsize; // padsize用于边缘渐变的填充大小
    int isQuick = paramsExpertSet.isQuick;  // 单帧处理模式：0-多帧处理（默认），1-单帧快速处理

    // 背景设置
    int maxtime;
    std::vector<double> deg_matrix, dep_matrix, hl_matrix;

    // 0-middle,1-severve
    if (background == 1) {
        maxtime = 2;
        deg_matrix = parseExpertVector(paramsExpertSet.deg);
        dep_matrix = parseExpertVector(paramsExpertSet.dep);
        hl_matrix = parseExpertVector(paramsExpertSet.hl);
    } else if (background == 0) {
        maxtime = 1;
        deg_matrix = parseExpertVector(paramsExpertSet.deg);
        dep_matrix = parseExpertVector(paramsExpertSet.dep);
        hl_matrix = parseExpertVector(paramsExpertSet.hl);
    }

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
        ui->textEdit_log->append("Error: 无法读取输入图片栈");
    // 检查输入路径是否含有中文字符
    // 遍历路径字符串中的每个字符，若Unicode码点超出ASCII范围（>127）则判定为非ASCII字符（如中文）
        bool hasChinese = false;
        for (const QChar &ch : inputPathQt) {
            if (ch.unicode() > 127) {
                hasChinese = true;
                break;
            }
        }
        if (hasChinese) {
            ui->textEdit_log->append("Error: 输入目录或文件名中含有中文，请选择纯英文路径");
            return;
        }
        else{
            ui->textEdit_log->append(QString::fromStdString("Error: 请确定输入图片是否已损坏: " + inputPath));
        }
        return;
    } 

    // 获取图像栈信息
    int Nz0 = imageStack.size();
    int Nx0, Ny0, Nc;
    getImageDimensions(imageStack[0], Nx0, Ny0, Nc);

    // 检查图像尺寸是否有效
    if (Nx0 <= 0 || Ny0 <= 0 || Nc <= 0) {
        std::cout << "图像尺寸无效" << std::endl;
        ui->textEdit_log->append("Error: 图片尺寸无效");
        return;
    }

    int Nx = Nx0;
    int Ny = Ny0;
    int Nz;
    if (isQuick == 1) {
        Nz = 1;    // 单帧处理模式：只处理第一帧
        ui->textEdit_log->append("选择了单帧处理模式，将只处理第一帧");
    } else {
        Nz = Nz0;  // 默认处理模式：处理全部帧
    }

    ui->textEdit_log->append("Dark-based optical Sectioning算法已成功接收到图像: " + QString::number(Nx0) + "x" +
                         QString::number(Ny0) + ", " + QString::number(Nz) + " 帧" +
                         " (通道数: " + QString::number(Nc) + ")");

    // 初始化进度计算参数（maxtime、Nc、Nz在此处均已获取）
    progressValue_calcu = 0;
    progressValue_step = 87.0 / (maxtime * Nc * Nz);

    // 保持UI响应：处理事件循环
    QApplication::processEvents();

    // 数据类型转换和归一化（对每一帧和每一通道）- 通道独立归一化
    // 彩色图片分别对B、G、R通道取最小最大值，分别归一化
    std::vector<std::vector<cv::Mat>> imageStack_processed(Nc, std::vector<cv::Mat>(Nz));
    for (int z = 0; z < Nz; z++) {
        // 单通道图像，直接归一化
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

    //进度提示，后面可据此更新progressBar
    ui->textEdit_log->append("归一化完成..." );
    //更新进度条
    progressValue_calcu = 3;
    m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
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

    imageStack_processed.clear();
    imageStack_processed.shrink_to_fit();

    // 进度提示，后面可据此更新progressBar
    ui->textEdit_log->append("维度校准完成..." );
    //更新进度条
    progressValue_calcu = 6;
    m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
    QApplication::processEvents();



    // 预处理：通道级边缘填充
    // pad_size 已在函数开头从 paramsExpertSet.padsize 读取， padsize用于边缘渐变的填充大小
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

    image0Stack.clear();
    image0Stack.shrink_to_fit();

    // 进度提示，后面可据此更新progressBar
    ui->textEdit_log->append("边缘填充完成..." );
    //更新进度条
    progressValue_calcu = 9;
    m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
    QApplication::processEvents();

    // 高低频分离：参数初始化（使用 paramsBasicSet 成员变量，NA/emwavelength/pixelsize/factor 已由 MainWindow 设置）
    paramsBasicSet.Nx = imageStack_padded[0][0].rows;
    paramsBasicSet.Ny = imageStack_padded[0][0].cols;

    // Dark sectioning 主处理流程
    for (int time = 0; time < maxtime; time++) {
        // 参数设置
        double deg = deg_matrix[time];   // 3-10，极通滤波器的截止频率
        double dep = dep_matrix[time];   // 0.7-2，去雾阈值
        double hl = hl_matrix[maxtime - 1];    // 3-8，加权因子，用于低通滤波器的权重计算
        // 进度提示，后面可据此更新progressBar
        ui->textEdit_log->append("正在第 " + QString::number(time + 1) + "/" + QString::number(maxtime) + " 次处理..."); 
        QApplication::processEvents();
        // 对每个通道和每一帧进行处理
        for (int c = 0; c < Nc; c++) {
            // 进度提示，后面可据此更新progressBar
            ui->textEdit_log->append("正在处理第 " + QString::number(c + 1) + "/" + QString::number(Nc) + " 个通道..."); 
            QApplication::processEvents();
            for (int z = 0; z < Nz; z++) {
                // 进度提示，后面可据此更新progressBar
                ui->textEdit_log->append("正在处理第 " + QString::number(z + 1) + "/" + QString::number(Nz) + " 帧..."); 
                QApplication::processEvents();
                // 高低频分离：分离频谱
                cv::Mat Hi, Lo, lp, EL;
                separateHiLo(imageStack_padded[c][z], paramsBasicSet, deg, divide, Hi, Lo, lp, EL);
                //imageStack_padded[c][z]是经过处理的单通道二维mat了,separateHiLo的输入输出后续都可以直接当灰白单帧图像处理

                //腐蚀核尺寸确定：暗通道通过形态学腐蚀得到，该步为形态学腐蚀核尺寸确定步骤
                int block_size = confirm_block(paramsBasicSet, lp);

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
                
                //更新进度条
                progressValue_calcu += progressValue_step;
                m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
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
    ////////////////////////// Dark sectioning 主处理流程结束于此/////////////////////////////////////

        imageStack_padded.clear();
        imageStack_padded.shrink_to_fit();
        Lo_process_stack.clear();
        Lo_process_stack.shrink_to_fit();
        Hi_stack.clear();
        Hi_stack.shrink_to_fit();

        progressValue_calcu = 96;
        m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
        QApplication::processEvents();


    // 后处理优化：多通道遍历，用高斯去噪（以后或许可以换成引导滤波去噪？）
    std::vector<std::vector<cv::Mat>> result_final(Nc, std::vector<cv::Mat>(Nz));
    for (int c = 0; c < Nc; c++) {
        for (int z = 0; z < Nz; z++) {
            if (denoise == 0) {
                // 不需要去噪，直接使用原始图像
                result_final[c][z] = result_stack[c][z].clone();
            } else if(denoise == 1){
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
                cv::GaussianBlur(temp, temp, cv::Size(3, 3), 1, 1, cv::BORDER_REPLICATE);

                // 边缘裁剪
                int crop_rows = std::floor(Nx / pad_size) + 1;
                int crop_cols = std::floor(Ny / pad_size) + 1;
                int start_row = crop_rows;
                int start_col = crop_cols;

                result_final[c][z] = temp(cv::Rect(start_col, start_row, Ny, Nx)).clone();
            } else if(denoise == 2) {
                result_final[c][z] = result_stack[c][z].clone();
            }
        }
    }

    result_stack.clear();
    result_stack.shrink_to_fit();

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

    // 进度提示，后面可据此更新progressBar
    ui->textEdit_log->append("后处理优化完成..." );
    //更新进度条
    progressValue_calcu = 99;
    m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
    QApplication::processEvents();

    // 后处理优化：动态范围归一化和最终结果输出
    // 清空成员变量并保存结果到成员变量（供MainWindow访问）
    this->final_images.clear();
    
    bool mdbutf_logged = false; // 避免每帧重复打印
    for (int z = 0; z < Nz; z++) {
        if (Nc == 1) {
            // 单通道灰度图像
            double minVal, maxVal;
            cv::minMaxLoc(result_final[0][z], &minVal, &maxVal);
            cv::Mat final_image;
            result_final[0][z].convertTo(final_image, CV_16U, 65535.0 / maxVal);
            
            // denoise == 2：在范围转换后直接应用 MDBUTMF
            if (denoise == 2) {
                if (!mdbutf_logged) {
                    ui->textEdit_log->append("正在执行 MDBUTMF 改进中值滤波...");
                    mdbutf_logged = true;
                }
                final_image = applyMDBUTMF(final_image);
            }
            
            final_images.push_back(final_image);
        } else {
            // 多通道彩色图像，合并通道
            std::vector<cv::Mat> channels;
            for (int c = 0; c < Nc; c++) {
                double minVal, maxVal;
                cv::minMaxLoc(result_final[c][z], &minVal, &maxVal);
                cv::Mat channel;
                result_final[c][z].convertTo(channel, CV_16U, 65535.0 / maxVal);
                
                // denoise == 2：在范围转换后直接对每个通道应用 MDBUTMF
                if (denoise == 2) {
                    if (!mdbutf_logged) {
                        ui->textEdit_log->append("正在执行 MDBUTMF 改进中值滤波...");
                        mdbutf_logged = true;
                    }
                    channel = applyMDBUTMF(channel);
                }
                
                channels.push_back(channel);
            }
            cv::Mat final_image;
            cv::merge(channels, final_image);
            final_images.push_back(final_image);
        }
    }

    // denoise == 2：打印完成日志
    if (denoise == 2) {
        ui->textEdit_log->append("MDBUTMF 改进中值滤波完成");
    }

    result_final.clear();
    result_final.shrink_to_fit();

    // 获取用户选择的输出目录
    std::string outputPath = outputPathQt.toStdString();

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
        // 单帧图像，使用imwrite，可以输出多种格式图像，这是为悬着图片保存格式留的接口
        std::string savePath = outputPath + outputFileNameStd;
        bool success = cv::imwrite(savePath, final_images[0]);
        if (success) {
            ui->textEdit_log->append("成功保存单帧TIFF图像");
        } else {
            // 检查输出路径是否含有中文字符，给出可能的失败原因提示
            bool hasChinese = false;
            for (const QChar &ch : outputPathQt) {
                if (ch.unicode() > 127) {
                    hasChinese = true;
                    break;
                }
            }
            if (hasChinese) {
                ui->textEdit_log->append("Error: 输出目录中含有中文，请选择纯英文路径");
            }
            ui->textEdit_log->append("保存单帧TIFF图像失败");
        }
        //暂时先默认存为tif，imwrite还可以存png，jpg文件，后面再加功能
    } else {
        // 多帧图像，使用imwritemulti，多帧只能存tif或tiff图像
        std::string savePath = outputPath + outputFileNameStd;
        bool success = cv::imwritemulti(savePath, final_images);
        if (success) {
            ui->textEdit_log->append("成功保存多帧TIFF图像");
        } else {
            // 检查输出路径是否含有中文字符，给出可能的失败原因提示
            bool hasChinese = false;
            for (const QChar &ch : outputPathQt) {
                if (ch.unicode() > 127) {
                    hasChinese = true;
                    break;
                }
            }
            if (hasChinese) {
                ui->textEdit_log->append("Error: 输出目录中含有中文，请选择纯英文路径");
            }
            else{
             ui->textEdit_log->append("保存多帧TIFF图像失败");
             }
        }
    }

    // 输出保存路径信息
    ui->textEdit_log->append("输出路径： " + QString::fromStdString(outputPath));
    ui->textEdit_log->append("图像处理完成!");
    //更新进度条
    progressValue_calcu = 100;
    m_orangeBar->setProgress(static_cast<int>(progressValue_calcu));
    QApplication::processEvents();

    // 计时结束
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    ui->textEdit_log->append("Processing time: " + QString::number(duration.count()) + " ms");
    ui->textEdit_log->append("Processed " + QString::number(Nz) + " frames");
}

// ========== MDBUTMF 改进中值滤波实现 ==========

// 辅助函数：获取窗口内所有像素值（用于16位深 0-65535）
std::vector<ushort> DarkSectioning::getWindowValues(const cv::Mat &input, int i, int j, int halfWin) {
    std::vector<ushort> values;
    for (int di = -halfWin; di <= halfWin; di++) {
        for (int dj = -halfWin; dj <= halfWin; dj++) {
            int ni = i + di;
            int nj = j + dj;
            // 边界处理：超出边界时用边缘像素填充
            ni = std::max(0, std::min(ni, input.rows - 1));
            nj = std::max(0, std::min(nj, input.cols - 1));
            values.push_back(input.at<ushort>(ni, nj));
        }
    }
    return values;
}

// 辅助函数：计算窗口均值
double DarkSectioning::getWindowMean(const cv::Mat &input, int i, int j, int halfWin) {
    std::vector<ushort> values = getWindowValues(input, i, j, halfWin);
    double sum = 0.0;
    for (ushort v : values) {
        sum += v;
    }
    return sum / values.size();
}

// MDBUTMF 主函数：改进的中值滤波（适配16位深 0-65535）
cv::Mat DarkSectioning::applyMDBUTMF(const cv::Mat &input, int windowSize) {
    CV_Assert(input.depth() == CV_16U && input.channels() == 1);
    
    cv::Mat output = input.clone();
    int halfWin = windowSize / 2;
    
    for (int i = 0; i < input.rows; i++) {
        for (int j = 0; j < input.cols; j++) {
            ushort p = input.at<ushort>(i, j);
            
            if (p > 0 && p < 65535) {
                output.at<ushort>(i, j) = static_cast<ushort>(getWindowMean(input, i, j, halfWin));
            } else {
                std::vector<ushort> window = getWindowValues(input, i, j, halfWin);
                
                bool allExtreme = true;
                for (ushort v : window) {
                    if (v > 0 && v < 65535) {
                        allExtreme = false;
                        break;
                    }
                }
                
                if (allExtreme) {
                    output.at<ushort>(i, j) = static_cast<ushort>(getWindowMean(input, i, j, halfWin));
                } else {
                    std::vector<ushort> filtered;
                    for (ushort v : window) {
                        if (v > 0 && v < 65535) {
                            filtered.push_back(v);
                        }
                    }
                    
                    std::sort(filtered.begin(), filtered.end());
                    int midIdx = filtered.size() / 2;
                    output.at<ushort>(i, j) = filtered[midIdx];
                }
            }
        }
    }
    
    return output;
}
