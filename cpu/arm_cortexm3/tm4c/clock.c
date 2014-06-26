/*
 * Copyright (C) 2011
 * Dmitry Cherukhin, Emcraft Systems, dima_ch@emcraft.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <asm/io.h>
#include "clock.h"

#define PART_TM4C1294NCPDT
#include <common.h>
#include "tivaware/gpio.c"
#include "tivaware/pinout.c"

/*
 * System register clock control mask and shift for PCLK dividers.
 */
#define PCLK_DIV_MASK		0x00000003
#define PCLK0_DIV_SHIFT		2
#define PCLK1_DIV_SHIFT		4
#define ACE_DIV_SHIFT		6

/*
 * System register MSS_CCC_DIV_CR mask and shift for GLB (FPGA fabric clock).
 */
#define OBDIV_SHIFT		8
#define OBDIV_MASK		0x0000001F
#define OBDIVHALF_SHIFT		13
#define OBDIVHALF_MASK		0x00000001

/*
 * Actel system boot version defines used to extract the system clock from eNVM
 * spare pages.
 * These defines allow detecting the presence of Actel system boot in eNVM spare
 * pages and the version of that system boot executable and associated
 * configuration data.
 */
#define SYSBOOT_KEY_ADDR	0x6008081C
#define SYSBOOT_KEY_VALUE	0x4C544341
#define SYSBOOT_VERSION_ADDR	0x60080840
#define SYSBOOT_1_3_FCLK_ADDR	0x6008162C
#define SYSBOOT_2_x_FCLK_ADDR	0x60081EAC

/*
 * The system boot version is stored in the least significant 24 bits of a word.
 * The FCLK is stored in eNVM from version 1.3.1 of the system boot. We expect
 * that the major version number of the system boot version will change if the
 * system boot configuration data layout needs to change.
 */
#define SYSBOOT_VERSION_MASK	0x00FFFFFF
#define MIN_SYSBOOT_VERSION	0x00010301
#define SYSBOOT_VERSION_2_X	0x00020000
#define MAX_SYSBOOT_VERSION	0x00030000

#include "tivaware/sysctl.h"
#include "tivaware/hw_sysctl.h"
#include "tivaware/hw_types.h"

//*****************************************************************************
//
// This macro converts the XTAL value provided in the ui32Config parameter to
// an index to the g_pui32Xtals array.
//
//*****************************************************************************
#define SysCtlXtalCfgToIndex(a) ((a & 0x7c0) >> 6)

//*****************************************************************************
//
// An array that maps the crystal number in RCC to a frequency.
//
//*****************************************************************************
static const uint32_t g_pui32Xtals[] =
{
    1000000,
    1843200,
    2000000,
    2457600,
    3579545,
    3686400,
    4000000,
    4096000,
    4915200,
    5000000,
    5120000,
    6000000,
    6144000,
    7372800,
    8000000,
    8192000,
    10000000,
    12000000,
    12288000,
    13560000,
    14318180,
    16000000,
    16384000,
    18000000,
    20000000,
    24000000,
    25000000
};

//*****************************************************************************
//
// Maximum number of VCO entries in the g_pui32XTALtoVCO and
// g_pui32VCOFrequencies structures for a device.
//
//*****************************************************************************
#define MAX_VCO_ENTRIES         2
#define MAX_XTAL_ENTRIES        18

//*****************************************************************************
//
// These macros are used in the g_pui32XTALtoVCO table to make it more
// readable.
//
//*****************************************************************************
#define PLL_M_TO_REG(mi, mf)                                                  \
        ((uint32_t)mi | (uint32_t)(mf << SYSCTL_PLLFREQ0_MFRAC_S))
#define PLL_N_TO_REG(n)                                                       \
        ((uint32_t)(n - 1) << SYSCTL_PLLFREQ1_N_S)

