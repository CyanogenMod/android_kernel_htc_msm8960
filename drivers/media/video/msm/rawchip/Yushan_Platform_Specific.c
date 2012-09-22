/*******************************************************************************
################################################################################
#                             C STMicroelectronics
#    Reproduction and Communication of this document is strictly prohibited
#      unless specifically authorized in writing by STMicroelectronics.
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
File Name:	Yushan_Platform_Specific.c
Author:		Rajdeep Patel
Description:Contains Platform specific functions to be used by ST for verification
            purpose
********************************************************************************/
#include "yushan_registermap.h"
#include "DxODOP_regMap.h"
#include "DxODPP_regMap.h"
#include "DxOPDP_regMap.h"
#include "Yushan_API.h"
#include "Yushan_Platform_Specific.h"

#include <mach/board.h>
#include <linux/platform_device.h>

#include <mach/gpio.h>

#ifdef YUSHAN_PLATFORM_SPECIFIC_DEBUG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

struct yushan_int_t {
	spinlock_t yushan_spin_lock;
	wait_queue_head_t yushan_wait;
};

/* Used by Interrupts */
//extern uint32_t	udwProtoInterruptList[3];
uint32_t	udwProtoInterruptList_Pad0[3];
uint32_t	udwProtoInterruptList_Pad1[3];

extern int rawchip_intr0, rawchip_intr1;
extern atomic_t interrupt, interrupt2;
extern struct yushan_int_t yushan_int;


