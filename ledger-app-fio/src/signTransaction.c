#include "common.h"
#include "handlers.h"

#include "getSerial.h"
#include "state.h"
#include "hash.h"
#include "endian.h"
#include "eos_utils.h"
#include "securityPolicy.h"
#include "uiHelpers.h"
#include "uiScreens.h"
#include "textUtils.h"

static ins_sign_transaction_context_t* ctx = &(instructionState.signTransactionContext);


void respondSuccessEmptyMsg()
{
	TRACE();
	io_send_buf(SUCCESS, NULL, 0);
	ui_displayBusy(); // needs to happen after I/O
}

//Taken from EOS app. Needed to produce signatures.
uint8_t const SECP256K1_N[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
                               0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b,
                               0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41
                              };

// ============================== INIT ==============================

enum {
	HANDLE_INIT_STEP_DISPLAY_DETAILS = 100,
	HANDLE_INIT_STEP_RESPOND,
	HANDLE_INIT_STEP_INVALID,
} ;

static void signTx_handleInit_ui_runStep()
{
	TRACE("UI step handleInit");
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleInitAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE_STACK_USAGE();
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		struct {
			uint8_t chainId[32];
		}* wireData = (void*) wireDataBuffer;

		VALIDATE(SIZEOF(*wireData) == wireDataSize, ERR_INVALID_DATA);

		TRACE("SHA_256_init");
		sha_256_init(&ctx->hashContext);
		sha_256_append(&ctx->hashContext, wireData->chainId, SIZEOF(wireData->chainId));

		uint8_t ins_code[2] = {0x30, 0x01}; // TODO also add p1 to hash
		// ins_code[0] = 0x30;
		// ins_code[1] = 0x01;

		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(&ctx->integrityHashContext, ins_code, 1); // TODO change this buffer, it is ugly

		ctx->network = getNetworkByChainId(wireData->chainId, SIZEOF(wireData->chainId));
		TRACE("Network %d:", ctx->network);
	}

	signTx_handleInit_ui_runStep();
}

// =============================== END ==================================

static void signTx_handleEnd_ui_runStep()
{
	TRACE("UI step handleEnd");
	io_send_buf(SUCCESS, G_io_apdu_buffer, 32);
	ui_displayBusy(); // needs to happen after I/O
}

__noinline_due_to_stack__
void signTx_handleEndAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE_STACK_USAGE();
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		// VALIDATE(SIZEOF(*wireData) == wireDataSize, ERR_INVALID_DATA);


		uint8_t ins_code[2] = {0x30, 0x06};
		// ins_code[0] = 0x30;
		// ins_code[1] = 0x06;

		sha_256_append(&ctx->integrityHashContext, ins_code, 2); // TODO change this buffer, it is ugly

		//We finish the tx hash appending a 32-byte empty buffer
		uint8_t hashBuf[32];
		explicit_bzero(hashBuf, SIZEOF(hashBuf));
		sha_256_append(&ctx->hashContext, hashBuf, SIZEOF(hashBuf));

		//we get the resulting tx hash
		sha_256_finalize(&ctx->hashContext, hashBuf, SIZEOF(hashBuf));
		TRACE("SHA_256_finalize, resulting tx hash:");
		TRACE_BUFFER(hashBuf, 32);

		// We finish the integrity hash appending a 32-byte empty buffer
		uint8_t integrityHashBuf[32]; // TODO only use 1 buffer for both tx and integrity hash
		explicit_bzero(integrityHashBuf, SIZEOF(integrityHashBuf));
		sha_256_append(&ctx->integrityHashContext, integrityHashBuf, SIZEOF(integrityHashBuf));
		// TRACE("SHA_256_finalize, resulting integrity hash:");
		// TRACE_BUFFER(integrityHashBuf, 32);

		// We save the tx hash into APDU buffer to return it
		memcpy(G_io_apdu_buffer, hashBuf, SIZEOF(hashBuf));

		// We get the resulting integrity hash and check whether it is in the list of allowed hashes (TODO)
		sha_256_finalize(&ctx->integrityHashContext, integrityHashBuf, SIZEOF(integrityHashBuf));

		// TODO: check if integrityHash is in allowed hashes (integrity hash is in integrityHashBuf)
	}

	signTx_handleEnd_ui_runStep();
}

// ========================== SEND DATA NO DISPLAY =============================

static void signTx_handleSendDataNoDisplay_ui_runStep()
{
	TRACE("UI step handleSendDataNoDisplay");
	// respondSuccessEmptyMsg();
	io_send_buf(SUCCESS, NULL, 0);
}

__noinline_due_to_stack__
void signTx_handleSendDataNoDisplayAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE_STACK_USAGE();
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		// Add to integrity hash
		uint8_t ins_code[2] = {0x30, 0x07};
		// ins_code[0] = 0x30;
		// ins_code[1] = 0x07;
		sha_256_append(&ctx->integrityHashContext, ins_code, 2); // TODO change this buffer, it is ugly

		// Add to tx hash
		sha_256_append(&ctx->hashContext, wireDataBuffer, wireDataSize);
	}

	signTx_handleSendDataNoDisplay_ui_runStep();
}

// ============================== MAIN HANDLER ==============================

typedef void subhandler_fn_t(uint8_t p2, uint8_t* dataBuffer, size_t dataSize);

static subhandler_fn_t* lookup_subhandler(uint8_t p1)
{
	switch(p1) {
#	define  CASE(P1, HANDLER) case P1: return HANDLER;
#	define  DEFAULT(HANDLER)  default: return HANDLER;
		CASE(0x01, signTx_handleInitAPDU);
		// CASE(0x02, signTx_handleHeaderAPDU);
		// CASE(0x03, signTx_handleActionHeaderAPDU);
		// CASE(0x04, signTx_handleActionAuthorizationAPDU);
		// CASE(0x05, signTx_handleActionDataAPDU);
		// CASE(0x10, signTx_handleWitnessAPDU);
		CASE(0x06, signTx_handleEndAPDU);
		CASE(0x07, signTx_handleSendDataNoDisplayAPDU);
		DEFAULT(NULL)
#	undef   CASE
#	undef   DEFAULT
	}
}

void signTransaction_handleAPDU(
        uint8_t p1,
        uint8_t p2,
        uint8_t* wireDataBuffer,
        size_t wireDataSize,
        bool isNewCall
)
{
	TRACE("P1 = 0x%x, P2 = 0x%x, isNewCall = %d", p1, p2, isNewCall);

	if (isNewCall) {
		explicit_bzero(ctx, SIZEOF(*ctx));
	}

	subhandler_fn_t* subhandler = lookup_subhandler(p1);
	VALIDATE(subhandler != NULL, ERR_INVALID_REQUEST_PARAMETERS);
	subhandler(p2, wireDataBuffer, wireDataSize);
}
