#ifndef H_HANDLERS

#include <os_io_seproxyhal.h>
#include <stdlib.h>

#include "handlers.h"
#include "getVersion.h"
#include "getSerial.h"
#include "getPublicKey.h"
#include "signTransaction.h"
#include "runTests.h"

// The APDU protocol uses a single-byte instruction code (INS) to specify
// which command should be executed. We'll use this code to dispatch on a
// table of function pointers.
handler_fn_t* lookupHandler(uint8_t ins)
{
	switch (ins) {
#	define  CASE(INS, HANDLER) case INS: return HANDLER;
		// 0x0* -  app status calls
		CASE(0x00, getVersion_handleAPDU);
		CASE(0x01, getSerial_handleAPDU);

		// 0x1* -  public-key related
		CASE(0x10, getPublicKey_handleAPDU);

		// 0x2* -  transaction related
		// CASE(0x20, signTransaction_handleAPDU);

		// 0x3* - init Hash
		CASE(0x30, signTransaction_handleAPDU); // TODO change this to signTransaction_initHash_handleAPDU or something when possible

		#ifdef DEVEL
		// 0xF* -  debug_mode related
		CASE(0xF0, handleRunTests);
		//   0xF1  reserved for INS_SET_HEADLESS_INTERACTION
		#endif // DEVEL
#	undef   CASE
	default:
		return NULL;
	}
}

#endif
