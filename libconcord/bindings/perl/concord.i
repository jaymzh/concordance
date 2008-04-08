/*
 *  vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  (C) Copyright Phil Dibowitz 2008
 */

%module concord
%{

/*
 * This is the C callback that will wrap our perl callback and call
 * perl for us. SWIG can't handle this directly, we must do it.
 */
void lc_cb_wrapper(uint32_t count, uint32_t curr, uint32_t total, void *arg)
{
	AV *args = (AV*)arg;
	SV *cb;
	SV *cbdata;
	int i;

	/* get a copy of the stack pointer into SP */
	dSP;

	/* Create boundry for all mortal and temp variables */
	ENTER;
	SAVETMPS;

	/*
	 * Tell Perl to make a note of the current stack pointer, so it
	 * can count how many things we add and knows how big to make
	 * @_.
	 */
	PUSHMARK(SP);

	/*
	 * Push the first three variables into the stack and mortalize
	 * them so that perl will clean them up after the call to cb.
	 */
	XPUSHs(sv_2mortal(newSViv(count)));
	XPUSHs(sv_2mortal(newSViv(curr)));
	XPUSHs(sv_2mortal(newSViv(total)));

	/*
	 * Next up, we recurse through the array in void *arg and pull
	 * the perl function and it's arguments out.
	 */

	/* not really length - highest index */
	if (av_len(args) < 1) {
		SWIG_croak("Less than 2 args passed to lc_cb_wrapper");
	}

	cb = *av_fetch(args, 0, 0);

	for (i = 1; i <= av_len(args); i++) {
		XPUSHs(*av_fetch(args, i, 0));
	}

	/*
	 * Tell it we're done pushing things onto the stack, so it should
	 * now know how big @_ is.
	 */ 
	PUTBACK;

	/* Actually call our function */
	call_sv(cb, G_VOID);

	/* Refresh our copy of the stack pointer after the call */
	SPAGAIN;

	/*
	 * Close the boundry for temp/mortal variables so the GC
	 * can clean them up.
	 */
	FREETMPS;
	LEAVE;

	return;

   fail:
	SWIG_croak_null();

}

#include <libconcord.h>

%}

/*
 * Tell perl how to handle functions expecting an lc_callback. In this case
 * we save the perl coderef to a global and then set the lc_callback arg to
 * our wrapper function which is a lc_callback
 */
%typemap(in) lc_callback %{
	AV* args;
	SV *tmp;

	/*
	 * create a new array, store our perl callback in it. We also
	 * need to increase the refcount when we do this.
	 */
	args = newAV();
	SvREFCNT_inc($input);
	av_store(args, 0, $input);

	/*
	 * FIXME:
	 * 	We SHOULD allow ulimited arguments to our callback
	 * 	and push them onto the array, something like this:
	 *
	 *	for (count = items; count > 0; count--) {
	 *		tmp = POPs;
	 *		SvREFCNT_inc(tmp);
	 *		av_store(args, count - 1, tmp);
	 *	}
	 *
	 *	but, this code will add any function arguments prior
	 * 	to the callback also to the array and you end up trying
	 * 	to call some integer as your call back function, or
	 *	some other similar problem.
	 */

	/* Pass our C callback wrapper as the callback */
	$1 = lc_cb_wrapper;
%}

/*
 * Tell perl how to handle void* - i.e., the cb_arg
 */
%typemap(in) void *cb_arg {
	/*
	 * add the arg to our array. See the typemap for lc_callback
	 * for more details
	 */
	av_store(args, 1, $input);
	/* The array is our callback's argument */
	$1 = (void *)args;
}

/*
 * We need to undef the array when we're done with it...
 */
%typemap(argout) lc_callback %{
	av_undef(args);
%}

/*
 * We want to "ignore" the arguement that expects the pointer to
 * the blob. Actually, here in C, we'll make the pointer and pass
 * in the reference wants to see - the user can't do this in perl,
 * so we don't make them pass in anything. We'll return the address
 * to them as a scalar in the return list on the stack.
 */
%typemap(in, numinputs=0) uint8_t** (uint8_t *blob) {
	$1 = &blob;
}

/*
 * Same here as above - for pointers to ints - i.e. a way to get
 * the size returned to us. In this case we'll just return an int
 * as a scalar in the return list.
 */
%typemap(in, numinputs=0) uint32_t* (uint32_t num) {
	$1 = &num;
}

/*
 * When passing in a pointer to a blob, we just need to grab the
 * unsigned value and cast it.
 */
%typemap(in) uint8_t* {
	$1 = (uint8_t*)SvUV($input);
}

/*
 * When passing in a uint32_t, just grab the number and cast it
 */
%typemap(in) uint32_t {
	$1 = (uint32_t)SvUV($input);
}

/*
 * OK, here's where we take the pointer to the blob and return it
 * to the user. We add it to the stack which is part of the returned
 * list.
 *
 * FIXME: we probably need to add the NEWOBJ stuff here
 *	and the FREEOBJ stuff on delete_blob... maybe?
 */
%typemap(argout) uint8_t** {
	if (argvi >= items) {
		EXTEND(sp,1);
	}
	$result = sv_newmortal();
	sv_setuv($result, (UV) *($1));
	argvi++;
}

/*
 * Here's where we return sizes
 */
%typemap(argout) uint32_t* {
	if (argvi >= items) {
	  EXTEND(sp,1);
	}
	$result = sv_newmortal();
	sv_setuv($result, (UV) (*$1));
	argvi++;
}

%include "../../libconcord.h"
%include "typemaps.i"

