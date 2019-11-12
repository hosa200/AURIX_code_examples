/**
 * \file IfxPsi5s_Psi5s.c
 * \brief PSI5S PSI5S details
 *
 * \version iLLD_1_0_1_11_0
 * \copyright Copyright (c) 2018 Infineon Technologies AG. All rights reserved.
 *
 *
 *                                 IMPORTANT NOTICE
 *
 *
 * Use of this file is subject to the terms of use agreed between (i) you or 
 * the company in which ordinary course of business you are acting and (ii) 
 * Infineon Technologies AG or its licensees. If and as long as no such 
 * terms of use are agreed, use of this file is subject to following:


 * Boost Software License - Version 1.0 - August 17th, 2003

 * Permission is hereby granted, free of charge, to any person or 
 * organization obtaining a copy of the software and accompanying 
 * documentation covered by this license (the "Software") to use, reproduce,
 * display, distribute, execute, and transmit the Software, and to prepare
 * derivative works of the Software, and to permit third-parties to whom the 
 * Software is furnished to do so, all subject to the following:

 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer, must
 * be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are
 * solely in the form of machine-executable object code generated by a source
 * language processor.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.

 *
 */

/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

#include "IfxPsi5s_Psi5s.h"

/** \addtogroup IfxLld_Psi5s_Psi5s_Utility
 * \{ */

/******************************************************************************/
/*-----------------------Private Function Prototypes--------------------------*/
/******************************************************************************/

/** \brief get the fracDiv clock frequency
 * \param psi5s Pointer to the base of PSI5S register space
 * \return Returns the configured fracDiv psi5s clock frequency in Hz.
 */
IFX_STATIC uint32 IfxPsi5s_Psi5s_getFracDivClock(Ifx_PSI5S *psi5s);

/** \brief Configure the fracDiv clock.
 * \param psi5s Pointer to the base of PSI5S register space
 * \param clock Specifies the required clock frequency in Hz.
 * \return Returns the configured clock frequency in Hz.
 */
IFX_STATIC uint32 IfxPsi5s_Psi5s_initializeClock(Ifx_PSI5S *psi5s, const IfxPsi5s_Psi5s_Clock *clock);

/** \brief Configure the baudrate at the ASC interface.
 * \param psi5s Pointer to the base of PSI5S register space
 * \param baudrate Frequency Specifies the required baudrate frequency in Hz.
 * \param ascConfig pointer to the configuration structure for ASC
 * \return Returns the configured baudrate frequency in Hz.
 */
IFX_STATIC uint32 IfxPsi5s_Psi5s_setBaudrate(Ifx_PSI5S *psi5s, uint32 baudrate, IfxPsi5s_Psi5s_AscConfig *ascConfig);

/** \} */

/******************************************************************************/
/*-------------------------Function Implementations---------------------------*/
/******************************************************************************/

void IfxPsi5s_Psi5s_deInitModule(IfxPsi5s_Psi5s *psi5s)
{
    Ifx_PSI5S *psi5sSFR = psi5s->psi5s;
    IfxPsi5s_Psi5s_resetModule(psi5sSFR);
}


void IfxPsi5s_Psi5s_enableModule(Ifx_PSI5S *psi5s)
{
    psi5s->CLC.U = 0x00000000;
}


IFX_STATIC uint32 IfxPsi5s_Psi5s_getFracDivClock(Ifx_PSI5S *psi5s)
{
    uint32 result;
    uint32 fPsi5s = IfxScuCcu_getSpbFrequency();

    switch (psi5s->FDR.B.DM)
    {
    case IfxPsi5s_DividerMode_spb:
        result = fPsi5s;
        break;
    case IfxPsi5s_DividerMode_normal:
        result = fPsi5s / (IFXPSI5S_STEP_RANGE - psi5s->FDR.B.STEP);
        break;
    case IfxPsi5s_DividerMode_fractional:
        result = (fPsi5s * IFXPSI5S_STEP_RANGE) / psi5s->FDR.B.STEP;
        break;
    case IfxPsi5s_DividerMode_off:
        result = 0;
        break;
    default:
        result = 0;
    }

    return result;
}