//*****************************************************************************
//
// Look up of the values that go into the PLLFREQ0 and PLLFREQ1 registers.
//
//*****************************************************************************
static const uint32_t g_pppui32XTALtoVCO[MAX_VCO_ENTRIES][MAX_XTAL_ENTRIES][2] =
{
    {
        //
        // VCO 320 MHz
        //
        { PLL_M_TO_REG(64, 0),   PLL_N_TO_REG(1) },     // 5 MHz
        { PLL_M_TO_REG(62, 512), PLL_N_TO_REG(1) },     // 5.12 MHz
        { PLL_M_TO_REG(160, 0),  PLL_N_TO_REG(3) },     // 6 MHz
        { PLL_M_TO_REG(52, 85),  PLL_N_TO_REG(1) },     // 6.144 MHz
        { PLL_M_TO_REG(43, 412), PLL_N_TO_REG(1) },     // 7.3728 MHz
        { PLL_M_TO_REG(40, 0),   PLL_N_TO_REG(1) },     // 8 MHz
        { PLL_M_TO_REG(39, 64),  PLL_N_TO_REG(1) },     // 8.192 MHz
        { PLL_M_TO_REG(32, 0),   PLL_N_TO_REG(1) },     // 10 MHz
        { PLL_M_TO_REG(80, 0),   PLL_N_TO_REG(3) },     // 12 MHz
        { PLL_M_TO_REG(26, 43),  PLL_N_TO_REG(1) },     // 12.288 MHz
        { PLL_M_TO_REG(23, 613), PLL_N_TO_REG(1) },     // 13.56 MHz
        { PLL_M_TO_REG(22, 358), PLL_N_TO_REG(1) },     // 14.318180 MHz
        { PLL_M_TO_REG(20, 0),   PLL_N_TO_REG(1) },     // 16 MHz
        { PLL_M_TO_REG(19, 544), PLL_N_TO_REG(1) },     // 16.384 MHz
        { PLL_M_TO_REG(160, 0),  PLL_N_TO_REG(9) },     // 18 MHz
        { PLL_M_TO_REG(16, 0),   PLL_N_TO_REG(1) },     // 20 MHz
        { PLL_M_TO_REG(40, 0),   PLL_N_TO_REG(3) },     // 24 MHz
        { PLL_M_TO_REG(64, 0),   PLL_N_TO_REG(5) },     // 25 MHz
    },
    {
        //
        // VCO 480 MHz
        //
        { PLL_M_TO_REG(96, 0),   PLL_N_TO_REG(1) },     // 5 MHz
        { PLL_M_TO_REG(93, 768), PLL_N_TO_REG(1) },     // 5.12 MHz
        { PLL_M_TO_REG(80, 0),   PLL_N_TO_REG(1) },     // 6 MHz
        { PLL_M_TO_REG(78, 128), PLL_N_TO_REG(1) },     // 6.144 MHz
        { PLL_M_TO_REG(65, 107), PLL_N_TO_REG(1) },     // 7.3728 MHz
        { PLL_M_TO_REG(60, 0),   PLL_N_TO_REG(1) },     // 8 MHz
        { PLL_M_TO_REG(58, 608), PLL_N_TO_REG(1) },     // 8.192 MHz
        { PLL_M_TO_REG(48, 0),   PLL_N_TO_REG(1) },     // 10 MHz
        { PLL_M_TO_REG(40, 0),   PLL_N_TO_REG(1) },     // 12 MHz
        { PLL_M_TO_REG(39, 64),  PLL_N_TO_REG(1) },     // 12.288 MHz
        { PLL_M_TO_REG(35, 408), PLL_N_TO_REG(1) },     // 13.56 MHz
        { PLL_M_TO_REG(33, 536), PLL_N_TO_REG(1) },     // 14.318180 MHz
        { PLL_M_TO_REG(30, 0),   PLL_N_TO_REG(1) },     // 16 MHz
        { PLL_M_TO_REG(29, 304), PLL_N_TO_REG(1) },     // 16.384 MHz
        { PLL_M_TO_REG(80, 0),   PLL_N_TO_REG(3) },     // 18 MHz
        { PLL_M_TO_REG(24, 0),   PLL_N_TO_REG(1) },     // 20 MHz
        { PLL_M_TO_REG(20, 0),   PLL_N_TO_REG(1) },     // 24 MHz
        { PLL_M_TO_REG(96, 0),   PLL_N_TO_REG(5) },     // 25 MHz
    },
};

