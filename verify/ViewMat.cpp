#include "ViewMat.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>

void ViewMat(const cv::Mat& mat, const QString& name)
{
    // 检查 Mat 是否为空
    if (mat.empty()) {
        qDebug() << "ViewMat: Mat is empty!";
        return;
    }

    // 获取 Mat 基本信息
    int rows = mat.rows;
    int cols = mat.cols;
    int channels = mat.channels();
    int depth = mat.depth();
    int type = mat.type();

    qDebug() << "\n========== ViewMat: " << name << " ==========";
    qDebug() << "Size: " << cols << " x " << rows;
    qDebug() << "Channels: " << channels;
    
    // 输出类型名称
    QString typeName;
    switch (type) {
        case CV_8UC1: typeName = "CV_8UC1     (grayscale)"; break;
        case CV_8UC2: typeName = "CV_8UC2"; break;
        case CV_8UC3: typeName = "CV_8UC3     (color BGR)"; break;
        case CV_8UC4: typeName = "CV_8UC4     (with alpha channel)"; break;
        case CV_8SC1: typeName = "CV_8SC1"; break;
        case CV_8SC2: typeName = "CV_8SC2"; break;
        case CV_8SC3: typeName = "CV_8SC3"; break;
        case CV_8SC4: typeName = "CV_8SC4"; break;
        case CV_16UC1: typeName = "CV_16UC1     (high res grayscale)"; break;
        case CV_16UC2: typeName = "CV_16UC2"; break;
        case CV_16UC3: typeName = "CV_16UC3     (high res color)"; break;
        case CV_16UC4: typeName = "CV_16UC4"; break;
        case CV_16SC1: typeName = "CV_16SC1"; break;
        case CV_16SC2: typeName = "CV_16SC2"; break;
        case CV_16SC3: typeName = "CV_16SC3"; break;
        case CV_16SC4: typeName = "CV_16SC4"; break;
        case CV_32SC1: typeName = "CV_32SC1"; break;
        case CV_32SC2: typeName = "CV_32SC2"; break;
        case CV_32SC3: typeName = "CV_32SC3"; break;
        case CV_32SC4: typeName = "CV_32SC4"; break;
        case CV_32FC1: typeName = "CV_32FC1"; break;
        case CV_32FC2: typeName = "CV_32FC2"; break;
        case CV_32FC3: typeName = "CV_32FC3"; break;
        case CV_32FC4: typeName = "CV_32FC4"; break;
        case CV_64FC1: typeName = "CV_64FC1     (double grayscale)"; break;
        case CV_64FC2: typeName = "CV_64FC2     (double with alpha channel)"; break;
        case CV_64FC3: typeName = "CV_64FC3     (double colorBGR)"; break;
        case CV_64FC4: typeName = "CV_64FC4     (double colorBGR with alpha channel)"; break;
        default: typeName = "Unknown";
    }
    qDebug() << "Type: " << type << " (" << typeName << ")";
    
    // 输出深度类型
    QString depthName;
    switch (depth) {
        case CV_8U: depthName = "CV_8U (unsigned char)"; break;
        case CV_8S: depthName = "CV_8S (signed char)"; break;
        case CV_16U: depthName = "CV_16U (unsigned short)"; break;
        case CV_16S: depthName = "CV_16S (signed short)"; break;
        case CV_32S: depthName = "CV_32S (signed int)"; break;
        case CV_32F: depthName = "CV_32F (float)"; break;
        case CV_64F: depthName = "CV_64F (double)"; break;
        default: depthName = "Unknown";
    }
    qDebug() << "Depth Type: " << depthName;

    // 确保输出目录存在
    QString outputDir = "D:/Qt/QtWorkSpace/DarkWidgets_V5_0_0/Output";
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(outputDir);
    }

    // 生成文件名：将 [5] 转换为 _5
    QString fileName = name;
    fileName.replace(QRegularExpression("\\[(\\d+)\\]"), "_\\1");
    fileName.replace("[", "_");
    fileName.replace("]", "");
    fileName.replace(" ", "_");
    QString csvPath = outputDir + "/" + fileName + ".csv";

    // 创建 CSV 文件
    QFile file(csvPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to create CSV file: " << csvPath;
        return;
    }

    QTextStream out(&file);

    // 直接写入矩阵数据，从 A1 开始
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            QString valueStr;

            if (channels == 1) {
                // 单通道（原有逻辑，完全不变）
                switch (depth) {
                    case CV_8U:  valueStr = QString::number(mat.at<uchar>(i, j)); break;
                    case CV_8S:  valueStr = QString::number(mat.at<schar>(i, j)); break;
                    case CV_16U: valueStr = QString::number(mat.at<ushort>(i, j)); break;
                    case CV_16S: valueStr = QString::number(mat.at<short>(i, j)); break;
                    case CV_32S: valueStr = QString::number(mat.at<int>(i, j)); break;
                    case CV_32F: valueStr = QString::number(mat.at<float>(i, j), 'f', 6); break;
                    case CV_64F: valueStr = QString::number(mat.at<double>(i, j), 'f', 6); break;
                    default:     valueStr = "?";
                }
            }
            else if (channels == 2) {
                // ===================== 新增：双通道（专门适配你的fft复数矩阵）=====================
                // 通道0=实部(Real)，通道1=虚部(Imag)
                if (depth == CV_32F) {
                    // CV_32FC2：float 双通道复数
                    cv::Vec2f pixel = mat.at<cv::Vec2f>(i, j);
                    valueStr = QString("(%1, %2)")
                                    .arg(pixel[0], 0, 'f', 6)  // 实部
                                    .arg(pixel[1], 0, 'f', 6); // 虚部
                }
                else if (depth == CV_64F) {
                    // CV_64FC2：double 双通道复数
                    cv::Vec2d pixel = mat.at<cv::Vec2d>(i, j);
                    valueStr = QString("(%1, %2)")
                                    .arg(pixel[0], 0, 'f', 6)  // 实部
                                    .arg(pixel[1], 0, 'f', 6); // 虚部
                }
                else {
                    valueStr = "(?, ?)";
                }
            }
            else if (channels == 3) {
                // 三通道（原有逻辑，完全不变）
                if (depth == CV_8U) {
                    cv::Vec3b pixel = mat.at<cv::Vec3b>(i, j);
                    valueStr = QString("(%1,%2,%3)").arg(pixel[0]).arg(pixel[1]).arg(pixel[2]);
                } else if (depth == CV_32F) {
                    cv::Vec3f pixel = mat.at<cv::Vec3f>(i, j);
                    valueStr = QString("(%1,%2,%3)").arg(pixel[0], 0, 'f', 2).arg(pixel[1], 0, 'f', 2).arg(pixel[2], 0, 'f', 2);
                } else {
                    valueStr = "(?,?,?)";
                }
            }
            else {
                // 其他通道数
                valueStr = "...";
            }

            out << valueStr;
            if (j < cols - 1) out << ",";
        }
        out << "\n";
    }

    file.close();

    qDebug() << "CSV file saved to: " << csvPath;
    qDebug() << "========================================\n";
}





