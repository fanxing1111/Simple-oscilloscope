# Simple-oscilloscope
基于探索者STM32F4开发板的简易示波器

一、	功能指标

1、	单片机ADC输入电压范围: 0-3V

2、	垂直分辨率：12bits

3、	显示屏的刻度为：8div×10div

4、	垂直灵敏度要求含1V/div、0.1V/div、10mV/div 三档

5、	扫描速度要求含20ms/div、1ms/div、10μs /div三档

6、  输入频率范围为：10Hz～500kHz（实际测量出最大的频率范围是0~20KHz）


         ADC 的基准为3.3V
垂直灵敏度(V/div)	             0.01V/div                  	0.1V/div	          1V/div

输入信号范围(v)	   

放大倍数	           41.25	         4.125	           0.4125
