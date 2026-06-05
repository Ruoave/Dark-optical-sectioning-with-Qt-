# Dark Optical Sectioning with Qt

[![Download](https://img.shields.io/badge/Download-Release-blue?style=for-the-badge&logo=github)](https://github.com/Ruoave/Dark-optical-sectioning-with-Qt-/releases)

基于 [Dark-sectioning](https://github.com/Cao-ruijie/Dark-sectioning) 项目的 MATLAB 代码，使用 C++/Qt 移植并添加了 Material Design 风格图形用户界面。

**原项目作者：** 曹睿杰等

**原论文：** *Dark-based Optical Sectioning assists Background Removal in Fluorescence Microscopy* (Nature Methods, 2025)

**开源协议：** Apache License 



## 功能简介

本软件用于荧光显微图像的暗光学分层（Dark Optical Sectioning）处理，实现背景去除与信号增强。主要功能包括：

- 荧光显微图像多帧图像栈处理

- 参数参数可视化配置

- 处理前后图像实时对比显示

- 单帧处理快速验证参数可用性

- 批量处理

- 参数导出/导入

  

## 技术栈

| 项目 | 版本/说明 |
|------|-----------|
| 语言 | C++ |
| GUI 框架 | Qt 5.14.2（MinGW 64-bit） |
| 图像处理 | OpenCV 4.10.0 |
| 构建工具 | qmake（.pro） |
| 编译器 | MinGW 7.3.0 64-bit |
| UI 组件库 | [qt-material-widgets](https://github.com/laserpants/qt-material-widgets)（静态库部署） |
| IDE | Qt Creator 4.11.1 |



## 项目结构

```
Dark_WidgetsMaterial_V1_0_0/
├── main.cpp                          // 程序入口
├── Dark_WidgetsMaterial_V1_0_0.pro   // qmake 工程文件
│
├── algorithm/                        // 算法层：核心图像处理算法
│   ├── darkSectioning.cpp/h              // 单帧处理主流程
│   ├── darkSectioning_cleanForBatch.cpp/h // 批量处理主流程（继承 darkSectioning）
│   ├── separateHiLo.cpp                  // 高低频分离
│   ├── dehaze_fast2.cpp                  // 暗通道去雾
│   ├── confirm_block.cpp                 // 腐蚀核尺寸确定
│   ├── PSF_Generator.cpp                 // 点扩散函数（PSF）生成
│   ├── get_dark_channel.cpp              // 暗通道计算
│   ├── get_atmosphere.cpp                // 大气光估计
│   ├── get_transmission_estimate.cpp     // 透射率初始估计
│   ├── get_radiance.cpp                  // 辐照度恢复
│   ├── get_laplacian.cpp                 // 拉普拉斯滤波器
│   ├── guided_filter.cpp                 // 引导滤波
│   ├── window_sum_filter.cpp             // 窗口求和滤波
│   └── port_matlab2opencv.cpp/h          // MATLAB→OpenCV 移植辅助函数
│
├── ui/                               // 界面层：Qt 界面控件与交互逻辑
│   ├── mainwindow.cpp/h/ui               // 主窗口（菜单栏、路径选择、参数面板、图像显示、日志）
│   ├── orangewidget.cpp/h/ui             // 橙区控件（图像对比显示区）
│   ├── orangebar.cpp/h/ui                // 进度条控件
│   ├── greenwidget.cpp/h/ui              // 绿区控件（参数配置区，含基本参数和高级参数两页）
│   ├── batchdialog.cpp/h/ui              // 批量处理对话框
│   └── aboutdialog.cpp/h                 // 关于对话框
│
├── params/                           // 参数传递头文件
│   ├── paramsBasic.h                     // 基本参数结构体
│   └── paramsExpert.h                    // 高级参数结构体与将字符串解析为vector的辅助函数
│
├── verify/                           // 调试辅助工具（仅调试用，正式构建不参与）
│   └── ViewMat.cpp/h                    // 将OpenCV::Mat输出为.csv文件
│
├── help/                             // 帮助文档
│   ├── help.md                           // Markdown 源文件
│   ├── help.html                         // Typora 导出的 HTML 渲染文件
│   └── help.assets/                     // 帮助文档使用的图片和 GIF 动图
│
└── SDK/Material/                      // 第三方控件库：qt-material-widgets
    ├── components/                       // 控件源码与头文件（按钮、滑块、文本框等Material组件）
    │   ├── icons/                            // Material Design 图标资源（SVG）
    │   ├── lib/                              // 库内部实现（主题、样式、涟漪效果等）
    │   └── layouts/                          // 布局辅助组件
    ├── staticlib/                        // 预编译静态库（libcomponents.a）
    └── fonts/                            // Roboto 字体文件（Material Design 标准字体）
```



## 使用说明

详细的使用说明、参数解释、批量处理教程和项目源码说明，请参阅帮助文档：

👉 [help/help.md](help/help.md)

软件内也可通过菜单栏 **帮助 → 打开帮助文档** 打开help.html。



## 致谢

- 原项目：[Cao-ruijie/Dark-sectioning](https://github.com/Cao-ruijie/Dark-sectioning)
- UI 组件库：[qt-material-widgets](https://github.com/laserpants/qt-material-widgets)



## License

本项目基于 [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) 开源发布。

本项目是 [Cao-ruijie/Dark-sectioning](https://github.com/Cao-ruijie/Dark-sectioning)（Apache License 2.0）中 MATLAB 代码的 C++/Qt 移植版本，遵循原项目相同的开源协议。
