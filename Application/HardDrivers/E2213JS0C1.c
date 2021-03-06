#include "E2213JS0C1.h"
#include "stdint.h"
#include "stm32f1xx_hal_spi.h"
#include "font.h"


static void E2213JS0C1_SendReadByte(uint8_t byte);
static void E2213JS0C1_WriteRegIndex(uint8_t cmd);
static void E2213JS0C1_WriteData8(uint8_t data);
//static void E2213JS0C1_WriteData16(uint16_t data);
static void E2213JS0C1_WriteMultipleData(uint8_t *pData, uint32_t Size);
static void E2213JS0C1_WaiteUntilNotBusy(void);

uint8_t E2213JS0C1_FirstFrameBuffer[E2213JS0C1_BUFFER_SIZE];
uint8_t E2213JS0C1_SecondFrameBuffer[E2213JS0C1_BUFFER_SIZE];


/**
 * @brief	SPI收/发数据
 * @param	byte：需要发送的数据
 * @retval	收到的数据
 */
static void E2213JS0C1_SendReadByte(uint8_t byte)
{
//	/* 检查Tx缓冲区是否为空 */
//	while (!LL_SPI_IsActiveFlag_TXE(E2213JS0C1_SPI));
//	/* 发送数据 */
//	LL_SPI_TransmitData8(E2213JS0C1_SPI, byte);
//	/* 等待数据被发送完毕 */
//	while(LL_SPI_IsActiveFlag_BSY(E2213JS0C1_SPI));
	while (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY_TX);
	HAL_SPI_Transmit(&hspi1, &byte, 1, 0xff);
}

/**
 * @brief	发指令
 * @param	cmd：需要发送的指令
 * @retval	none
 */
static void E2213JS0C1_WriteRegIndex(uint8_t cmd)
{
    E2213JS0C1_CS_ENABLE();
	E2213JS0C1_DC_CMD();
	E2213JS0C1_SendReadByte(cmd);
	E2213JS0C1_DC_DATA();
    E2213JS0C1_CS_DISABLE();
}

/**
 * @brief	发数据（8bit）
 * @param	data：需要发送的数据(8bit)
 * @retval	none
 */
static void E2213JS0C1_WriteData8(uint8_t data)
{
    E2213JS0C1_CS_ENABLE();
	E2213JS0C1_SendReadByte(data);
    E2213JS0C1_CS_DISABLE();
}

///**
// * @brief	发数据（16bit）
// * @param	data：需要发送的数据(16bit)
// * @retval	none
// */
//static void E2213JS0C1_WriteData16(uint16_t data)
//{
//    E2213JS0C1_CS_ENABLE();
//	E2213JS0C1_SendReadByte(data >> 8);
//	E2213JS0C1_SendReadByte(data);
//    E2213JS0C1_CS_DISABLE();
//}

/**
 * @brief	等待，直到BUSY脚为高电平
 * @param	none
 * @retval	none
 */
static void E2213JS0C1_WaiteUntilNotBusy(void)
{
    while(E2213JS0C1_BUSY_READ() == 0)
    { 
        HAL_Delay(100);
    }    
}

/**
 * @brief	向屏幕发送任意长度的数据
 * @param	pData：需要发送数据的指针；
 * @param	Size：需要发送数据的长度
 * @retval	none
 */
static void E2213JS0C1_WriteMultipleData(uint8_t *pData, uint32_t Size)
{
	/* 定义一个uint32_t变量，用于计数 */
	uint32_t counter = 0;
    
	E2213JS0C1_CS_ENABLE();
    E2213JS0C1_DC_DATA();
	
	/* 如果长度只有1个字节 */
	if (Size == 1)
	{
		/* 如果只有一个字节，就直接调用通用函数发送就好 */
		E2213JS0C1_SendReadByte(*pData);	
	}
	/* 如果长度大于1 */
	else
	{
		/* 用for循环，把数据一个一个的发出去，这里是一次发两个u8，循环一次就发一个u16 */
		for (counter = Size; counter != 0; counter--)
		{
			E2213JS0C1_SendReadByte(*pData);
			E2213JS0C1_SendReadByte(*(pData + 1));
			counter--;
			pData += 2;
		}
	}
    E2213JS0C1_CS_DISABLE();
}

/**
 * @brief	初始化
 * @param	none
 * @retval	none
 */
