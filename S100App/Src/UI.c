/********************* (C) COPYRIGHT 2015 e-Design Co.,Ltd. **********************
File Name :      UI.c
Version :        S100 APP Ver 2.11
Description:
Author :         Celery
Data:            2015/08/03
History:
2015/07/07   统一命名；
2015/08/03   优化移动判定
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "APP_Version.h"
#include "UI.h"
#include "CTRL.h"
#include "WordLib.h"
#include "Bios.h"
#include "OLed.h"
#include "HARDWARE.h"
#include "disk.h"
#include "MMA8652FC.h"
#include "own_sprintf.h"
#include "I2C.h"


char *gSys_settings[] = {"WkTemp ","StbTemp","SlpTime","IdlTime","TempStp","OffVolt","Temp   ","Hand   "};

/******************************************************************************/
u8 gTemp_array[16*16 + 16];
u8 direction_flag = 0; //方向标志位 ，0：无 1：左 2：右
u8 gTemperatureshowflag = 0;     /* 1 是摄氏度,0是华氏度*/
u8 digit = 0;//位数
u8 gUp_flag = 0,gDown_flag = 0,gLevel_flag = 0,gTempset_showctrl = 0;
u16 gTemp_array_u16[208];
s32 gSet_table[9][3] = {{4000,1000,100},
                        {4000,1000,100},
                        {90000,6000,3000},
                        {90000,10000,3000},
                        {250,10,10},
                        {120,50,1},
                        {1,0,1},
                        {1,0,1},
                        {7520,2120,100}
                        }; /*温度选择*/
u32 gCont = 0;   /*gCont 可以用作状态转换间的刷屏标识*/
u8 Exit_pos = 0;

extern u8 TEMP_SET_Pos;

static u8 Ver_s[] = {  /*12*16*/
    0x04,0x0C,0x74,0x80,0x00,0x00,0x00,0x80,0x74,0x0C,0x04,0x00,
    0x00,0x00,0x00,0x03,0x1C,0x60,0x1C,0x03,0x00,0x00,0x00,0x00,/*"V",0*/
    0x00,0x00,0x80,0x40,0x40,0x40,0x40,0x80,0x00,0x00,0x00,0x00,
    0x00,0x1F,0x22,0x42,0x42,0x42,0x42,0x22,0x13,0x00,0x00,0x00,/*"e",1*/
    0x00,0x40,0x40,0xC0,0x00,0x80,0x40,0x40,0x40,0xC0,0x00,0x00,
    0x00,0x40,0x40,0x7F,0x41,0x40,0x40,0x00,0x00,0x00,0x00,0x00,/*"r",2*/
    0x00,0x00,0x00,0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,/*":",3*/
    0x00,0x78,0x04,0x04,0x04,0x04,0x04,0x88,0x78,0x00,0x00,0x00,
    0x00,0x60,0x50,0x48,0x44,0x42,0x41,0x40,0x70,0x00,0x00,0x00,/*"2",4*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*".",5*/
    0x00,0xE0,0x18,0x08,0x04,0x04,0x04,0x08,0x18,0xE0,0x00,0x00,
    0x00,0x0F,0x30,0x20,0x40,0x40,0x40,0x20,0x30,0x0F,0x00,0x00,/*"0",6*/
    0x00,0x00,0x00,0x08,0x08,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x40,0x40,0x7F,0x40,0x40,0x40,0x00,0x00,0x00,/*"1",7*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*" ",5*/
};
/******************************************************************************/



/*******************************************************************************
函数名: Get_UpdataFlag
函数作用:获取屏幕刷新标志
输入参数:NULL
返回参数:屏幕刷新标志
*******************************************************************************/
u32 Get_UpdataFlag(void)
{
    return gCont;
}
/*******************************************************************************
函数名: Set_UpdataFlag
函数作用:设置屏幕刷新标志
输入参数:1 刷新，0 不刷新
返回参数:NULL
*******************************************************************************/
void Set_UpdataFlag(u32 cont)
{
    gCont = cont;
}
/*******************************************************************************
函数名: Set_TemperatureShowFlag
函数作用:设置温度显示方式
输入参数:flag 摄氏度(0),华氏度(1)标志
返回参数:NULL
*******************************************************************************/
void Set_TemperatureShowFlag(u8 flag)
{
    gTemperatureshowflag = flag;
}
/*******************************************************************************
函数名: Get_TemperatureShowFlag
函数作用:获取温度显示方式
输入参数:NULL
返回参数:flag 摄氏度(0),华氏度(1)标志
*******************************************************************************/
u8 Get_TemperatureShowFlag(void)
{
    return gTemperatureshowflag;
}
/*******************************************************************************
函数名: TemperatureShow_Change
函数作用:摄氏度华氏度相互转换
输入参数: flag 0,摄氏度转换华氏度
          flag 1,华氏度转换摄氏度
          tmp 要转换的温度
返回参数:转换后的值
*******************************************************************************/
s16 TemperatureShow_Change(u8 flag ,s16 tmp)
{
    if(flag == 0) {
        return (320 + tmp*9/5);
    } else {
        return (5*tmp - 1600)/9;
    }
}
/*******************************************************************************
函数名: APP_Init
函数作用:根据电压初始化开始状态
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void APP_Init(void)
{
    int rev;

    HEATING_TIMER = 0;
    UI_TIMER = 0;

    rev = Read_Vb(0);
    if(rev == 0)  Set_CtrlStatus(ALARM);
    else if(rev >= 4) {
        Set_LongKeyFlag(1);
        Set_CtrlStatus(CONFIG);
    } else {
        Set_CtrlStatus(IDLE);
        G6_TIMER = device_info.idle_time;
    }

}
void Show_Volt(void)
{       
    u32 tmp,i,sum = 0;
    u8 buf[20];
    
    for(i = 0; i < 10; i++) {
        tmp = ADC_GetConversionValue(ADC2);
        sum += tmp;
    }
    tmp = sum/10;

    tmp = (tmp*100/144);//电压vb = 3.3 * 85 *ad / 40950 扩大100倍
    
    own_sprintf((char *)buf,"%fV",tmp);
    
    Display_Str8(1,(char *)buf,0);
}

/*******************************************************************************
函数名: Display_Temp
函数作用:显示温度
输入参数:x:显示位置 Temp:温度
返回参数:NULL
*******************************************************************************/
void Display_Temp(u8 x,s16 temp)
{
    char Str[8];

    memset(Str,0x0,6);
    own_sprintf(Str,"%d",temp);

    if(gTemperatureshowflag == 0) {
        if(temp > 99){
            Str[3] = 'C';
        }else if(temp > 9 ) {
            Str[2] = 'C';
            Str[3] = ' ';
        } else if(temp < 0) {
            if(temp + 9 >= 0) {
                Str[2] = 'C';
                Str[3] = ' ';
            } else {
                Str[3] = 'C';
            }
        } else {
            Str[1] = 'C';
            Str[2] = ' ';
            Str[3] = ' ';
        }
        Str[4] = ' ';
    } else {
        if(temp > 99) Str[3] = 'E';
        else if(temp > 9 ) {
            Str[2] = 'E';
            Str[3] = ' ';
        } else if(temp < 0) {
            if(temp + 9 >= 0) {
                Str[2] = 'E';
                Str[3] = ' ';
            } else {
                Str[3] = 'E';
            }
        } else {
            Str[1] = 'E';
            Str[2] = ' ';
            Str[3] = ' ';
        }
        Str[4] = ' ';
    }
    Display_Str(x,Str);
    TEMPSHOW_TIMER = 20;//200ms
}
/*******************************************************************************
函数名: Show_Notice
函数作用:显示待机提示信息
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void Show_Notice(void)
{
    int j,k;
    static u8* ptr0;
    static u8 posi = 0,i = 0;

    if(i == 0) { //第1  步
        ptr0 = Oled_DrawArea(0,0,96,16,(u8*)Maplib);
    } else if(i == 1) {//第2  步
        ptr0 = Oled_DrawArea(0,0,96,16,ptr0);
    } else if(i == 2) {//第3  步
        ptr0 = Oled_DrawArea(0,0,96,16,(u8*)Maplib);
    } else if(i == 3) {//第4  步
        for(j = 0 ; j < 6; j++) {
            k = 84;
            while(k >= posi) {
                ptr0 = (u8*)Maplib1 + j*28;
                Clean_Char(k+7,14);
                ptr0 = Oled_DrawArea(k,0,14,16,ptr0);
                k-=7;
                Delay_Ms(10);
            }
            posi += 14;
        }
        posi = 0;
    }
    i++;
    if(i == 4)  i = 0;
}
/*******************************************************************************
函数名: Show_Ver
函数作用:显示版本
输入参数:ver 版本号flag (0 :滚动显示 )(1不滚动)
返回参数:NULL
*******************************************************************************/
void Show_Ver(u8 ver[],u8 flag)
{
    u8 *ptr;
    int k,i;
    u8 temp0,temp1,temp2;
  
    if(ver[2] >= 0x30 && ver[2] < 0x3a)  temp1 = ver[2] - 0x30;
    if(ver[3] >= 0x30 && ver[3] < 0x3a)  temp2 = ver[3] - 0x30;
    if(ver[0] >= 0x30 && ver[0] < 0x3a)  temp0 = ver[0] - 0x30;

    for(i = 0; i < 24; i++) {
        Ver_s[4 * 24 + i] = Number12[temp0 * 24 + i];
        Ver_s[6 * 24 + i] = Number12[temp1 * 24 + i];
        Ver_s[7 * 24 + i] = Number12[temp2 * 24 + i];
    }

    for(k = 0; k < 16; k++) {
        if(flag == 0) {
            Show_ReverseChar((u8*)Ver_s,8,12,2);
            ptr = (u8*)gTemp_array;
        } else {
            ptr = (u8*)Ver_s;
        }
        for(i = 0; i < 8; i++) {
            ptr = Oled_DrawArea(i*12,0,12,16,ptr);
        }
    }
    if(flag == 0) {
        Delay_Ms(1000);
        Clear_Screen();
        Clear_Watchdog();
    }   
}

