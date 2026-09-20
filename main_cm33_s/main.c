/*******************************************************************************
* File Name:   main.c
*
* Description: This is the main file for the main CPU non safe application of
* the code example. It initializes the peripherals, PPCA CPU cores and starts
* it. Then it reads the data shared by the PPCA CPUs and prints.
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
#include "cy_system_ppca_init.h"
#include "cybsp.h"
#include <stdio.h>
#include "cy_retarget_io.h"

/******************************************************************************
* Macros
*******************************************************************************/
/* These are the addresses where the core0 and core1 images are located. */
#define CORE0_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca0_nvm_C_S_START    //  0x12030000
#define CORE1_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca1_nvm_C_S_START    //  0x12038000
#define PPCA0_IMAGE_SIZE       CYMEM_CM33_0_S_ppca0_code_SIZE
#define PPCA1_IMAGE_SIZE       CYMEM_CM33_0_S_ppca1_code_SIZE

/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 0 is using for sharing data with main CPU. */
#define PPCA_M33_0_SHARED_ADDRESS 0x53020400

/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 1 is using for sharing data with main CPU. */
#define PPCA_M33_1_SHARED_ADDRESS 0x53020800

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Debug UART variables */
static cy_stc_scb_uart_context_t    DEBUG_UART_context; /* DEBUG_UART context */
static mtb_hal_uart_t               DEBUG_UART_hal_obj; /* Debug DEBUG_UART HAL object */

uint32_t *gain_select = (uint32_t*)PPCA_M33_0_SHARED_ADDRESS;
uint32_t *conv_sync   = (uint32_t*)PPCA_M33_0_SHARED_ADDRESS + 1;
int32_t  adc_data     = 0;

/* AFE configuration for different gain settings. */
cy_stc_ppca_afe_config_t afe_cnfg_gain_8_25 = {.afe_gain = CY_AFE_GAIN_3,
                                               .afe_pwr  = CY_AFE_PWR_HFLG,
                                               .afe_clk_ratio = CY_AFE_CLK_RATIO_ATOPCLK_BY_4,};

cy_stc_ppca_afe_config_t afe_cnfg_gain_16_5 = {.afe_gain = CY_AFE_GAIN_6,
                                               .afe_pwr = CY_AFE_PWR_HFLG,
                                               .afe_clk_ratio = CY_AFE_CLK_RATIO_ATOPCLK_BY_4,};

cy_stc_ppca_afe_config_t afe_cnfg_gain_33   = {.afe_gain = CY_AFE_GAIN_12,
                                               .afe_pwr = CY_AFE_PWR_LFHG,
                                               .afe_clk_ratio = CY_AFE_CLK_RATIO_ATOPCLK_BY_32,};