boolean IfxPsi5s_Psi5s_initChannel(IfxPsi5s_Psi5s_Channel *channel, const IfxPsi5s_Psi5s_ChannelConfig *config)
{
    boolean       status = TRUE;

    uint16        passwd = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(passwd);

    Ifx_PSI5S    *psi5s = config->module->psi5s;
    channel->module    = (IfxPsi5s_Psi5s *)config->module;
    channel->channelId = config->channelId;

    Ifx_PSI5S_PGC tempPGC;
    tempPGC.U        = psi5s->PGC[config->channelId].U;
    tempPGC.B.TXCMD  = config->pulseGeneration.codeforZero;
    tempPGC.B.ATXCMD = config->pulseGeneration.codeforOne;
    tempPGC.B.TBS    = config->pulseGeneration.timeBaseSelect;
    tempPGC.B.ETB    = config->pulseGeneration.externalTimeBaseSelect;
    tempPGC.B.ETS    = config->pulseGeneration.externalTriggerSelect;

    switch (config->pulseGeneration.periodicOrExternal)
    {
    case IfxPsi5s_TriggerType_periodic:
        tempPGC.B.PTE = TRUE;
        tempPGC.B.ETE = FALSE;
        break;

    case IfxPsi5s_TriggerType_external:
        tempPGC.B.PTE = FALSE;
        tempPGC.B.ETE = TRUE;
        break;
    }

    psi5s->PGC[config->channelId].U = tempPGC.U;

    Ifx_PSI5S_CTV tempCTV;
    tempCTV.U                       = psi5s->CTV[config->channelId].U;
    tempCTV.B.CTV                   = config->channelTrigger.channelTriggerValue;
    tempCTV.B.CTC                   = config->channelTrigger.channelTriggerCounter;
    psi5s->CTV[config->channelId].U = tempCTV.U;

    psi5s->WDT[config->channelId].U = config->watchdogTimerLimit;

    Ifx_PSI5S_RCRA tempRCRA;
    tempRCRA.U                       = psi5s->RCRA[config->channelId].U;
    tempRCRA.B.CRC0                  = config->receiveControl.crcOrParity[0];
    tempRCRA.B.CRC1                  = config->receiveControl.crcOrParity[1];
    tempRCRA.B.CRC2                  = config->receiveControl.crcOrParity[2];
    tempRCRA.B.CRC3                  = config->receiveControl.crcOrParity[3];
    tempRCRA.B.CRC4                  = config->receiveControl.crcOrParity[4];
    tempRCRA.B.CRC5                  = config->receiveControl.crcOrParity[5];
    tempRCRA.B.TSEN                  = config->receiveControl.timestampEnabled;
    tempRCRA.B.TSP                   = config->receiveControl.timestampSelect;
    tempRCRA.B.TSTS                  = config->receiveControl.timestampTriggerSelect;
    tempRCRA.B.FIDS                  = config->receiveControl.frameIdSelect;
    tempRCRA.B.WDMS                  = config->receiveControl.watchdogTimerModeSelect;
    tempRCRA.B.UFC0                  = config->receiveControl.uartFrameCount[0];
    tempRCRA.B.UFC1                  = config->receiveControl.uartFrameCount[1];
    tempRCRA.B.UFC2                  = config->receiveControl.uartFrameCount[2];
    tempRCRA.B.UFC3                  = config->receiveControl.uartFrameCount[3];
    tempRCRA.B.UFC4                  = config->receiveControl.uartFrameCount[4];
    tempRCRA.B.UFC5                  = config->receiveControl.uartFrameCount[5];
    psi5s->RCRA[config->channelId].U = tempRCRA.U;

    Ifx_PSI5S_RCRB tempRCRB;
    tempRCRB.U                       = psi5s->RCRB[config->channelId].U;
    tempRCRB.B.PDL0                  = config->receiveControl.payloadLength[0];
    tempRCRB.B.PDL1                  = config->receiveControl.payloadLength[1];
    tempRCRB.B.PDL2                  = config->receiveControl.payloadLength[2];
    tempRCRB.B.PDL3                  = config->receiveControl.payloadLength[3];
    tempRCRB.B.PDL4                  = config->receiveControl.payloadLength[4];
    tempRCRB.B.PDL5                  = config->receiveControl.payloadLength[5];
    psi5s->RCRB[config->channelId].U = tempRCRB.U;

    psi5s->NFC.U                     = (config->receiveControl.numberOfFramesExpected << (config->channelId * 3));

    Ifx_PSI5S_SCR tempSCR;
    tempSCR.U                       = psi5s->SCR[config->channelId].U;
    tempSCR.B.PLL                   = config->sendControl.payloadLength;
    tempSCR.B.EPS                   = config->sendControl.enhancedProtocolSelection;
    tempSCR.B.BSC                   = config->sendControl.bitStuffControl;
    tempSCR.B.CRC                   = config->sendControl.crcGenerationControl;
    tempSCR.B.STA                   = config->sendControl.startSequenceGenerationControl;
    psi5s->SCR[config->channelId].U = tempSCR.U;

    IfxScuWdt_setCpuEndinit(passwd);

    return status;
}