//*****************************************************************************
//
// The mapping of system clock frequency to flash memory timing parameters.
//
//*****************************************************************************
static const struct
{
    uint32_t ui32Frequency;
    uint32_t ui32MemTiming;
}
g_sXTALtoMEMTIM[] =
{
    { 16000000, (SYSCTL_MEMTIM0_FBCHT_0_5 | SYSCTL_MEMTIM0_FBCE |
                 (0 << SYSCTL_MEMTIM0_FWS_S) |
                 SYSCTL_MEMTIM0_EBCHT_0_5 | SYSCTL_MEMTIM0_EBCE |
                 (0 << SYSCTL_MEMTIM0_EWS_S) |
                 SYSCTL_MEMTIM0_MB1) },
    { 40000000, (SYSCTL_MEMTIM0_FBCHT_1_5 | (1 << SYSCTL_MEMTIM0_FWS_S) |
                 SYSCTL_MEMTIM0_FBCHT_1_5 | (1 << SYSCTL_MEMTIM0_EWS_S) |
                 SYSCTL_MEMTIM0_MB1) },
    { 60000000, (SYSCTL_MEMTIM0_FBCHT_2 | (2 << SYSCTL_MEMTIM0_FWS_S) |
                 SYSCTL_MEMTIM0_EBCHT_2 | (2 << SYSCTL_MEMTIM0_EWS_S) |
                 SYSCTL_MEMTIM0_MB1) },
    { 80000000, (SYSCTL_MEMTIM0_FBCHT_2_5 | (3 << SYSCTL_MEMTIM0_FWS_S) |
                 SYSCTL_MEMTIM0_EBCHT_2_5 | (3 << SYSCTL_MEMTIM0_EWS_S) |
                 SYSCTL_MEMTIM0_MB1) },
    { 100000000, (SYSCTL_MEMTIM0_FBCHT_3 | (4 << SYSCTL_MEMTIM0_FWS_S) |
                  SYSCTL_MEMTIM0_EBCHT_3 | (4 << SYSCTL_MEMTIM0_EWS_S) |
                  SYSCTL_MEMTIM0_MB1) },
    { 120000000, (SYSCTL_MEMTIM0_FBCHT_3_5 | (5 << SYSCTL_MEMTIM0_FWS_S) |
                  SYSCTL_MEMTIM0_EBCHT_3_5 | (5 << SYSCTL_MEMTIM0_EWS_S) |
                  SYSCTL_MEMTIM0_MB1) },
};

//*****************************************************************************
//
// Get the correct memory timings for a given system clock value.
//
//*****************************************************************************
static uint32_t
_SysCtlMemTimingGet(uint32_t ui32SysClock)
{
    uint8_t ui8Idx;

    //
    // Loop through the flash memory timings.
    //
    for(ui8Idx = 0;
        ui8Idx < (sizeof(g_sXTALtoMEMTIM) / sizeof(g_sXTALtoMEMTIM[0]));
        ui8Idx++)
    {
        //
        // See if the system clock frequency is less than the maximum frequency
        // for this flash memory timing.
        //
        if(ui32SysClock <= g_sXTALtoMEMTIM[ui8Idx].ui32Frequency)
        {
            //
            // This flash memory timing is the best choice for the system clock
            // frequency, so return it now.
            //
            return(g_sXTALtoMEMTIM[ui8Idx].ui32MemTiming);
        }
    }

    //
    // An appropriate flash memory timing could not be found, so the device is
    // being clocked too fast.  Return the default flash memory timing.
    //
    return(0);
}

//*****************************************************************************
//
// Calculate the system frequency from the register settings base on the
// oscillator input.
//
//*****************************************************************************
static uint32_t
_SysCtlFrequencyGet(uint32_t ui32Xtal)
{
    uint32_t ui32Result;
    uint16_t ui16F1, ui16F2;
    uint16_t ui16PInt, ui16PFract;
    uint8_t ui8Q, ui8N;

    //
    // Extract all of the values from the hardware registers.
    //
    ui16PFract = ((HWREG(SYSCTL_PLLFREQ0) & SYSCTL_PLLFREQ0_MFRAC_M) >>
                  SYSCTL_PLLFREQ0_MFRAC_S);
    ui16PInt = HWREG(SYSCTL_PLLFREQ0) & SYSCTL_PLLFREQ0_MINT_M;
    ui8Q = (((HWREG(SYSCTL_PLLFREQ1) & SYSCTL_PLLFREQ1_Q_M) >>
             SYSCTL_PLLFREQ1_Q_S) + 1);
    ui8N = (((HWREG(SYSCTL_PLLFREQ1) & SYSCTL_PLLFREQ1_N_M) >>
             SYSCTL_PLLFREQ1_N_S) + 1);

    //
    // Divide the crystal value by N.
    //
    ui32Xtal /= (uint32_t)ui8N;

    //
    // Calculate the multiplier for bits 9:5.
    //
    ui16F1 = ui16PFract / 32;

    //
    // Calculate the multiplier for bits 4:0.
    //
    ui16F2 = ui16PFract - (ui16F1 * 32);

    //
    // Get the integer portion.
    //
    ui32Result = ui32Xtal * (uint32_t)ui16PInt;

    //
    // Add first fractional bits portion(9:0).
    //
    ui32Result += (ui32Xtal * (uint32_t)ui16F1) / 32;

    //
    // Add the second fractional bits portion(4:0).
    //
    ui32Result += (ui32Xtal * (uint32_t)ui16F2) / 1024;

    //
    // Divide the result by Q.
    //
    ui32Result = ui32Result / (uint32_t)ui8Q;

    //
    // Return the resulting PLL frequency.
    //
    return(ui32Result);
}

