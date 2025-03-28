/**
 * @file tal_oled.c
 * @brief This is tal_oled file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "stdio.h"

#include "tal_i2c.h"
#include "tal_oled.h"
#include "tal_log.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define I2C_WRITE                       0
#define I2C_READ                        1

#define SLAVE_ADDRE                     (0x3C)
#define SLAVE_ADDRE_WRITE               ((SLAVE_ADDRE<<1) | I2C_WRITE)
#define SLAVE_ADDRE_READ                ((SLAVE_ADDRE<<1) | I2C_READ)

#define WRITE_CMD_MSG                   {0x00,}
#define CMD_MSG_LEN                     2
#define WRITE_DATA_MSG                  {0x40,}
#define DATA_MSG_LEN                    2

#define OLED_CMD                        0x00
#define OLED_DATA                       0x01

#define SSD1306_SET_PAGE_ADDR           0xB0
#define SSD1306_SET_DISPLAY_START_LINE  0x40
#define SSD1306_CHARGE_PUMP             0x8D
#define SSD1306_DISPLAY_OFF             0xAE
#define SSD1306_DISPLAY_ON              0xAF
#define SSD1306_SET_CONTRAST            0x81
#define SSD1306_SEGRE_MAP               0xA1
#define SSD1306_NORMAL_DISPLAY          0xA6
#define SSD1306_SET_MULTIPLEX           0xA8
#define SSD1306_COM_SCANDEC             0xC8
#define SSD1306_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_SET_DISPLAY_CLOCK_DIV   0xD5
#define SSD1306_SET_PRECHARGE           0xD9
#define SSD1306_SET_COM_PINS            0xDA
#define SSD1306_SET_VCOM_DETECT         0xDB
#define SSD1306_MEMORY_MODE             0x20
#define SSD1306_DISPLAY_ALLON_RESUME    0xA4
#define SSD1306_DEACTIVATE_SCROLL       0x2E
#define SSD1306_ACTIVATE_SCROLL         0x2f

#define SSD1306_LCD_WIDTH               128
#define SSD1306_LCD_HEIGHT              32
#if defined(TAL_OLED_TYPE) && (TAL_OLED_TYPE == 1)
#define SSD1306_SET_LOW_COLUMN          0x02
#else
#define SSD1306_SET_LOW_COLUMN          0x00
#endif
#define SSD1306_SET_HIGH_COLUMN         0x10 //here is 0x20 not 0x10
#define SSD1306_CHARGE_PUMP_ON          0x14
#define SSD1306_CHARGE_PUMP_OFF         0x10

#if defined(TAL_OLED_TYPE) && (TAL_OLED_TYPE == 1)
#define OLED_PAGE_NUM                8
#else
#define OLED_PAGE_NUM                4
#endif

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
/************************************6*8 lattice************************************/
CONST UINT8_T F6x8[][6] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,// sp
0x00, 0x00, 0x00, 0x2f, 0x00, 0x00,// !
0x00, 0x00, 0x07, 0x00, 0x07, 0x00,// "
0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14,// #
0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12,// $
0x00, 0x62, 0x64, 0x08, 0x13, 0x23,// %
0x00, 0x36, 0x49, 0x55, 0x22, 0x50,// &
0x00, 0x00, 0x05, 0x03, 0x00, 0x00,// '
0x00, 0x00, 0x1c, 0x22, 0x41, 0x00,// (
0x00, 0x00, 0x41, 0x22, 0x1c, 0x00,//)
0x00, 0x14, 0x08, 0x3E, 0x08, 0x14,// *
0x00, 0x08, 0x08, 0x3E, 0x08, 0x08,// +
0x00, 0x00, 0x00, 0xA0, 0x60, 0x00,// ,
0x00, 0x08, 0x08, 0x08, 0x08, 0x08,// -
0x00, 0x00, 0x60, 0x60, 0x00, 0x00,// .
0x00, 0x20, 0x10, 0x08, 0x04, 0x02,// /
0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
0x00, 0x00, 0x42, 0x7F, 0x40, 0x00,// 1
0x00, 0x42, 0x61, 0x51, 0x49, 0x46,// 2
0x00, 0x21, 0x41, 0x45, 0x4B, 0x31,// 3
0x00, 0x18, 0x14, 0x12, 0x7F, 0x10,// 4
0x00, 0x27, 0x45, 0x45, 0x45, 0x39,// 5
0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
0x00, 0x01, 0x71, 0x09, 0x05, 0x03,// 7
0x00, 0x36, 0x49, 0x49, 0x49, 0x36,// 8
0x00, 0x06, 0x49, 0x49, 0x29, 0x1E,// 9
0x00, 0x00, 0x36, 0x36, 0x00, 0x00,// :
0x00, 0x00, 0x56, 0x36, 0x00, 0x00,// ;
0x00, 0x08, 0x14, 0x22, 0x41, 0x00,// <
0x00, 0x14, 0x14, 0x14, 0x14, 0x14,// =
0x00, 0x00, 0x41, 0x22, 0x14, 0x08,// >
0x00, 0x02, 0x01, 0x51, 0x09, 0x06,// ?
0x00, 0x32, 0x49, 0x59, 0x51, 0x3E,// @
0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C,// A
0x00, 0x7F, 0x49, 0x49, 0x49, 0x36,// B
0x00, 0x3E, 0x41, 0x41, 0x41, 0x22,// C
0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C,// D
0x00, 0x7F, 0x49, 0x49, 0x49, 0x41,// E
0x00, 0x7F, 0x09, 0x09, 0x09, 0x01,// F
0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A,// G
0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F,// H
0x00, 0x00, 0x41, 0x7F, 0x41, 0x00,// I
0x00, 0x20, 0x40, 0x41, 0x3F, 0x01,// J
0x00, 0x7F, 0x08, 0x14, 0x22, 0x41,// K
0x00, 0x7F, 0x40, 0x40, 0x40, 0x40,// L
0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F,// M
0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F,// N
0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E,// O
0x00, 0x7F, 0x09, 0x09, 0x09, 0x06,// P
0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
0x00, 0x7F, 0x09, 0x19, 0x29, 0x46,// R
0x00, 0x46, 0x49, 0x49, 0x49, 0x31,// S
0x00, 0x01, 0x01, 0x7F, 0x01, 0x01,// T
0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F,// U
0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F,// V
0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F,// W
0x00, 0x63, 0x14, 0x08, 0x14, 0x63,// X
0x00, 0x07, 0x08, 0x70, 0x08, 0x07,// Y
0x00, 0x61, 0x51, 0x49, 0x45, 0x43,// Z
0x00, 0x00, 0x7F, 0x41, 0x41, 0x00,// [
0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55,// 55
0x00, 0x00, 0x41, 0x41, 0x7F, 0x00,// ]
0x00, 0x04, 0x02, 0x01, 0x02, 0x04,// ^
0x00, 0x40, 0x40, 0x40, 0x40, 0x40,// _
0x00, 0x00, 0x01, 0x02, 0x04, 0x00,// '
0x00, 0x20, 0x54, 0x54, 0x54, 0x78,// a
0x00, 0x7F, 0x48, 0x44, 0x44, 0x38,// b
0x00, 0x38, 0x44, 0x44, 0x44, 0x20,// c
0x00, 0x38, 0x44, 0x44, 0x48, 0x7F,// d
0x00, 0x38, 0x54, 0x54, 0x54, 0x18,// e
0x00, 0x08, 0x7E, 0x09, 0x01, 0x02,// f
0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C,// g
0x00, 0x7F, 0x08, 0x04, 0x04, 0x78,// h
0x00, 0x00, 0x44, 0x7D, 0x40, 0x00,// i
0x00, 0x40, 0x80, 0x84, 0x7D, 0x00,// j
0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,// k
0x00, 0x00, 0x41, 0x7F, 0x40, 0x00,// l
0x00, 0x7C, 0x04, 0x18, 0x04, 0x78,// m
0x00, 0x7C, 0x08, 0x04, 0x04, 0x78,// n
0x00, 0x38, 0x44, 0x44, 0x44, 0x38,// o
0x00, 0xFC, 0x24, 0x24, 0x24, 0x18,// p
0x00, 0x18, 0x24, 0x24, 0x18, 0xFC,// q
0x00, 0x7C, 0x08, 0x04, 0x04, 0x08,// r
0x00, 0x48, 0x54, 0x54, 0x54, 0x20,// s
0x00, 0x04, 0x3F, 0x44, 0x40, 0x20,// t
0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C,// u
0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C,// v
0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C,// w
0x00, 0x44, 0x28, 0x10, 0x28, 0x44,// x
0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C,// y
0x00, 0x44, 0x64, 0x54, 0x4C, 0x44,// z
0x14, 0x14, 0x14, 0x14, 0x14, 0x14,// horiz lines
};