void IfxPsi5s_Psi5s_initChannelConfig(IfxPsi5s_Psi5s_ChannelConfig *config, IfxPsi5s_Psi5s *psi5s)
{
    IfxPsi5s_Psi5s_ChannelConfig IfxPsi5s_Psi5s_defaultChannelConfig = {
        .channelId       = IfxPsi5s_ChannelId_0,
        .module          = NULL_PTR,
        .pulseGeneration = {
            .codeforZero            = 0,
            .codeforOne             = 1,
            .timeBaseSelect         = IfxPsi5s_TimeBase_internal,
            .externalTimeBaseSelect = IfxPsi5s_Trigger_0,
            .periodicOrExternal     = IfxPsi5s_TriggerType_periodic,
            .externalTriggerSelect  = IfxPsi5s_Trigger_0,
        },
        .channelTrigger                     = {
            .channelTriggerValue   = 0x20,
            .channelTriggerCounter = 0x0
        },
        .watchdogTimerLimit = 0x0,
        .receiveControl     = {
            .crcOrParity[0]          = IfxPsi5s_CrcOrParity_parity,
            .crcOrParity[1]          = IfxPsi5s_CrcOrParity_parity,
            .crcOrParity[2]          = IfxPsi5s_CrcOrParity_parity,
            .crcOrParity[3]          = IfxPsi5s_CrcOrParity_parity,
            .crcOrParity[4]          = IfxPsi5s_CrcOrParity_parity,
            .crcOrParity[5]          = IfxPsi5s_CrcOrParity_parity,
            .timestampEnabled        = FALSE,
            .timestampSelect         = IfxPsi5s_TimestampRegister_a,
            .timestampTriggerSelect  = IfxPsi5s_TimestampTrigger_syncPulse,
            .frameIdSelect           = IfxPsi5s_FrameId_frameHeader,
            .watchdogTimerModeSelect = IfxPsi5s_WatchdogTimerMode_frame,
            .uartFrameCount[0]       = IfxPsi5s_UartFrameCount_3,
            .uartFrameCount[1]       = IfxPsi5s_UartFrameCount_3,
            .uartFrameCount[2]       = IfxPsi5s_UartFrameCount_3,
            .uartFrameCount[3]       = IfxPsi5s_UartFrameCount_3,
            .uartFrameCount[4]       = IfxPsi5s_UartFrameCount_3,
            .uartFrameCount[5]       = IfxPsi5s_UartFrameCount_3,
            .payloadLength[0]        = 0,
            .payloadLength[1]        = 0,
            .payloadLength[2]        = 0,
            .payloadLength[3]        = 0,
            .payloadLength[4]        = 0,
            .payloadLength[5]        = 0,
            .numberOfFramesExpected  = IfxPsi5s_NumberExpectedFrames_1,
        },
        .sendControl                        = {
            .payloadLength                  = 6,
            .enhancedProtocolSelection      = IfxPsi5s_EnhancedProtocol_toothGapMethod,
            .bitStuffControl                = FALSE,
            .crcGenerationControl           = FALSE,
            .startSequenceGenerationControl = FALSE
        }
    };
    *config        = IfxPsi5s_Psi5s_defaultChannelConfig;
    config->module = psi5s;
}


