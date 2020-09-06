# QT串口调试助手
  一个基于QT的串口调试助手，实现了基本收发功能、绘图功能、数据保存、关键字高亮等功能，并支持STM32F1和F4系列的自动下载。

# 特色功能
  - 数据可视化绘图
  - 语法高亮
  - 文本分类显示
  - 全局弹出热键
  - STM32串口ISP下载

# 动图展示
![demo](screenshoot/demo.gif)
# 如何绘图
当打开绘图器后，按照如下协议发送数据即可绘制曲线：
```c
//ASCII格式协议
//几条曲线就几个变量。
printf("{:%f,%f}\r\n", data1, data2);
```

# 如何使用分类文本显示
```c
//语法
"{name:text_data}\n"
```
name为文本名称

text_data为文本数据

不同name的文本数据会显示在不同文本窗口
```c
//C语言示例

#define PRINT_VOL(fmt, ...) \
        printf("{voltage:"##fmt"}\n", __VA_ARGS__)

#define PRINT_CUR(fmt, ...) \
        printf("{current:"##fmt"}\n", __VA_ARGS__)

    float vol = 3.32;
    float cur = 0.23;
    void fun()
    {
      PRINT_VOL("the voltage is %.2f V", vol);
      PRINT_CUR("the current is %.2f A", cur);
    }
```
![demo1](screenshoot/demo1.gif)
![mainwindow](screenshoot/mainwindow.png)
![mainwindow](screenshoot/mainwindow2.jpg)
![graphwindow](screenshoot/graphwindow.png)
![scatterline](screenshoot/scatterline.png)
![multistring](screenshoot/multistring.png)


# 计划清单
  - 代码重构/优化/注释
  - 自定义高亮规则
  
# 考虑中的功能
  - XYZModen协议支持
  - DeBug转log文件
  - 修改绘图器数据池结构，实现只显示范围内的数据。（现在全部显示也未发现卡顿现象）
  - 要不要彻底重构串口对象
  - 绘图器线程化
  - 一些变量结构体化
  - 增加标签页、绘图器、拖出来单独形成窗口，拖进去自动组合。不知道这个功能会不会和上面的互斥？
  - ui->customPlot->protocol的操作整合到customPlot类中

# 暂时无法实现的功能
  - 图像Y轴只按显示的区域最大值调整：暂时没有合适的方法实现，
    1. 自己造轮子在高密度绘图时容易遇到性能问题；
    2. 图形库的API间接实现时，在Y轴不变化即直线的情形时会正反馈发散，可能是由于API对直线设为居中，但对Y轴长度不处理
  - 隐藏绘图数据：没有适合的办法辨别满足绘图协议但未接收完的数据，也可以改为提取的思路
  - 串口热插拔检测：
    1. 使用WinAPI则无法在Linux上应用；
    2. 周期轮询的话在检测时会造成几秒钟的无响应，若插拔的串口不是当前正在用的串口则十分影响操作体验；
    3. 目前已经实现了故障检测，若正在使用的串口松动，能够正确识别并关闭该串口

# 奇思妙想
  - 布尔控件、滑动条控件显示
  - 绘图器游标功能与差值显示
  - 信息发布功能可针对版本号发布信息，更具有目标性