/****************************************8*16 lattice************************************/
CONST UINT8_T F8X16[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,// 0
  0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00,//! 1
  0x00,0x10,0x0C,0x06,0x10,0x0C,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//" 2
  0x40,0xC0,0x78,0x40,0xC0,0x78,0x40,0x00,0x04,0x3F,0x04,0x04,0x3F,0x04,0x04,0x00,//# 3
  0x00,0x70,0x88,0xFC,0x08,0x30,0x00,0x00,0x00,0x18,0x20,0xFF,0x21,0x1E,0x00,0x00,//$ 4
  0xF0,0x08,0xF0,0x00,0xE0,0x18,0x00,0x00,0x00,0x21,0x1C,0x03,0x1E,0x21,0x1E,0x00,//% 5
  0x00,0xF0,0x08,0x88,0x70,0x00,0x00,0x00,0x1E,0x21,0x23,0x24,0x19,0x27,0x21,0x10,//& 6
  0x10,0x16,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//' 7
  0x00,0x00,0x00,0xE0,0x18,0x04,0x02,0x00,0x00,0x00,0x00,0x07,0x18,0x20,0x40,0x00,//(8
  0x00,0x02,0x04,0x18,0xE0,0x00,0x00,0x00,0x00,0x40,0x20,0x18,0x07,0x00,0x00,0x00,//) 9
  0x40,0x40,0x80,0xF0,0x80,0x40,0x40,0x00,0x02,0x02,0x01,0x0F,0x01,0x02,0x02,0x00,//* 10
  0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x1F,0x01,0x01,0x01,0x00,//+ 11
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xB0,0x70,0x00,0x00,0x00,0x00,0x00,//, 12
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,//- 13
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,0x00,0x00,//. 14
  0x00,0x00,0x00,0x00,0x80,0x60,0x18,0x04,0x00,0x60,0x18,0x06,0x01,0x00,0x00,0x00,/// 15
  0x00,0xE0,0x10,0x08,0x08,0x10,0xE0,0x00,0x00,0x0F,0x10,0x20,0x20,0x10,0x0F,0x00,//0 16
  0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,//1 17
  0x00,0x70,0x08,0x08,0x08,0x88,0x70,0x00,0x00,0x30,0x28,0x24,0x22,0x21,0x30,0x00,//2 18
  0x00,0x30,0x08,0x88,0x88,0x48,0x30,0x00,0x00,0x18,0x20,0x20,0x20,0x11,0x0E,0x00,//3 19
  0x00,0x00,0xC0,0x20,0x10,0xF8,0x00,0x00,0x00,0x07,0x04,0x24,0x24,0x3F,0x24,0x00,//4 20
  0x00,0xF8,0x08,0x88,0x88,0x08,0x08,0x00,0x00,0x19,0x21,0x20,0x20,0x11,0x0E,0x00,//5 21
  0x00,0xE0,0x10,0x88,0x88,0x18,0x00,0x00,0x00,0x0F,0x11,0x20,0x20,0x11,0x0E,0x00,//6 22
  0x00,0x38,0x08,0x08,0xC8,0x38,0x08,0x00,0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00,//7 23
  0x00,0x70,0x88,0x08,0x08,0x88,0x70,0x00,0x00,0x1C,0x22,0x21,0x21,0x22,0x1C,0x00,//8 24
  0x00,0xE0,0x10,0x08,0x08,0x10,0xE0,0x00,0x00,0x00,0x31,0x22,0x22,0x11,0x0F,0x00,//9 25
  0x00,0x00,0x00,0xC0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,//: 26
  0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x60,0x00,0x00,0x00,0x00,//; 27
  0x00,0x00,0x80,0x40,0x20,0x10,0x08,0x00,0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x00,//< 28
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x00,//= 29
  0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,0x00,0x20,0x10,0x08,0x04,0x02,0x01,0x00,//> 30
  0x00,0x70,0x48,0x08,0x08,0x08,0xF0,0x00,0x00,0x00,0x00,0x30,0x36,0x01,0x00,0x00,//? 31
  0xC0,0x30,0xC8,0x28,0xE8,0x10,0xE0,0x00,0x07,0x18,0x27,0x24,0x23,0x14,0x0B,0x00,//@ 32
  0x00,0x00,0xC0,0x38,0xE0,0x00,0x00,0x00,0x20,0x3C,0x23,0x02,0x02,0x27,0x38,0x20,//A 33
  0x08,0xF8,0x88,0x88,0x88,0x70,0x00,0x00,0x20,0x3F,0x20,0x20,0x20,0x11,0x0E,0x00,//B 34
  0xC0,0x30,0x08,0x08,0x08,0x08,0x38,0x00,0x07,0x18,0x20,0x20,0x20,0x10,0x08,0x00,//C 35
  0x08,0xF8,0x08,0x08,0x08,0x10,0xE0,0x00,0x20,0x3F,0x20,0x20,0x20,0x10,0x0F,0x00,//D 36
  0x08,0xF8,0x88,0x88,0xE8,0x08,0x10,0x00,0x20,0x3F,0x20,0x20,0x23,0x20,0x18,0x00,//E 37
  0x08,0xF8,0x88,0x88,0xE8,0x08,0x10,0x00,0x20,0x3F,0x20,0x00,0x03,0x00,0x00,0x00,//F 38
  0xC0,0x30,0x08,0x08,0x08,0x38,0x00,0x00,0x07,0x18,0x20,0x20,0x22,0x1E,0x02,0x00,//G 39
  0x08,0xF8,0x08,0x00,0x00,0x08,0xF8,0x08,0x20,0x3F,0x21,0x01,0x01,0x21,0x3F,0x20,//H 40
  0x00,0x08,0x08,0xF8,0x08,0x08,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,//I 41
  0x00,0x00,0x08,0x08,0xF8,0x08,0x08,0x00,0xC0,0x80,0x80,0x80,0x7F,0x00,0x00,0x00,//J 42
  0x08,0xF8,0x88,0xC0,0x28,0x18,0x08,0x00,0x20,0x3F,0x20,0x01,0x26,0x38,0x20,0x00,//K 43
  0x08,0xF8,0x08,0x00,0x00,0x00,0x00,0x00,0x20,0x3F,0x20,0x20,0x20,0x20,0x30,0x00,//L 44
  0x08,0xF8,0xF8,0x00,0xF8,0xF8,0x08,0x00,0x20,0x3F,0x00,0x3F,0x00,0x3F,0x20,0x00,//M 45
  0x08,0xF8,0x30,0xC0,0x00,0x08,0xF8,0x08,0x20,0x3F,0x20,0x00,0x07,0x18,0x3F,0x00,//N 46
  0xE0,0x10,0x08,0x08,0x08,0x10,0xE0,0x00,0x0F,0x10,0x20,0x20,0x20,0x10,0x0F,0x00,//O 47
  0x08,0xF8,0x08,0x08,0x08,0x08,0xF0,0x00,0x20,0x3F,0x21,0x01,0x01,0x01,0x00,0x00,//P 48
  0xE0,0x10,0x08,0x08,0x08,0x10,0xE0,0x00,0x0F,0x18,0x24,0x24,0x38,0x50,0x4F,0x00,//Q 49
  0x08,0xF8,0x88,0x88,0x88,0x88,0x70,0x00,0x20,0x3F,0x20,0x00,0x03,0x0C,0x30,0x20,//R 50
  0x00,0x70,0x88,0x08,0x08,0x08,0x38,0x00,0x00,0x38,0x20,0x21,0x21,0x22,0x1C,0x00,//S 51
  0x18,0x08,0x08,0xF8,0x08,0x08,0x18,0x00,0x00,0x00,0x20,0x3F,0x20,0x00,0x00,0x00,//T 52
  0x08,0xF8,0x08,0x00,0x00,0x08,0xF8,0x08,0x00,0x1F,0x20,0x20,0x20,0x20,0x1F,0x00,//U 53
  0x08,0x78,0x88,0x00,0x00,0xC8,0x38,0x08,0x00,0x00,0x07,0x38,0x0E,0x01,0x00,0x00,//V 54
  0xF8,0x08,0x00,0xF8,0x00,0x08,0xF8,0x00,0x03,0x3C,0x07,0x00,0x07,0x3C,0x03,0x00,//W 55
  0x08,0x18,0x68,0x80,0x80,0x68,0x18,0x08,0x20,0x30,0x2C,0x03,0x03,0x2C,0x30,0x20,//X 56
  0x08,0x38,0xC8,0x00,0xC8,0x38,0x08,0x00,0x00,0x00,0x20,0x3F,0x20,0x00,0x00,0x00,//Y 57
  0x10,0x08,0x08,0x08,0xC8,0x38,0x08,0x00,0x20,0x38,0x26,0x21,0x20,0x20,0x18,0x00,//Z 58
  0x00,0x00,0x00,0xFE,0x02,0x02,0x02,0x00,0x00,0x00,0x00,0x7F,0x40,0x40,0x40,0x00,//[ 59
  0x00,0x0C,0x30,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x38,0xC0,0x00,//\ 60
  0x00,0x02,0x02,0x02,0xFE,0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x7F,0x00,0x00,0x00,//] 61
  0x00,0x00,0x04,0x02,0x02,0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//^ 62
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,//_ 63
  0x00,0x02,0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//` 64
  0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x19,0x24,0x22,0x22,0x22,0x3F,0x20,//a 65
  0x08,0xF8,0x00,0x80,0x80,0x00,0x00,0x00,0x00,0x3F,0x11,0x20,0x20,0x11,0x0E,0x00,//b 66
  0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,0x00,0x0E,0x11,0x20,0x20,0x20,0x11,0x00,//c 67
  0x00,0x00,0x00,0x80,0x80,0x88,0xF8,0x00,0x00,0x0E,0x11,0x20,0x20,0x10,0x3F,0x20,//d 68
  0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x22,0x22,0x22,0x22,0x13,0x00,//e 69
  0x00,0x80,0x80,0xF0,0x88,0x88,0x88,0x18,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,//f 70
  0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x6B,0x94,0x94,0x94,0x93,0x60,0x00,//g 71
  0x08,0xF8,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x3F,0x21,0x00,0x00,0x20,0x3F,0x20,//h 72
  0x00,0x80,0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,//i 73
  0x00,0x00,0x00,0x80,0x98,0x98,0x00,0x00,0x00,0xC0,0x80,0x80,0x80,0x7F,0x00,0x00,//j 74
  0x08,0xF8,0x00,0x00,0x80,0x80,0x80,0x00,0x20,0x3F,0x24,0x02,0x2D,0x30,0x20,0x00,//k 75
  0x00,0x08,0x08,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,//l 76
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x20,0x3F,0x20,0x00,0x3F,0x20,0x00,0x3F,//m 77
  0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x3F,0x21,0x00,0x00,0x20,0x3F,0x20,//n 78
  0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x20,0x20,0x20,0x20,0x1F,0x00,//o 79
  0x80,0x80,0x00,0x80,0x80,0x00,0x00,0x00,0x80,0xFF,0xA1,0x20,0x20,0x11,0x0E,0x00,//p 80
  0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x0E,0x11,0x20,0x20,0xA0,0xFF,0x80,//q 81
  0x80,0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x20,0x20,0x3F,0x21,0x20,0x00,0x01,0x00,//r 82
  0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x33,0x24,0x24,0x24,0x24,0x19,0x00,//s 83
  0x00,0x80,0x80,0xE0,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x1F,0x20,0x20,0x00,0x00,//t 84
  0x80,0x80,0x00,0x00,0x00,0x80,0x80,0x00,0x00,0x1F,0x20,0x20,0x20,0x10,0x3F,0x20,//u 85
  0x80,0x80,0x80,0x00,0x00,0x80,0x80,0x80,0x00,0x01,0x0E,0x30,0x08,0x06,0x01,0x00,//v 86
  0x80,0x80,0x00,0x80,0x00,0x80,0x80,0x80,0x0F,0x30,0x0C,0x03,0x0C,0x30,0x0F,0x00,//w 87
  0x00,0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x31,0x2E,0x0E,0x31,0x20,0x00,//x 88
  0x80,0x80,0x80,0x00,0x00,0x80,0x80,0x80,0x80,0x81,0x8E,0x70,0x18,0x06,0x01,0x00,//y 89
  0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x21,0x30,0x2C,0x22,0x21,0x30,0x00,//z 90
  0x00,0x00,0x00,0x00,0x80,0x7C,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x3F,0x40,0x40,//{ 91
  0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,//| 92
  0x00,0x02,0x02,0x7C,0x80,0x00,0x00,0x00,0x00,0x40,0x40,0x3F,0x00,0x00,0x00,0x00,//} 93
  0x00,0x06,0x01,0x01,0x02,0x02,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//~ 94
};