/*******************************************************************
Yushan_WaitForInterruptEvent:
Waiting for Interrupt and adding the same in the Interrupt list.
*******************************************************************/
bool_t Yushan_WaitForInterruptEvent (uint8_t bInterruptId ,uint32_t udwTimeOut)
{

	int					 counterLimit;
	//int counter =0;
	bool_t				fStatus = 0; /*, INTStatus=0;*/
	//int rc = 0;

	switch ( udwTimeOut )
	{
		case TIME_5MS :
			counterLimit=100 ;
			break;
		case TIME_10MS :
			counterLimit=200 ;
			break;
		case TIME_20MS :
			counterLimit=400 ;
			break;
		case TIME_50MS :
			counterLimit=1000 ;
			break;
		default :
			counterLimit=50 ;
			break;
	}

	fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, udwProtoInterruptList_Pad0);		// Work on ProtoIntrList
	CDBG("[CAM] %s Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
	if ((fStatus)/*||(INTStatus)*/) {
		// Interrupt has been served, so remove the same from the InterruptList.
		Yushan_AddnRemoveIDInList(bInterruptId, udwProtoInterruptList_Pad0, DEL_INTR_FROM_LIST); // Worked on ProtoIntrList
		//fStatus = Yushan_CheckForInterruptIDInList(bInterruptId);		// Work on ProtoIntrList
		CDBG("[CAM] %s Del Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
		return SUCCESS;
	}

	CDBG("[CAM] %s begin interrupt wait\n",__func__);
	//init_waitqueue_head(&yushan_int.yushan_wait);
	//rc = wait_event_interruptible_timeout(yushan_int.yushan_wait,
	wait_event_timeout(yushan_int.yushan_wait,
	atomic_read(&interrupt),
		counterLimit/200);//counterLimit/200
	CDBG("[CAM] %s end interrupt: %d; interrupt id:%d wait\n",__func__, atomic_read(&interrupt), bInterruptId);
	if(atomic_read(&interrupt))
	{
		/* INTStatus = 1; */
		atomic_set(&interrupt, 0);
		Yushan_Interrupt_Manager_Pad0();
		fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, udwProtoInterruptList_Pad0);		// Work on ProtoIntrList
		CDBG("[CAM] %s Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
		// Interrupt has been served, so remove the same from the InterruptList.
		Yushan_AddnRemoveIDInList(bInterruptId, udwProtoInterruptList_Pad0, DEL_INTR_FROM_LIST); // Worked on ProtoIntrList
		//fStatus = Yushan_CheckForInterruptIDInList(bInterruptId);		// Work on ProtoIntrList
		CDBG("[CAM] %s Del Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
	}
	if ((fStatus)/*||(INTStatus)*/)
		return SUCCESS;
	else
		return FAILURE;
}

bool_t Yushan_WaitForInterruptEvent2 (uint8_t bInterruptId ,uint32_t udwTimeOut)
{

	int					 counterLimit;
	//int counter =0;
	bool_t				fStatus = 0; /*, INTStatus=0;*/
	//int rc = 0;

	switch ( udwTimeOut )
	{
		case TIME_5MS :
			counterLimit=100 ;
			break;
		case TIME_10MS :
			counterLimit=200 ;
			break;
		case TIME_20MS :
			counterLimit=400 ;
			break;
		case TIME_50MS :
			counterLimit=1000 ;
			break;
		default :
			counterLimit=50 ;
			break;
	}

	fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, udwProtoInterruptList_Pad1);		// Work on ProtoIntrList
	CDBG("[CAM] %s Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
	if ((fStatus)/*||(INTStatus)*/) {
		// Interrupt has been served, so remove the same from the InterruptList.
		Yushan_AddnRemoveIDInList(bInterruptId, udwProtoInterruptList_Pad1, DEL_INTR_FROM_LIST); // Worked on ProtoIntrList
		//fStatus = Yushan_CheckForInterruptIDInList(bInterruptId);		// Work on ProtoIntrList
		CDBG("[CAM] %s Del Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
		return SUCCESS;
	}

	CDBG("[CAM] %s begin interrupt wait\n",__func__);
	//init_waitqueue_head(&yushan_int.yushan_wait);
	//rc = wait_event_interruptible_timeout(yushan_int.yushan_wait,
	wait_event_timeout(yushan_int.yushan_wait,
	atomic_read(&interrupt2),
		counterLimit/200);//counterLimit/200
	CDBG("[CAM] %s end interrupt: %d; interrupt id:%d wait\n",__func__, atomic_read(&interrupt2), bInterruptId);
	if(atomic_read(&interrupt2))
	{
		/* INTStatus = 1; */
		atomic_set(&interrupt2, 0);
		Yushan_Interrupt_Manager_Pad1();
		fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, udwProtoInterruptList_Pad1);		// Work on ProtoIntrList
		CDBG("[CAM] %s Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
		// Interrupt has been served, so remove the same from the InterruptList.
		Yushan_AddnRemoveIDInList(bInterruptId, udwProtoInterruptList_Pad1, DEL_INTR_FROM_LIST); // Worked on ProtoIntrList
		//fStatus = Yushan_CheckForInterruptIDInList(bInterruptId);		// Work on ProtoIntrList
		CDBG("[CAM] %s Del Yushan_CheckForInterruptIDInList:%d \n",__func__, fStatus);
	}
	if ((fStatus)/*||(INTStatus)*/)
		return SUCCESS;
	else
		return FAILURE;
}



uint8_t Yushan_parse_interrupt(int intr_pad)
{

	uint8_t		bCurrentInterruptID = 0;
	uint8_t		bAssertOrDeassert = 0, bInterruptWord = 0;
	/* The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	*  and set every element to 0. */
	uint32_t	*udwListOfInterrupts;
	uint8_t	bSpiData;
	uint32_t udwSpiBaseIndex;
	uint8_t interrupt_type = 0;

	udwListOfInterrupts	= kmalloc(96, GFP_KERNEL);

	/* Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts. */
	/* bInterruptID = Yushan_ReadIntr_Status(); */
	Yushan_Intr_Status_Read((uint8_t *)udwListOfInterrupts, intr_pad);

	/* Clear InterruptStatus */
	Yushan_Intr_Status_Clear((uint8_t *) udwListOfInterrupts);

	/* Adding the Current Interrupt asserted to the Proto List */
	while (bCurrentInterruptID < (TOTAL_INTERRUPT_COUNT + 1)) {
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if (bAssertOrDeassert) {
			CDBG("[CAM] %s:bCurrentInterruptID:%d\n", __func__, bCurrentInterruptID+1);
			switch (bCurrentInterruptID+1) {
			case EVENT_PDP_EOF_EXECCMD :
				CDBG("[CAM] %s:[AF_INT]EVENT_PDP_EOF_EXECCMD\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_PDP_EOF_EXECCMD;
				break;

			case EVENT_DPP_EOF_EXECCMD :
				CDBG("[CAM] %s:[AF_INT]EVENT_DPP_EOF_EXECCMD\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_DPP_EOF_EXECCMD;
				break;

			case EVENT_DOP7_EOF_EXECCMD :
				CDBG("[CAM] %s:[AF_INT]EVENT_DOP7_EOF_EXECCMD\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_DOP_EOF_EXECCMD;
				break;

			case EVENT_DXODOP7_NEWFRAMEPROC_ACK :
				CDBG("[CAM] %s:[AF_INT]EVENT_DXODOP7_NEWFRAMEPROC_ACK\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_NEW_FRAME;
				break;

			case EVENT_CSI2RX_ECC_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_CSI2RX_ECC_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2RX_CHKSUM_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_CSI2RX_CHKSUM_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2RX_SYNCPULSE_MISSED :
				pr_err("[CAM] %s:[ERR]EVENT_CSI2RX_SYNCPULSE_MISSED\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_DXOPDP_NEWFRAME_ERR :
				SPI_Read(DXO_PDP_BASE_ADDR+DxOPDP_error_code_7_0, 1, &bSpiData);
				pr_err("[CAM] %s:[ERR]EVENT_DXOPDP_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_DXODPP_NEWFRAME_ERR :
				udwSpiBaseIndex = 0x010000;
				SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

				SPI_Read(DXO_DPP_BASE_ADDR+DxODPP_error_code_7_0-0x8000, 1, &bSpiData);
				pr_err("[CAM] %s:[ERR]EVENT_DXODPP_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);

				udwSpiBaseIndex = 0x08000;
				SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_DXODOP7_NEWFRAME_ERR :
				SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_error_code_7_0, 1, &bSpiData);
				pr_err("[CAM] %s:[ERR]EVENT_DXODOP7_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2TX_SP_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_CSI2TX_SP_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2TX_LP_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_CSI2TX_LP_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2TX_DATAINDEX_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_CSI2TX_DATAINDEX_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL1 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL1 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL1 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL1 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL1 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL2 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL2 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL2 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL2 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL2 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL3 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL3 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL3 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL3 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL3:
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL4 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL4 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL4:
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL4:
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL4 :
				pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D1 :
				pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D2 :
				pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D3 :
				pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D4 :
				pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_UNMATCHED_IMAGE_SIZE_ERROR :
				pr_err("[CAM] %s:[ERR]EVENT_UNMATCHED_IMAGE_SIZE_ERROR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case PRE_DXO_WRAPPER_PROTOCOL_ERR :
				pr_err("[CAM] %s:[ERR]PRE_DXO_WRAPPER_PROTOCOL_ERR\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case PRE_DXO_WRAPPER_FIFO_OVERFLOW :
				pr_err("[CAM] %s:[ERR]PRE_DXO_WRAPPER_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_BAD_FRAME_DETECTION :
				pr_err("[CAM] %s:[ERR]EVENT_BAD_FRAME_DETECTION\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_DATA_FIFO_OVERFLOW :
				pr_err("[CAM] %s:[ERR]EVENT_TX_DATA_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_INDEX_FIFO_OVERFLOW :
				pr_err("[CAM] %s:[ERR]EVENT_TX_INDEX_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_0_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_0_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_1_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_1_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_2_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_2_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_3_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_3_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_4_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_4_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_5_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_5_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_6_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_6_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_7_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_7_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR :
				pr_err("[CAM] %s:[ERR]EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW :
				pr_err("[CAM] %s:[ERR]EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_DATA_UNDERFLOW :
				pr_err("[CAM] %s:[ERR]EVENT_TX_DATA_UNDERFLOW\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_INDEX_UNDERFLOW :
				pr_err("[CAM] %s:[ERR]EVENT_TX_INDEX_UNDERFLOW\n", __func__);
				/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			}
		}
		bCurrentInterruptID++;

		if (bCurrentInterruptID%32 == 0)
			bInterruptWord++;
	}

	kfree(udwListOfInterrupts);

	if (intr_pad == INTERRUPT_PAD_0)
		enable_irq(MSM_GPIO_TO_INT(rawchip_intr0));
	else if (intr_pad == INTERRUPT_PAD_1)
		enable_irq(MSM_GPIO_TO_INT(rawchip_intr1));

	return interrupt_type;
}

/******************************************************************************
Function:	Yushan_Interrupt_Manager_Pad0: Look for Interrupts asserted.
			Add the ID to the list only once. Usually ISR provoked
			by any associated Intr, but this function has to be called
			whenever interrupts are expected.
Input:					None
Return:					InterruptID or FAILURE
*******************************************************************************/
void Yushan_Interrupt_Manager_Pad0(void)
{

	uint8_t		bCurrentInterruptID = 0;
	uint8_t		bAssertOrDeassert=0, bInterruptWord = 0;
	/* The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	*  and set every element to 0. */
	uint32_t	*udwListOfInterrupts;
	uint8_t	bSpiData;
	uint32_t udwSpiBaseIndex;

	udwListOfInterrupts	= (uint32_t *) kmalloc(96/8, GFP_KERNEL);

	/* Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts. */
	/* bInterruptID = Yushan_ReadIntr_Status(); */
	Yushan_Intr_Status_Read ((uint8_t *)udwListOfInterrupts, INTERRUPT_PAD_0);

	/* Clear InterruptStatus */
	Yushan_Intr_Status_Clear((uint8_t *) udwListOfInterrupts);

	/* Adding the Current Interrupt asserted to the Proto List */
	while (bCurrentInterruptID < (TOTAL_INTERRUPT_COUNT + 1)) {
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if (bAssertOrDeassert) {
			Yushan_AddnRemoveIDInList((uint8_t)(bCurrentInterruptID+1), udwProtoInterruptList_Pad0, ADD_INTR_TO_LIST);

			CDBG("[CAM] %s:bCurrentInterruptID:%d\n",__func__, bCurrentInterruptID+1);
			switch (bCurrentInterruptID + 1) {
				case EVENT_PDP_EOF_EXECCMD :
					CDBG("[CAM] %s:[AF_INT]EVENT_PDP_EOF_EXECCMD\n", __func__);
					break;

				case EVENT_DPP_EOF_EXECCMD :
					CDBG("[CAM] %s:[AF_INT]EVENT_DPP_EOF_EXECCMD\n", __func__);
					break;

				case EVENT_DOP7_EOF_EXECCMD :
					CDBG("[CAM] %s:[AF_INT]EVENT_DOP7_EOF_EXECCMD\n", __func__);
					break;

				case EVENT_DXODOP7_NEWFRAMEPROC_ACK :
					CDBG("[CAM] %s:[AF_INT]EVENT_DXODOP7_NEWFRAMEPROC_ACK\n", __func__);
					break;
				case EVENT_CSI2RX_ECC_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_CSI2RX_ECC_ERR\n",__func__);
					break;
				case EVENT_CSI2RX_CHKSUM_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_CSI2RX_CHKSUM_ERR\n",__func__);
					break;
				case EVENT_CSI2RX_SYNCPULSE_MISSED :
					pr_err("[CAM] %s:[ERR]EVENT_CSI2RX_SYNCPULSE_MISSED\n",__func__);
					break;
				case EVENT_DXOPDP_NEWFRAME_ERR :
				{
					SPI_Read(DXO_PDP_BASE_ADDR+DxOPDP_error_code_7_0,1,&bSpiData);
					pr_err("[CAM] %s:[ERR]EVENT_DXOPDP_NEWFRAME_ERR, error code =%d\n",__func__, bSpiData);
					/* Reset_Yushan(); */
					break;
				}
				case EVENT_DXODPP_NEWFRAME_ERR :
				{
					udwSpiBaseIndex = 0x010000;
					SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

					SPI_Read(DXO_DPP_BASE_ADDR+DxODPP_error_code_7_0-0x8000,1,&bSpiData);
					pr_err("[CAM] %s:[ERR]EVENT_DXODPP_NEWFRAME_ERR, error code =%d\n",__func__, bSpiData);

					udwSpiBaseIndex = 0x08000;
					SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
					/* Reset_Yushan(); */
					break;
				}
				case EVENT_DXODOP7_NEWFRAME_ERR :
				{
					SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_error_code_7_0,1,&bSpiData);
					pr_err("[CAM] %s:[ERR]EVENT_DXODOP7_NEWFRAME_ERR, error code =%d\n",__func__, bSpiData);
					/* Reset_Yushan(); */
					break;
				}
				case EVENT_CSI2TX_SP_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_CSI2TX_SP_ERR\n",__func__);
					break;
				case EVENT_CSI2TX_LP_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_CSI2TX_LP_ERR\n",__func__);
					break;
				case EVENT_CSI2TX_DATAINDEX_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_CSI2TX_DATAINDEX_ERR\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_SOFT_DL1 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL1\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_HARD_DL1 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL1\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_EOT_DL1 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL1\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_ESC_DL1 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL1\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_CTRL_DL1 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL1\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_SOFT_DL2 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL2\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_HARD_DL2 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL2\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_EOT_DL2 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL2\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_ESC_DL2 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL2\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_CTRL_DL2 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL2\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_SOFT_DL3 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL3\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_HARD_DL3 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL3\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_EOT_DL3 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL3\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_ESC_DL3 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL3\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_CTRL_DL3:
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL3\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_SOFT_DL4 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL4\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_SOT_HARD_DL4 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL4\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_EOT_DL4:
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL4\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_ESC_DL4:
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL4\n",__func__);
					break;
				case EVENT_RX_PHY_ERR_CTRL_DL4 :
					pr_err("[CAM] %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL4\n",__func__);
					break;
				case EVENT_TXPHY_CTRL_ERR_D1 :
					pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D1\n",__func__);
					break;
				case EVENT_TXPHY_CTRL_ERR_D2 :
					pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D2\n",__func__);
					break;
				case EVENT_TXPHY_CTRL_ERR_D3 :
					pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D3\n",__func__);
					break;
				case EVENT_TXPHY_CTRL_ERR_D4 :
					pr_err("[CAM] %s:[ERR]EVENT_TXPHY_CTRL_ERR_D4\n",__func__);
					break;
				case EVENT_UNMATCHED_IMAGE_SIZE_ERROR :
					pr_err("[CAM] %s:[ERR]EVENT_UNMATCHED_IMAGE_SIZE_ERROR\n",__func__);
					break;
				case PRE_DXO_WRAPPER_PROTOCOL_ERR :
					pr_err("[CAM] %s:[ERR]PRE_DXO_WRAPPER_PROTOCOL_ERR\n",__func__);
					//Reset_Yushan();
					break;
				case PRE_DXO_WRAPPER_FIFO_OVERFLOW :
					pr_err("[CAM] %s:[ERR]PRE_DXO_WRAPPER_FIFO_OVERFLOW\n",__func__);
					/* Reset_Yushan(); */
					break;
				case EVENT_BAD_FRAME_DETECTION :
					pr_err("[CAM] %s:[ERR]EVENT_BAD_FRAME_DETECTION\n",__func__);
					break;
				case EVENT_TX_DATA_FIFO_OVERFLOW :
					pr_err("[CAM] %s:[ERR]EVENT_TX_DATA_FIFO_OVERFLOW\n",__func__);
					/* Reset_Yushan(); */
					break;
				case EVENT_TX_INDEX_FIFO_OVERFLOW :
					pr_err("[CAM] %s:[ERR]EVENT_TX_INDEX_FIFO_OVERFLOW\n",__func__);
					/* Reset_Yushan(); */
					break;
				case EVENT_RX_CHAR_COLOR_BAR_0_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_0_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_1_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_1_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_2_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_2_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_3_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_3_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_4_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_4_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_5_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_5_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_6_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_6_ERR\n",__func__);
					break;
				case EVENT_RX_CHAR_COLOR_BAR_7_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_7_ERR\n",__func__);
					break;
				case EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR :
					pr_err("[CAM] %s:[ERR]EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR\n",__func__);
					/* Reset_Yushan(); */
					break;
				case EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW :
					pr_err("[CAM] %s:[ERR]EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW\n",__func__);
					/* Reset_Yushan(); */
					break;
				case EVENT_TX_DATA_UNDERFLOW :
					pr_err("[CAM] %s:[ERR]EVENT_TX_DATA_UNDERFLOW\n",__func__);
					/* Reset_Yushan(); */
					break;
				case EVENT_TX_INDEX_UNDERFLOW :
					pr_err("[CAM] %s:[ERR]EVENT_TX_INDEX_UNDERFLOW\n",__func__);
					/* Reset_Yushan(); */
					break;
			}
		}
		bCurrentInterruptID++;

		if(bCurrentInterruptID%32==0)
			bInterruptWord++;
	}

	kfree(udwListOfInterrupts);

	enable_irq(MSM_GPIO_TO_INT(rawchip_intr0));

}


/******************************************************************************
Function:		Yushan_Interrupt_Manager_Pad1: Look for Interrupts asserted.
				Add the ID to the list only once. Usually ISR provoked
				by any associated Intr, but this function has to be called
				whenever interrupts are expected.
Input:			None
Return:			InterruptID or FAILURE
*******************************************************************************/
void Yushan_Interrupt_Manager_Pad1(void)
{

	uint8_t		bCurrentInterruptID = 0;
	uint8_t		bAssertOrDeassert=0, bInterruptWord = 0;
	/* The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	*  and set every element to 0. */
	uint32_t	*udwListOfInterrupts;
	uint8_t	bSpiData;

	udwListOfInterrupts	= (uint32_t *) kmalloc(96/8, GFP_KERNEL);

	/* Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts. */
	/* bInterruptID = Yushan_ReadIntr_Status(); */
	Yushan_Intr_Status_Read ((uint8_t *)udwListOfInterrupts, INTERRUPT_PAD_1);


	/* Clear InterruptStatus */
	Yushan_Intr_Status_Clear((uint8_t *) udwListOfInterrupts);
#if 1

	/* Adding the Current Interrupt asserted to the Proto List */
	while (bCurrentInterruptID < (TOTAL_INTERRUPT_COUNT + 1)) {
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if(bAssertOrDeassert)
		{
			Yushan_AddnRemoveIDInList((uint8_t)(bCurrentInterruptID+1), udwProtoInterruptList_Pad1, ADD_INTR_TO_LIST);
#if 1
			CDBG("[CAM] %s:bCurrentInterruptID:%d\n", __func__, bCurrentInterruptID+1);
			switch(bCurrentInterruptID+1)
			{
				case EVENT_DXODOP7_NEWFRAMEPROC_ACK :
					pr_info("[CAM] %s:[AF_INT]EVENT_DXODOP7_NEWFRAMEPROC_ACK\n", __func__);
					break;
				case EVENT_DXODOP7_NEWFRAME_ERR :
					{
						SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_error_code_7_0,1,&bSpiData);
						pr_err("[CAM] %s:[ERR]EVENT_DXODOP7_NEWFRAME_ERR, error code =%d\n",__func__, bSpiData);
						/* Reset_Yushan(); */
						break;
					}
			}
#endif
		}
		bCurrentInterruptID++;

		if(bCurrentInterruptID%32==0)
			bInterruptWord++;
	}
#endif

	kfree(udwListOfInterrupts);

	enable_irq(MSM_GPIO_TO_INT(rawchip_intr1));

}