/*******************************************************************************
函数名: Show_Config
函数作用:显示CONFIG
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void Show_Config(void)
{
    u8* ptr;
    u8 j;

    ptr = (u8*)Config;
    for(j = 0; j < 6; j++) {
        ptr = Oled_DrawArea(j*16,0,16,16,ptr);
    }
}

/*******************************************************************************
函数名: Show_TempDown
函数作用:显示温度下降: 实际温度>>>目标温度
输入参数:temp 实际温度,dst_temp 目标温度
返回参数:NULL
*******************************************************************************/
void Show_TempDown(s16 temp,s16 dst_temp)
{
    static u8 guide_ui = 0;
    char str[8];

    memset(str,0x0,6);
    own_sprintf(str,"%d",temp);
    if(gTemperatureshowflag == 0) {
        if(temp > 99) str[3] = 'C';
        else {
            str[2] = 'C';
            str[3] = ' ';
        }
    } else {
        if(temp > 99) str[3] = 'E';
        else {
            str[2] = 'E';
            str[3] = ' ';
        }
    }

    Display_Str10(1,str);

    Oled_DrawArea(41,0,16,16,(u8*)(Guide + (guide_ui++) * 32));

    if(guide_ui == 3)   guide_ui = 0;

    memset(str,0x0,6);
    own_sprintf(str,"%d",dst_temp);
    if(gTemperatureshowflag == 0) {
        if(temp > 99) str[3] = 'C';
        else {
            str[2] = 'C';
            str[3] = ' ';
        }
    } else {
        if(temp > 99) str[3] = 'E';
        else {
            str[2] = 'E';
            str[3] = ' ';
        }
    }

    Display_Str10(56,str);
}
/*******************************************************************************
函数名: Show_Cal
函数作用:显示校准结果
输入参数:flag = 1 校准成功 flag = 2 校准失败
返回参数:NULL
*******************************************************************************/
void Show_Cal(u8 flag)
{
    u8 i;
    u8* ptr;

    if(flag == 1)       ptr = (u8*)Cal_Done;
    else if(flag == 2)  ptr = (u8*)Cal_UnDone;

    ptr = Oled_DrawArea(0,0,16,16,(u8*)ptr);

    for(i = 1; i < 6; i++)
        ptr = Oled_DrawArea(16 * i,0,16,16,(u8*)ptr);
    Delay_Ms(1000);
    Clear_Watchdog();
}
/*******************************************************************************
函数名: Show_Warning
函数作用:显示警告界面
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void Show_Warning(void)
{
    u8 i;
    u8* ptr;
    static u8 flag = 0;

    switch(Get_AlarmType()) {
    case HIGH_TEMP:
        ptr = (u8*)Warning;
        break;
    case SEN_ERR:
        ptr = (u8*)SenErr;
        break;
    case HIGH_VOLTAGE:
        ptr = (u8*)HighVt;
        break;
    case LOW_VOLTAGE:
        ptr = (u8*)LowVot;
        break;
    }

    Oled_DrawArea(0,0,16,16,(u8*)Warning + 20 * 7);
    if(flag == 0) {
        for(i = 2; i < 9; i++)  Oled_DrawArea(10 * i,0,10,16,(u8*)ptr +  (i - 2) * 20);
        flag = 1;
    } else {
        Clean_Char(16,80);
        flag = 0;
    }
}

/*******************************************************************************
函数名: Show_OrderChar
函数作用:横向动态显示字符
输入参数: ptr:字节库num:个数width:宽度
返回参数:NULL
*******************************************************************************/
void Show_OrderChar(u8* ptr,u8 num,u8 width)
{
    static u8 i = 1,j = 0,k = 0,m = 10;
    //i是源数组开始位
    //j是目标数组开始位
    //m是整个的长度

    if(gLevel_flag == 0) { //前一状态不是恒温
        i = 1;
        j = 0;
        m = 8;
        gUp_flag     = 0;
        gDown_flag   = 0;
        gLevel_flag  = 1;
    }

    if(i == 0) {
        for(k = 0; k <= m ; k++ ) {
            gTemp_array[k] = *(ptr + 52 + j + k - 1);
            gTemp_array[k + width] = *(ptr + 52 + width + k + j - 1);
        }
        m++;
        j--;
        if(m == 11) {
            m--;
            j = 0;
            i = 1;
            return ;
        }
    }

    if(j == 0) {
        for(k = 1; k < m ; k++ ) {
            gTemp_array[k + i] = *(ptr + 52 + j + k - 1);
            gTemp_array[k + i + width] = *(ptr + 52 + width + k + j - 1);
        }
        m--;
        i++;
        if(m == 0) {
            m++;
            j = 9;
            i = 0;
        }
    }
}

