/*	$NetBSD: longjmp.c,v 1.2 2008/04/28 20:22:57 martin Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christian Limpach.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "namespace.h"
#include <sys/types.h>
#include <ucontext.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define __LIBC12_SOURCE__
#include <setjmp.h>
#include <compat/include/setjmp.h>

typedef struct {
	__greg_t	__glob[5];
} __jmp_buf_regs_t;

void
__longjmp14(jmp_buf env, int val)
{
	struct sigcontext *sc = (void *)env;
	__jmp_buf_regs_t *r = (void *)&sc[1];
	ucontext_t uc;

	/* Ensure non-zero SP */
	if (sc->sc_sp == 0)
		goto err;

	/*
	 * Set _UC_CPU. No FPU data saved, so we can't restore
	 * that. Set _UC_{SET,CLR}STACK according to SS_ONSTACK
	 */
	memset(&uc, 0, sizeof(uc));
	uc.uc_flags = _UC_CPU | (sc->sc_onstack ? _UC_SETSTACK : _UC_CLRSTACK);

	/*
	 * Set the signal mask - this is a weak symbol, so don't use
	 * _UC_SIGMASK in the mcontext, libpthread might override sigprocmask.
	 */
	sigprocmask(SIG_SETMASK, &sc->sc_mask, NULL);

	/* Fill other registers */
	uc.uc_mcontext.__gregs[_REG_CCR] = sc->sc_tstate;
	uc.uc_mcontext.__gregs[_REG_PC] = sc->sc_pc;
	uc.uc_mcontext.__gregs[_REG_nPC] = sc->sc_npc;
	uc.uc_mcontext.__gregs[_REG_G1] = sc->sc_g1;
	uc.uc_mcontext.__gregs[_REG_G2] = sc->sc_o0;
	uc.uc_mcontext.__gregs[_REG_G3] = r->__glob[0];
	uc.uc_mcontext.__gregs[_REG_G4] = r->__glob[1];
	uc.uc_mcontext.__gregs[_REG_G5] = r->__glob[2];
	uc.uc_mcontext.__gregs[_REG_G6] = r->__glob[3];
	uc.uc_mcontext.__gregs[_REG_G7] = r->__glob[4];
	uc.uc_mcontext.__gregs[_REG_O6] = sc->sc_sp;

	/* Make return value non-zero */
	if (val == 0)
		val = 1;

	/* Save return value in context */
	uc.uc_mcontext.__gregs[_REG_O0] = val;

	setcontext(&uc);

 err:
	longjmperror();
	abort();
	/* NOTREACHED */
}
