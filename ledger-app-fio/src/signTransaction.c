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

const uint8_t ALLOWED_HASHES[][32] = {
	{
    	0x37, 0x64, 0xd1, 0x17, 0xae, 0xd0, 0xcf, 0xe6,
    	0x40, 0xbd, 0x7c, 0xe4, 0x18, 0x1a, 0x8c, 0xe7,
    	0xf2, 0x7a, 0xea, 0xa3, 0x21, 0x7f, 0x13, 0x67,
    	0xa3, 0x06, 0x07, 0x37, 0x41, 0xb6, 0x90, 0x93
	},
	{
    	0xc5, 0x32, 0xde, 0x89, 0xd3, 0xca, 0x7c, 0x31,
    	0xcc, 0x25, 0x64, 0x25, 0x8a, 0x86, 0x68, 0x42,
    	0xac, 0xcd, 0xf2, 0x9c, 0xe2, 0x54, 0xc6, 0x2b,
    	0xba, 0x64, 0x44, 0xe2, 0x8c, 0x0d, 0xdc, 0x91
	},
	{
    	0x1e, 0x20, 0x37, 0x29, 0x0b, 0xe0, 0xff, 0xaf,
    	0x22, 0x85, 0xfd, 0xeb, 0x5b, 0x58, 0x92, 0xc5,
    	0x00, 0x25, 0x5a, 0x11, 0xac, 0x82, 0x9a, 0x66,
    	0x12, 0xa3, 0x76, 0xd7, 0x61, 0x9f, 0x61, 0xbe
	},
	{
    	0xfb, 0xe4, 0x03, 0xff, 0xcf, 0x99, 0x3b, 0xe3,
    	0xcc, 0xec, 0x10, 0xb6, 0x38, 0xe4, 0xa1, 0xd1,
    	0xbd, 0x55, 0xd1, 0xc5, 0x17, 0x75, 0xae, 0x5c,
    	0x05, 0xf6, 0xd3, 0x86, 0xf8, 0xc1, 0x58, 0x7e
	}
};
const uint8_t NUM_ALLOWED_HASHES = SIZEOF(ALLOWED_HASHES) / SIZEOF(ALLOWED_HASHES[0]);

// ============================== INIT ==============================

enum {
	HANDLE_INIT_STEP_DISPLAY_DETAILS = 100,
	HANDLE_INIT_STEP_RESPOND,
	HANDLE_INIT_STEP_INVALID,
} ;

static void signTx_handleInit_ui_runStep()
{
	// TRACE("UI step handleInit");
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleInitAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	// TRACE_STACK_USAGE();
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

		// TRACE("SHA_256_init");
		sha_256_init(&ctx->hashContext);
		sha_256_append(&ctx->hashContext, wireData->chainId, SIZEOF(wireData->chainId));

		uint8_t ins_code[2] = {0x30, 0x01};

		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(&ctx->integrityHashContext, ins_code, SIZEOF(ins_code));

		ctx->network = getNetworkByChainId(wireData->chainId, SIZEOF(wireData->chainId));
		// TRACE("Network %d:", ctx->network);
	}

	signTx_handleInit_ui_runStep();
}

// ========================== SEND DATA NO DISPLAY =============================

static void signTx_handleSendDataNoDisplay_ui_runStep()
{
	// TRACE("UI step handleSendDataNoDisplay");
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleSendDataNoDisplayAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	// TRACE_STACK_USAGE();
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		// Add to integrity hash
		uint8_t ins_code[2] = {0x30, 0x07};
		sha_256_append(&ctx->integrityHashContext, ins_code, 2);

		// TODO create format, add encoding to integrity hash
		// TODO add constant to integrity hash

		// Add to tx hash
		sha_256_append(&ctx->hashContext, wireDataBuffer, wireDataSize);
	}

	signTx_handleSendDataNoDisplay_ui_runStep();
}

// ========================== SEND DATA DISPLAY =============================

enum {
	HANDLE_SEND_DATA_DISPLAY_DETAILS = 500,
	HANDLE_SEND_DATA_RESPOND,
	HANDLE_SEND_DATA_INVALID
};

static void signTx_handleSendDataDisplay_ui_runStep()
{
	TRACE("UI step %d", ctx->ui_step);
	TRACE_STACK_USAGE();
	ui_callback_fn_t* this_fn = signTx_handleSendDataDisplay_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step, this_fn);

	UI_STEP(HANDLE_SEND_DATA_DISPLAY_DETAILS) {
		ui_displayPaginatedText(ctx->textToDisplayBuf, ctx->valueBuf, this_fn);
	}

	UI_STEP(HANDLE_SEND_DATA_RESPOND) {
		respondSuccessEmptyMsg();
	}

	UI_STEP_END(HANDLE_SEND_DATA_INVALID);
}