STATIC TUYA_I2C_NUM_E sg_i2c_num = TUYA_I2C_NUM_0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




STATIC OPERATE_RET i2c_write_data(UINT8_T data)
{
    UINT8_T cfg_msg[DATA_MSG_LEN] = WRITE_DATA_MSG;
    cfg_msg[1] = data;

    tal_i2c_master_send(sg_i2c_num, SLAVE_ADDRE, cfg_msg, DATA_MSG_LEN);

    return OPRT_OK;
}

STATIC OPERATE_RET i2c_write_command(UINT8_T cmd)
{
    UINT8_T cfg_msg[CMD_MSG_LEN] = WRITE_CMD_MSG;
    cfg_msg[1] = cmd;

    tal_i2c_master_send(sg_i2c_num, SLAVE_ADDRE, cfg_msg, CMD_MSG_LEN);

    return OPRT_OK;
}

STATIC OPERATE_RET oled_write_byte(UINT8_T dat, UINT8_T cmd)
{
    if (cmd) {
        i2c_write_data(dat);
    } else {
        i2c_write_command(dat);
    }

    return OPRT_OK;
}

UINT32_T tal_oled_check_i2c_port_num(VOID_T)
{
    return sg_i2c_num;
}

STATIC OPERATE_RET find_oled(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;
    UINT8_T sample_data[2];

    for (UINT8_T address = SLAVE_ADDRE; address <= SLAVE_ADDRE; address++) {
        ret = tal_i2c_master_receive(sg_i2c_num, address, sample_data, 2);
        if (ret == OPRT_OK && (address == SLAVE_ADDRE)) {
            TAL_PR_INFO("Find oled");
            return OPRT_OK;
        }
    }

    TAL_PR_INFO("Not find oled");

    return OPRT_COM_ERROR;
}