boolean IfxPsi5s_Psi5s_initModule(IfxPsi5s_Psi5s *psi5s, const IfxPsi5s_Psi5s_Config *config)
{
    boolean    status   = TRUE;

    Ifx_PSI5S *psi5sSFR = config->module;
    psi5s->psi5s = psi5sSFR;

    uint16     passwd = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(passwd);
    IfxPsi5s_Psi5s_enableModule(psi5sSFR);

    if (IfxPsi5s_Psi5s_initializeClock(psi5sSFR, &config->fracDiv) == 0)
    {
        status = FALSE;
        return status;
    }
    else
    {}

    if (IfxPsi5s_Psi5s_initializeClock(psi5sSFR, &config->timestampClock) == 0)
    {
        status = FALSE;
        return status;
    }
    else
    {}

    Ifx_PSI5S_CON tempCON;
    tempCON.U       = psi5sSFR->CON.U;
    tempCON.B.M     = config->ascConfig.receiveMode;
    tempCON.B.STP   = config->ascConfig.stopBits;
    tempCON.B.PEN   = config->ascConfig.parityCheckEnabled;
    tempCON.B.FEN   = config->ascConfig.framingCheckEnabled;
    tempCON.B.OEN   = config->ascConfig.overrunCheckEnabled;
    tempCON.B.FDE   = config->ascConfig.fractionalDividerEnabled;
    tempCON.B.ODD   = config->ascConfig.receiverOddParityEnabled;
    tempCON.B.BRS   = config->ascConfig.baudrateSelection;
    tempCON.B.LB    = config->ascConfig.loopbackEnabled;
    tempCON.B.MTX   = config->ascConfig.transmitMode;
    tempCON.B.ODDTX = config->ascConfig.transmitterOddParityEnabled;
    psi5sSFR->CON.U = tempCON.U;

    if (IfxPsi5s_Psi5s_setBaudrate(psi5sSFR, config->ascConfig.baudrateFrequency, (IfxPsi5s_Psi5s_AscConfig *)&(config->ascConfig))
        == 0)
    {
        status = FALSE;
        return status;
    }
    else
    {}

    if (IfxPsi5s_Psi5s_initializeClock(psi5sSFR, &config->ascConfig.clockOutput) == 0)
    {
        status = FALSE;
        return status;
    }
    else
    {}

    Ifx_PSI5S_TSCNTA tempTSCNTA;
    tempTSCNTA.U       = psi5sSFR->TSCNTA.U;
    tempTSCNTA.B.ETB   = config->timestampCounterA.externalTimeBaseSelect;
    tempTSCNTA.B.TBS   = config->timestampCounterA.timeBaseSelect;
    psi5sSFR->TSCNTA.U = tempTSCNTA.U;

    Ifx_PSI5S_TSCNTB tempTSCNTB;
    tempTSCNTB.U       = psi5sSFR->TSCNTB.U;
    tempTSCNTB.B.ETB   = config->timestampCounterB.externalTimeBaseSelect;
    tempTSCNTB.B.TBS   = config->timestampCounterB.timeBaseSelect;
    psi5sSFR->TSCNTB.U = tempTSCNTB.U;

    Ifx_PSI5S_GCR tempGCR;
    tempGCR.U       = psi5sSFR->GCR.U;
    tempGCR.B.CRCI  = config->globalControlConfig.crcErrorConsideredForRSI;
    tempGCR.B.XCRCI = config->globalControlConfig.xcrcErrorConsideredForRSI;
    tempGCR.B.TEI   = config->globalControlConfig.transmitErrorConsideredForRSI;
    tempGCR.B.PE    = config->globalControlConfig.parityErrorConsideredForRSI;
    tempGCR.B.FE    = config->globalControlConfig.framingErrorConsideredForRSI;
    tempGCR.B.OE    = config->globalControlConfig.overrunErrorConsideredForRSI;
    tempGCR.B.RBI   = config->globalControlConfig.receiveBufferErrorConsideredForRSI;
    tempGCR.B.HDI   = config->globalControlConfig.headerErrorConsideredForRSI;
    tempGCR.B.IDT   = config->globalControlConfig.idleTime;
    tempGCR.B.ASC   = config->globalControlConfig.ascOnlyMode;
    psi5sSFR->GCR.U = tempGCR.U;

    IfxScuWdt_setCpuEndinit(passwd);

    // Pin mapping //
    const IfxPsi5s_Psi5s_Pins *pins = config->pins;

    if (pins != NULL_PTR)
    {
        const IfxPsi5s_Rx_In *rx = pins->rx;

        if (rx != NULL_PTR)
        {
            IfxPsi5s_initRxPin(rx, pins->rxMode, pins->pinDriver);
        }

        const IfxPsi5s_Tx_Out *tx = pins->tx;

        if (tx != NULL_PTR)
        {
            IfxPsi5s_initTxPin(tx, pins->txMode, pins->pinDriver);
        }

        const IfxPsi5s_Clk_Out *clk = pins->clk;

        if (clk != NULL_PTR)
        {
            IfxPsi5s_initClkPin(clk, pins->clkMode, pins->pinDriver);
        }
    }

    return status;
}


