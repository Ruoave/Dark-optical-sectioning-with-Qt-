#ifndef VIEWMAT_H
#define VIEWMAT_H

#include <opencv2/opencv.hpp>
#include <QString>

void ViewMat(const cv::Mat& mat, const QString& name = "Mat");

void CheckMatDepth_1input(const cv::Mat& mat);
void CheckMatDepth_2input(const cv::Mat& mat1, const cv::Mat& mat2);

#endif // VIEWMAT_H