//*****************************************************************************
//
// Look up of the possible VCO frequencies.
//
//*****************************************************************************
static const uint32_t g_pui32VCOFrequencies[MAX_VCO_ENTRIES] =
{
    320000000,                              // VCO 320
    480000000,                              // VCO 480
};

static unsigned long clock[CLOCK_END];

uint32_t
SysCtlClockFreqSet(uint32_t ui32Config, uint32_t ui32SysClock)
{
    int32_t i32Timeout, i32VCOIdx, i32XtalIdx;
    uint32_t ui32MOSCCTL;
    uint32_t ui32SysDiv, ui32Osc, ui32OscSelect, ui32RSClkConfig;
    int bNewPLL;

    //
    // Get the index of the crystal from the ui32Config parameter.
    //
    i32XtalIdx = SysCtlXtalCfgToIndex(ui32Config);

    //
    // Determine which non-PLL source was selected.
    //
    if((ui32Config & 0x38) == SYSCTL_OSC_INT)
    {
        //
        // Use the nominal frequency for the PIOSC oscillator and set the
        // crystal select.
        //
        ui32Osc = 16000000;
        ui32OscSelect = SYSCTL_RSCLKCFG_OSCSRC_PIOSC;
        ui32OscSelect |= SYSCTL_RSCLKCFG_PLLSRC_PIOSC;

        //
        // Force the crystal index to the value for 16-MHz.
        //
        i32XtalIdx = SysCtlXtalCfgToIndex(SYSCTL_XTAL_16MHZ);
    }
    else if((ui32Config & 0x38) == SYSCTL_OSC_INT30)
    {
        //
        // Use the nominal frequency for the low frequency oscillator.
        //
        ui32Osc = 30000;
        ui32OscSelect = SYSCTL_RSCLKCFG_OSCSRC_LFIOSC;
    }
    else if((ui32Config & 0x38) == (SYSCTL_OSC_EXT32 & 0x38))
    {
        //
        // Use the RTC frequency.
        //
        ui32Osc = 32768;
        ui32OscSelect = SYSCTL_RSCLKCFG_OSCSRC_RTC;
    }
    else if((ui32Config & 0x38) == SYSCTL_OSC_MAIN)
    {
        //
        // Bounds check the source frequency for the main oscillator.  The is
        // because the PLL tables in the g_pppui32XTALtoVCO structure range
        // from 5MHz to 25MHz.
        //
        if((i32XtalIdx > (SysCtlXtalCfgToIndex(SYSCTL_XTAL_25MHZ))) ||
           (i32XtalIdx < (SysCtlXtalCfgToIndex(SYSCTL_XTAL_5MHZ))))
        {
            return(0);
        }

        ui32Osc = g_pui32Xtals[i32XtalIdx];

        //
        // Set the PLL source select to MOSC.
        //
        ui32OscSelect = SYSCTL_RSCLKCFG_OSCSRC_MOSC;
        ui32OscSelect |= SYSCTL_RSCLKCFG_PLLSRC_MOSC;

        //
        // Clear MOSC power down, high oscillator range setting, and no crystal
        // present setting.
        //
        ui32MOSCCTL = HWREG(SYSCTL_MOSCCTL) &
                      ~(SYSCTL_MOSCCTL_OSCRNG | SYSCTL_MOSCCTL_PWRDN |
                        SYSCTL_MOSCCTL_NOXTAL);

        //
        // Increase the drive strength for MOSC of 10 MHz and above.
        //
        if(i32XtalIdx >= (SysCtlXtalCfgToIndex(SYSCTL_XTAL_10MHZ) -
                          (SysCtlXtalCfgToIndex(SYSCTL_XTAL_5MHZ))))
        {
            ui32MOSCCTL |= SYSCTL_MOSCCTL_OSCRNG;
        }

        HWREG(SYSCTL_MOSCCTL) = ui32MOSCCTL;
    }
    else
    {
        //
        // This was an invalid request because no oscillator source was
        // indicated.
        //
        ui32Osc = 0;
        ui32OscSelect = SYSCTL_RSCLKCFG_OSCSRC_PIOSC;
    }

    //
    // Check if the running with the PLL enabled was requested.
    //
    if((ui32Config & SYSCTL_USE_OSC) == SYSCTL_USE_PLL)
    {
        //
        // ui32Config must be SYSCTL_OSC_MAIN or SYSCTL_OSC_INT.
        //
        if(((ui32Config & 0x38) != SYSCTL_OSC_MAIN) &&
           ((ui32Config & 0x38) != SYSCTL_OSC_INT))
        {
            return(0);
        }

        //
        // Get the VCO index out of the ui32Config parameter.
        //
        i32VCOIdx = (ui32Config >> 24) & 7;

        //
        // Check that the VCO index is not out of bounds.
        //
        //ASSERT(i32VCOIdx < MAX_VCO_ENTRIES);

        //
        // Set the memory timings for the maximum external frequency since
        // this could be a switch to PIOSC or possibly to MOSC which can be
        // up to 25MHz.
        //
        HWREG(SYSCTL_MEMTIM0) = _SysCtlMemTimingGet(25000000);

        //
        // Clear the old PLL divider and source in case it was set.
        //
        ui32RSClkConfig = HWREG(SYSCTL_RSCLKCFG) &
                          ~(SYSCTL_RSCLKCFG_PSYSDIV_M |
                            SYSCTL_RSCLKCFG_OSCSRC_M |
                            SYSCTL_RSCLKCFG_PLLSRC_M | SYSCTL_RSCLKCFG_USEPLL);

        //
        // Update the memory timings to match running from PIOSC.
        //
        ui32RSClkConfig |= SYSCTL_RSCLKCFG_MEMTIMU;

        //
        // Update clock configuration to switch back to PIOSC.
        //
        HWREG(SYSCTL_RSCLKCFG) = ui32RSClkConfig;

        //
        // The table starts at 5 MHz so modify the index to match this.
        //
        i32XtalIdx -= SysCtlXtalCfgToIndex(SYSCTL_XTAL_5MHZ);

        //
        // If there were no changes to the PLL do not force the PLL to lock by
        // writing the PLL settings.
        //
        if((HWREG(SYSCTL_PLLFREQ1) !=
            g_pppui32XTALtoVCO[i32VCOIdx][i32XtalIdx][1]) ||
           (HWREG(SYSCTL_PLLFREQ0) !=
            (g_pppui32XTALtoVCO[i32VCOIdx][i32XtalIdx][0] |
             SYSCTL_PLLFREQ0_PLLPWR)))
        {
            bNewPLL = 1;
        }
        else
        {
            bNewPLL = 0;
        }

        //
        // If there are new PLL settings write them.
        //
        if(bNewPLL)
        {
            //
            // Set the oscillator source.
            //
            HWREG(SYSCTL_RSCLKCFG) |= ui32OscSelect;

            //
            // Set the M, N and Q values provided from the table and preserve
            // the power state of the main PLL.
            //
            HWREG(SYSCTL_PLLFREQ1) =
                g_pppui32XTALtoVCO[i32VCOIdx][i32XtalIdx][1];
            HWREG(SYSCTL_PLLFREQ0) =
                (g_pppui32XTALtoVCO[i32VCOIdx][i32XtalIdx][0] |
                 (HWREG(SYSCTL_PLLFREQ0) & SYSCTL_PLLFREQ0_PLLPWR));
        }

        //
        // Calculate the System divider such that we get a frequency that is
        // the closest to the requested frequency without going over.
        //
        ui32SysDiv = (g_pui32VCOFrequencies[i32VCOIdx] + ui32SysClock - 1) /
                     ui32SysClock;

        //
        // Calculate the actual system clock.
        //
        ui32SysClock = _SysCtlFrequencyGet(ui32Osc) / ui32SysDiv;

        //
        // Set the Flash and EEPROM timing values.
        //
        HWREG(SYSCTL_MEMTIM0) = _SysCtlMemTimingGet(ui32SysClock);

        //
        // Check if the PLL is already powered up.
        //
        if(HWREG(SYSCTL_PLLFREQ0) & SYSCTL_PLLFREQ0_PLLPWR)
        {
            if(bNewPLL)
            {
                //
                // Trigger the PLL to lock to the new frequency.
                //
                HWREG(SYSCTL_RSCLKCFG) |= SYSCTL_RSCLKCFG_NEWFREQ;
            }
        }
        else
        {
            //
            // Power up the PLL.
            //
            HWREG(SYSCTL_PLLFREQ0) |= SYSCTL_PLLFREQ0_PLLPWR;
        }

        //
        // Wait until the PLL has locked.
        //
        for(i32Timeout = 32768; i32Timeout > 0; i32Timeout--)
        {
            if((HWREG(SYSCTL_PLLSTAT) & SYSCTL_PLLSTAT_LOCK))
            {
                break;
            }
        }

        //
        // If the loop above did not timeout then switch over to the PLL
        //
        if(i32Timeout)
        {
            ui32RSClkConfig = HWREG(SYSCTL_RSCLKCFG);
            ui32RSClkConfig |= ((ui32SysDiv - 1) <<
                                SYSCTL_RSCLKCFG_PSYSDIV_S) | ui32OscSelect |
                               SYSCTL_RSCLKCFG_USEPLL;
            ui32RSClkConfig |= SYSCTL_RSCLKCFG_MEMTIMU;

            //
            // Set the new clock configuration.
            //
            HWREG(SYSCTL_RSCLKCFG) = ui32RSClkConfig;
        }
        else
        {
            ui32SysClock = 0;
        }
    }
    else
    {
        //
        // Set the Flash and EEPROM timing values for PIOSC.
        //
        HWREG(SYSCTL_MEMTIM0) = _SysCtlMemTimingGet(16000000);

        //
        // Make sure that the PLL is powered down since it is not being used.
        //
        HWREG(SYSCTL_PLLFREQ0) &= ~SYSCTL_PLLFREQ0_PLLPWR;

        //
        // Clear the old PLL divider and source in case it was set.
        //
        ui32RSClkConfig = HWREG(SYSCTL_RSCLKCFG);
        ui32RSClkConfig &= ~(SYSCTL_RSCLKCFG_OSYSDIV_M |
                             SYSCTL_RSCLKCFG_OSCSRC_M |
                             SYSCTL_RSCLKCFG_USEPLL);

        //
        // Update the memory timings.
        //
        ui32RSClkConfig |= SYSCTL_RSCLKCFG_MEMTIMU;

        //
        // Set the new clock configuration.
        //
        HWREG(SYSCTL_RSCLKCFG) = ui32RSClkConfig;

        //
        // If zero given as the system clock then default to divide by 1.
        //
        if(ui32SysClock == 0)
        {
            ui32SysDiv = 0;
        }
        else
        {
            //
            // Calculate the System divider based on the requested
            // frequency.
            //
            ui32SysDiv = ui32Osc / ui32SysClock;

            //
            // If the system divisor is not already zero, subtract one to
            // set the value in the register which requires the value to
            // be n-1.
            //
            if(ui32SysDiv != 0)
            {
                ui32SysDiv -= 1;
            }

            //
            // Calculate the system clock.
            //
            ui32SysClock = ui32Osc / (ui32SysDiv + 1);
        }

        //
        // Set the memory timing values for the new system clock.
        //
        HWREG(SYSCTL_MEMTIM0) = _SysCtlMemTimingGet(ui32SysClock);

        //
        // Set the new system clock values.
        //
        ui32RSClkConfig = HWREG(SYSCTL_RSCLKCFG);
        ui32RSClkConfig |= (ui32SysDiv << SYSCTL_RSCLKCFG_OSYSDIV_S) |
                           ui32OscSelect;

        //
        // Update the memory timings.
        //
        ui32RSClkConfig |= SYSCTL_RSCLKCFG_MEMTIMU;

        //
        // Set the new clock configuration.
        //
        HWREG(SYSCTL_RSCLKCFG) = ui32RSClkConfig;
    }

    return(ui32SysClock);
}
/*
 * Retrieve the system clock frequency from eNVM spare page if available.
 * Returns the frequency defined through SMARTFUSION_FCLK_FREQ
 * if FCLK cannot be retrieved from eNVM spare pages.
 * The FCLK frequency value selected in the MSS Configurator software tool
 * is stored in eNVM spare pages as part of the Actel system boot
 * configuration data.
 * @returns		FCLK clock frequency
 */