/*******************************************************************************
函数名: Reverse_Bin8
函数作用:按位逆向8位二进制  10101010 变为01010101
输入参数:逆向的数据
返回参数:逆向后的数据
*******************************************************************************/
u8 Reverse_Bin8(u8 data)
{
    u8 i, cache, result = 0;

    for (i = 0; i < 8; i++) {
        cache = data & (1 << i);
        if (i < 4) {
            cache <<= 7 - 2 * i;
        } else {
            cache >>= 2 * i - 7;
        }
        result += cache;
    }
    return result;
}
/*******************************************************************************
函数名: Show_ReverseChar
函数作用:竖向动态显示字符
输入参数: ptr:字节库   num:个数
              width:宽度   direction :方向 (0 up, 1 down)
返回参数:NULL
*******************************************************************************/
void Show_ReverseChar(u8* ptr,u8 num,u8 width,u8 direction)
{
    static u32 j = 0,m = 0,po_j[3] = {0,0,0},po_m[3] = {0,0,16};
    u32 i,k;

    if(direction == 0) { //up
        if(gUp_flag == 0) { //前一状态不是加热
            j = 0;
            m = 0;
            gUp_flag     = 1;
            gDown_flag   = 0;
            gLevel_flag  = 0;
        } else {
            j = po_j[0];
            m = po_m[0];
        }
    } else if(direction == 1) {
        if(gDown_flag == 0) { //前一状态不是降温
            j = 0;
            m = 0;
            gUp_flag     = 0;
            gDown_flag   = 1;
            gLevel_flag  = 0;
        } else {
            j = po_j[1];
            m = po_m[1];
        }
    } else {
        j = po_j[2];
        m = po_m[2];
    }
    for(i = 0; i < width * 2 * num; i++)   gTemp_array[i] = Reverse_Bin8(*(ptr + i)); //逆向8位

    for(k = 0; k < width * 2 * num; k += width * 2)
        for(i = 0; i < width ; i++) {
            gTemp_array_u16[i + k] = ((gTemp_array[i + k] & 0x00FF) << 8) | gTemp_array[i + k + width] ;//上半部下半部与成u16 便于移位
            if(direction == 1) {
                if(j == 0)  gTemp_array_u16[i + k] <<= m;//下面空，上面显示
                else        gTemp_array_u16[i + k] >>= j;//上面空，下面显示
            } else { //上
                if(m == 0)  gTemp_array_u16[i + k] <<= j;//下面空，上面显示
                else        gTemp_array_u16[i + k] >>= m;//上面空，下面显示
            }
            gTemp_array[i + k] = (gTemp_array_u16[i + k] & 0xFF00) >> 8;
            gTemp_array[i + k + width] = gTemp_array_u16[i + k] & 0x00FF;
        }

    for(i = 0; i < width * 2 * num; i++)   gTemp_array[i] = Reverse_Bin8(gTemp_array[i]); //移位后再逆向

    if(m == 0 && j == 16) { //全显示，换显示'头数'
        j = 0;
        m = 16;
    }
    if(m == 0)  j++;
    else        m--;

    if(direction == 0) { //up
        po_j[0] = j;
        po_m[0] = m;
    } else if(direction == 1) {
        po_j[1] = j;
        po_m[1] = m;
    } else {
        po_j[2] = j;
        po_m[2] = m;
    }
}
/*******************************************************************************
函数名: Show_HeatingIcon
函数作用:动态选择加热等状态标识
输入参数: ht_flag 恒温表示  active 移动标识
返回参数:NULL
*******************************************************************************/
void Show_HeatingIcon(u32 ht_flag,u16 active)
{
    u8* ptr;

    memset(gTemp_array,0,20);
    if(ht_flag == 0)         Show_ReverseChar((u8*)TempIcon,1,10,0);     //(5,ptr,16);//加热//
    else if(ht_flag == 1)    Show_ReverseChar((u8*)TempIcon + 32,1,10,1);//(5,ptr+32,16);//降温//
    else if(ht_flag == 2)    Show_OrderChar((u8*)TempIcon,1,10);       //(5,ptr+64,16);//恒温//

    ptr = (u8*)gTemp_array;
    Oled_DrawArea(86,0,10,16,(u8*)ptr);

    if(Get_CtrlStatus() == WAIT) Oled_DrawArea(80,0,6,16,(u8*)IhIcon + 12);
    else {
        if(active == 0) Oled_DrawArea(80,0,6,16,(u8*)TempIcon + 20);
        else            Oled_DrawArea(80,0,6,16,(u8*)IhIcon);
    }
}
/*******************************************************************************
函数名: Display_Str
函数作用:显示16*16字符串
输入参数:x: 位置 str :显示字符串
返回参数:NULL
*******************************************************************************/
void Display_Str(u8 x, char* str)
{
    u8* ptr;
    u8  temp;

    if((x < 1)||(x > 8))  x = 0;
    else x--;
    while(*str != 0) {
        temp = *str++;
        if(temp == ' ') temp = 10;
        else if(temp == '-')temp = 12;
        else {
            if((temp >= 0x30) && (temp < 0x3a)) temp = temp - 0x30; /* 0 --9 */
            else {
                if((temp >= 0x41) && (temp <= 0x46))  temp = temp - 0x41 + 11;/*ABD*/
                else return ;
            }
        }
        ptr = (u8*)wordlib;
        ptr += temp*32;
        Oled_DrawArea(x * 16,0,16,16,(u8*)ptr);
        x++;
    }
}

/*******************************************************************************
函数名: Display_Str
函数作用:显示12*16字符串
输入参数:x: 位置 str :显示字符串
返回参数:NULL
*******************************************************************************/
void Display_Str12(u8 x, char* str)
{
    u8* ptr;
    u8  temp;

    if((x < 1)||(x > 16))  x = 0;
    else x--;
    while(*str != 0) {
        temp = *str++ - ' ';//得到偏移后的值

        ptr = (u8*)ASCII12X16;
        ptr += temp*24;
        Oled_DrawArea(x * 12,0,12,16,(u8*)ptr);
        x++;
    }
}
/*******************************************************************************
函数名: Display_Str
函数作用:显示8*16字符串
输入参数:x: 位置 str :显示字符串  mode: 1：小数点字库
返回参数:NULL
*******************************************************************************/
void Display_Str8(u8 x, char* str ,u8 mode)
{
    u8* ptr;
    u8  temp;
  
    if( mode==0 )
    {
        if((x < 1)||(x > 16))  x = 0;
        else x--;
        while(*str != 0) {
            temp = *str++ - ' ';//得到偏移后的值

            ptr = (u8*)ASCII8X16;
            ptr += temp*16;
            Oled_DrawArea(x * 8,0,8,16,(u8*)ptr);
            x++;
        }
    }
    else
    {
        if((x < 1)||(x > 16))  x = 0;
        else x--;
        while(*str != 0) {
            temp = *str++ - '0';//得到偏移后的值

            ptr = (u8*)Decimal;
            ptr += temp*16;
            Oled_DrawArea(x * 8,0,8,16,(u8*)ptr);
            x++;
        }
    }
}
/*******************************************************************************
函数名: Triangle_Str
函数作用:显示宽度为16的箭头
输入参数:x: 位置 pos :箭头标志位
返回参数:NULL
*******************************************************************************/
void Triangle_Str(u8 x, u8 pos)
{
    u8* ptr;
    ptr = (u8*)Triangle + 16*2*pos ;
    Oled_DrawArea(x,0,16,16,(u8*)ptr);
}

/*****************************************************************
函数名: Show_Triangle
函数作用:显示设置模式中按键方向的三角方向的各种形态
输入参数: empty_trgl: 0,1,2  不加粗,左加粗,右加粗
          fill_trgl:  0,1,2  不实体,左实体,右实体
返回参数:NULL
*******************************************************************/
void Show_Triangle(u8 empty_trgl,u8 fill_trgl)
{
    int j;
    u8* ptr;

    ptr = (u8*)Triangle;

    if((empty_trgl == 0)&&(fill_trgl == 0)) {
        for(j = 0; j < 2; j++) {
            if(j == 0)        ptr = Oled_DrawArea(0,0,16,16,(u8*)ptr);
            else if(j == 1)   ptr = Oled_DrawArea(5*16,0,16,16,(u8*)ptr);
        }
    } else if((empty_trgl != 0)&&(fill_trgl == 0)) {
        if(empty_trgl == 1) {
            ptr += 32;
            Oled_DrawArea(5*16,0,16,16,(u8*)ptr);
            ptr += 32;
            Oled_DrawArea(0,0,16,16,(u8*)ptr);
        } else if (empty_trgl == 2) {
            Oled_DrawArea(0,0,16,16,(u8*)ptr);
            ptr += 32*3;
            Oled_DrawArea(5*16,0,16,16,(u8*)ptr);
        }
    } else if((empty_trgl == 0)&&(fill_trgl != 0)) {
        if(fill_trgl == 1) {
            ptr += 32;
            Oled_DrawArea(5*16,0,16,16,(u8*)ptr);
            ptr += 32*3;
            Oled_DrawArea(0,0,16,16,(u8*)ptr);
        } else if (fill_trgl == 2) {
            Oled_DrawArea(0,0,16,16,(u8*)ptr);
            ptr += 32*5;
            Oled_DrawArea(5*16,0,16,16,(u8*)ptr);
        }
    }
}

