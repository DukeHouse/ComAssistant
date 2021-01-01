# Dart ComAssistant
  一个基于QT的串口调试助手，具有实时绘图、文本分类显示、自定义字符串过滤、关键字高亮、FFT显示、数据保存等功能，并支持STM32F1和F4系列的自动下载。

# 特色功能
  - 数据实时绘图
  - 数值表格显示
  - 关键字高亮
  - 自定义字符串过滤
  - 文本分类显示
  - 实时FFT显示
  - 全局弹出热键
  - 发送注释, 多命令处理
  - STM32串口ISP下载

# 动图展示
![demo](screenshoot/demo.gif)
# 如何绘图
当打开绘图器后，按照如下协议发送字符串即可绘制曲线：
```c
/**** 语法 ****/
"{name:num_1,num_2,num_3,...}\n"
```
name为自定义文本名称

num_x为想要绘图的文本数据，其形式依然为ASCII字符

```c
/**** C语言示例 ****/

// step1. 定义打印宏减少后期工作
#define MY_PRINT(name, fmt, ...) printf("{"#name":"fmt"}\n", __VA_ARGS__)

// step2. 像用printf一样使用
    float vol = 3.32;
    float cur = 0.23;
    MY_PRINT(plotter, "%f,%f", vol, cur);
    
```

# 如何使用分类文本显示
```c
/**** 语法 ****/
"{name:text_data}\n"
```
name为自定义文本名称

text_data为自定义文本内容

不同name的文本数据会显示在不同文本窗口
```c
/**** C语言示例 ****/

// step1. 定义打印宏减少后期工作
#define MY_PRINT(name, fmt, ...) printf("{"#name":"fmt"}\n", __VA_ARGS__)

// step2. 像用printf一样使用
    float vol = 3.32;
    float cur = 0.23;
    MY_PRINT(vol, "the voltage is %.2f V", vol);
    MY_PRINT(cur, "the current is %.2f A", cur);
```

# 如何使用regMatch正则匹配

输入要被匹配的关键字符即可提取包含指定字符的字符串。字符串末尾需要包含换行符\n，最长为512字节。匹配语法参考QT5正则语法。

# 动图展示
![demo1](screenshoot/demo1.gif)
![mainwindow](screenshoot/mainwindow.png)
![mainwindow](screenshoot/mainwindow2.jpg)
![graphwindow](screenshoot/graphwindow.png)
![scatterline](screenshoot/scatterline.png)
![multistring](screenshoot/multistring.png)


# 计划清单
  - 代码重构/优化/注释
  
# 考虑中的功能
  - XYZModen协议支持
  - 标签页可拖出来单独形成窗口，拖进去自动组合。
  - 信息发布功能可针对版本号发布信息，更具有目标性
  - 自定义高亮规则
  - 多窗口绘图根据name分窗

# 奇思妙想
  - 布尔控件、滑动条控件显示
  - 绘图器游标功能与差值显示