STATIC OPERATE_RET oled_set_pos(UINT8_T x, UINT8_T y)
{
#if defined(TAL_OLED_TYPE) && (TAL_OLED_TYPE == 1)
    x += 2; // IC存在2列地址偏移
#endif
    oled_write_byte(SSD1306_SET_PAGE_ADDR + y, OLED_CMD); // write page address
    oled_write_byte((x & 0x0F), OLED_CMD);                // write column low address
    oled_write_byte(((x & 0xF0)>>4) | 0x10, OLED_CMD);    // write column high address

    return OPRT_OK;
}

OPERATE_RET oled_clear_pos(UINT8_T x, UINT8_T y)
{
    oled_write_byte(SSD1306_SET_PAGE_ADDR + y, OLED_CMD); // write page address
    oled_write_byte((x & 0x0F), OLED_CMD);                // write column low address
    oled_write_byte(((x & 0xF0)>>4) | 0x10, OLED_CMD);    // write column high address

    return OPRT_OK;
}

OPERATE_RET oled_display_on(VOID_T)
{
    oled_write_byte(SSD1306_CHARGE_PUMP, OLED_CMD);        // set INT8_Tge pump
    oled_write_byte(SSD1306_CHARGE_PUMP_ON, OLED_CMD);     // INT8_Tge pump on
    oled_write_byte(SSD1306_DISPLAY_ON, OLED_CMD);        // display on

    return OPRT_OK;
}