void E2213JS0C1_Init(void)
{
    /* 开启SPI */
//    LL_SPI_Enable(E2213JS0C1_SPI); 
    
    /* 硬件复位 */
    E2213JS0C1_RST_ENABLE();
    HAL_Delay(5);
    E2213JS0C1_RST_DISABLE();
    HAL_Delay(10);
    E2213JS0C1_RST_ENABLE();
    HAL_Delay(5);
    /* 软件复位 */
    E2213JS0C1_WriteRegIndex(SOFT_RESET_CMD);
    E2213JS0C1_WriteData8(SOFT_RESET_DATA);
    HAL_Delay(5);
    /* 设置环境温度（写死25摄氏度） */
    E2213JS0C1_WriteRegIndex(SET_TEMP_CMD);
    E2213JS0C1_WriteData8(SET_TEMP_25_DATA);
    /* Active Temperature */
    E2213JS0C1_WriteRegIndex(ACTIVE_TEMP_CMD);
    E2213JS0C1_WriteData8(ACTIVE_TEMP_25_DATA);
//    /* Panel Settings，官方手册中说要，但是代码中没有体现，所以注释掉 */
//    E2213JS0C1_WriteRegIndex(PANEL_SET_CMD);
//    E2213JS0C1_WriteData8(PANEL_SET_DATA_1);
//    E2213JS0C1_WriteData8(PANEL_SET_DATA_2);   
}

/**
 * @brief	发送图像数据（在此之前图像数据已准备好）
 * @param	none
 * @retval	none
 */
void E2213JS0C1_SendImageData(void)
{
    /* 发送第一个Frame的数据 */
    E2213JS0C1_WriteRegIndex(FIRST_FRAME_CMD); 
    E2213JS0C1_WriteMultipleData(E2213JS0C1_FirstFrameBuffer,E2213JS0C1_BUFFER_SIZE);    
    /* 发送第二个Frame的数据 */  
    E2213JS0C1_WriteRegIndex(SECOND_FRAME_CMD);
    E2213JS0C1_WriteMultipleData(E2213JS0C1_SecondFrameBuffer,E2213JS0C1_BUFFER_SIZE);
}

/**
 * @brief	发送更新命令
 * @param	none
 * @retval	none
 */
void E2213JS0C1_SendUpdateCmd(void)
{
    /* 等待BUSY变成高电平 */
    E2213JS0C1_WaiteUntilNotBusy();
    /* Power on command，打开DC/DC */
    E2213JS0C1_WriteRegIndex(TURN_ON_DCDC_CMD);
    /* 等待BUSY变成高电平 */
    E2213JS0C1_WaiteUntilNotBusy();
    /* 刷新显示 */
    E2213JS0C1_WriteRegIndex(DISPLAY_REFRESH_CMD);
    /* 等待BUSY变成高电平 */
    E2213JS0C1_WaiteUntilNotBusy();
}

/**
 * @brief	关闭DC/DC，在发送更新命令后使用
 * @param	none
 * @retval	none
 */
void E2213JS0C1_TurnOffDCDC(void)
{
    /* 关闭DC/DC命令 */
    E2213JS0C1_WriteRegIndex(TURN_OFF_DCDC_CMD);
    /* 等待BUSY变成高电平 */
    E2213JS0C1_WaiteUntilNotBusy();
    /* 按技术手册，后续要把CS、MOSI、CLK都置0，切断CVV供电，但是我忽略这一步 */
}


/**
 * @brief	清除整个屏幕
 * @param	none
 * @retval	none
 */