/*******************************************************************************
函数名: Display_Str6       
函数作用:显示宽度为6的字符串
输入参数:x: 位置 str :显示字符串
返回参数:NULL
*******************************************************************************/
void Display_Str6(u8 x, char* str)
{
    u8* ptr;
    u8  temp;
  
    if((x < 1)||(x > 96))  x = 0;
    else x--;
    while(*str != 0) {
        temp = *str++ - ' ';//得到偏移后的值

        ptr = (u8*)ASCII6X12;
        ptr += temp*12;
        Oled_DrawArea(x,0,6,12,(u8*)ptr);
        x = x + 6;
    }
}

/*******************************************************************************
函数名: Display_Str10
函数作用:显示宽度为10的字符串
输入参数:x: 位置 str :显示字符串
返回参数:NULL
*******************************************************************************/
void Display_Str10(u8 x, char* str)
{
    u8* ptr;
    u8 temp;
    u8 position = 0;

    while(*str != 0) {
        temp = *str++;
        if(temp == 'C') temp = 11;
        else if(temp == 'E') temp = 12;
        else {
            if((temp >= 0x30) && (temp < 0x3a)) temp = temp - 0x30; /* 0 --9 */
            else {
                temp = 10;
            }
        }
        ptr = (u8*)Number10;
        ptr += temp*20;
        Oled_DrawArea(x + position*10,0,10,16,(u8*)ptr);
        position++;
    }
}


/*******************************************************************************
函数名: Print_Integer
函数作用:整形数据打印函数
输入参数:data 数据,posi 位置
返回参数:NULL
*******************************************************************************/
void Print_Integer(s32 data,u8 posi)
{
    char str[8];

    Clear_Screen();
    own_sprintf(str,"%d",data);
    Display_Str(posi,str);
}
/*******************************************************************************
函数名: Calculated_Digit 
函数作用:计算一个整数的位数
输入参数:number ：整数
返回参数:整数的位数
*******************************************************************************/
u8 Calculated_Digit(int number)
{
    int n = 0;
    while(number!=0)
    {
        number = number/10;
        n++;
    }
    return n;
}

u8 gSet_opt = HD; //项目标志位
#define EFFECTIVE_KEY 800              //无按键等待返回时间
/*******************************************************************************
函数名: SetOpt_Detailed
函数作用:设置选中项具体信息
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void SetOpt_Detailed(void)
{
    int *temporary_set;//参数的地址
    s32 max_value,min_value;
    u16 step;
    UI_TIMER=HIGHLIGHT_FREQUENCY*2-HIGHLTGHT_REDUCE;
    EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
    SetOpt_UI(1);
    Delay_Ms(300);
    Set_LongKeyFlag(1);
    Set_gKey(NO_KEY);
    
    switch(gSet_opt){//在不同的选项下
    case WKT:
    case SDT:
        if(gSet_opt == SDT)         temporary_set = (int *)&device_info.t_standby;
        else if(gSet_opt == WKT)    temporary_set = (int *)&device_info.t_work;
        gSet_table[gSet_opt][2] = device_info.t_step;// 设置步长
    break;
    case WTT:
        temporary_set = (int *)&device_info.wait_time;
    break;
    case IDT:
        temporary_set = (int *)&device_info.idle_time;
    break;
    case STP:
        temporary_set = (int *)&device_info.t_step;
    break;
    case TOV:
        temporary_set = (int *)&gTurn_offv;
    break;
    case DGC://温度单位选择
        temporary_set = (int *)&gTemperatureshowflag;
    break;
    case HD:
        temporary_set = (int *)&device_info.handers;
    break;
    }
  /*----------------范围和增加单位赋值-------------------*/
    max_value = gSet_table[gSet_opt][0];
    min_value = gSet_table[gSet_opt][1];
    step = gSet_table[gSet_opt][2];
    if(gTemperatureshowflag == 1 && (gSet_opt == WKT || gSet_opt ==SDT))//F°模式最大值最小值转换
    {
        max_value = gSet_table[8][0];
        min_value = gSet_table[8][1];
        step = gSet_table[8][2];
    }
    while(1)
    {
        SetOpt_UI(1);
    /*------------------进入选项后的操作-------------------*/
        if(Get_gKey() == KEY_V1){//减
            if(gSet_opt == DGC && gTemperatureshowflag > 0)//切换温度模式
            {
                gTemperatureshowflag--;
                device_info.t_work = TemperatureShow_Change(1,device_info.t_work);
                device_info.t_standby = TemperatureShow_Change(1,device_info.t_standby);
                if(device_info.t_work < 600)    device_info.t_work = 600;//越界判断
                if(device_info.t_standby < 600)    device_info.t_standby = 600;
            }
            else if(gSet_opt != DGC)
            {
                if(*temporary_set > min_value) {//大于最小值(可以减少)
                    *temporary_set -= step;
                    if(*temporary_set < min_value) *temporary_set = min_value;//小于最小值(不能减少)
                }
            }
            Set_gKey(NO_KEY);//按键擦出
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;//退出倒计时重新计数
            UI_TIMER=HIGHLIGHT_FREQUENCY-HIGHLTGHT_REDUCE;//闪烁显示重新计数
            direction_flag = 1;
            SetOpt_UI(1);//显示闪烁图像
        }else if(Get_gKey() == KEY_V2){//加
            if(gSet_opt == DGC && gTemperatureshowflag < 1)//切换温度模式
            {
                gTemperatureshowflag++;
                device_info.t_work = TemperatureShow_Change(0,device_info.t_work);
                device_info.t_standby = TemperatureShow_Change(0,device_info.t_standby);
            }
            else if(gSet_opt != DGC)
            {
                if((*temporary_set) < max_value) {
                    *temporary_set += step;
                    if((*temporary_set) > max_value) *temporary_set = max_value;
                }
            }
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
            UI_TIMER=HIGHLIGHT_FREQUENCY-HIGHLTGHT_REDUCE;
            direction_flag = 2;
            SetOpt_UI(1);
        }else if(Get_gKey() == (KEY_CN | KEY_V1))//连续减
        {
            
            if(gSet_opt == DGC && gTemperatureshowflag > 0)//切换温度模式
            {
                gTemperatureshowflag--;
                device_info.t_work = TemperatureShow_Change(1,device_info.t_work);
                device_info.t_standby = TemperatureShow_Change(1,device_info.t_standby);
                if(device_info.t_work < 600)    device_info.t_work = 600;//越界判断
                if(device_info.t_standby < 600)    device_info.t_standby = 600;
            }
            else if(gSet_opt != DGC)
            {
                if(*temporary_set > min_value) {//大于最小值(可以减少)
                    *temporary_set -= step;
                    if(*temporary_set < min_value) *temporary_set = min_value;//小于最小值(不能减少)
                }
            }
            Delay_Ms(100);
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
            UI_TIMER=HIGHLIGHT_FREQUENCY-HIGHLTGHT_REDUCE;
            direction_flag = 1;
            SetOpt_UI(1);
        }else if(Get_gKey() == (KEY_CN | KEY_V2))//连续加
        {
            if(gSet_opt == DGC && gTemperatureshowflag < 1)//切换温度模式
            {
                gTemperatureshowflag++;
                device_info.t_work = TemperatureShow_Change(0,device_info.t_work);
                device_info.t_standby = TemperatureShow_Change(0,device_info.t_standby);
            }
            else if(gSet_opt != DGC)
            {
                if((*temporary_set) < max_value) {
                    *temporary_set += step;
                    if((*temporary_set) > max_value) *temporary_set = max_value;
                }
            }
            Delay_Ms(100);
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
            UI_TIMER=HIGHLIGHT_FREQUENCY-HIGHLTGHT_REDUCE;
            direction_flag = 2;
            SetOpt_UI(1);
        }
        else if(Get_gKey() == (KEY_CN | KEY_V3))//双键同时按，退出设置
        {
            EFFECTIVE_KEY_TIMER = 0;
        }
        Clear_Watchdog();
        if(EFFECTIVE_KEY_TIMER == 0)
        {
            Set_LongKeyFlag(0);
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY;
            UI_TIMER = 200;
            Exit_pos = 1;
            return ;
        }
    }
}