OPERATE_RET oled_display_off(VOID_T)
{
    oled_write_byte(SSD1306_CHARGE_PUMP, OLED_CMD);         // set INT8_Tge pump
    oled_write_byte(SSD1306_CHARGE_PUMP_OFF, OLED_CMD);     // INT8_Tge pump off
    oled_write_byte(SSD1306_DISPLAY_OFF, OLED_CMD);        // display off

    return OPRT_OK;
}

OPERATE_RET oled_on(VOID_T)
{
    UINT8_T i, n;
    for (i=0; i < OLED_PAGE_NUM; i++) {
        oled_write_byte (SSD1306_SET_PAGE_ADDR + i, OLED_CMD);    //page num (0~7)
        //set oled height
        oled_write_byte (SSD1306_SET_LOW_COLUMN, OLED_CMD);       //column low addr
        oled_write_byte (SSD1306_SET_HIGH_COLUMN, OLED_CMD);      //column high addr

        //clear every column
        for (n = 0; n < SSD1306_LCD_WIDTH; n++) {
            oled_write_byte(0xff, OLED_DATA);
        }
    }

    return OPRT_OK;
}

STATIC UINT32_T math_pow(UINT8_T m, UINT8_T n)
{
    UINT32_T result = 1;
    while (n--) {
        result *= m;
    }
    return result;
}

