#include "darkSectioning_cleanForBatch.h"  // 改为包含批量处理专用头文件（内含 DarkSectioningBatch 类声明）
#include <QStandardPaths>                    // 用于获取桌面路径（默认输出目录）
#include <QTextEdit>                         // 用于 batchLog() 输出日志到 textEdit_batchLog
#include <QFileInfo>                         // 用于 QFileInfo 提取文件名/baseName/image_name

// ========== 外部算法函数声明（与原文件 darkSectioning.cpp 完全相同，不可修改）==========
// 函数功能：将图像分离为高频（Hi）和低频（Lo）两个部分
// 参数说明：image-输入图像, paramsBasicSet-基本参数, deg-分离度, divide-分界值, Hi/Lo-输出高频/低频, lp-低通滤波结果, EL-边缘图
void separateHiLo(cv::Mat image, ParamsBasic paramsBasicSet, double deg, double divide, cv::Mat& Hi, cv::Mat& Lo, cv::Mat& lp, cv::Mat& EL);
// 函数功能：确认腐蚀核尺寸（暗通道形态学处理用）
int confirm_block(ParamsBasic paramsBasicSet, cv::Mat lp);
// 函数功能：对低频部分执行快速暗通道去雾
cv::Mat dehaze_fast2(cv::Mat image, double omega, int win_size, cv::Mat EL, double dep, int thres);

// ========== 构造函数：调用父类构造并传入 nullptr（批量处理完全无UI依赖）==========
DarkSectioningBatch::DarkSectioningBatch()
    : DarkSectioning(nullptr)  // 传入 nullptr → 父类 ui 指针为 null，所有 UI 操作由 std::cout 替代
{}

// ========== 析构函数 ==========
DarkSectioningBatch::~DarkSectioningBatch()
{}

// ========== 一次性初始化批量处理参数（所有文件共用，不必每次重新读取）==========
// 调用时机：BatchDialog 在注入 paramsBasicSet/paramsExpertSet 后、循环处理前调用一次
// 功能：从父类成员变量读取参数值并缓存到本类成员变量
void DarkSectioningBatch::initBatchParams()
{
    // 从父类 paramsBasicSet 读取基本参数并缓存
    m_background = paramsBasicSet.background; // 背景类型：0-离焦不严重，1-离焦严重
    m_pad        = paramsBasicSet.pad;        // 填充方式：0-零填充，1-对称填充
    m_denoise    = paramsBasicSet.denoise;    // 去噪方式：0-不去噪，1-高斯平滑，2-中值滤波

    // 从父类 paramsExpertSet 读取高级参数并缓存
    m_thres      = paramsExpertSet.thres;     // 划分信息和背景的阈值（荧光信号越强阈值越高）
    m_divide     = paramsExpertSet.divide;    // 划分高频/低频部分的边界值
    m_padSize    = paramsExpertSet.padsize;   // 边缘渐变填充大小
    m_isQuick    = paramsExpertSet.isQuick;   // 单帧处理模式：0-多帧处理（批量固定），1-单帧快速
}

// ========== 统一日志输出函数 ==========
// 若 m_logWidget 不为空 → 输出到 BatchDialog 的 textEdit_batchLog
// 若 m_logWidget 为空 → 回退到 std::cout（调试/独立运行场景）
// 参数：message - 要输出的日志文本（QString 类型）
void DarkSectioningBatch::batchLog(const QString &message)
{
    if (m_logWidget) {
        m_logWidget->append(message);           // 输出到 BatchDialog 的文本日志控件
    } else {
        std::cout << message.toStdString() << std::endl;  // 回退到标准输出
    }
}

// ========== 辅助函数：获取图像尺寸（与原版算法完全相同，仅类名前缀改为 DarkSectioningBatch）==========
void DarkSectioningBatch::getImageDimensions(const cv::Mat &image, int &Nx, int &Ny, int &Nc)
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