/*******************************************************************************
函数名: SetOpt_Proc  
函数作用:设置参数过程  
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void SetOpt_Proc(void)
{  
    if(KD_TIMER > 100){
        Set_gKey(NO_KEY);
        return ;//first
    }
    if(EFFECTIVE_KEY_TIMER < (EFFECTIVE_KEY - 100) & Exit_pos == 1)
    {
        Exit_pos = 0;
    }    
    if((Get_gKey() & KEY_CN) == KEY_CN){//长按之后,1S之内不能再长按
        Set_LongKeyFlag(0);
        KD_TIMER = 100;
    }
    if(KD_TIMER == 0)  Set_LongKeyFlag(1);//可以长按标志
    
    if(Get_gKey() == KEY_V1){//单机左右键选择菜单项
        if(gSet_opt > WKT)  gSet_opt--;
        else                gSet_opt = EXW;
        Set_gKey(NO_KEY);
        EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY;
        Clear_Screen();//清屏
    }else if(Get_gKey() == KEY_V2){
        if(gSet_opt < EXW)  gSet_opt++;
        else               gSet_opt = WKT;
        Set_gKey(NO_KEY);
        EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY;
        Clear_Screen();
    }

    if(Get_gKey() == (KEY_CN | KEY_V1) ||
       Get_gKey() == (KEY_CN | KEY_V2))//进入选中项的设置
    { 
        UI_TIMER=0;
        if(gSet_opt != EXW && gSet_opt != WDJ)
        {
            SetOpt_Detailed();
        }else if(gSet_opt == EXW){//初始化
            Restore_Setting();
            Set_gKey(NO_KEY);
            Clear_Screen();
        }else if(gSet_opt == WDJ){//校准
            Clear_Watchdog();
            Delay_Ms(1000);
            Clear_Screen();
            Zero_Calibration();//校准零点
            if(Get_CalFlag() == 1) {
                Disk_BuffInit();
                Config_Analysis();         // 启动虚拟U盘
            }
            if(gCalib_flag != 0) {
                //显示校准完成
                Show_Cal(gCalib_flag);
                Clear_Watchdog();
                Delay_Ms(1000);
                gCalib_flag = 0;
                Clear_Screen();
            }
            Set_gKey(NO_KEY);
        }
        EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY;
    }
    else if(Get_gKey() == (KEY_CN|KEY_V3) && Exit_pos == 0){//退出
        Set_LongKeyFlag(0);
        Set_gKey(NO_KEY);
        EFFECTIVE_KEY_TIMER = 0;//清除计数     
    }
    SetOpt_UI(0);    
}
/*******************************************************************************
函数名: SetOpt_UI
函数作用:设置参数显示
输入参数:key:闪烁开关
返回参数:NULL
*******************************************************************************/
void SetOpt_UI(u8 key)
{
    u8 buf[20] = {0};//显示的字符串
    u8 buf_TOV[2] = {0};
    u8 wdj[5];
    u8 buf_R[2] = ">";
    u8 buf_L[2] = "<";
    s16 temp_val;
    
    if(gSet_opt != EXW && gSet_opt != WDJ)//在显示温度校准/电压的选项时不显示
    {
        Display_Str8(0,gSys_settings[gSet_opt],0);//选项名
    }
    if(gSet_opt == WDJ)
    {}
    else
    {
        if(gSet_opt == WKT){//选项里面内容
            own_sprintf((char *)buf,"%d",device_info.t_work/10);
            digit = Calculated_Digit(device_info.t_work/10);//求位数
        }else if(gSet_opt == SDT){
            own_sprintf((char *)buf,"%d",device_info.t_standby/10);
            digit = Calculated_Digit(device_info.t_standby/10);
        }else if(gSet_opt == WTT){
            own_sprintf((char *)buf,"%d",device_info.wait_time/100);
            digit = Calculated_Digit(device_info.wait_time/100);
        }else if(gSet_opt == IDT){    
            own_sprintf((char *)buf,"%d",device_info.idle_time/100);
            digit = Calculated_Digit(device_info.idle_time/100);
        }else if(gSet_opt == STP){    
            own_sprintf((char *)buf,"%d",device_info.t_step/10);
            digit = Calculated_Digit(device_info.t_step/10);
        }else if(gSet_opt == TOV){    
            own_sprintf((char *)buf,"%d",gTurn_offv/10);
            own_sprintf((char *)buf_TOV,"%d",gTurn_offv%10);
            digit = Calculated_Digit(gTurn_offv);
        }else if(gSet_opt == DGC){
            if(gTemperatureshowflag == 0)   strcpy((char *)buf,"CT");
            else                            strcpy((char *)buf,"FT");
            digit = 2;
        }else if(gSet_opt == HD){
            if(device_info.handers == 0){
                strcpy((char *)buf,"RT");
            }else{
                strcpy((char *)buf,"LT");
            }
            if(gHanders != device_info.handers)
            {
                Init_Oled();//界面初始化
                gHanders = device_info.handers;
            }
            digit = 2;
        }else if(gSet_opt == EXW){
            own_sprintf((char *)buf," Factory Reset");
        }
    }

    if(key == 1)//是否打开闪烁开关
    {
        if(UI_TIMER <HIGHLIGHT_FREQUENCY)//显示
        {
            if(direction_flag == 1)//左
            {
                Display_Str8(SET_INFO_X,(char *)buf_L,0);//<
                if(gSet_opt == TOV){                         
                    Display_Str8(SET_PROMPT_X,(char *)buf,0);
                    Display_Str8(SET_PROMPT_X + digit - 1,(char *)buf_TOV,1);
                }
                else    Display_Str8(SET_PROMPT_X,(char *)buf,0);//数值
                Display_Str8(SET_PROMPT_X + digit," ",0);
            }
            else if(direction_flag == 2)//右
            {
                Display_Str8(SET_PROMPT_X-1," ",0);
                if(gSet_opt == TOV){                         
                    Display_Str8(SET_PROMPT_X,(char *)buf,0);
                    Display_Str8(SET_PROMPT_X + digit - 1,(char *)buf_TOV,1);
                }
                else    Display_Str8(SET_PROMPT_X,(char *)buf,0);//数值
                Display_Str8(SET_PROMPT_X + digit,(char *)buf_R,0);//>
            }
            else//无操作
            {
                Display_Str8(SET_INFO_X,(char *)buf_L,0);//<
                if(gSet_opt == TOV){                         
                    Display_Str8(SET_PROMPT_X,(char *)buf,0);
                    Display_Str8(SET_PROMPT_X + digit - 1,(char *)buf_TOV,1);
                }
                else    Display_Str8(SET_PROMPT_X,(char *)buf,0);//数值
                Display_Str8(SET_PROMPT_X + digit,(char *)buf_R,0);//>
            } 
            if(UI_TIMER == 0)//一次高亮周期结束重新计数
            {
                UI_TIMER=(HIGHLIGHT_FREQUENCY*2) -HIGHLTGHT_REDUCE;
                direction_flag=0;
            }
        }
        else//擦出
        {
            Display_Str8(SET_INFO_X," ",0);
            Display_Str8(SET_PROMPT_X + digit,"   ",0);
        }
    }
    else//没打开闪烁开关
    {
        if(gSet_opt == WDJ)//温度计和电压显示
        {
            if(UI_TIMER == 0) {
                temp_val = Get_Temp(0);//获取当前温度
                if(Get_TemperatureShowFlag() == 1) {//获取温度显示方式
                    temp_val = TemperatureShow_Change(0,temp_val);
                    own_sprintf((char *)wdj,"%d} ",temp_val/10);
                }
                else    own_sprintf((char *)wdj,"%d{ ",temp_val/10);
                Display_Str8(8,(char *)wdj,0);//显示温度
                Show_Volt();//显示电压
                UI_TIMER = 20;
            }
        }       
        else if(gSet_opt == EXW)//恢复出厂设备
        {
            Display_Str6(4,(char *)buf);
        }
        else
        {   
            Display_Str8(SET_INFO_X," ",0);
            if(gSet_opt == TOV){                         
                Display_Str8(SET_PROMPT_X,(char *)buf,0);
                Display_Str8(SET_PROMPT_X + digit - 1,(char *)buf_TOV,1);
            }
            else    Display_Str8(SET_PROMPT_X,(char *)buf,0);//数值
            Display_Str8(SET_PROMPT_X + digit,"   ",0);
        }
    }
}   
    