__noinline_due_to_stack__
void signTx_handleSendDataDisplayAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE_STACK_USAGE();
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
		ASSERT(wireDataSize >= 3); // At least text_to_display_len, data_len, encoding have to be present
	}

	TRACE("Parse buffer: %d", wireDataBuffer[0]);

	// Parse buffer
	// explicit_bzero(ctx->textToDisplayBuf, NAME_STRING_MAX_LENGTH);
	uint8_t text_to_display_len = wireDataBuffer[0];
	ASSERT(text_to_display_len < NAME_STRING_MAX_LENGTH);
	for(uint8_t i = 0; i < text_to_display_len; i++) {
		ctx->textToDisplayBuf[i] = wireDataBuffer[1 + i]; // Offset by 1 byte containg text_to_display_len
	}
	ctx->textToDisplayBuf[text_to_display_len] = '\n';
	TRACE("headerBuf length before: %d", strlen(ctx->textToDisplayBuf));

	// explicit_bzero(ctx->valueBuf, 200);
	uint8_t value_len = wireDataBuffer[1 + text_to_display_len];
	ASSERT(value_len < 200);
	TRACE("Value len: %d", value_len);
	for(uint8_t i = 0; i < value_len; i++) {
		ctx->valueBuf[i] = wireDataBuffer[1 + text_to_display_len + 1 + i];
	}
	ctx->valueBuf[value_len] = '\0';
	TRACE("valueBuf length before: %d", strlen(ctx->valueBuf));

	uint8_t encoding[1] = {wireDataBuffer[1 + text_to_display_len + 1 + value_len]};
	ASSERT(encoding[0] <= 1);

	{
		// Add to integrity hash
		uint8_t ins_code[2] = {0x30, 0x08};
		sha_256_append(&ctx->integrityHashContext, ins_code, SIZEOF(ins_code));
		sha_256_append(&ctx->integrityHashContext, ctx->textToDisplayBuf, text_to_display_len);
		sha_256_append(&ctx->integrityHashContext, encoding, SIZEOF(encoding));

		// Add to tx hash
		sha_256_append(&ctx->hashContext, ctx->valueBuf, value_len);
	}

	ctx->ui_step = HANDLE_SEND_DATA_DISPLAY_DETAILS;

	security_policy_t policy = POLICY_DENY;
	{
		// get policy
		policy = policyForSendDataDisplay();
		TRACE("Policy: %d", (int) policy);
		ENSURE_NOT_DENIED(policy);
	}

	signTx_handleSendDataDisplay_ui_runStep();
}


// =============================== END ==================================

enum {
	HANDLE_END_STEP_SUCCESS = 400,
	HANDLE_END_STEP_HASH_NOT_ALLOWED,
} ;


static void signTx_handleEnd_ui_runStep()
{
	// TRACE("UI step %d", ctx->ui_step);
	// TRACE_STACK_USAGE();
	ui_callback_fn_t* this_fn = signTx_handleEnd_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step, this_fn);

	UI_STEP(HANDLE_END_STEP_SUCCESS) {
		io_send_buf(SUCCESS, G_io_apdu_buffer, 32);
		ui_displayBusy(); // needs to happen after I/O
		ui_idle();
	}

	UI_STEP_END(HANDLE_INIT_STEP_INVALID);
}

__noinline_due_to_stack__
void signTx_handleEndAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	// TRACE_STACK_USAGE();
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		uint8_t ins_code[2] = {0x30, 0x06};

		sha_256_append(&ctx->integrityHashContext, ins_code, SIZEOF(ins_code));

		//We finish the tx hash appending a 32-byte empty buffer
		uint8_t txHashBuf[32];
		explicit_bzero(txHashBuf, SIZEOF(txHashBuf));
		sha_256_append(&ctx->hashContext, txHashBuf, SIZEOF(txHashBuf));

		//we get the resulting tx hash
		sha_256_finalize(&ctx->hashContext, txHashBuf, SIZEOF(txHashBuf));
		// TRACE("SHA_256_finalize, resulting tx hash:");
		// TRACE_BUFFER(txHashBuf, 32);

		// We finish the integrity hash appending a 32-byte empty buffer
		uint8_t integrityHashBuf[32]; // TODO only use 1 buffer for both tx and integrity hash
		explicit_bzero(integrityHashBuf, SIZEOF(integrityHashBuf));
		sha_256_append(&ctx->integrityHashContext, integrityHashBuf, SIZEOF(integrityHashBuf));
		// TRACE("SHA_256_finalize, resulting integrity hash:");
		// TRACE_BUFFER(integrityHashBuf, 32);

		// We save the tx hash into APDU buffer to return it
		memcpy(G_io_apdu_buffer, txHashBuf, SIZEOF(txHashBuf));

		// We get the resulting integrity hash and check whether it is in the list of allowed hashes (TODO)
		sha_256_finalize(&ctx->integrityHashContext, integrityHashBuf, SIZEOF(integrityHashBuf));

		TRACE("Integrity hash:");
		TRACE_BUFFER(integrityHashBuf, SIZEOF(integrityHashBuf));

		// Check if integrity hash is in the list of allowed hashes
		ctx->ui_step = HANDLE_END_STEP_HASH_NOT_ALLOWED;
		for(uint8_t i = 0; i < NUM_ALLOWED_HASHES; i++) {
			if(memcmp(integrityHashBuf, ALLOWED_HASHES[i], 32) == 0) {
					ctx->ui_step = HANDLE_END_STEP_SUCCESS;
					break;
			}
		}

		if(ctx->ui_step == HANDLE_END_STEP_HASH_NOT_ALLOWED) {
			TRACE("Hash NOT ALLOWED!!!");
		}
		else {
			TRACE("Hash ALLOWED :)))");
		}

	}

	signTx_handleEnd_ui_runStep();
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
		CASE(0x08, signTx_handleSendDataDisplayAPDU);
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