STATIC OPERATE_RET oled_show_int8_t(UINT8_T x, UINT8_T y, UINT8_T chr, UINT8_T INT8_T_size)
{
//x:0~127
//y:0~63
//size:16/6
    UINT8_T offset = 0, i = 0;

    // get offset from INT8_T font table
    offset = chr - ' ';

    if (x > SSD1306_LCD_WIDTH - 1) {
        x = 0;
        y = y + 2;
    }

    if (INT8_T_size == 16) { // size 8*16

        oled_set_pos(x, y);
        for (i = 0; i < 8; i++) {
            oled_write_byte(F8X16[offset*16+i], OLED_DATA);
        }

        oled_set_pos(x, y + 1);
        for (i = 0; i < 8; i++) {
            oled_write_byte(F8X16[offset*16+i+8], OLED_DATA);
        }
    } else {                    // size 6*8
        oled_set_pos(x, y);
        for (i = 0; i < 6; i++) {
            oled_write_byte(F6x8[offset][i], OLED_DATA);
        }
    }

    return OPRT_OK;
}

OPERATE_RET tal_oled_clear(VOID_T)
{
    UINT8_T i, n;
    for (i = 0; i < OLED_PAGE_NUM; i++) {
        oled_write_byte (SSD1306_SET_PAGE_ADDR + i, OLED_CMD);    //page num (0~7)
        //set oled height
        oled_write_byte (SSD1306_SET_LOW_COLUMN, OLED_CMD);       //column low addr
        oled_write_byte (SSD1306_SET_HIGH_COLUMN, OLED_CMD);      //column high addr

        //clear every column
        for (n = 0; n < SSD1306_LCD_WIDTH; n++) {
            oled_write_byte(0, OLED_DATA);
        }
    }

    return OPRT_OK;
}

OPERATE_RET tal_oled_clear_page(UINT8_T page)
{
    UINT8_T  n;
    oled_write_byte (SSD1306_SET_PAGE_ADDR + page, OLED_CMD);    //page num (0~7)
    //set oled height
    oled_write_byte (SSD1306_SET_LOW_COLUMN, OLED_CMD);       //column low addr
    oled_write_byte (SSD1306_SET_HIGH_COLUMN, OLED_CMD);      //column high addr

    //clear every column
    for (n = 0; n < SSD1306_LCD_WIDTH; n++) {
        oled_write_byte(0, OLED_DATA);
    }

    return OPRT_OK;
}