void IfxPsi5s_Psi5s_initModuleConfig(IfxPsi5s_Psi5s_Config *config, Ifx_PSI5S *psi5s)
{
    uint32 spbFrequency = IfxScuCcu_getSpbFrequency();
    config->module                                                 = psi5s;
    config->fracDiv.frequency                                      = spbFrequency;
    config->fracDiv.mode                                           = IfxPsi5s_DividerMode_normal;
    config->fracDiv.type                                           = IfxPsi5s_ClockType_fracDiv;
    config->timestampClock.frequency                               = spbFrequency;
    config->timestampClock.mode                                    = IfxPsi5s_DividerMode_normal;
    config->timestampClock.type                                    = IfxPsi5s_ClockType_timeStamp;
    config->timestampCounterA.externalTimeBaseSelect               = IfxPsi5s_Trigger_0;
    config->timestampCounterA.timeBaseSelect                       = IfxPsi5s_TimeBase_internal;
    config->timestampCounterB.externalTimeBaseSelect               = IfxPsi5s_Trigger_0;
    config->timestampCounterB.timeBaseSelect                       = IfxPsi5s_TimeBase_internal;
    config->ascConfig.baudrateFrequency                            = IFXPSI5S_BAUDRATE_1562500;
    config->ascConfig.clockOutput.frequency                        = spbFrequency;
    config->ascConfig.clockOutput.mode                             = IfxPsi5s_DividerMode_normal;
    config->ascConfig.clockOutput.type                             = IfxPsi5s_ClockType_ascOutput;
    config->ascConfig.receiveMode                                  = IfxPsi5s_AscMode_sync;
    config->ascConfig.stopBits                                     = IfxPsi5s_AscStopBits_1;
    config->ascConfig.parityCheckEnabled                           = FALSE;
    config->ascConfig.framingCheckEnabled                          = FALSE;
    config->ascConfig.overrunCheckEnabled                          = FALSE;
    config->ascConfig.fractionalDividerEnabled                     = FALSE;
    config->ascConfig.receiverOddParityEnabled                     = FALSE;
    config->ascConfig.baudrateSelection                            = IfxPsi5s_AscBaudratePrescalar_divideBy2;
    config->ascConfig.loopbackEnabled                              = IfxPsi5s_LoopBackMode_disable;
    config->ascConfig.transmitMode                                 = IfxPsi5s_AscMode_sync;
    config->ascConfig.transmitterOddParityEnabled                  = FALSE;
    config->globalControlConfig.crcErrorConsideredForRSI           = TRUE;
    config->globalControlConfig.xcrcErrorConsideredForRSI          = TRUE;
    config->globalControlConfig.transmitErrorConsideredForRSI      = TRUE;
    config->globalControlConfig.parityErrorConsideredForRSI        = TRUE;
    config->globalControlConfig.framingErrorConsideredForRSI       = TRUE;
    config->globalControlConfig.overrunErrorConsideredForRSI       = FALSE;
    config->globalControlConfig.receiveBufferErrorConsideredForRSI = FALSE;
    config->globalControlConfig.headerErrorConsideredForRSI        = FALSE;
    config->globalControlConfig.idleTime                           = IfxPsi5s_IdleTime_1;
    config->globalControlConfig.ascOnlyMode                        = FALSE;
    config->pins                                                   = NULL_PTR;
}


