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


SOURCES += \
    greenwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    darkSectioning.cpp \
    Lp_denoise.cpp \
    OTF_estimate.cpp \
    PSF_Generator.cpp \
    confirm_block.cpp \
    dehaze.cpp \
    dehaze_fast2.cpp \
    get_atmosphere.cpp \
    get_dark_channel.cpp \
    get_laplacian.cpp \
    get_radiance.cpp \
    get_transmission_estimate.cpp \
    guided_filter.cpp \
    guidedfilter.cpp \
    kmeans.cpp \
    orangewidget.cpp \
    separateHiLo.cpp \
    window_sum_filter.cpp \
    ViewMat.cpp \
    port_matlab2opencv.cpp
HEADERS += \
    greenwidget.h \
    mainwindow.h \
    darkSectioning.h \
    orangewidget.h \
    params.h \
    ViewMat.h \
    port_matlab2opencv.h
FORMS += \
    greenwidget.ui \
    mainwindow.ui \
    orangewidget.ui

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

