/*
 * io.c
 *
 *  Created on: Jan 16, 2019
 *      Author: f001yfp
 */
#include "io.h"

#define CHANNEL1 1

static void (*btn_saved_callback)(int btn);
static void (*sw_saved_callback)(int btn);


static XGpio btnport;	       /* btn GPIO port instance */
static XGpio swport;
static u32 swState;


void btn_handler(void *devicep) {
	//coerce the generic pointer into a gpio
	XGpio *dev = (XGpio*)devicep;
	u32 btnPressed;
	int button;
	bool btnDown = true;

	btnPressed = XGpio_DiscreteRead(dev, CHANNEL1);

	if (btnPressed == 0x1) button = 0;
	else if (btnPressed == 0x2) button = 1;
	else if (btnPressed == 0x4) button = 2;
	else if (btnPressed == 0x8) button = 3;
	else if (btnPressed == 0x0) btnDown = false;

	if (btnDown == true) {
		btn_saved_callback(button);
	}

	XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);

	return;
}


/*
 * initialize the btns providing a callback
 */
void io_btn_init(void (*btn_callback)(int btn)) {

	s32 error_check_s32;
	int error_check_int;

	btn_saved_callback = btn_callback;

	/* initialize the gic (c.f. gic.h) */
	error_check_s32 = gic_init();
	if (error_check_s32 != XST_SUCCESS) printf("GIC initialization error!\n");


	/* initialize btnport (c.f. module 1) and immediately dissable interrupts */
	error_check_int = XGpio_Initialize(&btnport, XPAR_AXI_GPIO_1_DEVICE_ID); /* initialize device AXI_GPIO_1 */
	if (error_check_int != XST_SUCCESS) printf("GPIO1 initialization error!\n");
	XGpio_InterruptDisable(&btnport, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&btnport);

	/* connect handler to the gic (c.f. gic.h) */
	error_check_s32 = gic_connect(XPAR_FABRIC_GPIO_1_VEC_ID, btn_handler, &btnport);
	if (error_check_s32 != XST_SUCCESS) printf("GIC connection error! (GPIO1 and btn_handler)\n");


	/* enable interrupts on channel (c.f. table 2.1) */
	XGpio_InterruptEnable(&btnport, XGPIO_IR_CH1_MASK);

	/* enable interrupt to processor (c.f. table 2.1) */
	XGpio_InterruptGlobalEnable(&btnport);

	return;
}

/*
 * close the btns
 */
void io_btn_close(void) {
	  //  disconnect the interrupts (c.f. gic.h)
	  XGpio_InterruptDisable(&btnport, XGPIO_IR_CH1_MASK);
	  XGpio_InterruptGlobalDisable(&btnport);
	  gic_disconnect(XPAR_FABRIC_GPIO_1_VEC_ID);

	  //close the gic (c.f. gic.h)
	  gic_close();

	  return;
}


void sw_handler(void *devicep) {
	//coerce the generic pointer into a gpio
	XGpio *dev = (XGpio*)devicep;
	int whichSwitch;
	u32 swStateNew;
	u32 swFlipped;

	swStateNew = XGpio_DiscreteRead(dev, CHANNEL1);

	swFlipped = swState ^ swStateNew;

	swState = swStateNew;

	if (swFlipped == 0x1) whichSwitch = 0;
	else if (swFlipped == 0x2) whichSwitch = 1;
	else if (swFlipped == 0x4) whichSwitch = 2;
	else if (swFlipped == 0x8) whichSwitch = 3;


	sw_saved_callback(whichSwitch);

	XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);

	return;
}

/*
 * initialize the switches providing a callback
 */
void io_sw_init(void (*sw_callback)(int sw)) {
	s32 error_check_s32;
	int error_check_int;

	sw_saved_callback = sw_callback;

	/* initialize swnport (c.f. module 1) and immediately dissable interrupts */
	error_check_int = XGpio_Initialize(&swport, XPAR_AXI_GPIO_2_DEVICE_ID); /* initialize device AXI_GPIO_2 */
	if (error_check_int != XST_SUCCESS) printf("GPIO2 initialization error!\n");
	XGpio_InterruptDisable(&swport, XGPIO_IR_CH1_MASK);

	/* connect handler to the gic (c.f. gic.h) */
	error_check_s32 = gic_connect(XPAR_FABRIC_GPIO_2_VEC_ID, sw_handler, &swport);
	if (error_check_s32 != XST_SUCCESS) printf("GIC connection error! (GPIO2 and sw_handler)\n");


	/* enable interrupts on channel (c.f. table 2.1) */
	XGpio_InterruptEnable(&swport, XGPIO_IR_CH1_MASK);

	/* enable interrupt to processor (c.f. table 2.1) */
	XGpio_InterruptGlobalEnable(&swport);

	swState = XGpio_DiscreteRead(&swport, CHANNEL1);

	return;
}

/*
 * close the switches
 */
void io_sw_close(void) {

	XGpio_InterruptDisable(&swport, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&swport);

	gic_disconnect(XPAR_FABRIC_GPIO_2_VEC_ID);

	return;
}



