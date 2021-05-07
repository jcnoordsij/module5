/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * main.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"
#include "gic.h"
#include "io.h"
#include "led.h"
#include <unistd.h>

#define PACKAGE_SIZE 1
#define CONFIGURE 0
#define PING 1
#define UPDATE 2
#define ID 17


typedef struct {
	int type;
	int id;
} ping_t;

typedef struct {
	int type;
	int id;
	int value;
} update_request_t;

static XUartPs Uart1;
static XUartPs Uart0;
static int done = 0;
static int mode;
static int byte_count = 0;
static u8 bigBuff[33*4];
static u8 input;
static int * type;
static int * id;
static int * average;
static int * val;
static u8 newLine = '\n';


void button_callback(int btn) {
	if (btn == 3) led_toggle(btn);
	else {
		led_set(ALL, LED_OFF);
		led_set(btn, LED_ON);
		mode = btn;
		if (mode == CONFIGURE) {
		} else if (mode == PING) {
			printf("\n[PING SENT]\n");
			ping_t ping_mssg;
			ping_t * pp = &ping_mssg;
			pp->type = PING;
			pp->id = ID;
			XUartPs_Send(&Uart0, (u8*)(pp), sizeof(ping_t));
		} else if (mode == UPDATE) {
			printf("\n[UPDATE SENT]\n");
			update_request_t update_mssg;
			update_request_t * up = &update_mssg;
			up->type = UPDATE;
			up->id = ID;
			up->value = input;
			XUartPs_Send(&Uart0, (u8*)(up), sizeof(update_request_t));
		}
	}
}

void switch_callback(int sw) {
	led_toggle(sw);
}

void uart1_handler(void *CallBackRef, u32 Event, unsigned int EventData) {
	//coerce the generic pointer into a uart
	XUartPs *uartDev1 = (XUartPs*)CallBackRef;
	u8 buffer;

	if (Event == XUARTPS_EVENT_RECV_DATA) {
			XUartPs_Recv(uartDev1, &buffer, 1);
			if (buffer == 'q') {
				done = 1;
			} else if (buffer == '\r') XUartPs_Send(&Uart1, &newLine, PACKAGE_SIZE);

			input = buffer;

			XUartPs_Send(&Uart0, &buffer, 1);
			XUartPs_Send(uartDev1, &buffer, 1);
	}


	return;
}

void uart0_handler(void *CallBackRef, u32 Event, unsigned int EventData) {
	//coerce the generic pointer into a uart
	XUartPs *uartDev0 = (XUartPs*)CallBackRef;
	//u8 buffer;


	if (Event == XUARTPS_EVENT_RECV_DATA) {
		XUartPs_Recv(uartDev0, bigBuff+byte_count, 1);
		byte_count++;
		if (byte_count == 4) {
			type = (int*)bigBuff;
			printf("[RECEIVED TYPE = %d]\n",*type);
		} else if (byte_count == 8 && (*type == PING)) {
			byte_count = 0;
			id = (int*)(&bigBuff[4]);
			printf("[ID = %d]\n",*id);
			memset(bigBuff, 0, sizeof(int)*33);
		} else if (byte_count == 33*4 && (*type == UPDATE)) {
			byte_count = 0;
			id = (int*)(&bigBuff[4]);
			printf("[ID = %d]\n",*id);
			average = (int*)(&(bigBuff[8]));
			printf("[AVERAGE = %d]\n",*average);
			printf("[VALUES =");
			for (int i = 0; i < 30; i++) {
				val = (int*)(&(bigBuff[12+4*i]));
				printf(" %d", *val);
			}
			printf("]\n");
			memset(bigBuff, 0, sizeof(int)*33);
		}

	}

	return;
}


int main()
{
    init_platform();

    gic_init();

    led_init();

    io_btn_init(button_callback);


    // Setup uart1
    XUartPs_Config *uart1CfgPtr = XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID);
    if (XUartPs_CfgInitialize(&Uart1, uart1CfgPtr, uart1CfgPtr->BaseAddress) != XST_SUCCESS) printf("UART1 initialization error!\n");
    XUartPs_SetFifoThreshold(&Uart1, 1);
    XUartPs_SetInterruptMask(&Uart1, XUARTPS_IXR_RXOVR);
    XUartPs_SetHandler(&Uart1, (XUartPs_Handler) uart1_handler, &Uart1);
    if (gic_connect(XPAR_XUARTPS_1_INTR, (Xil_InterruptHandler) XUartPs_InterruptHandler, &Uart1) != XST_SUCCESS) printf("UART1 GIC Connection Error");


    // Setup uart0
    XUartPs_Config *uart0CfgPtr = XUartPs_LookupConfig(XPAR_PS7_UART_0_DEVICE_ID);
    if (XUartPs_CfgInitialize(&Uart0, uart0CfgPtr, uart0CfgPtr->BaseAddress) != XST_SUCCESS) printf("UART0 initialization error!\n");
    XUartPs_SetBaudRate(&Uart0, 9600);
    XUartPs_SetFifoThreshold(&Uart0, 1);
    XUartPs_SetInterruptMask(&Uart0, XUARTPS_IXR_RXOVR);
    XUartPs_SetHandler(&Uart0, (XUartPs_Handler) uart0_handler, &Uart0);
    if (gic_connect(XPAR_XUARTPS_0_INTR, (Xil_InterruptHandler) XUartPs_InterruptHandler, &Uart0) != XST_SUCCESS) printf("UART0 GIC Connection Error");


    printf("[hello]\n");
    while (done == 0) {
    	sleep(1);
    }


    //cleanup
    io_btn_close();

    gic_disconnect(XPAR_XUARTPS_1_INTR);
    gic_disconnect(XPAR_XUARTPS_0_INTR);
    gic_close();

    printf("\n[done]\n\n");

    cleanup_platform();
    return 0;
}