static unsigned long clock_get_fclk(void)
{
	unsigned long fclk = 0;

	/*
	 * XXX:
	 * Try some reading magic and if that fails, return a hard-coded
	 * default.
	 * Step one, just return hard coded value, step two see if we can
	 * calculate this.
	 */
	/*
	 * Looking at TivaWare, MAP_SysCtlClockFreqSet sets the freq and then
	 * returns if we were able to, or what we did.
	 * 120000000 is clock freq in Hz.
	 */
	fclk = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
				SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
				SYSCTL_CFG_VCO_480), 120000000);


	return fclk;
}

/*
 * Initialize the reference clocks.
 * Get the reference clock settings from the hardware.
 * System frequency (FCLK) and the other derivative clocks
 * coming out from firmware. These are defined by the Libero
 * project programmed onto SmartFusion and then, optionally, by firmware.
 */
void clock_init(void)
{
#if 0
	unsigned long clk_cr;
	unsigned long pclk0_div;
	unsigned long pclk1_div;
	unsigned long ace_div;

	const unsigned long pclk_div_lut[4] = {1, 2, 4, 1};

	/*
	 * Read PCLK dividers from system registers.
	 */
	clk_cr = readl(&A2F_SYSREG->mss_clk_cr);
	pclk0_div = pclk_div_lut[(clk_cr >> PCLK0_DIV_SHIFT) & PCLK_DIV_MASK];
	pclk1_div = pclk_div_lut[(clk_cr >> PCLK1_DIV_SHIFT) & PCLK_DIV_MASK];
	ace_div	 = pclk_div_lut[(clk_cr >> ACE_DIV_SHIFT) & PCLK_DIV_MASK];
#endif

	/*
	 * Retrieve FCLK from eNVM spare pages
	 * if Actel system boot programmed as part of the system.
	 */
	clock[CLOCK_FCLK]	= clock_get_fclk();

	clock[CLOCK_PCLK0]	= clock[CLOCK_FCLK] ;
	clock[CLOCK_PCLK1]	= clock[CLOCK_FCLK] ;
	clock[CLOCK_ACE]	= clock[CLOCK_FCLK] ;

	/*
	 * Now, initialize the system timer clock source.
	 * Release systimer from reset
	 */
	//A2F_SYSREG->soft_rst_cr &= ~A2F_SOFT_RST_TIMER_SR;
	/*
	 * enable 32bit timer1
	 */
	//A2F_TIMER->timer64_mode &= ~A2F_TIM64_64MODE_EN;
	/*
	 * timer1 is used by envm driver
	 */
	//A2F_TIMER->timer1_ctrl = A2F_TIM_CTRL_MODE_ONESHOT | A2F_TIM_CTRL_EN;
	/*
	 * No reference clock
	 */
	//A2F_SYSREG->systick_cr &= ~A2F_SYSTICK_NOREF;
	/*
	 * div by 32
	 */
	//A2F_SYSREG->systick_cr |= (A2F_SYSTICK_STCLK_DIV_32 <<
	//			A2F_SYSTICK_STCLK_DIV_SHIFT);
	//A2F_SYSREG->systick_cr &= ~A2F_SYSTICK_TENMS_MSK;
	//A2F_SYSREG->systick_cr |= 0x7a12;

	clock[CLOCK_SYSTICK] = clock[CLOCK_FCLK] / 32;
}

/*
 * Return a clock value for the specified clock.
 */
/* XXX: Should be OK? */
ulong clock_get(enum clock clck)
{
	ulong res = 0;

	if (clck >= 0 && clck < CLOCK_END) {
		res = clock[clck];
	}

	return res;
}