/*******************************************************************************
函数名: Shift_Char
函数作用:字符从右往左移动到指定位置
输入参数:ptr 移动的字符 pos 移动到的指定位置
返回参数:NULL
*******************************************************************************/
void Shift_Char(u8* ptr,u8 pos)
{
    int k;
    u8* ptr0;

    k = 80;
    pos = pos * 16;//位置*  字符=   字符所在位置
    while(k >= pos) {
        ptr0 = (u8*)ptr;
        Clean_Char(k + 16,16);
        ptr0 = Oled_DrawArea(k,0,16,16,(u8*)ptr0);
        k-=16;
        Delay_Ms(25);
    }
}
/*******************************************************************************
Show_TempReverse 竖向动态显示温度字符
word_num:     个数
word_width:   宽度
direction :   方向 (0 up, 1 down)
*******************************************************************************/
u8 Show_TempReverse(u8 num,u8 width,u8 direction)
{
    static int i,j = 0,m = 16,k;
    u8 g , s ,b;
    u8* ptr;
    s16 num_temp; 
   
    num_temp = device_info.t_work;


    num_temp = num_temp/10;
    b = num_temp/100;
    s = (num_temp - b * 100)/10;
    g = (num_temp - b * 100 - s * 10);

    if(gTempset_showctrl == 1) {
        j = 1;
        m = 0;
        gTempset_showctrl = 0;
    } else if(gTempset_showctrl == 2) {
        j = 0;
        m = 16;
        gTempset_showctrl = 0;
    }
    for(i = 0; i < width * 2; i++) {
        gTemp_array[0*32 + i] = Reverse_Bin8(*(wordlib + b*32 + i)); //逆向8位
        gTemp_array[1*32 + i] = Reverse_Bin8(*(wordlib + s*32 + i)); //逆向8位
        gTemp_array[2*32 + i] = Reverse_Bin8(*(wordlib + g*32 + i)); //逆向8位
        if(Get_TemperatureShowFlag() == 1) {
            gTemp_array[3*32 + i] = Reverse_Bin8(*(wordlib + 15*32 + i)); //逆向8位
        } else {
            gTemp_array[3*32 + i] = Reverse_Bin8(*(wordlib + 13*32 + i)); //逆向8位
        }
    }
    //修改K 的初始值能改变开始滚动的字符位置个位还是十位还是百位
    for(k = (3 - num) * width * 2; k < width * 2 * 3; k += width * 2)
        for(i = 0; i < width ; i++) {
            gTemp_array_u16[i + k] = ((gTemp_array[i + k] & 0x00FF) << 8) | gTemp_array[i + k + width] ;//上半部下半部与成u16 便于移位
            if(direction == 0) { //上
                if(m == 0)  gTemp_array_u16[i + k] <<= j;//下面空，上面显示
                else        gTemp_array_u16[i + k] >>= m;//上面空，下面显示
            } else {
                if(j == 0)  gTemp_array_u16[i + k] <<= m;//下面空，上面显示
                else        gTemp_array_u16[i + k] >>= j;//上面空，下面显示
            }
            gTemp_array[i + k] = (gTemp_array_u16[i + k] & 0xFF00) >> 8;
            gTemp_array[i + k + width] = gTemp_array_u16[i + k] & 0x00FF;
        }
    for(i = 0; i < width * 2; i++) {
        gTemp_array[0*32 + i] = Reverse_Bin8(gTemp_array[0*32 + i]);
        gTemp_array[1*32 + i] = Reverse_Bin8(gTemp_array[1*32 + i]);
        gTemp_array[2*32 + i] = Reverse_Bin8(gTemp_array[2*32 + i]);
        gTemp_array[3*32 + i] = Reverse_Bin8(gTemp_array[3*32 + i]);
    }

    ptr = (u8*)gTemp_array;
    for(i = 1; i <= 4; i++) ptr = Oled_DrawArea(16 * i,0,16,16,(u8*)ptr);
    if((m == 0 && j == 0)||(m == 0 && j == 16))  return 0 ;
    if(m == 0)  j++;
    else        m--;
    return 1;
}

/*******************************************************************************
函数名: Show_Set
函数作用:显示设置界面,从右到左移动过来
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void Show_Set(void)
{
    u8  m ,k;
    s16 num_temp;
    num_temp = device_info.t_work;
    
    Shift_Char((u8*)Triangle + 0*32,0);//<
    m = num_temp/1000;   //百
    Shift_Char((u8*)wordlib + m*32,1);
    k = (num_temp - m * 1000)/100;//十
    Shift_Char((u8*)wordlib + k*32,2);
    m = (num_temp - m * 1000 - k * 100)/10;//个
    Shift_Char((u8*)wordlib + m*32,3);
    
    if(gTemperatureshowflag)    Shift_Char((u8*)wordlib + 15*32,4);//F
    else                        Shift_Char((u8*)wordlib + 13*32,4);//C
    
    Shift_Char((u8*)Triangle + 1*32,5);
}
/*******************************************************************************
函数名: Roll_Num
函数作用:设置中，需要滚动的温度值的位数
输入参数:step 步长,Flag (0+ or 1-)
返回参数:需要滚动的个数
*******************************************************************************/
u8 Roll_Num(u16 step,u8 flag)
{
    u16 b1,b2,g1,g2;
    s16 num_temp;

    num_temp = device_info.t_work;    
    b2 = (num_temp)/1000;       
    g2 = (num_temp)/100;        

    if(flag == 0) {
        b1 = (num_temp + step)/1000;
        g1 = (num_temp + step)/100;
        if(b1 != b2)  return 3;
        else if(g1 != g2)  return 2;
        else        return 1;
    } else {
        b1 = (num_temp - step)/1000;
        g1 = (num_temp - step)/100;

        if(b1 != b2)  return 3;
        else if(g1 != g2)  return 2;
        else        return 1;
    }
}