void E2213JS0C1_ClearFullScreen(enum ENUM_COLOR color)
{
    uint16_t i,j;
    uint8_t buffer1,buffer2;
    uint8_t buffer1Single,buffer1Double,buffer2Single,buffer2Double;
    /* 根据函数输入的颜色，确定要刷入两个buffer的数据 */
    switch(color)
    {
        case RED:
            buffer1Single = RED_BUFFER1;
            buffer1Double = RED_BUFFER1;
            buffer2Single = RED_BUFFER2;
            buffer2Double = RED_BUFFER2;
            break;
        case WHITE:
            buffer1Single = WHITE_BUFFER1;
            buffer1Double = WHITE_BUFFER1;
            buffer2Single = WHITE_BUFFER2;
            buffer2Double = WHITE_BUFFER2;
            break;
        case BLACK:
            buffer1Single = BLACK_BUFFER1;
            buffer1Double = BLACK_BUFFER1;
            buffer2Single = BLACK_BUFFER2;
            buffer2Double = BLACK_BUFFER2;
            break;
        case LIGHTRED:
            buffer1Single = LIGHTRED_BUFFER1;
            buffer1Double = LIGHTRED_BUFFER1;
            buffer2Single = LIGHTRED_BUFFER2_SINGLE;
            buffer2Double = LIGHTRED_BUFFER2_DOUBLE;
            break;
        case GREY:
            buffer1Single = GREY_BUFFER1_SINGLE;
            buffer1Double = GREY_BUFFER1_DOUBLE;
            buffer2Single = GREY_BUFFER2;
            buffer2Double = GREY_BUFFER2;
            break;
        case DARKRED:
            buffer1Single = DARKRED_BUFFER1_SINGLE;
            buffer1Double = DARKRED_BUFFER1_DOUBLE;
            buffer2Single = DARKRED_BUFFER2_SINGLE;
            buffer2Double = DARKRED_BUFFER2_DOUBLE;
            break;
    } 
    /* 将颜色数据填入两个Buffer */
    for (i = 0; i < E2213JS0C1_BUFFER_HEIGHT_SIZE; i++)
    {
        /* 单双行交错1位,于实现棋盘效果，从而显示浅红色等颜色 */
        buffer1 = (i % 2) ? buffer1Single : buffer1Double;
        buffer2 = (i % 2) ? buffer2Single : buffer2Double;
        for(j = 0; j < E2213JS0C1_BUFFER_WIDTH_SIZE; j++)
        {
            E2213JS0C1_FirstFrameBuffer[i * E2213JS0C1_BUFFER_WIDTH_SIZE + j] = buffer1;
            E2213JS0C1_SecondFrameBuffer[i * E2213JS0C1_BUFFER_WIDTH_SIZE + j] = buffer2;
        }
    }
}

/**
 * @brief	画点
 * @param	xPos：X轴坐标
 * @param	yPos：Y轴坐标
 * @param	color：颜色（只能红黑白）
 * @retval	none
 */
void E2213JS0C1_DrawPoint(uint8_t xPos, uint8_t yPos, enum ENUM_COLOR color)
{
    uint16_t i;
    uint8_t n;
    /* 判断坐标是否合法 */
    if ((xPos > E2213JS0C1_XPOS_MAX) || (yPos > E2213JS0C1_YPOS_MAX))
    {
        return;
    }
    /* 计算要对数组的序号为i的第n位进行操作 */
    i = yPos * E2213JS0C1_BUFFER_WIDTH_SIZE + (xPos / 8);
    n = 7- (xPos % 8);
    /* 进行位操作 */
    switch(color)
    {
        case RED:
            ClearBit(E2213JS0C1_FirstFrameBuffer[i], n);
            SetBit(E2213JS0C1_SecondFrameBuffer[i], n);
            break;
        case WHITE:
            ClearBit(E2213JS0C1_FirstFrameBuffer[i], n);
            ClearBit(E2213JS0C1_SecondFrameBuffer[i], n);
            break;
        case BLACK:
            SetBit(E2213JS0C1_FirstFrameBuffer[i], n);
            ClearBit(E2213JS0C1_SecondFrameBuffer[i], n);
            break;
        default: 
            return;
    }
}

/**
 * @brief	画线（只能水平/垂直）
 * @param	xStart：X轴起点坐标
 * @param	yStart：Y轴起点坐标
 * @param	length：长度
 * @param	orientation：方向，HORIZONTAL或VERTICAL
 * @param	color：颜色（只能红黑白）
 * @retval	none
 */
void E2213JS0C1_DrawLine(uint8_t xStart, uint8_t yStart, uint8_t length, 
    enum ENUM_ORIENTATION orientation, enum ENUM_COLOR color)
{
    /* 判断坐标是否合法 */
    if ((xStart > E2213JS0C1_XPOS_MAX) || (yStart > E2213JS0C1_YPOS_MAX))
    {
        return;
    }
    else if ((orientation == HORIZONTAL) && (xStart + length - 1 > E2213JS0C1_XPOS_MAX))
    {
        return;
    }
    else if ((orientation == VERTICAL) && (yStart + length - 1 > E2213JS0C1_YPOS_MAX))
    {
        return;
    }
    /* 画线 */
    for (uint8_t i = 0; i < length; i++)
    {
        E2213JS0C1_DrawPoint(xStart, yStart, color); 
        switch(orientation)
        {
            case HORIZONTAL:
                xStart++;           
                break;
            case VERTICAL:
                yStart++;
                break;
        }  
    }
}

