#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "dac.h"
#include "adc.h"
#include "waveform.h"
#include "timer.h"
#include "stm32f4xx_it.h"
#include "exti.h"
#include "key.h"
#include "arm_math.h"  
#define NPT 1024 //采样次数

void clear_point(u16 num);//更新显示屏当前列	
void Set_BackGround(void);//设置背景
void Lcd_DrawNetwork(void);//画网格
float get_vpp(float *buf);//获取峰峰值
void DrawOscillogram(float* buf);//画波形图	
void Draw_Prompt(void);//画提示符
void Get_Data(u8 speed,u8 start);//选择扫描速度并得到AD转换的值
void DSP(void);  //FFT处理
void get_memory(void); //存储波形数据
float buff[NPT];//数值缓存
float buff3[400];//存储数值缓存
float buff2[NPT*2]={0};//数值缓存
float fft_outputbuf[NPT];	//FFT输出数组
u8 scan_speed=1;
u8 a=0;
u8 b=0;
u8 c=0;
float f=0;    //最大频率分量
float g=0;
u8    f_buff[20] = {0};
u32 sat=0;   //采样频率
u8 vs=0;  //垂直灵敏度
u8 Memory=0; //存储波形  0:功能失能  1：开始存储  2：显示波形
int main(void)
{
	float Adresult = 0;
	u8    Vpp_buff[20] = {0};
	u32  y=0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);            //初始化延时函数
	uart_init(115200);		    //初始化串口波特率为115200
	LED_Init();					//初始化LED
 	LCD_Init();                 //初始化LCD FSMC接口
    Adc_Init();                 //初始化ADC
	EXTIX_Init();
	Set_BackGround();	        //显示背景
	Lcd_DrawNetwork();
    MYDAC_Init();               //产生正弦波
	LED0 = 0;
	Draw_Prompt();				//画提示符
  	while(1) 
	{	if(a==0)  Get_Data(scan_speed,1);
		DrawOscillogram(buff);//画波形
		Adresult = get_vpp(buff);//峰峰值mv	
		sprintf((char*)Vpp_buff,"%0.3fV",Adresult);
		if(b==0) get_memory();   //存储波形
		POINT_COLOR = WHITE;
		BACK_COLOR = DARKBLUE;
		LCD_ShowString(410,20,80,16,16,Vpp_buff);
		LCD_ShowString(410,60,80,16,16,f_buff);
		LED0 = !LED0;
		y++;
		if(y==10)
		{
			DSP();
			y=0;
		}
	} 
}

void clear_point(u16 num)//更新显示屏当前列
{
	u16 index_clear_lie = 0; 
	POINT_COLOR = DARKBLUE ;
	for(index_clear_lie = 1;index_clear_lie < 320;index_clear_lie++)
	{		
		LCD_DrawPoint(num,index_clear_lie );
	}
	if(!(num%40))//判断hang是否为40的倍数 画列点
	{
		for(index_clear_lie = 10;index_clear_lie < 320;index_clear_lie += 10)
		{		
			LCD_Fast_DrawPoint(num ,index_clear_lie,WHITE );
		}
	}
	if(!(num%10))//判断hang是否为10的倍数 画行点
	{
		for(index_clear_lie = 40;index_clear_lie <320;index_clear_lie += 40)
		{		
			LCD_Fast_DrawPoint(num ,index_clear_lie,WHITE );
		}
	}	
	POINT_COLOR = YELLOW;	
}

void DrawOscillogram(float *buff)//画波形图
{
	static u16 Ypos1 = 0,Ypos2 = 0;
//	u16 Yinit=100;
	u16 i = 0;
	POINT_COLOR = YELLOW;
	if(c==0)    //0：实时更新  1：显示存储的波形
	{
		for(i = 1;i < 400;i++)
		{
			clear_point(i );	
			Ypos2 = 320-(buff[i] * 320/ 4096);//转换坐标
			//Ypos2 = Ypos2 * bei;
			if(Ypos2 >400)
				Ypos2 =400; //超出范围不显示
			LCD_DrawLine (i ,Ypos1 , i+1 ,Ypos2);
			Ypos1 = Ypos2 ;
		}
	}
	else
	{
		for(i = 1;i < 400;i++)
		{
			clear_point(i );	
			Ypos2 = 320-(buff3[i] * 320/ 4096);//转换坐标
			//Ypos2 = Ypos2 * bei;
			if(Ypos2 >400)
				Ypos2 =400; //超出范围不显示
			LCD_DrawLine (i ,Ypos1 , i+1 ,Ypos2);
			Ypos1 = Ypos2 ;
		}	
	}
    Ypos1 = 0;	
}

