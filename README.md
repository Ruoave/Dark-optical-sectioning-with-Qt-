# Dark Optical Sectioning with Qt

基于 [Dark-sectioning](https://github.com/Cao-ruijie/Dark-sectioning) 项目的 MATLAB 代码，使用 C++/Qt 移植并添加了 Material Design 风格图形用户界面。

**原项目作者：** 曹睿杰等

**原论文：** *Dark-based Optical Sectioning assists Background Removal in Fluorescence Microscopy* (Nature Methods, 2025)

**开源协议：** Apache License 2.0



## 功能简介

本软件用于荧光显微图像的暗光学分层（Dark Optical Sectioning）处理，实现背景去除与信号增强。主要功能包括：

- 单帧/批量图像处理
- 基本参数与高级算法参数配置
- 处理前后图像实时对比显示
- 参数导出/导入，方便批量复用



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
├── algorithm/                        // 算法层：核心图像处理算法
├── ui/                               // 界面层：Qt 界面控件与交互逻辑
├── params/                           // 参数传递头文件
├── verify/                           // 调试辅助工具
├── help/                             // 帮助文档（Markdown + HTML）
└── SDK/Material/                     // 第三方控件库：qt-material-widgets
    ├── components/                   // 控件源码与头文件
    ├── staticlib/                    // 预编译静态库
    └── fonts/                        // Roboto 字体文件
```



## 使用说明

详细的使用说明、参数解释、批量处理教程和项目源码说明，请参阅帮助文档：

👉 [help/help.md](help/help.md)

软件内也可通过菜单栏 **帮助 → 开始使用** 打开帮助文档。



## 致谢

- 原项目：[Cao-ruijie/Dark-sectioning](https://github.com/Cao-ruijie/Dark-sectioning)（Apache License）
- UI 组件库：[qt-material-widgets](https://github.com/laserpants/qt-material-widgets)