/**
 * @brief	画矩形
 * @param	xStart：X轴起点坐标
 * @param	yStart：Y轴起点坐标
 * @param	width：宽度
 * @param	height：高度
 * @param	fill：填充，SOLID/HOLLOW
 * @param	borderColor：边框颜色（只能红黑白）
 * @param	fillColor：填充颜色（只能红黑白）
 * @retval	none
 */
void E2213JS0C1_DrawRectangle(uint8_t xStart, uint8_t yStart, uint8_t width, 
    uint8_t height, enum ENUM_FILL fill, enum ENUM_COLOR borderColor, enum ENUM_COLOR fillColor)
{
    /* 判断坐标是否合法 */
    if ((xStart > E2213JS0C1_XPOS_MAX) || (yStart > E2213JS0C1_YPOS_MAX)  ||
        (xStart + width - 1 > E2213JS0C1_XPOS_MAX) || (yStart + height - 1 > E2213JS0C1_YPOS_MAX))
    {
        return;
    }    
    /* 画外框 */
    E2213JS0C1_DrawLine(xStart, yStart, width, HORIZONTAL,borderColor);    
    E2213JS0C1_DrawLine(xStart, yStart + height - 1, width, HORIZONTAL,borderColor);
    E2213JS0C1_DrawLine(xStart, yStart + 1, height - 2, VERTICAL,borderColor);
    E2213JS0C1_DrawLine(xStart + width - 1, yStart + 1, height - 2, VERTICAL,borderColor);
    /* 填充 */
    if (fill == SOLID)
    {
        yStart++; 
        for (uint8_t i = 0; i < (height - 2); i++)
        {
           E2213JS0C1_DrawLine(xStart + 1, yStart, width - 2, HORIZONTAL,fillColor);  
           yStart++; 
        }
    }
}

/**
 * @brief	显示一个字符
 * @param	xStart：X轴起点坐标
 * @param	yStart：Y轴起点坐标
 * @param	ch：需要显示的字符
 * @param	font：字库
 * @param	fontColor：文字颜色颜色（只能红黑白）
 * @param	backgroundColor：背景颜色（只能红黑白）
 * @retval	显示字符的宽（用于计算下一个字符的左上角X轴坐标）
 */
uint8_t E2213JS0C1_ShowChar(uint8_t xStart, uint8_t yStart, char chr, 
    uint8_t font, enum ENUM_COLOR fontColor, enum ENUM_COLOR backgroundColor)
{
    uint8_t xPos, yPos, temp, fontWidth, fontHeight;
    chr = chr - ' ';//得到偏移后的值
    xPos = xStart;
    yPos = yStart;
    switch(font)
	{
        case FONT_1608:
            fontWidth = FONT_1608_WIDTH;
            fontHeight = FONT_1608_HEIGHT;
            /* 列循环 */
            for (uint8_t t = 0; t < fontHeight; t++)
            {                   
                temp = ACSII_1608[chr][t];
                /* 行循环 */
                for (uint8_t i = 0; i < fontWidth; i++)
                {
                    if (temp & 0x80)
                    {
                        E2213JS0C1_DrawPoint(xPos, yPos, fontColor);
                    }
                    else 
                    {
                        E2213JS0C1_DrawPoint(xPos, yPos, backgroundColor);
                    }
                    temp <<= 1;
                    xPos++;
                }
                xPos = xStart;
                yPos++;
            }                    
        break;
    }
    /* 返回下一个字的起点 */
	return fontWidth;  
}

/**
 * @brief	显示字符串
 * @param	startX：左上角x轴坐标
 * @param	startY：左上角y轴坐标
 * @param	str:需要显示的字符串
 * @param	font：字库
 * @param	fontColor：文字颜色颜色（只能红黑白）
 * @param	backgroundColor：背景颜色（只能红黑白）
 * @retval	下一个字符串左上角X轴的坐标
 */
uint8_t E2213JS0C1_ShowCharStr(uint8_t xStart, uint8_t yStart, char* str, 
    uint8_t font, enum ENUM_COLOR fontColor, enum ENUM_COLOR backgroundColor)
{	
	while(1)
	{	
		/* 判断是不是为非法字符，即ACSII中不在 ' ' 到 '~'的字符，这些字符是没有显示内容的*/
		if ((*str <= '~') && (*str >= ' '))
		{
			xStart += E2213JS0C1_ShowChar(xStart, yStart, *str, font, fontColor, backgroundColor);
		}		
		/* 判断是不是0x00的结束符号 */
		else if (*str == 0x00)
		{
			/* 结束循环 */
			break;
		}
		/* 下一个字符 */
		str++;
	}	
	return xStart;
}