void Set_BackGround(void)
{
	POINT_COLOR = YELLOW;
    LCD_Clear(DARKBLUE);
	LCD_DrawRectangle(0,0,400,320);//矩形
	//LCD_DrawLine(0,220,700,220);//横线
	//LCD_DrawLine(350,20,350,420);//竖线
	//POINT_COLOR = WHITE;
	//BACK_COLOR = DARKBLUE;
	//LCD_ShowString(330,425,210,24,24,(u8*)"vpp=");	
}

void Lcd_DrawNetwork(void)
{
	u16 index_y = 0;
	u16 index_x = 0;	
	
    //画列点	
	for(index_x = 40;index_x < 400;index_x += 40)
	{
		for(index_y = 10;index_y < 320;index_y += 10)
		{
			LCD_Fast_DrawPoint(index_x,index_y,WHITE);	
		}
	}
	//画行点
	for(index_y = 40;index_y < 320;index_y += 40)
	{
		for(index_x = 10;index_x < 400;index_x += 10)
		{
			LCD_Fast_DrawPoint(index_x,index_y,WHITE);	
		}
	}
}

float get_vpp(float *buf)	   //获取峰峰值
{
	
	u32 max_data=buff[0];
	u32 min_data=buff[0];//buf[0];
	u32 n=0;
	float Vpp=0;
	for(n = 1;n<NPT;n++)
	{
		if(buff[n] > max_data)
		{
			max_data = buff[n];
		}
		if(buff[n] < min_data)
		{
			min_data = buff[n];
		}			
	} 
//	for(n = 1;n<NPT;n++)
//	{
//		if(fft_outputbuf[n] == max_data) f=n;	
//	}
	if(vs==0)
	{
		LCD_ShowString(410,150,80,16,16,"  1V/div");
		Vpp = (float)(max_data - min_data);
		Vpp = Vpp*(3.3/4096);
	}
	else if(vs==1)
	{
		LCD_ShowString(410,150,80,16,16,"0.1V/div");
		Vpp = (float)(max_data - min_data);
		Vpp = Vpp*(3.3/4096);
	}
	else 
	{
		LCD_ShowString(410,150,80,16,16,"10mV/div");
		Vpp = (float)(max_data - min_data);
		Vpp = Vpp*(3.3/4096);
	}
	return Vpp;	
}

