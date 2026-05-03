#ifndef PARAMSBASIC_H
#define PARAMSBASIC_H

struct ParamsBasic {
    int Nx;
    int Ny;
    double NA = 1.49;              // 数值孔径
    double emwavelength = 610;     // 发射波长(nm)
    double pixelsize = 65;         // 像素尺寸(nm)
    int factor = 2;                // 分辨率比例因子

    int background = 0;            // 0-离焦不严重, 1-离焦严重
    int pad = 1;                   // 1-对称填充, 0-零填充
    int denoise = 0;               // 0-不去噪, 1-高斯平滑, 2-中值滤波
};

#endif // PARAMSBASIC_H
