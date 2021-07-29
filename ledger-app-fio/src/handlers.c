#ifndef H_HANDLERS

#include <os_io_seproxyhal.h>
#include <stdlib.h>

#include "handlers.h"
#include "getVersion.h"

// The APDU protocol uses a single-byte instruction code (INS) to specify
// which command should be executed. We'll use this code to dispatch on a
// table of function pointers.
handler_fn_t* lookupHandler(uint8_t ins)
{
	switch (ins) {
#	define  CASE(INS, HANDLER) case INS: return HANDLER;
		// 0x0* -  app status calls
		CASE(0x00, getVersion_handleAPDU);

/*		#ifdef DEVEL
		// 0xF* -  debug_mode related
		CASE(0xF0, handleRunTests);
		//   0xF1  reserved for INS_SET_HEADLESS_INTERACTION
		#endif // DEVEL*/
#	undef   CASE
	default:
		return NULL;
	}
}

#endif