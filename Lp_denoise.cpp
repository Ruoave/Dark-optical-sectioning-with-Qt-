#include <opencv2/opencv.hpp>
#include "port_matlab2opencv.h"

using namespace cv;
using namespace std;

// Lp去噪函数
// 输入：
//   image - 输入图像
//   params - 系统参数
// 输出：
//   Lo - 去噪后的低频分量

// Lp_denoise implementation