cy_stc_ppca_afe_config_t afe_cnfg_gain_66   = {.afe_gain = CY_AFE_GAIN_24,
                                               .afe_pwr = CY_AFE_PWR_LFHG,
                                               .afe_clk_ratio = CY_AFE_CLK_RATIO_ATOPCLK_BY_32,};

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* This is the main function for the non safe project for the main core. It
* performs the initialization of the peripherals, initialization and starting
* of the PPCA CPU cores, and send the data received from PPCA CPU Core 0 through
* UART.
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
    cy_rslt_t result;
    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);
    Cy_SCB_UART_Enable(DEBUG_UART_HW);

    /* Setup the HAL DEBUG_UART */
    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj, &DEBUG_UART_hal_config,
            &DEBUG_UART_context, NULL);

    /* HAL DEBUG_UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize redirecting of low level IO */
    result = cy_retarget_io_init(&DEBUG_UART_hal_obj);

    /* retarget IO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Transmit header to the terminal */
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("************************************************************\r\n");
    printf("PSOC Control C3M/P8: ADC AFE Update\r\n");
    printf("************************************************************\r\n\n");

    /* enable interrupts */
    __enable_irq();

    /* Initialize variables in the shared memory. */
    *gain_select = 0;
    *conv_sync = 0;

    /* Initializing and enabling the PPCA Configuration. */
    Cy_PPCA_CNFG_Init(PPCA_CNFG_HW, &PPCA_CNFG_config);
    Cy_PPCA_Enable(PPCA_CNFG_HW);

    /* Initializing ATOP Analog reference */
    Cy_PPCA_AREF_Init(AREF_HW, &AREF_config);

    /* Enabling AREF */
    Cy_PPCA_AREF_Enable(AREF_HW);

    /* Initializing ATOP ADC */
    Cy_PPCA_ADC_Init(ADC_HW, &ADC_config);

    /* Enabling ADC */
    Cy_PPCA_ADC_Enable(ADC_HW);

    /* Initializing and starting PPCA CPU Core 0. */
    Cy_System_Init_CPU0((void*)CORE0_IMAGE_ADDRESS, PPCA0_IMAGE_SIZE);

    /* Initializing and starting PPCA CPU Core 1. Use the below line to start PPCA Core 1 */
    /*Cy_System_Init_CPU1((void*)CORE1_IMAGE_ADDRESS, PPCA1_IMAGE_SIZE);*/

    for (;;)
    {
        /* Sets the flag to mark that the ADC conversion is not started. */
        *conv_sync = 1;

        /* Printig the CE menu */
        printf("\r\n Using the potentiometers R212 and R221 set the required,");
        printf("\r\n differential input and select ADC AFE Gain from the list.");
        printf("\r\n 1. for no AFE");
        printf("\r\n 2. for gain 8.25.");
        printf("\r\n 3. for gain 16.5.");
        printf("\r\n 4. for gain 33.");
        printf("\r\n 5. for gain 66.\r\n");

        /* Reading the ADC result. */
        scanf("%d", (int*)gain_select);

        /* Switch to select the user selection. */
        switch(*gain_select)
        {
        case 1: /* Use ADC without AFE */
        {
            /* Waiting for the ADC conversion completion flag from PPCA CPU 0 */
            while(*conv_sync);

            /* Read ADC data from ADC mirror and print it. */
            adc_data = Cy_PPCA_CNFG_PPCA_Read_ADC_Mirror_Data(PPCA_CNFG, 0);
            printf("\r\n For the set input. With gain 0, read ADC value is %d.\r\n", (int)adc_data);
            break;
        }
        case 2: /* Use ADC with AFE and gain 8.25 */
        {
            /* Initialize AFE with the gain 8.25 based on the user selection. */
            Cy_PPCA_AFE_Init(ADC_AFE_HW, &afe_cnfg_gain_8_25);

            /* Waiting for the ADC conversion completion flag from PPCA CPU 0 */
            while(*conv_sync);

            /* Read ADC data from ADC mirror and print it. */
            adc_data = Cy_PPCA_CNFG_PPCA_Read_ADC_Mirror_Data(PPCA_CNFG, 0);
            printf("\r\n For the set input. With gain 8.25, read ADC value is %d.\r\n", (int)adc_data);
            break;
        }
        case 3: /* Use ADC with AFE and gain 16.5 */
        {
            /* Initialize AFE with the gain 16.5 based on the user selection. */
            Cy_PPCA_AFE_Init(ADC_AFE_HW, &afe_cnfg_gain_16_5);

            /* Waiting for the ADC conversion completion flag from PPCA CPU 0 */
            while(*conv_sync);

            /* Read ADC data from ADC mirror and print it. */
            adc_data = Cy_PPCA_CNFG_PPCA_Read_ADC_Mirror_Data(PPCA_CNFG, 0);
            printf("\r\n For the set input. With gain 16.5, read ADC value is %d.\r\n", (int)adc_data);
            break;
        }
        case 4: /* Use ADC with AFE and gain 33 */
        {
            /* Initialize AFE with the gain 33 based on the user selection. */
            Cy_PPCA_AFE_Init(ADC_AFE_HW, &afe_cnfg_gain_33);

            /* Waiting for the ADC conversion completion flag from PPCA CPU 0 */
            while(*conv_sync);

            /* Read ADC data from ADC mirror and print it. */
            adc_data = Cy_PPCA_CNFG_PPCA_Read_ADC_Mirror_Data(PPCA_CNFG, 0);
            printf("\r\n For the set input. With gain 33, read ADC value is %d.\r\n", (int)adc_data);
            break;
        }
        case 5: /* Use ADC with AFE and gain 66 */
        {
            /* Initialize AFE with the gain 66 based on the user selection. */
            Cy_PPCA_AFE_Init(ADC_AFE_HW, &afe_cnfg_gain_66);

            /* Waiting for the ADC conversion completion flag from PPCA CPU 0 */
            while(*conv_sync);

            /* Read ADC data from ADC mirror and print it. */
            adc_data = Cy_PPCA_CNFG_PPCA_Read_ADC_Mirror_Data(PPCA_CNFG, 0);
            printf("\r\n For the set input. With gain 66, read ADC value is %d.\r\n", (int)adc_data);
            break;
        }
        default:
        {
            /* Wrong option selected. */
            printf("\r\n Wrong option selected.\r\n");
            break;
        }
        }
        /* Clear the gain selected by the user to avoid triggering another conversion by the PPCA CPU0 */
        *gain_select = 0;

        /* Disable and deinit the AFE to reconfigure it with the
         * required configuration based on the user selection. */
        Cy_PPCA_AFE_Disable(ADC_AFE_HW);
        Cy_PPCA_AFE_DeInit(ADC_AFE_HW);
    }
}

