/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of 
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and 
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all 
 * derivative works of the Software, unless such copies or derivative works are solely in the form of 
 * machine-executable object code generated by a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *********************************************************************************************************************/
 /*\title Die Temperature Sensor
 * \abstract The die temperature is measured by an internal sensor and printed on a terminal program
 * \description The internal Die Temperature Sensor (DTS) is continuously read in an infinite loop.
 *              When a new temperature result is available an interrupt service routine (ISR) is triggered.
 *              The ISR notifies the availability of a new die temperature value by setting a flag,
 *              which is used to start printing the temperature via UART communication using the
 *              ASCLIN module. The temperature value can be read by using a terminal program
 *              connected to the virtual COM port of the board/kit.
 *
 * \name SCU_Die_Temp_Sensor_1
 * \version V1.0.0
 * \board APPLICATION KIT TC2X7 V1.1, KIT_AURIX_TC297_TFT_BC-Step, TC29xTA/TX_BC-step
 * \keywords AURIX, DTS, Interrupt, SCU, SCU_Die_Temp_Sensor_1, UART, VCOM, sensor, serial monitor, temperature, terminal
 * \documents https://www.infineon.com/aurix-expert-training/Infineon-AURIX_UART_VCOM_1-TR-v01_00_00-EN.pdf
 * \documents https://www.infineon.com/aurix-expert-training/iLLD_UM_TC29B.chm
 * \lastUpdated 2019-10-17
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "SCU_Die_Temp_Sensor.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* Wait for CPU sync event */
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    initPeripherals();

    while(1)
    {
        printDTSMeasurement();
    }
    return (1);
}
