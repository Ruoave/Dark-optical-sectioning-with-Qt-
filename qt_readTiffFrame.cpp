#include "qt_readTiffFrame.h"
#include <QImageReader>

QImage readTiffFrame(const QString& filePath,
                     int frameIndex,
                     std::function<void(const QString&)> logCallback)
{
    QImageReader reader(filePath);

    if (!reader.canRead()) {
        if (logCallback) {
            logCallback("错误: 无法读取文件 " + filePath);
        }
        return QImage();
    }

    int totalFrames = reader.imageCount();

    if (totalFrames <= 0) {
        QImage image = reader.read();
        if (frameIndex == 0 || frameIndex < 0) {
            return image;
        } else {
            if (logCallback) {
                logCallback(QString("警告: 请求帧 %1 但图像为单帧").arg(frameIndex));
            }
            return image;
        }
    }

    if (frameIndex < 0 || frameIndex >= totalFrames) {
        if (logCallback) {
            logCallback(QString("错误: 帧索引 %1 超出范围 (总帧数: %2)")
                         .arg(frameIndex)
                         .arg(totalFrames));
        }
        return QImage();
    }

    bool jumpSuccess = reader.jumpToImage(frameIndex);

    if (!jumpSuccess) {
        reader.setFileName(filePath);
        for (int i = 0; i < frameIndex; i++) {
            if (!reader.jumpToNextImage()) {
                if (logCallback) {
                    logCallback(QString("错误: 无法跳转到帧 %1 (在第 %2 帧失败)")
                                 .arg(frameIndex).arg(i));
                }
                return QImage();
            }
        }
    }

    QImage image = reader.read();

    if (image.isNull()) {
        if (logCallback) {
            logCallback(QString("错误: 读取帧 %1 失败 (可能是TIFF格式问题)")
                         .arg(frameIndex));
        }
    }

    return image;
}
