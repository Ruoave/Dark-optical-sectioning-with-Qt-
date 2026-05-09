QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#MATERIAL-SDK
LIBS += $$PWD/SDK/Material/staticlib/libcomponents.a
INCLUDEPATH += $$PWD/SDK/Material/components
PRE_TARGETDEPS += $$PWD/SDK/Material/staticlib/libcomponents.a
RESOURCES += $$PWD/SDK/Material/components/resources.qrc


# 子目录头文件搜索路径（文件移动到子文件夹后，#include "xxx.h" 仍可被编译器找到）
INCLUDEPATH += $$PWD/ui \
               $$PWD/algorithm \
               $$PWD/params \
               $$PWD/verify

SOURCES += \
    ui/batchdialog.cpp \
    ui/greenwidget.cpp \
    main.cpp \
    ui/mainwindow.cpp \
    algorithm/darkSectioning.cpp \
    algorithm/darkSectioning_cleanForBatch.cpp \
    algorithm/PSF_Generator.cpp \
    algorithm/confirm_block.cpp \
    algorithm/dehaze_fast2.cpp \
    algorithm/get_atmosphere.cpp \
    algorithm/get_dark_channel.cpp \
    algorithm/get_laplacian.cpp \
    algorithm/get_radiance.cpp \
    algorithm/get_transmission_estimate.cpp \
    algorithm/guided_filter.cpp \
    ui/orangebar.cpp \
    ui/orangewidget.cpp \
    algorithm/separateHiLo.cpp \
    algorithm/window_sum_filter.cpp \
    verify/ViewMat.cpp \
    algorithm/port_matlab2opencv.cpp
HEADERS += \
    ui/batchdialog.h \
    ui/greenwidget.h \
    ui/mainwindow.h \
    algorithm/darkSectioning.h \
    algorithm/darkSectioning_cleanForBatch.h \
    ui/orangebar.h \
    ui/orangewidget.h \
    params/paramsBasic.h \
    params/paramsExpert.h \
    algorithm/port_matlab2opencv.h \
    verify/ViewMat.h
FORMS += \
    ui/batchdialog.ui \
    ui/greenwidget.ui \
    ui/mainwindow.ui \
    ui/orangebar.ui \
    ui/orangewidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


INCLUDEPATH += D:\Qt\opencv-4.10.0\opencvBuild\install\include
INCLUDEPATH += D:/Qt/Tools/mingw730_64/x86_64-w64-mingw32/include

# OpenCV library configuration - OpenCV 4.10.0
LIBS += -L D:/Qt/opencv-4.10.0/opencvBuild/install/x64/mingw/lib \
        -lopencv_core4100 \
        -lopencv_imgproc4100 \
        -lopencv_imgcodecs4100 \
        -lopencv_highgui4100 \
        -lopencv_videoio4100 \
        -lopencv_calib3d4100 \
        -lopencv_features2d4100 \
        -lopencv_flann4100 \
        -lopencv_photo4100