// ========== 核心处理函数：处理单张图片（算法逻辑完全不变，仅 UI 交互改为 std::cout 输出）==========
void DarkSectioningBatch::process()
{
    // 计时开始
    auto start = std::chrono::high_resolution_clock::now();

    // ========== 使用 initBatchParams() 预先缓存的参数（所有文件共用，避免每次重复读取）==========
    int background = m_background; // 0-离焦不严重, 1-离焦严重
    int pad = m_pad;               // 1-对称填充, 0-零填充
    int denoise = m_denoise;       // 0-不去噪, 1-高斯平滑, 2-中值滤波

    int thres = m_thres;           // 划分信息和背景的阈值;荧光信号越强，阈值要越高
    double divide = m_divide;      // 划分高频/低频部分的边界;基本不用调
    int pad_size = m_padSize;      // padsize用于边缘渐变的填充大小
    int isQuick = m_isQuick;       // 单帧处理模式：0-多帧处理（默认），1-单帧快速处理

    // ========== 背景设置 ==========
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

    // ========== 预处理：多通道图像栈读取和处理 ==========
    // 清空之前的图像数据
    imageStack.clear();
    final_images.clear();

    // ========== 从 m_inputPath 成员变量读取输入路径（替代原 ui->lineEdit_inputPath->text()）==========
    QString inputPathQt = m_inputPath;
    if (inputPathQt.isEmpty()) {
        batchLog(QString::fromUtf8("[ERROR-1] 输入图片路径为空，请先调用 setInputPath() 设置路径"));
        return;
    }

    // 提取输入文件名（不含路径）
    QString image_name = QFileInfo(inputPathQt).fileName();

    // ========== 使用 m_outputPath 成员变量获取输出目录 ==========
    QString outputPathQt = m_outputPath;
    if (outputPathQt.isEmpty()) {
        // 设置默认输出目录为桌面
        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        outputPathQt = desktopPath;  // 使用桌面作为默认输出路径
        batchLog(QString::fromUtf8("没有选择输出目录，图片将输出到桌面: ") + desktopPath);
    }

    std::string inputPath = inputPathQt.toStdString();
    bool success = cv::imreadmulti(inputPath, imageStack, cv::IMREAD_UNCHANGED);

    // ========== 检查图像读取结果 ==========
    if (!success || imageStack.empty()) {
        batchLog(QString::fromUtf8("[ERROR] 图片读取失败（imreadmulti 返回 false 或图像栈为空）"));
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
            batchLog(QString::fromUtf8("[ERROR-2] 图片读取失败：输入路径含有中文字符，请选择纯英文路径"));
            return;
        }
        else {
            batchLog(QString::fromUtf8("[ERROR-3] 图片读取失败：文件可能已损坏 → ") + QString::fromStdString(inputPath));
        }
        return;
    }

    // ========== 获取图像栈信息 ==========
    int Nz0 = imageStack.size();
    int Nx0, Ny0, Nc;
    getImageDimensions(imageStack[0], Nx0, Ny0, Nc);

    // 检查图像尺寸是否有效
    if (Nx0 <= 0 || Ny0 <= 0 || Nc <= 0) {
        batchLog(QString::fromUtf8("[ERROR-4] 图像尺寸无效：Nx0=%1, Ny0=%2, Nc=%3").arg(Nx0).arg(Ny0).arg(Nc));
        return;
    }

    int Nx = Nx0;
    int Ny = Ny0;
    int Nz = Nz0;  // 批量处理固定使用全帧模式（不使用单帧快速预览）

    // ========== 数据类型转换和归一化（对每一帧和每一通道）- 通道独立归一化 ==========
    // 彩色图片分别对B、G、R通道取最小最大值，分别归一化
    std::vector<std::vector<cv::Mat>> imageStack_processed(Nc, std::vector<cv::Mat>(Nz));
    for (int z = 0; z < Nz; z++) {
        // 单通道图像，直接归一化
        if (Nc == 1) {
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

    // ========== 维度校准（补0对齐）- 对每一帧和每一通道进行处理 ==========
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

    // ========== 预处理：通道级边缘填充 ==========
    // pad_size 已在函数开头从 paramsExpertSet.padsize 读取，padsize用于边缘渐变的填充大小
    std::vector<std::vector<cv::Mat>> result_stack(Nc, std::vector<cv::Mat>(Nz));      // 用于存储最终高低频融合后的结果图像数据
    std::vector<std::vector<cv::Mat>> Lo_process_stack(Nc, std::vector<cv::Mat>(Nz));  // 用于存储经过去雾处理中的低频部分图像数据
    std::vector<std::vector<cv::Mat>> Hi_stack(Nc, std::vector<cv::Mat>(Nz));          // 用于存储分离出的高频部分图像数据
    std::vector<std::vector<cv::Mat>> imageStack_padded(Nc, std::vector<cv::Mat>(Nz)); // 用于存储经过边缘填充（pad）处理后的图像数据

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

    // ========== 高低频分离：参数初始化 ==========
    paramsBasicSet.Nx = imageStack_padded[0][0].rows;
    paramsBasicSet.Ny = imageStack_padded[0][0].cols;

    // ========== Dark sectioning 主处理流程 ==========
    for (int time = 0; time < maxtime; time++) {
        // 参数设置
        double deg = deg_matrix[time];
        double dep = dep_matrix[time];
        double hl = hl_matrix[maxtime - 1];

        // 对每个通道和每一帧进行处理
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                // 高低频分离：分离频谱
                cv::Mat Hi, Lo, lp, EL;
                separateHiLo(imageStack_padded[c][z], paramsBasicSet, deg, divide, Hi, Lo, lp, EL);
                // imageStack_padded[c][z]是经过处理的单通道二维mat了,separateHiLo的输入输出后续都可以直接当灰白单帧图像处理

                // 腐蚀核尺寸确定：暗通道通过形态学腐蚀得到，该步为形态学腐蚀核尺寸确定步骤
                int block_size = confirm_block(paramsBasicSet, lp);

                // 暗通道去雾：对低频部分进行去雾处理
                cv::Mat Lo_process = dehaze_fast2(Lo, 0.95, block_size, EL, dep, thres);

                // 高低频融合：加权叠加融合
                cv::Mat result = Lo_process / hl + Hi;
                // 其中hl是加权因子,用于低通滤波器的权重计算

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

    } // Dark sectioning 主处理流程
    ////////////////////////// Dark sectioning 主处理流程结束于此/////////////////////////////////////

    // ========== 后处理优化：多通道遍历，用所选方式去噪 ==========
    std::vector<std::vector<cv::Mat>> result_final(Nc, std::vector<cv::Mat>(Nz));
    for (int c = 0; c < Nc; c++) {
        for (int z = 0; z < Nz; z++) {
            if (denoise == 0) {
                // 不需要去噪，直接使用原始图像
                result_final[c][z] = result_stack[c][z].clone();
            } else if (denoise == 1) {
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
            } else if (denoise == 2) {
                // 中值滤波去噪，执行填充-去噪-裁剪流程
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

                // 中值滤波去噪
                cv::medianBlur(temp, temp, 3);

                // 边缘裁剪
                int crop_rows = std::floor(Nx / pad_size) + 1;
                int crop_cols = std::floor(Ny / pad_size) + 1;
                int start_row = crop_rows;
                int start_col = crop_cols;

                result_final[c][z] = temp(cv::Rect(start_col, start_row, Ny, Nx)).clone();
            }
        }
    }

    // ========== 后处理优化：尺寸校准 ==========
    if (Nx0 != Nx || Ny0 != Ny) {
        for (int c = 0; c < Nc; c++) {
            for (int z = 0; z < Nz; z++) {
                if (Nx > Nx0) {
                    result_final[c][z] = result_final[c][z](cv::Rect(0, 0, Ny0, Nx0)).clone();
                }
                if (Ny0 > Ny) {
                    result_final[c][z] = result_final[c][z](cv::Rect(0, 0, Ny0, Nx0)).clone();
                }
            }
        }
    }

    // ========== 后处理优化：动态范围归一化和最终结果输出 ==========
    // 清空成员变量并保存结果到父类成员变量 final_images
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

    // ========== 获取用户选择的输出目录 ==========
    std::string outputPath = outputPathQt.toStdString();

    // 确保路径以斜杠结尾
    if (!outputPath.empty() && outputPath.back() != '/' && outputPath.back() != '\\') {
        outputPath += '/';
    }

    // 构建输出文件名：原始文件名 + _Darked.tif
    QString baseName = QFileInfo(image_name).baseName();  // 去除扩展名的文件名
    QString outputFileName = baseName + "_Darked.tif";
    std::string outputFileNameStd = outputFileName.toStdString();

    // ========== 保存多页TIFF ==========
    if (Nz == 1) {
        // 单帧图像，使用imwrite，可以输出多种格式图像，这是为选择图片保存格式留的接口
        std::string savePath = outputPath + outputFileNameStd;
        bool success = cv::imwrite(savePath, final_images[0]);
        if (success) {
            batchLog(QString::fromUtf8("成功保存单帧TIFF图像: ") + QString::fromStdString(savePath));
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
                batchLog(QString::fromUtf8("Error: 输出目录中含有中文，请选择纯英文路径"));
            }
            batchLog(QString::fromUtf8("保存单帧TIFF图像失败: ") + QString::fromStdString(savePath));
        }
        // 暂时先默认存为tif，imwrite还可以存png，jpg文件，后面再加功能
    } else {
        // 多帧图像，使用imwritemulti，多帧只能存tif或tiff图像
        std::string savePath = outputPath + outputFileNameStd;
        bool success = cv::imwritemulti(savePath, final_images);
        if (success) {
            batchLog(QString::fromUtf8("成功保存多帧TIFF图像: ") + QString::fromStdString(savePath));
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
                batchLog(QString::fromUtf8("Error: 输出目录中含有中文，请选择纯英文路径"));
            } else {
                batchLog(QString::fromUtf8("保存多帧TIFF图像失败: ") + QString::fromStdString(savePath));
            }
        }
    }

    // ========== 输出处理完成信息 ==========
    batchLog(QString::fromUtf8("图像处理完成: ") + baseName);

    // 计时结束
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    batchLog(QString("Processing time: %1 ms").arg(duration.count()));
    batchLog(QString("Processed %1 frames").arg(Nz));
}