/*******************************************************************************
函数名: Temp_SetProc
函数作用:临时设置温度
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void TempSet_Proc(void)
{
    u8 theRoll_num = 3;
    
    static u8 add_step = 0;//step=1时长按递加
    int *temporary_set;
    s32 max_value,min_value;
    u16 step = device_info.t_step;
    Set_LongKeyFlag(1);
    temporary_set = (int *)&device_info.t_work;
    
    if(gTemperatureshowflag == 0)
    {
        max_value = gSet_table[WKT][0];
        min_value = gSet_table[WKT][1];
    }
    else
    {
        max_value = gSet_table[8][0];
        min_value = gSet_table[8][1];
    }
    step = device_info.t_step;

    if(EFFECTIVE_KEY_TIMER > 0) {
        if(gCont != 0) {
            //设置界面从右往左出来
            gCont = 0;
            Set_LongKeyFlag(0);
            Clear_Screen();
            Show_Set();
        }if(Get_gKey() == KEY_V3)//双键单机查看电压
        {
            Clear_Screen();
            Set_gKey(NO_KEY);//按键擦除
            while(1)
            {
                if(Get_gKey() == (KEY_V3 | KEY_CN))
                {
                    EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
                    break;
                }
                if(EFFECTIVE_KEY_TIMER%20 ==0)
                {
                    Show_Volt();//显示电压
                }
                if(EFFECTIVE_KEY_TIMER == 0)
                {   
                    EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
                }
                Clear_Watchdog();                
            }
        }else if(Get_gKey() == KEY_V1)//减
        {
            if(*temporary_set > min_value) {//大于最小值(可以减少)
                add_step = 0;
                gTempset_showctrl = 1;
                theRoll_num = Roll_Num(step,1);//计算要滚动的位数
                *temporary_set -= step;
                if(*temporary_set < min_value) *temporary_set = min_value;//小于最小值(不能减少)
                while(Show_TempReverse(theRoll_num,16,1));
                gTempset_showctrl = 2;
                while(Show_TempReverse(theRoll_num,16,1));
                Show_Triangle(1,0);
                if(*temporary_set == min_value)  Show_Triangle(0,1);
            }
            Set_gKey(NO_KEY);//按键擦除
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;//退出倒计时重新计数
        }
        else if(Get_gKey() == KEY_V2)//加
        {
            if((*temporary_set) < max_value) {
                add_step = 0;
                gTempset_showctrl = 1;
                theRoll_num = Roll_Num(step,0);
                *temporary_set += step;
                if((*temporary_set) > max_value) *temporary_set = max_value;
                while(Show_TempReverse(theRoll_num,16,0));
                gTempset_showctrl = 2;
                while(Show_TempReverse(theRoll_num,16,0));
                Show_Triangle(2,0);
                if(*temporary_set == max_value)  Show_Triangle(0,2);
            }
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
        }
        else if(Get_gKey() == (KEY_CN | KEY_V1))//连续减
        {
            if(*temporary_set > min_value) {
                if(add_step <250 && device_info.t_step == 10)  add_step += 10;
                step = device_info.t_step + add_step;
                gTempset_showctrl = 1;
                theRoll_num = Roll_Num(step,1);//计算要滚动的位数
                *temporary_set -= step;
                if(*temporary_set < min_value) *temporary_set = min_value;
                while(Show_TempReverse(theRoll_num,16,1));
                gTempset_showctrl = 2;
                while(Show_TempReverse(theRoll_num,16,1));
                Show_Triangle(1,0);
                if(*temporary_set == min_value)  Show_Triangle(0,1);
            }
            Delay_Ms(50);
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
        }
        else if(Get_gKey() == (KEY_CN | KEY_V2))//连续加
        {
            if((*temporary_set) < max_value) {
                if(add_step <250 && device_info.t_step == 10)  add_step += 10;
                step = device_info.t_step + add_step;
                gTempset_showctrl = 1;
                theRoll_num = Roll_Num(step,0);
                *temporary_set += step;
                if((*temporary_set) > max_value) *temporary_set = max_value;
                while(Show_TempReverse(theRoll_num,16,0));
                gTempset_showctrl = 2;
                while(Show_TempReverse(theRoll_num,16,0));
                Show_Triangle(2,0);
                if(*temporary_set == max_value)  Show_Triangle(0,2);
            }
            Delay_Ms(50);
            Set_gKey(NO_KEY);
            EFFECTIVE_KEY_TIMER = EFFECTIVE_KEY/2;
        }
        if(EFFECTIVE_KEY_TIMER < EFFECTIVE_KEY/2 -50)
        {
            add_step = 0;
        }
    }
}
/*******************************************************************************
函数名: Temp_SetProc
函数作用:设置相关参数或临时温度
输入参数:key  0:只设置临时的温度  1:设置并保存参数
返回参数:NULL
*******************************************************************************/
void Temp_SetProc(u8 key)
{       
    if(key)//设置参数
    {
        if(EFFECTIVE_KEY_TIMER > 0) {
           SetOpt_Proc();
        }
    }
    else//临时温度
    {
        TempSet_Proc();
    }
}

/*******************************************************************************
函数名: OLed_Display
函数作用:屏幕显示主控制函数
输入参数:NULL
返回参数:NULL
*******************************************************************************/
void OLed_Display(void)
{
    u16 ht_flag;
    s16 temp_val,dst_temp;
    static u8 td_flag = 0,ver_flag = 0,config_show = 0;
    static u16 td_cnt = 0;
    static u16 bk = 0x33,id_cnt = 0;

    switch (Get_CtrlStatus()) {//状态判断
    case IDLE://待机状态
        if(gCont == 1) {
            gCont = 0;
            Clear_Screen();
        }

        if(ver_flag == 0) {
            Display_BG();
            Show_Ver(device_info.ver,0);
            ver_flag = 1;
        } else if(UI_TIMER == 0 && G6_TIMER != 0) {
            Show_Notice();//显示待机图像
            UI_TIMER = 50;
        }
        if(G6_TIMER == 0) { //屏保
            id_cnt++;
            if(id_cnt == 50)Sc_Pt(bk--);
            if(bk == 0) Oled_DisplayOff();
        }
        if((Get_MmaShift() == 1) || (Get_gKey() != NO_KEY)) {
            G6_TIMER = device_info.idle_time;
            bk = 0x33;
            Sc_Pt(bk);
            Oled_DisplayOn();
        }
        break;
    case TEMP_CTR://温控模式
        if(gCont == 0) {
            gCont = 1;
            Set_LongKeyFlag(1);
            Clear_Screen();
        }

        ht_flag = Get_HtFlag();

        if(ht_flag != 1) {
            if(td_flag == 1)  Clear_Screen();//清屏
            td_cnt  = 0;
            td_flag = 0;
        }

        if(td_cnt == 75 && td_flag == 0) {
            Clear_Screen();
            td_flag = 1;
        }

        if(td_flag && UI_TIMER == 0) {
            temp_val = Get_TempVal();
            dst_temp = device_info.t_work;
            if(Get_TemperatureShowFlag() == 1) {
                temp_val = TemperatureShow_Change(0,temp_val);
                dst_temp = TemperatureShow_Change(0,dst_temp);
            }
            Show_TempDown(temp_val,dst_temp);
            UI_TIMER = 50;
        }
        if((TEMPSHOW_TIMER == 0) && (!td_flag)) {
            temp_val = Get_TempVal();
            if((temp_val > device_info.t_work) && (temp_val - device_info.t_work < 18))       temp_val = device_info.t_work;
            else if((temp_val <= device_info.t_work) && (device_info.t_work - temp_val < 18)) temp_val = device_info.t_work;
            if(Get_TemperatureShowFlag() == 1) {
                temp_val = TemperatureShow_Change(0,temp_val);
            }

            Display_Temp(1,temp_val/10);
            Show_HeatingIcon(ht_flag,Get_MmaActive());//0升温1降温2恒温
            td_cnt++;
        }
        break;
    case TEMP_SET:
        if(TEMP_SET_Pos == 0)           Temp_SetProc(0);//只改变临时温度
        else                            Temp_SetProc(1);//改变参数并保存
        break;
//    case VOLT:
//        Show_Volt();//显示电压
//        break;
    case CONFIG://CONFIG
        if(gCont == 1) {
            gCont = 0;
            Clear_Screen();
        }
        switch(Get_gKey()) {
        case KEY_CN|KEY_V1:
            config_show = 1;
            break;
        case KEY_CN|KEY_V2:
            config_show = 2;
            break;
        case KEY_CN|KEY_V3:
            config_show = 0;
            break;
        default:
            break;
        }
        if(config_show == 0) {
            Show_Config();//显示CONFIG
        } else if(config_show == 1) {
            Display_BG();
        } else if(config_show == 2) {
            Show_Ver(device_info.ver,1);
            Delay_Ms(250);
            bTetris = 1;
        }
        if(config_show != 3) {
            Set_gKey(NO_KEY);
            config_show = 3;
        }
        break;
//    case THERMOMETER://温度计模式
//        if(gCont == 0) {
//            gCont = 1;
//            Clear_Screen();//清屏
//        }
//        if(gCalib_flag != 0) {
//            //显示校准完成
//            Show_Cal(gCalib_flag);
//            gCalib_flag = 0;
//            Clear_Screen();
//        }
//        if(UI_TIMER == 0) {
//            temp_val = Get_Temp(0);
//            if(Get_TemperatureShowFlag() == 1) {
//                temp_val = TemperatureShow_Change(0,temp_val);
//            }
//            Display_Temp(1,temp_val/10);
//            Display_Str(6,"F");
//            UI_TIMER = 20;
//        }
//        break;
    case ALARM:
        if(gCont == 0) {
            gCont = 1;
            Clear_Screen();
        }
        if(gCont == 1 && UI_TIMER == 0) {
            Show_Warning();
            UI_TIMER = 50;
        }
        break;
    case WAIT:
        temp_val = Get_TempVal();
        if((temp_val > device_info.t_standby) && (temp_val - device_info.t_standby <= 18))       temp_val = device_info.t_standby;
        else if((temp_val <= device_info.t_standby) && (device_info.t_standby - temp_val <= 18)) temp_val = device_info.t_standby;

        ht_flag = Get_HtFlag();

        if(ht_flag != 1) {
            if(td_flag == 1)  Clear_Screen();
            td_cnt  = 0;
            td_flag = 0;
        }

        if(td_cnt == 50 && td_flag == 0) {
            Clear_Screen();
            td_flag = 1;
        }

        if(td_flag && UI_TIMER == 0) {
            dst_temp = device_info.t_standby;
            if(Get_TemperatureShowFlag() == 1) {
                temp_val = TemperatureShow_Change(0,temp_val);
                dst_temp = TemperatureShow_Change(0,dst_temp);
            }
            Show_TempDown(temp_val,dst_temp);
            UI_TIMER = 50;
        }
        if((TEMPSHOW_TIMER == 0) && (!td_flag)) {
            if(Get_TemperatureShowFlag() == 1) {
                temp_val = TemperatureShow_Change(0,temp_val);
            }
            Display_Temp(1,temp_val/10);
            Show_HeatingIcon(ht_flag,Get_MmaActive());//0升温1降温2恒温
            td_cnt++;
        }
        break;
    default:
        break;
    }
}