OPERATE_RET tal_oled_show_num(UINT8_T x, UINT8_T y, UINT32_T num, UINT8_T len, UINT8_T size)
{
    UINT8_T t, temp;
    UINT8_T enshow = 0;

    for (t = 0; t < len; t++) {
        temp = (num / math_pow(10, len-t-1)) % 10;

        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                oled_show_int8_t(x+(size/2) * t, y, ' ', size);
                continue;
            }
            else {
                enshow = 1;
            }
        }
         oled_show_int8_t(x + (size/2) * t, y, temp+'0', size);
    }

    return OPRT_OK;
}

OPERATE_RET tal_oled_show_string(UINT8_T x, UINT8_T y, INT8_T *chr, UINT8_T INT8_T_size)
{
    UINT8_T j = 0;

    while (chr[j] != '\0') {
        oled_show_int8_t(x, y, chr[j], INT8_T_size);
        x += 8;
        if (x > 120) {
            x = 0;
            y += 2;
        }
        j++;
    }

    return OPRT_OK;
}

OPERATE_RET tal_oled_show_string_continue(UINT8_T x, UINT8_T y, CHAR_T *chr)
{
    UINT8_T j = 0;
    UINT8_T offset = 0, i = 0;

    if (x > SSD1306_LCD_WIDTH - 1) {
        x = 0;
    }
    oled_set_pos(x, y);
    while (chr[j] != '\0') {
        offset = chr[j] - ' ';
        for (i = 0; i < 6; i++) {
            oled_write_byte(F6x8[offset][i], OLED_DATA);
        }
        j++;
    }

    return OPRT_OK;
}

OPERATE_RET tal_oled_show_mac(UINT8_T x, UINT8_T y, UINT8_T *mac)
{
    UINT8_T j = 0, i = 0;
    CHAR_T data[25] = "MAC:";
    for (j=0; j<6; j++) {
        if (j == 5)
            sprintf(data+4+i, "%02x", mac[5-j]);
        else
            sprintf(data+4+i, "%02x-", mac[5-j]);
        i += 3;
    }
    tal_oled_show_string_continue(x, y, data);

    return OPRT_OK;
}

OPERATE_RET tal_oled_show_rssi(UINT8_T x, UINT8_T y, INT32_T rssi)
{
    CHAR_T data[15] = {0};
    sprintf(data, "RSSI:%d ", rssi);
    tal_oled_show_string_continue(x, y, data);

    return OPRT_OK;
}

STATIC OPERATE_RET ssd1306_init(VOID_T)
{
#if defined(TAL_OLED_TYPE) && (TAL_OLED_TYPE == 1)
     oled_write_byte(0xAE, OLED_CMD); /*display off*/
    oled_write_byte(0x02, OLED_CMD); /*set lower column address*/
    oled_write_byte(0x10, OLED_CMD); /*set higher column address*/
    oled_write_byte(0x40, OLED_CMD); /*set display start line*/
    oled_write_byte(0xB0, OLED_CMD); /*set page address*/
    oled_write_byte(0x81, OLED_CMD); /*contract control*/
    oled_write_byte(0xcf, OLED_CMD); /*128*/
    oled_write_byte(0xA1, OLED_CMD); /*set segment remap*/
    oled_write_byte(0xA6, OLED_CMD); /*normal / reverse*/
    oled_write_byte(0xA8, OLED_CMD); /*multiplex ratio*/
    oled_write_byte(0x3F, OLED_CMD); /*duty = 1/64*/
    oled_write_byte(0xad, OLED_CMD); /*set charge pump enable*/
    oled_write_byte(0x8b, OLED_CMD); /* 0x8B�ڹ�VCC */
    oled_write_byte(0x33, OLED_CMD); /*0X30---0X33 set VPP 9V */
    oled_write_byte(0xC8, OLED_CMD); /*Com scan direction*/
    oled_write_byte(0xD3, OLED_CMD); /*set display offset*/
    oled_write_byte(0x00, OLED_CMD); /* 0x20 */
    oled_write_byte(0xD5, OLED_CMD); /*set osc division*/
    oled_write_byte(0x80, OLED_CMD);
    oled_write_byte(0xD9, OLED_CMD); /*set pre-charge period*/
    oled_write_byte(0x1f, OLED_CMD); /*0x22*/
    oled_write_byte(0xDA, OLED_CMD); /*set COM pins*/
    oled_write_byte(0x12, OLED_CMD);
    oled_write_byte(0xdb, OLED_CMD); /*set vcomh*/
    oled_write_byte(0x40, OLED_CMD);
    tal_oled_clear();
    oled_write_byte(0xAF, OLED_CMD); /*display ON*/
#else
    oled_write_byte(0xAE, OLED_CMD);
    oled_write_byte(0xAE, OLED_CMD);

    oled_write_byte(0x40, OLED_CMD); //---set low column address
    oled_write_byte(0xB0, OLED_CMD); //---set high column address

    oled_write_byte(0xC8, OLED_CMD); //-not offset

    oled_write_byte(0x81, OLED_CMD);
    oled_write_byte(0xff, OLED_CMD);

    oled_write_byte(0xa1, OLED_CMD);

    oled_write_byte(0xa6, OLED_CMD);

    oled_write_byte(0xa8, OLED_CMD);
    oled_write_byte(0x1f, OLED_CMD);

    oled_write_byte(0xd3, OLED_CMD);
    oled_write_byte(0x00, OLED_CMD);

    oled_write_byte(0xd5, OLED_CMD);
    oled_write_byte(0xf0, OLED_CMD);

    oled_write_byte(0xd9, OLED_CMD);
    oled_write_byte(0x22, OLED_CMD);

    oled_write_byte(0xda, OLED_CMD);
    oled_write_byte(0x02, OLED_CMD);

    oled_write_byte(0xdb, OLED_CMD);
    oled_write_byte(0x49, OLED_CMD);

    oled_write_byte(0x8d, OLED_CMD);
    oled_write_byte(0x14, OLED_CMD);

    oled_write_byte(0xaf, OLED_CMD);

    tal_oled_clear();
#endif

    return OPRT_OK;
}