IFX_STATIC uint32 IfxPsi5s_Psi5s_initializeClock(Ifx_PSI5S *psi5s, const IfxPsi5s_Psi5s_Clock *clock)
{
    uint64               step           = 0;
    uint32               stepRange      = IFXPSI5S_STEP_RANGE;
    uint32               result         = 0;
    IfxPsi5s_DividerMode divMode        = clock->mode;
    IfxPsi5s_ClockType   clockType      = clock->type;
    uint32               clockFrequency = clock->frequency;
    uint32               fInput;
    Ifx_PSI5S_FDR        tempFDR;
    Ifx_PSI5S_FDRT       tempFDRT;
    Ifx_PSI5S_FDO        tempFDO;

    if (clockType == IfxPsi5s_ClockType_fracDiv)
    {
        fInput = IfxScuCcu_getSpbFrequency();
    }
    else if (clockType == IfxPsi5s_ClockType_ascOutput)
    {
        fInput    = IfxScuCcu_getSpbFrequency(); // assumption here is that fBaud2 is equal to fSPB
        stepRange = 2 * IFXPSI5S_STEP_RANGE;
    }
    else
    {
        fInput = IfxPsi5s_Psi5s_getFracDivClock(psi5s);

        if (fInput == 0)
        {
            result = 0;
            return result;
        }
        else
        {}
    }

    switch (divMode)
    {
    case IfxPsi5s_DividerMode_normal:
        step = stepRange - (fInput / clockFrequency);

        if (step > (stepRange - 1))
        {
            step = stepRange - 1;
        }
        else
        {
            /* do nothing */
        }

        result = (uint32)(fInput / (stepRange - step));
        break;

    case IfxPsi5s_DividerMode_fractional:
        step = (uint64)((uint64)clockFrequency * stepRange) / fInput;

        if (step > (stepRange - 1))
        {
            step = stepRange - 1;
        }
        else
        {
            /* do nothing */
        }

        result = (uint32)((uint64)((uint64)fInput * step)) / stepRange;
        break;

    case IfxPsi5s_DividerMode_off:
    default:
        step   = 0;
        result = 0;
        break;
    }

    if (result != 0)
    {
        switch (clockType)
        {
        case IfxPsi5s_ClockType_fracDiv:
            tempFDR.U      = 0;
            tempFDR.B.DM   = divMode;
            tempFDR.B.STEP = (uint32)step;
            psi5s->FDR.U   = tempFDR.U;
            break;

        case IfxPsi5s_ClockType_timeStamp:
            tempFDRT.U      = 0;
            tempFDRT.B.DM   = divMode;
            tempFDRT.B.STEP = (uint32)step;
            psi5s->FDRT.U   = tempFDRT.U;
            break;

        case IfxPsi5s_ClockType_ascOutput:
            tempFDO.U      = 0;
            tempFDO.B.DM   = divMode;
            tempFDO.B.STEP = (uint32)step;
            psi5s->FDO.U   = tempFDO.U;
            break;
        default:
            break;
        }
    }

    return result;
}


