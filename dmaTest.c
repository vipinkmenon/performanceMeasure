/*
 * dmaTest.c
 *
 *  Created on: Mar 1, 2020
 *      Author: VIPIN
 */


#include "xaxidma.h"
#include "xparameters.h"
#include "sleep.h"
#include "xil_cache.h"
#include <stdlib.h>
#include "xil_types.h"
#include "xuartps.h"
#include "sleep.h"
#include <xtime_l.h>
#include <stdio.h>

#define imageSize 512*512
#define headerSize 1080
#define fileSize imageSize+headerSize

u32 checkHalted(u32 baseAddress,u32 offset);

int main(){



	u32 *imageData;
	u32 receivedBytes=0;
	u32 totalReceivedBytes=0;
	u32 status;
	u32 totalTransmittedBytes=0;
	u32 transmittedBytes = 0;
	XUartPs_Config *myUartConfig;
	XUartPs myUart;
	XTime start,end;

	imageData = malloc(sizeof(u32)*(fileSize/4));
	if(imageData <= 0){
		print("Memory allocation failed\n\r");
		return -1;
	}

	myUartConfig = XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID);
	status = XUartPs_CfgInitialize(&myUart, myUartConfig, myUartConfig->BaseAddress);
	if(status != XST_SUCCESS)
		print("Uart initialization failed...\n\r");
	status = XUartPs_SetBaudRate(&myUart, 115200);
	if(status != XST_SUCCESS)
		print("Baudrate init failed....\n\r");

	XAxiDma_Config *myDmaConfig;
	XAxiDma myDma;

	myDmaConfig = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_0_BASEADDR);
	status = XAxiDma_CfgInitialize(&myDma, myDmaConfig);
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}
	//print("DMA initialization success..\n");

    //Data transfer from computer to DDR
	/*while(totalReceivedBytes < fileSize){
		receivedBytes = XUartPs_Recv(&myUart,(u8*)&imageData[totalReceivedBytes],100);
		totalReceivedBytes += receivedBytes;
	}*/

	//Xil_DCacheFlushRange((u32)imageData,fileSize);
	Xil_DCacheFlush();

	XTime_GetTime(&start);
	for(int i=headerSize/4;i<fileSize/4;i++)
		imageData[i] = 0xFFFFFFFF-imageData[i];
	XTime_GetTime(&end);

    printf("Processing time for Software %f us\n\r",((end-start)*1000000.0)/COUNTS_PER_SECOND);

	//DMA data to the IP core and process

    XTime_GetTime(&start);
	status = XAxiDma_SimpleTransfer(&myDma, (u32)&imageData[headerSize/4],imageSize,XAXIDMA_DEVICE_TO_DMA);
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}
	status = XAxiDma_SimpleTransfer(&myDma, (u32)&imageData[headerSize/4],imageSize,XAXIDMA_DMA_TO_DEVICE);//typecasting in C/C++
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}
    status = checkHalted(XPAR_AXI_DMA_0_BASEADDR,0x4);
    while(status != 1){
    	status = checkHalted(XPAR_AXI_DMA_0_BASEADDR,0x4);
    }
    status = checkHalted(XPAR_AXI_DMA_0_BASEADDR,0x34);
    while(status != 1){
    	status = checkHalted(XPAR_AXI_DMA_0_BASEADDR,0x34);
    }

    XTime_GetTime(&end);

    printf("Processing time for Hardware %f us\n\r",((end-start)*1000000.0)/COUNTS_PER_SECOND);

	//print("DMA transfer success..\n");

    //Send data back to the computer
	/*while(totalTransmittedBytes < fileSize){
		transmittedBytes =  XUartPs_Send(&myUart,(u8*)&imageData[totalTransmittedBytes],1);
		totalTransmittedBytes += transmittedBytes;
		usleep(1000);
	}*/

}


u32 checkHalted(u32 baseAddress,u32 offset){
	u32 status;
	status = (XAxiDma_ReadReg(baseAddress,offset))&XAXIDMA_HALTED_MASK;
	return status;
}
