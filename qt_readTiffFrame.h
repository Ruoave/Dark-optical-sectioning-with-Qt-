#ifndef QT_READTIFFFFRAME_H
#define QT_READTIFFFFRAME_H

#include <QString>
#include <QImage>
#include <functional>

QImage readTiffFrame(const QString& filePath,
                     int frameIndex,
                     std::function<void(const QString&)> logCallback = nullptr);

#endif // QT_READTIFFFFRAME_H
