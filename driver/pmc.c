/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaiimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "hardware.h"
#include "arch/at91_pmc.h"

static inline void write_pmc(unsigned int offset, const unsigned int value)
{
	writel(value, offset + AT91C_BASE_PMC);
}

static inline unsigned int read_pmc(unsigned int offset)
{
	return readl(offset + AT91C_BASE_PMC);
}

void lowlevel_clock_init()
{
	unsigned long tmp;
	unsigned int times;

#if defined(AT91SAM9X5) || defined(AT91SAM9N12) || defined(SAMA5D3X)
	/*
	 * Enable the 12MHz oscillator
	 * tST_max = 2ms
	 * Startup Time: 32768 * 2ms / 8 = 8
	 */
	tmp = read_pmc(PMC_MOR);
	tmp &= (~AT91C_CKGR_MOSCXTST);
	tmp &= (~AT91C_CKGR_KEY);
	tmp |= AT91C_CKGR_MOSCXTEN;
	tmp |= AT91_CKGR_MOSCXTST_SET(8);
	tmp |= AT91C_CKGR_PASSWD;
	write_pmc(PMC_MOR, tmp);

	times = 1000;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MOSCXTS)))
		;

	/* Switch from internal 12MHz RC to the 12MHz oscillator */
	tmp = read_pmc(PMC_MOR);
	tmp &= (~AT91C_CKGR_MOSCXTBY);
	tmp &= (~AT91C_CKGR_KEY);
	tmp |= AT91C_CKGR_PASSWD;
	write_pmc(PMC_MOR, tmp);

	tmp = read_pmc(PMC_MOR);
	tmp |= AT91C_CKGR_MOSCSEL;
	tmp &= (~AT91C_CKGR_KEY);
	tmp |= AT91C_CKGR_PASSWD;
	write_pmc(PMC_MOR, tmp);

	times = 1000;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MOSCSELS)))
		;

	/* Disable the 12MHz RC oscillator */
	tmp = read_pmc(PMC_MOR);
	tmp &= (~AT91C_CKGR_MOSCRCEN);
	tmp &= (~AT91C_CKGR_KEY);
	tmp |= AT91C_CKGR_PASSWD;
	write_pmc(PMC_MOR, tmp);

#else
	/*
	 * Enable the 12MHz oscillator
	 * tST_max = 2ms
	 * Startup Time: 32768 * 2ms / 8 = 8
	 */
	tmp = read_pmc(PMC_MOR);
	tmp &= (~AT91C_CKGR_MOSCXTST);
	tmp |= AT91C_CKGR_MOSCXTEN;
	tmp |= AT91_CKGR_MOSCXTST_SET(8);
	write_pmc(PMC_MOR, tmp);

	times = 1000;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MOSCXTS)))
		;
#endif

	/* After stablization, switch to Main Oscillator */
	if ((read_pmc(PMC_MCKR) & AT91C_PMC_CSS) == AT91C_PMC_CSS_SLOW_CLK) {
		tmp = read_pmc(PMC_MCKR);
		tmp &= ~AT91C_PMC_CSS;
		tmp |= AT91C_PMC_CSS_MAIN_CLK;
		write_pmc(PMC_MCKR, tmp);

		times = 1000;
		while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MCKRDY)))
			;

		tmp &= ~AT91C_PMC_PRES;
		tmp |= AT91C_PMC_PRES_CLK;
		write_pmc(PMC_MCKR, tmp);

		times = 1000;
		while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MCKRDY)))
			;
	}

	return;
}

void pmc_init_pll(unsigned int pmc_pllicpr)
{
	write_pmc(PMC_PLLICPR, pmc_pllicpr);
}

int pmc_cfg_plla(unsigned int pmc_pllar, unsigned int timeout)
{
	write_pmc((unsigned int)PMC_PLLAR, pmc_pllar);

	while ((timeout--) && !(read_pmc(PMC_SR) & AT91C_PMC_LOCKA)) ;
	return (timeout) ? 0 : (-1);
}

#ifndef PLLUTMI
int pmc_cfg_pllb(unsigned int pmc_pllbr, unsigned int timeout)
{
	write_pmc(PMC_PLLBR, pmc_pllbr);
	while ((timeout--) && !(read_pmc(PMC_SR) & AT91C_PMC_LOCKB)) ;

	return (timeout) ? 0 : (-1);
}
#else
int pmc_cfg_pllutmi(unsigned int pmc_pllutmi, unsigned int timeout)
{
	return 0;
}
#endif

int pmc_cfg_mck(unsigned int pmc_mckr, unsigned int timeout)
{
	unsigned int tmp;
	unsigned int times;

	/*
	 * Program the PRES field in the PMC_MCKR register,
	 * wait for MCKRDY bit to be set in the PMC_SR register
	 */
	tmp = read_pmc(PMC_MCKR);
#if defined(AT91SAM9X5) || defined(AT91SAM9N12) || defined(SAMA5D3X)
	tmp &= (~AT91C_PMC_ALT_PRES);
	tmp |= (pmc_mckr & AT91C_PMC_ALT_PRES);
#else
	tmp &= (~AT91C_PMC_PRES);
	tmp |= (pmc_mckr & AT91C_PMC_PRES);
#endif
	write_pmc(PMC_MCKR, tmp);

	times = timeout;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MCKRDY)))
		;

	/*
	 * Program the MDIV field in the PMC_MCKR register,
	 * wait for MCKRDY bit to be set in the PMC_SR register
	 */
	tmp = read_pmc(PMC_MCKR);
	tmp &= (~AT91C_PMC_MDIV);
	tmp |= (pmc_mckr & AT91C_PMC_MDIV);
	write_pmc(PMC_MCKR, tmp);

	times = timeout;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MCKRDY)))
		;

	/*
	 * Program the PLLADIV2 field in the PMC_MCKR register,
	 * wait for MCKRDY bit to be set in the PMC_SR register
	 */
	tmp = read_pmc(PMC_MCKR);
	tmp &= (~AT91C_PMC_PLLADIV2);
	tmp |= (pmc_mckr & AT91C_PMC_PLLADIV2);
	write_pmc(PMC_MCKR, tmp);

	times = timeout;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MCKRDY)))
		;

	/*
	 * Program the CSS field in the PMC_MCKR register,
	 * wait for MCKRDY bit to be set in the PMC_SR register
	 */
	tmp = read_pmc(PMC_MCKR);
	tmp &= (~AT91C_PMC_CSS);
	tmp |= (pmc_mckr & AT91C_PMC_CSS);
	write_pmc(PMC_MCKR, tmp);

	times = timeout;
	while ((times--) && (!(read_pmc(PMC_SR) & AT91C_PMC_MCKRDY)))
		;

	return 0;
}

int pmc_cfg_pck(unsigned char x, unsigned int clk_sel, unsigned int prescaler)
{
	write_pmc(PMC_PCKR + x * 4, clk_sel | prescaler);
	write_pmc(PMC_SCER, 1 << (x + 8));
	while (!(read_pmc(PMC_SR) & (1 << (x + 8)))) ;

	return 0;
}

int pmc_enable_periph_clock(unsigned int periph_id)
{
	unsigned int mask = 0x01 << (periph_id % 32);

	if ((periph_id / 32) == 1)
		write_pmc(PMC_PCER1, mask);
	else if ((periph_id / 32) == 0)
		write_pmc(PMC_PCER, mask);
	else
		return -1;

	return 0;
}

void pmc_enable_system_clock(unsigned int clock_id)
{
	 write_pmc(PMC_SCER, clock_id);
}