void IfxPsi5s_Psi5s_readFrame(IfxPsi5s_Psi5s_Channel *channel, IfxPsi5s_Psi5s_Frame *frame)
{
    frame->data.rdr                                       = channel->module->psi5s->RDR.U;
    frame->status.rds                                     = channel->module->psi5s->RDS.U;
    frame->timestamp.tsm                                  = channel->module->psi5s->TSM.U;

    channel->module->psi5s->INTCLR[channel->channelId].U |= (IFX_PSI5S_INTCLR_RDI_MSK << IFX_PSI5S_INTCLR_RDI_OFF) | (IFX_PSI5S_INTCLR_RSI_MSK << IFX_PSI5S_INTCLR_RSI_OFF);
}


void IfxPsi5s_Psi5s_resetModule(Ifx_PSI5S *psi5s)
{
    uint16 passwd = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearSafetyEndinit(passwd);
    psi5s->KRST1.B.RST = 1;     /* Only if both Kernel reset bits are set a reset is executed */
    psi5s->KRST0.B.RST = 1;

    while (psi5s->KRST0.B.RSTSTAT == 0)
    {
        /* Wait until reset is executed */
    }

    psi5s->KRSTCLR.B.CLR = 1;   /* Clear Kernel reset status bit */
    IfxScuWdt_setSafetyEndinit(passwd);
}


boolean IfxPsi5s_Psi5s_sendChannelData(IfxPsi5s_Psi5s_Channel *channel, uint32 data)
{
    channel->module->psi5s->SDR[channel->channelId].U = data & 0x00FFFFFF;

    if (channel->module->psi5s->INTSTAT[channel->channelId].B.TPOI)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


IFX_STATIC uint32 IfxPsi5s_Psi5s_setBaudrate(Ifx_PSI5S *psi5s, uint32 baudrate, IfxPsi5s_Psi5s_AscConfig *ascConfig)
{
    uint32 bgValue = 0;
    uint32 fdValue = 0;
    uint32 result  = 0;
    uint32 fInput;

    if (ascConfig->receiveMode == IfxPsi5s_AscMode_sync)
    {
        if (ascConfig->transmitMode != IfxPsi5s_AscMode_sync)
        {
            // sync modes must be set for both receive and transmit
        }

        fInput  = IfxScuCcu_getBaud2Frequency();
        bgValue = (uint32)(fInput / ((ascConfig->baudrateSelection + 2) * 4 * (uint64)baudrate) - 1);

        if (bgValue > (IFXPSI5S_BG_RANGE - 1))
        {
            bgValue = IFXPSI5S_BG_RANGE - 1;
        }
        else
        {
            /* do nothing */
        }

        result = fInput / ((ascConfig->baudrateSelection + 2) * 4 * (bgValue + 1));
    }
    else if (ascConfig->fractionalDividerEnabled == FALSE)
    {
        fInput  = IfxScuCcu_getBaud2Frequency();
        bgValue = (uint32)(fInput / ((ascConfig->baudrateSelection + 2) * 16 * (uint64)baudrate) - 1);

        if (bgValue > (IFXPSI5S_BG_RANGE - 1))
        {
            bgValue = IFXPSI5S_BG_RANGE - 1;
        }
        else
        {
            /* do nothing */
        }

        result = fInput / ((ascConfig->baudrateSelection + 2) * 16 * (bgValue + 1));
    }
    else
    {
        fInput  = IfxScuCcu_getBaud2Frequency();
        fdValue = (((uint64)baudrate * IFXPSI5S_FDV_RANGE * 16)) / (float)fInput;

        if (fdValue > (IFXPSI5S_FDV_RANGE - 1))
        {
            fdValue = IFXPSI5S_FDV_RANGE - 1;
            bgValue = ((float)fdValue / IFXPSI5S_FDV_RANGE) * (fInput / (16 * baudrate)) - 1;

            if (bgValue > (IFXPSI5S_BG_RANGE - 1))
            {
                bgValue = IFXPSI5S_BG_RANGE - 1;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            bgValue = 0;
        }

        result = ((float)fdValue / IFXPSI5S_FDV_RANGE) * (fInput / (16 * (bgValue + 1)));
    }

    psi5s->FDV.U = fdValue;
    psi5s->BG.U  = bgValue;

    return result;
}