/**
 * @brief	画一张bmp图片
 * @param	xStart：左上角起始点的x轴坐标。	    
 * @param	yStart：左上角起始点的y轴坐标。
 * @param	bmpWidth：x轴像素点的个数
 * @param	bmpHeight：y轴像素点个数
 * @param	fontColor:字体颜色。
 * @param	backgroundColor：背景颜色。
 * @param	pic：图片
 * @retval	none
 */
void E2213JS0C1_DrawBmp(uint8_t xStart, uint8_t yStart, uint8_t bmpWidth, 
    uint8_t bmpHeight, enum ENUM_COLOR fontColor, enum ENUM_COLOR backgroundColor, 
        const unsigned char* pic)
{
    /* 判断坐标是否合法 */
    if ((xStart > E2213JS0C1_XPOS_MAX) || (yStart > E2213JS0C1_YPOS_MAX)  ||
        (xStart + bmpWidth - 1 > E2213JS0C1_XPOS_MAX) || (yStart + bmpHeight - 1 > E2213JS0C1_YPOS_MAX))
    {
        return;
    }    
    uint8_t xPos, yPos, temp;
    xPos = xStart;
    yPos = yStart; 
    
    /* 列循环 */
    for (uint8_t t = 0; t < bmpHeight; t++)
    {   
        /* 行循环 */          
        for (uint8_t i = 0; i < (bmpWidth / 8); i++)
        {
            temp = *pic; 
            for(uint8_t j = 0; j < 8; j++)
            {
                if (temp & 0x80)
                {
                    E2213JS0C1_DrawPoint(xPos, yPos, fontColor);
                }
                else 
                {
                    E2213JS0C1_DrawPoint(xPos, yPos, backgroundColor);
                }
                temp <<= 1;
                xPos++;
            }
            pic++;
        }
        if ((bmpWidth % 8) != 0)
        {
            for (uint8_t i = 0; i < (bmpWidth % 8); i++)
            {
                temp = *pic; 
                if (temp & 0x80)
                {
                    E2213JS0C1_DrawPoint(xPos, yPos, fontColor);
                }
                else 
                {
                    E2213JS0C1_DrawPoint(xPos, yPos, backgroundColor);
                }
                temp <<= 1;
                xPos++;
            }
            pic++;
        }
        xPos = xStart;
        yPos++;
    }
}

/**
 * @brief	画一张图片（三色图）
 * @param	xStart：左上角起始点的x轴坐标。	    
 * @param	yStart：左上角起始点的y轴坐标。
 * @param	bmpWidth：x轴像素点的个数
 * @param	bmpHeight：y轴像素点个数
 * @param	pic：图片
 * @retval	none
 */
void E2213JS0C1_DrawImage(uint8_t xStart, uint8_t yStart, uint8_t imageWidth, 
    uint8_t imageHeight, const unsigned char* pic)
{
    /* 判断坐标是否合法 */
    if ((xStart > E2213JS0C1_XPOS_MAX) || (yStart > E2213JS0C1_YPOS_MAX)  ||
        (xStart + imageWidth - 1 > E2213JS0C1_XPOS_MAX) || (yStart + imageHeight - 1 > E2213JS0C1_YPOS_MAX))
    {
        return;
    }
	uint16_t temp;
    uint8_t xPos, yPos;  
    enum ENUM_COLOR color;
    xPos = xStart;
    yPos = yStart;
    /* 列循环 */
    for (uint8_t t = 0; t < imageHeight; t++)
    {            
        /* 行循环 */
        for (uint8_t i = 0; i < imageWidth; i++)
        {
            temp = *pic;
            temp = temp << 8;
            temp = temp + *++pic;            
            switch(temp)
            {
                /* 白色 */
                case RGB565_WHITE:
                    color = WHITE;                
                break;
                /* 红色 */
                case RGB565_RED:
                    color = RED;                  
                break;
                /* 黑色 */
                case RGB565_BLACK:
                    color = BLACK;                   
                break;
            }
            E2213JS0C1_DrawPoint(xPos, yPos, color);
            xPos++;
            pic++;
        }
        xPos = xStart;
        yPos++;
    }                
}