void Draw_Prompt(void)
{
	BACK_COLOR = DARKBLUE;
	POINT_COLOR=WHITE; 
	LCD_ShowString(405,0,80,16,16,"vpp(v):");
	LCD_ShowString(405,40,80,16,16,"Fre(Hz):");
	LCD_ShowString(405,90,80,16,16,"X-Time:");
	LCD_ShowString(405,130,80,16,16,"Y-Value:");
	LCD_ShowString(405,170,80,16,16,"Memory:");
	LCD_ShowString(405,280,80,16,16,"CHONGQING");
	LCD_ShowString(402,300,80,16,16,"UNIVERSITY");
	
}
//1：speed=2K
//2: speed=40K
//3: speed=400K
//4: speed=10M
//start:1 启动AD转换
void Get_Data(u8 speed,u8 start)
{
	if(speed==1)
	{
		TIM3_Int_Init(5-1,8400-1);   //1：speed=2K
		a=1;
		sat=2000;
		LCD_ShowString(410,110,80,16,16," 20ms/div");
	}
	else if(speed==2)
	{
		TIM3_Int_Init(5-1,420-1);   //1：speed=40K
		a=1;
		sat=40000;
		LCD_ShowString(410,110,80,16,16,"  1ms/div");
	}
	else if(speed==3)
	{
		TIM3_Int_Init(5-1,42-1);   //1：speed=400K
		a=1;
		sat=400000;
		LCD_ShowString(410,110,80,16,16," 10us/div");
	}
//	else if(speed==4)
//	{
//		TIM3_Int_Init(3-1,7-1);    //3: speed=10M
//		a=1;
//		sat=10000000;
//	}
	if(start==1)
	{
		TIM_Cmd(TIM3,ENABLE); //使能定时器3,启动AD转换
	}
	else
	{
		TIM_Cmd(TIM3,DISABLE); //不使能定时器3	
	}

}
void get_memory(void)
{
	u16 k=0;
	if(Memory==1) 
	{
		b=1;
		for(k = 0;k<400;k++)
		{
			buff3[k]=buff[k];
		}
		LCD_ShowString(410,190,80,16,16,"Stored");
	}
	else if(Memory==2)
	{
		c=1;
		b=1;
		LCD_ShowString(410,190,80,16,16,"  Show");
	}
	else
	{
		c=0;
		b=1;
		LCD_ShowString(410,190,80,16,16,"   Off");
	}

}
void DSP(void)
{
	float Mag,magmax;//各频率幅值，最大幅值
	int i;
	arm_cfft_radix4_instance_f32 scfft;
	TIM_Cmd(TIM3,DISABLE); //不使能定时器3
	for(i=0;i< NPT;i++)
	{
		buff2[i*2]=buff[i];
		buff2[i*2+1]=0;
		//printf("buf[%d]:%f\r\n",i,buff2[i*2]);
		//printf("buf[%d]:%f\r\n",i,buff2[i*2+1]);
	}
	TIM_Cmd(TIM3,ENABLE); //使能定时器3	
	arm_cfft_radix4_init_f32(&scfft,NPT,0,1);//初始化scfft结构体，设定FFT相关参数
	arm_cfft_radix4_f32(&scfft,buff2);	//FFT计算（基4）
	arm_cmplx_mag_f32(buff2,fft_outputbuf,NPT);	//把运算结果复数求模得幅值
	magmax=fft_outputbuf[1];
	for(i=1;i< NPT/2;i++)
	{
		//printf("fft_outputbuf[%d]:%f\r\n",i,fft_outputbuf[i]);
		//printf("buf[%d]:%f\r\n",i,buff[i]);
		Mag = fft_outputbuf[i];		
		//获取最大频率分量及其幅值
		if(Mag >= magmax)
		{
			magmax = Mag;
			f = i;
		}	
	}
	f=f*sat/1024;
	g = f;	
	sprintf((char*)f_buff,"%0.3fHz",g);
}
//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	static u16 j=0;
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
		if(j!=NPT)
		{	
			buff[j] = Get_Adc(ADC_Channel_5);//存储AD数值
			j=j+1;
		}
		else
		{
			j=0;
		}
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}
//外部中断2服务程序
void EXTI2_IRQHandler(void)
{
	delay_ms(10);	//消抖
	if(KEY2==0)	  
	{				 
		if(vs!=2) vs++;
		else vs=0;
	}		 
	 EXTI_ClearITPendingBit(EXTI_Line2);//清除LINE2上的中断标志位 
}
//外部中断3服务程序
void EXTI3_IRQHandler(void)
{
	delay_ms(10);	//消抖
	if(KEY1==0)	 
	{
		if(scan_speed!=3)  scan_speed++;
		else scan_speed=1;
		a=0;
	}		 
	 EXTI_ClearITPendingBit(EXTI_Line3);  //清除LINE3上的中断标志位  
}
//外部中断4服务程序
void EXTI4_IRQHandler(void)
{
	delay_ms(10);	//消抖
	if(KEY0==0)	 
	{				 
		if(Memory!=2) Memory++;
		else Memory=0;
		b=0;
	}		 
	 EXTI_ClearITPendingBit(EXTI_Line4);//清除LINE4上的中断标志位  
}					
