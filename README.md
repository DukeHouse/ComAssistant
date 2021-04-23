# Dart ComAssistant
  一个基于QT的串口/网络调试助手，具有多窗口实时绘图、文本分窗显示、自定义数据过滤功能，适合于多任务系统，或者有多种类型数据需要分开显示的情形。此外还有关键字高亮、FFT显示、数据保存等功能，并支持STM32F1和F4系列的自动下载。

# 特色功能
  - 串口/网络多模式
  - 多窗口高性能实时绘图
  - 数值表格显示
  - 关键字高亮
  - 自定义数据过滤
  - 文本分窗显示
  - 实时FFT显示
  - 全局弹出热键
  - 发送注释, 多命令处理
  - STM32串口ISP下载

# 动图展示
![demo](screenshoot/demo.gif)
# 如何绘图（分窗）
当选择ASCII协议后，按照如下协议发送字符串即可绘制曲线：
```c
/**** 语法 ****/
"{title:string}\n"
```
title为自定义标题，纸飞机将根据title进行分窗显示（绘图）

string为自定义英文文本，这将是被显示的内容
当string格式为逗号分隔形式的数字时，便可以被绘图器识别并显示为曲线。

\n为换行符

```c
/**** C语言示例 ****/

// step1. 首先定义打印宏减少后期工作
#define PRINT(title, fmt, ...) printf("{"#title":"fmt"}\n", __VA_ARGS__)

// step2. 若要进行绘图，可这样进行
PRINT(plotter, "%f,%f", data1, data2);

// step3. 若要进行分窗，可这样进行
PRINT(info, "the cpu usage is %d", data3);

```

# 如何使用数据过滤功能

输入要被过滤的关键字符即可提取包含指定字符的字符串显示在filter窗口中。字符串末尾需要包含换行符\n，最长为512字节。匹配语法参考QT5正则语法。

# 更多展示
![demo1](screenshoot/demo1.gif)
![mainwindow](screenshoot/mainwindow.png)
![mainwindow](screenshoot/mainwindow2.jpg)
![graphwindow](screenshoot/graphwindow.png)
![scatterline](screenshoot/scatterline.png)
![multistring](screenshoot/multistring.png)

# 考虑中的功能
  - XYZModen协议支持
  - 标签页可拖出来单独形成窗口，拖进去自动组合。
  - 信息发布功能可针对版本号发布信息，更具有目标性
  - 自定义高亮规则

# 奇思妙想
  - 布尔控件、滑动条控件显示
  - 绘图器游标功能与差值显示