OPERATE_RET tal_oled_init(VOID_T)
{
    if (find_oled() != OPRT_OK) {
        return OPRT_NOT_FOUND;
    }

    ssd1306_init();

    return OPRT_OK;
}

OPERATE_RET tal_oled_clear_position_data(UINT8_T x, UINT8_T y, UINT8_T length, UINT8_T high)
{
    UINT8_T i =0;
    if (high == 16) {
        oled_set_pos(x, y);
        for (i=0; i<8; i++)
            oled_write_byte(0, OLED_DATA);
        oled_set_pos(x, y+1);
        for (i=0; i<8; i++)
            oled_write_byte(0, OLED_DATA);
    } else {
         oled_set_pos(x, y);
         for (i=0; i<8; i++)
            oled_write_byte(0, OLED_DATA);
    }
    if (length == 16) {
        oled_set_pos(x+8, y);
        for (i=0; i<8; i++)
            oled_write_byte(0, OLED_DATA);
        oled_set_pos(x+8, y+1);
        for (i=0; i<8; i++)
            oled_write_byte(0, OLED_DATA);
    }
    return OPRT_OK;
}

OPERATE_RET tal_oled_clear_data(UINT8_T x1, UINT8_T y1, UINT8_T x2, UINT8_T y2)
{
    UINT8_T i, m;
    y2 = (y2-y1)/8 + (((y2 - y1)%8) ? 1 : 0);
    for (i=0; i<y2; i++) {
        oled_set_pos(x1, i + y1);
        for (m=0; m<(x2 - x1); m++) {
            oled_write_byte(0, OLED_DATA);
        }
    }
    return OPRT_OK;
}

#if defined(TAL_OLED_TYPE) && (TAL_OLED_TYPE == 1)

VOID_T tal_display_switch(UINT8_T val)
{
    if (val)
        oled_display_on();
    else
        oled_display_off();
}

VOID_T tal_oled_show_bmp(UINT8_T x1, UINT8_T y1, UINT8_T x2, UINT8_T y2, const UINT8_T BMP[])
{
    UINT16_T j=0;
    UINT8_T i, m;
    y2 = (y2-y1)/8 + (((y2 - y1)%8) ? 1 : 0);
    for (i=0; i<y2; i++) {
        oled_set_pos(x1, i + y1);
        for (m=0; m<(x2 - x1); m++) {
            oled_write_byte(BMP[j++], OLED_DATA);
        }
    }
}

VOID_T tal_oled_show_chinese(UINT8_T x, UINT8_T y, UINT8_T size, CONST UINT8_T font8X16[])
{
    if (size == 16) {
        UINT16_T i =0;
        oled_set_pos(x, y);
        for (i = 0; i < 16; i++) {
            oled_write_byte(font8X16[i], OLED_DATA);
        }

        oled_set_pos(x, y + 1);
        for (i = 0; i < 16; i++) {
            oled_write_byte(font8X16[16+i], OLED_DATA);
        }
    }
}

OPERATE_RET tal_oled_show_group(UINT8_T x, UINT8_T y, volatile UINT16_T group)
{
    CHAR_T data[3] ={0};
    sprintf(data, "%03d", group);
    for (UINT8_T i=0; i<3; i++) {
        oled_show_int8_t(x, y, data[i], 16);
        x += 8;
    }
    return OPRT_OK;
}

#endif

