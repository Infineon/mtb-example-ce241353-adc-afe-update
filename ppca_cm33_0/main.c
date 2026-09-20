/*******************************************************************************
* File Name:   main.c
*
* Description: This is the main file for the PPCA CPU core 0. This file contains
* the main function for the core, interrupt service routine, and global variables
* used by these functions.
*
* Related Document: See README.md
*
*
********************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cycfg.h"

/******************************************************************************
* Macros
*******************************************************************************/
/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 0 is using for sharing data with main CPU. */
#define PPCA_M33_0_SHARED_ADDRESS 0x20000400

/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 1 is using for sharing data with main CPU. */
#define PPCA_M33_1_SHARED_ADDRESS 0x20000800

/*******************************************************************************
* Global Variables
*******************************************************************************/
uint32_t *gain_select = (uint32_t*)PPCA_M33_0_SHARED_ADDRESS;
uint32_t *conv_sync   = (uint32_t*)PPCA_M33_0_SHARED_ADDRESS + 1;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void convert_adc_with_afe(void);

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* This is the main function for PPCA CPU 0. It performs the initialization of the
* variables used in the code, and initializes and enables the interrupt from the
* PPCA timer required to interrupt this CPU.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    for(;;)
    {
        if(*gain_select > 0 && *gain_select <= 5)
        {
            convert_adc_with_afe();

            /* Flag to inform the main CPU that the conversion is complete. */
            *conv_sync = 0;
        }

        Cy_SysLib_Delay(1);
    }
}

/*******************************************************************************
* Function Name: convert_adc_with_afe
*********************************************************************************
* Summary:
* This function is to enable the ADC AFE, trigger the conversion and wait for
* the conversion to complete.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
void convert_adc_with_afe(void)
{
    if(*gain_select > 1)
    {
        /* Enable ADC AFE */
        Cy_PPCA_AFE_Enable(ADC_AFE_HW);

        /* Delay for the AFE configuration to complete */
        Cy_SysLib_Delay(100);
    }

    /* Trigger ADC conversion */
    Cy_PPCA_ADC_Manual_Trigger(ADC_HW, 1);

    /* Wait for the ADC conversion to complete */
    while(Cy_PPCA_ADC_Is_ADC_Busy(ADC_HW));
}