//////////////////////////////////////////////////////////////

int bTetris = 0;

#define FIELD_W 8
#define FIELD_H 48
#define NUM_FIGURES 7

int fmap[7][4][4] = {
  {
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {1, 0, 0, 0},
    {1, 0, 0, 0},
    {1, 0, 0, 0},
    {1, 0, 0, 0}
  },
  {
    {0, 0, 1, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {1, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {0, 1, 1, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {1, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  }
};

int screen[FIELD_W][FIELD_H] = {0};
int map[4][4];
int px, py, score, nextmap;
u8 buf[192];

unsigned int seed = 0x269ec3;
int rnd( int range ) {
    seed = seed*0x343fD + 0x269ec3;
    return ((seed>>16)&0x7fff) % range;
}

void putpixel(int x, int y, int color, u8 * buf) {
  if (x<0 || x>=96) return;
  if (y<0 || y>=16) return;
  u8 b = 1 << (y % 8);
  buf += (y/8)*96 + (x%96);
  if (color)
    *buf |= b;
  else
    *buf &= ~b;
}

void draw_field(int x, int y, u8 * buf, int buff[FIELD_W][FIELD_H]) {
  int i,j,w,h;
  int p = 2;
  for (i=0; i<FIELD_H; i++)
    for (j=0; j<FIELD_W; j++)
      for (w=0; w<p; w++)
        for (h=0; h<p; h++)
          putpixel(96-1-(x+i*p+w), 16-1-(y+j*p+h),buff[j][i], buf);
}

void print(void) {
  int i, j;
  static int buff[FIELD_W][FIELD_H];

  for(i = 0; i < FIELD_H; i++)
    for(j = 0; j < FIELD_W; j++)
      buff[j][i] = screen[j][i];

  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      if(map[j][i])
        buff[j+px][i+py] = 1;

  draw_field(0,0,buf, buff);
}


int valid(int x, int y) {
  int i, j;
  if(x < 0) return FALSE;
  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      if(map[j][i]) {
        if ((j+x>=FIELD_W) || (i+y>= FIELD_H)) return FALSE;
        if (screen[j+x][i+y]) return FALSE;
      }
  return TRUE;
}

#define inv(x) ((x*(-1))+3)

void rotatemap(void) {
  int _map[4][4];
  int i, j, sx = 4, sy = 4;

  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++) {
      _map[j][i] = map[j][i];
      if(map[j][i]) {
        if(i < sx) sx = i;
        if(inv(j) < sy) sy = inv(j);
      }
    map[j][i] = 0;
  }

  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      if(_map[inv(i)][j]) map[j - sx][i - sy] = 1;

  if(!valid(px,py)) for(i = 0; i < 4; i++) for(j = 0; j < 4; j++) map[j][i] = _map[j][i];
}

void deleteline(void) {
  int i, j, k, cl;

  for(i = FIELD_H - 1; i >= 0; i--) {
    cl = 1;

    for(j = 0, cl = 1; j < FIELD_W; j++)
      if(!screen[j][i]) cl = 0;

    if(cl) {
        score += (((i * (-1)) + FIELD_H) * 10);
        for(k = i; k > 0; k--) {
            for(j = 0; j < FIELD_W; j++) {
                screen[j][k] = screen[j][k - 1];
            }
        }
        i++;
        print();
    }
  }
}

void createmap(void) {
  int i, j; 
  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      map[j][i] = fmap[nextmap][j][i];
  py = 0;
  px = FIELD_W / 2;
  nextmap = rnd(NUM_FIGURES);
}

void clearscreen(void) {
  int i, j;
  for(i = 0; i < FIELD_H; i++) 
    for(j = 0; j < FIELD_W; j++) 
      screen[j][i] = 0;
}

void startgame(void) {
  clearscreen();
  px = FIELD_W / 2;
  py = 0;
  score = 0;
  nextmap = rnd(NUM_FIGURES);
  createmap();
}

int gameover(void) {
  int i;
  for(i = 0; i < FIELD_W; i++)
    if (screen[i][0]) return TRUE;
  return FALSE;
}

void advancefigure(void) {
  int i, j;

  if (!valid(px, py + 1)) {
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            if (map[j][i])
              screen[px+j][py+i] = 1;

    createmap();
    deleteline();
  }
  else
    py++;
}

void dropfigure(void) {
  int i, j;
  for(; valid(px, py + 1); py++);
  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      if(map[j][i])
        screen[px+j][py+i] = 1;
}

int bInit = 0;

void Show_Tetris(void) {

  if (!bInit) {
     startgame();
     bInit = 1;
  }

  switch(Get_gKey()) {
    case KEY_V1: if(valid(px - 1, py)) px--; break;
    case KEY_V2: if(valid(px + 1, py)) px++; break;
    case KEY_V3: rotatemap(); break;
    case KEY_CN|KEY_V3: bTetris = 0; break;

    case KEY_CN|KEY_V1:
      if (KD_TIMER==0) {
        dropfigure();
        print();
        deleteline();
        createmap();
        Set_gKey(NO_KEY);
        KD_TIMER = 50;
      }
    break;

    case KEY_CN|KEY_V2:
      if (KD_TIMER==0) {
        rotatemap();
        Set_gKey(NO_KEY);
        KD_TIMER = 25;
      }
    break;
    default: break;
  }

  if (Get_gKey()!=0)
    Set_gKey(NO_KEY);

  if (UI_TIMER==0) {
    UI_TIMER = 50;
    advancefigure();
  }

  print();

  if (gameover())
    startgame();

  Oled_DrawArea(0,0,96,16,buf);
}


/******************************** END OF FILE *********************************/
