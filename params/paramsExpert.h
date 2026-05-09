#ifndef PARAMSEXPERT_H
#define PARAMSEXPERT_H

#include <string>
#include <vector>

struct ParamsExpert {
    int thres = 70;             // 划分信息和背景的阈值;荧光信号越强，阈值要越高
    double divide = 0.5;        // 划分高频/低频部分的边界;基本不用调
    int padsize = 15;           // 用于边缘渐变的填充大小

    std::string deg = "6,3";         // EL1/2参数，deg控制极低通滤波器宽度:sigmaLP/deg; deg越小→滤波器越宽→EL保留更多低频→大气光估计更平滑
    std::string dep = "3,3";         // 假设场景深度，rep_atmosphere = dep * rep_atmosphere; 场景越深去雾越激进
    std::string hl = "1,1";          // result = Lo_process/hl + Hi; hl越大→Lo_process/hl越小→低频贡献被压制，高频细节更突出

    int isQuick = 0;                  // 单帧处理模式：0-多帧处理（默认），1-单帧快速处理（SingleFrameRunning选中时）
};

// 辅助函数：将逗号分隔的字符串解析为 vector<double>
// 例如 "6,3,1.2" → {6.0, 3.0, 1.2}
inline std::vector<double> parseExpertVector(const std::string &str)
{
    std::vector<double> result;
    std::string s = str;
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = s.find(',', prev)) != std::string::npos) {
        result.push_back(std::stod(s.substr(prev, pos - prev)));
        prev = pos + 1;
    }
    result.push_back(std::stod(s.substr(prev)));
    return result;
}

#endif // PARAMSEXPERT_H