// CheckMatDepth 函数：检查单个 Mat 的深度类型
void CheckMatDepth_1input(const cv::Mat& mat)
{
    if (mat.empty()) {
        qDebug() << "CheckMatDepth: Mat is empty!";
        return;
    }

    int depth = mat.depth();
    QString depthName;
    
    switch (depth) {
        case CV_8U: depthName = "CV_8U (unsigned char)"; break;
        case CV_8S: depthName = "CV_8S (signed char)"; break;
        case CV_16U: depthName = "CV_16U (unsigned short)"; break;
        case CV_16S: depthName = "CV_16S (signed short)"; break;
        case CV_32S: depthName = "CV_32S (signed int)"; break;
        case CV_32F: depthName = "CV_32F (float)"; break;
        case CV_64F: depthName = "CV_64F (double)"; break;
        default: depthName = "Unknown";
    }
    
    qDebug() << "Mat depth: " << depth << " (" << depthName << ")";
}

// CheckMatDepth 函数：检查两个 Mat 的深度类型并判断是否一致
void CheckMatDepth_2input(const cv::Mat& mat1, const cv::Mat& mat2)
{
    if (mat1.empty()) {
        qDebug() << "CheckMatDepth: First Mat is empty!";
        return;
    }
    if (mat2.empty()) {
        qDebug() << "CheckMatDepth: Second Mat is empty!";
        return;
    }

    int depth1 = mat1.depth();
    int depth2 = mat2.depth();
    
    QString depthName1, depthName2;
    
    switch (depth1) {
        case CV_8U: depthName1 = "CV_8U (unsigned char)"; break;
        case CV_8S: depthName1 = "CV_8S (signed char)"; break;
        case CV_16U: depthName1 = "CV_16U (unsigned short)"; break;
        case CV_16S: depthName1 = "CV_16S (signed short)"; break;
        case CV_32S: depthName1 = "CV_32S (signed int)"; break;
        case CV_32F: depthName1 = "CV_32F (float)"; break;
        case CV_64F: depthName1 = "CV_64F (double)"; break;
        default: depthName1 = "Unknown";
    }
    
    switch (depth2) {
        case CV_8U: depthName2 = "CV_8U (unsigned char)"; break;
        case CV_8S: depthName2 = "CV_8S (signed char)"; break;
        case CV_16U: depthName2 = "CV_16U (unsigned short)"; break;
        case CV_16S: depthName2 = "CV_16S (signed short)"; break;
        case CV_32S: depthName2 = "CV_32S (signed int)"; break;
        case CV_32F: depthName2 = "CV_32F (float)"; break;
        case CV_64F: depthName2 = "CV_64F (double)"; break;
        default: depthName2 = "Unknown";
    }
    
    qDebug() << "First Mat depth: " << depth1 << " (" << depthName1 << ")";
    qDebug() << "Second Mat depth: " << depth2 << " (" << depthName2 << ")";
    
    if (depth1 == depth2) {
        qDebug() << "The two matrices can perform matrix arithmetic operations!";
    } else {
        qDebug() << "Warning! The two matrices cannot perform matrix arithmetic operations!";
    }
}