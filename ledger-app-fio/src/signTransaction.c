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
		0x7a, 0x9e, 0xa7, 0xd9, 0x1a, 0x63, 0xe0, 0xdf,
		0xd9, 0x23, 0xce, 0xee, 0x7f, 0xd2, 0x0f, 0x5d,
		0xf4, 0x44, 0x61, 0x32, 0xfa, 0xc0, 0x09, 0xe1,
		0x07, 0x24, 0xb7, 0x2d, 0x5e, 0xac, 0x79, 0x8a
	}
};
const uint8_t NUM_ALLOWED_HASHES = SIZEOF(ALLOWED_HASHES) / SIZEOF(ALLOWED_HASHES[0]);

enum {
	ENCODING_STRING = 150,
	ENCODING_UINT8,
	ENCODING_UINT16,
	ENCODING_UINT32,
	ENCODING_UINT64,
	ENCODING_HEX
};

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
		sha_256_init(&ctx->txHashContext);
		TRACE("APPEND CHAIN ID");
		sha_256_append(&ctx->txHashContext, wireData->chainId, SIZEOF(wireData->chainId));

		uint8_t ins_code[2] = {0x30, 0x01};

		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(&ctx->integrityHashContext, ins_code, SIZEOF(ins_code));

		ctx->network = getNetworkByChainId(wireData->chainId, SIZEOF(wireData->chainId));
		// TRACE("Network %d:", ctx->network);
	}

	signTx_handleInit_ui_runStep();
}

// ========================== SEND DATA NO DISPLAY =============================

enum {
	HANDLE_SEND_DATA_NO_DISPLAY_RESPOND = 300
};

static void signTx_handleSendDataNoDisplay_ui_runStep()
{
	// TRACE("UI step handleSendDataNoDisplay");
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleSendDataNoDisplayAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	struct {
		uint8_t encoding[1];
		uint8_t headerLength[1];
		uint8_t header[NAME_STRING_MAX_LENGTH];
	}* wireData1 = (void*) wireDataBuffer;
	const uint8_t expectedWireData1Length = SIZEOF(*wireData1) - NAME_STRING_MAX_LENGTH + wireData1->headerLength[0] + 1;

	struct {
		uint8_t bodyLength[1];
		uint8_t body[MAX_BODY_LENGTH];
	}* wireData2 = ((void*) wireDataBuffer) + expectedWireData1Length;
	const uint8_t expectedWireData2Length = SIZEOF(*wireData2) - MAX_BODY_LENGTH + wireData2->bodyLength[0] + 1;

	TRACE_BUFFER(wireDataBuffer, wireDataSize);
	TRACE_BUFFER(wireData1, expectedWireData1Length);
	TRACE_BUFFER(wireData2, expectedWireData2Length);

	TRACE("expected length: %d, actual length: %d", expectedWireData1Length + expectedWireData2Length, wireDataSize);

	VALIDATE(expectedWireData1Length + expectedWireData2Length == wireDataSize, ERR_INVALID_DATA);
	str_validateNullTerminatedTextBuffer(wireData1->header, wireData1->headerLength[0]);

	ctx->headerBuf = (char*)wireData1->header;
	ctx->encoding = u1be_read(wireData1->encoding);
	ctx->bodyBuf = (char*)wireData2->body;

	if(ctx->encoding == ENCODING_STRING) {
		str_validateNullTerminatedTextBuffer(wireData2->body, wireData2->bodyLength[0]);
	}

	TRACE("encoding: %d, ENCODING_STRING: %d, ENCODING_HEX: %d", ctx->encoding, ENCODING_STRING, ENCODING_HEX);
	VALIDATE(ctx->encoding >= ENCODING_STRING && ctx->encoding <= ENCODING_HEX, ERR_INVALID_DATA);

	// Set correct body and add to tx
	// Always add numbers to ctx->uint64Body for displaying it
	TRACE("APPEND IN SEND DATA NO DISPLAY");
	switch(ctx->encoding) {
		case ENCODING_UINT8:
			ctx->uint8Body = u1be_read((char*)wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint8Body;
			sha_256_append(&ctx->txHashContext, &ctx->uint8Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT16:
			ctx->uint16Body = u2be_read((char*)wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint16Body;
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint16Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT32:
			ctx->uint32Body = u4be_read((char*)wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint32Body;
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint32Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT64:
			ctx->uint64Body = u8be_read((char*)wireData2->body);
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint64Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_HEX:
		case ENCODING_STRING:
			sha_256_append(&ctx->txHashContext, ctx->bodyBuf, wireData2->bodyLength[0]);
			break;
		default:
			ASSERT(1 < 0);
			break;
	}

	{
		// Add to integrity hash
		uint8_t constants[] = {0x30, 0x07, ctx->encoding};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));
		sha_256_append(&ctx->integrityHashContext, ctx->headerBuf, wireData1->headerLength[0]);
	}

	security_policy_t policy = policyForSendDataNoDisplay();
	TRACE("Policy: %d", (int) policy);
	ENSURE_NOT_DENIED(policy);
	{
		// select UI steps
		switch (policy) {
#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
			CASE(POLICY_ALLOW_WITHOUT_PROMPT, HANDLE_SEND_DATA_NO_DISPLAY_RESPOND);
#	undef   CASE
		default:
			THROW(ERR_NOT_IMPLEMENTED);
		}
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
		if(ctx->encoding == ENCODING_STRING) {
			ui_displayPaginatedText(ctx->headerBuf, ctx->bodyBuf, this_fn);
		} else {
			ui_displayUint64Screen(ctx->headerBuf, ctx->uint64Body, this_fn);
		}
	}

	UI_STEP(HANDLE_SEND_DATA_RESPOND) {
		respondSuccessEmptyMsg();
	}

	UI_STEP_END(HANDLE_SEND_DATA_INVALID);
}

__noinline_due_to_stack__
void signTx_handleSendDataDisplayAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	struct {
		uint8_t encoding[1];
		uint8_t headerLength[1];
		uint8_t header[NAME_STRING_MAX_LENGTH];
	}* wireData1 = (void*) wireDataBuffer;
	const uint8_t expectedWireData1Length = SIZEOF(*wireData1) - NAME_STRING_MAX_LENGTH + wireData1->headerLength[0] + 1;

	struct {
		uint8_t bodyLength[1];
		uint8_t body[MAX_BODY_LENGTH];
	}* wireData2 = ((void*) wireDataBuffer) + expectedWireData1Length;
	const uint8_t expectedWireData2Length = SIZEOF(*wireData2) - MAX_BODY_LENGTH + wireData2->bodyLength[0] + 1;

	VALIDATE(expectedWireData1Length + expectedWireData2Length == wireDataSize, ERR_INVALID_DATA);
	str_validateNullTerminatedTextBuffer(wireData1->header, wireData1->headerLength[0]);

	ctx->headerBuf = (char*)wireData1->header;
	ctx->encoding = u1be_read(wireData1->encoding);
	ctx->bodyBuf = (char*)wireData2->body;

	if(ctx->encoding == ENCODING_STRING) {
		str_validateNullTerminatedTextBuffer(wireData2->body, wireData2->bodyLength[0]);
	}

	VALIDATE(ctx->encoding >= ENCODING_STRING && ctx->encoding <= ENCODING_HEX, ERR_INVALID_DATA);

	// Set correct body and add to tx
	// Always add numbers to ctx->uint64Body for displaying it
	TRACE("APPEND IN SEND_DATA_DISPLAY");
	switch(ctx->encoding) {
		case ENCODING_UINT8:
			ctx->uint8Body = u1be_read((char*)wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint8Body;
			sha_256_append(&ctx->txHashContext, &ctx->uint8Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT16:
			ctx->uint16Body = u2be_read((char*)wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint16Body;
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint16Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT32:
			ctx->uint32Body = u4be_read((char*)wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint32Body;
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint32Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT64:
			ctx->uint64Body = u8be_read((char*)wireData2->body);
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint64Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_HEX:
		case ENCODING_STRING:
			sha_256_append(&ctx->txHashContext, ctx->bodyBuf, wireData2->bodyLength[0]);
			break;
		default:
			ASSERT(1 < 0);
			break;
	}

	{
		// Add to integrity hash
		uint8_t constants[] = {0x30, 0x08, ctx->encoding};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));
		sha_256_append(&ctx->integrityHashContext, ctx->headerBuf, wireData1->headerLength[0]);
	}

	security_policy_t policy = policyForSendDataDisplay();
	TRACE("Policy: %d", (int) policy);
	ENSURE_NOT_DENIED(policy);
	{
		// select UI steps
		switch (policy) {
#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
			CASE(POLICY_PROMPT_BEFORE_RESPONSE, HANDLE_SEND_DATA_DISPLAY_DETAILS);
#	undef   CASE
		default:
			THROW(ERR_NOT_IMPLEMENTED);
		}
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
	ui_callback_fn_t* this_fn = signTx_handleEnd_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step, this_fn);

	// TODO: ask for confirmation
	UI_STEP(HANDLE_END_STEP_SUCCESS) {
		io_send_buf(SUCCESS, G_io_apdu_buffer, 65 + 32);
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

	explicit_bzero(&ctx->wittnessPath, SIZEOF(ctx->wittnessPath));
	{
		// parse
		TRACE_BUFFER(wireDataBuffer, wireDataSize);
		size_t parsedSize = bip44_parseFromWire(&ctx->wittnessPath, wireDataBuffer, wireDataSize);
		VALIDATE(parsedSize == wireDataSize, ERR_INVALID_DATA);
	}

	{
		uint8_t ins_code[2] = {0x30, 0x06};

		sha_256_append(&ctx->integrityHashContext, ins_code, SIZEOF(ins_code));

		//Extension points
		uint8_t epBuf[1];
		explicit_bzero(epBuf, SIZEOF(epBuf));
		TRACE("APPEND EXTENSION POINTS");
		sha_256_append(&ctx->txHashContext, epBuf, SIZEOF(epBuf));

		//We finish the tx hash appending a 32-byte empty buffer
		uint8_t txHashBuf[32];
		explicit_bzero(txHashBuf, SIZEOF(txHashBuf));
		TRACE("APPEND 32 zero bytes to finish");
		sha_256_append(&ctx->txHashContext, txHashBuf, SIZEOF(txHashBuf));

		//we get the resulting tx hash
		sha_256_finalize(&ctx->txHashContext, txHashBuf, SIZEOF(txHashBuf));

		// We finish the integrity hash appending a 32-byte empty buffer
		uint8_t integrityHashBuf[32]; // TODO only use 1 buffer for both tx and integrity hash
		explicit_bzero(integrityHashBuf, SIZEOF(integrityHashBuf));
		sha_256_append(&ctx->integrityHashContext, integrityHashBuf, SIZEOF(integrityHashBuf));

		// We get the resulting integrity hash and check whether it is in the list of allowed hashes
		sha_256_finalize(&ctx->integrityHashContext, integrityHashBuf, SIZEOF(integrityHashBuf));

		// // We save the tx hash into APDU buffer to return it
		memcpy(G_io_apdu_buffer, txHashBuf, SIZEOF(txHashBuf));

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

		//We derive the private key
		private_key_t privateKey;
		derivePrivateKey(&ctx->wittnessPath, &privateKey);
		TRACE("privateKey.d:");
		TRACE_BUFFER(privateKey.d, privateKey.d_len);

		//We want to show pubkey, thus we derive it
		derivePublicKey(&ctx->wittnessPath, &ctx->wittnessPathPubkey);
		TRACE_BUFFER(ctx->wittnessPathPubkey.W, SIZEOF(ctx->wittnessPathPubkey.W));

		//We sign the hash
		//Code producing signatures is taken from EOS app
		uint32_t tx = 0;
		uint8_t V[33];
		uint8_t K[32];
		int tries = 0;

		// Loop until a candidate matching the canonical signature is found
		// Taken from EOS app
		// We use G_io_apdu_buffer to save memory (and also to minimize changes to EOS code)
		// The code produces the signature right where we need it for the respons
		BEGIN_TRY {
			TRY {
				explicit_bzero(G_io_apdu_buffer, SIZEOF(G_io_apdu_buffer));
				for (;;)
				{
					if (tries == 0) {
						rng_rfc6979(G_io_apdu_buffer + 100, txHashBuf, privateKey.d, privateKey.d_len, SECP256K1_N, 32, V, K);
					} else {
						rng_rfc6979(G_io_apdu_buffer + 100, txHashBuf, NULL, 0, SECP256K1_N, 32, V, K);
					}
					uint32_t infos;
					tx = cx_ecdsa_sign(&privateKey, CX_NO_CANONICAL | CX_RND_PROVIDED | CX_LAST, CX_SHA256,
									txHashBuf, 32,
									G_io_apdu_buffer + 100, 100,
									&infos);
					TRACE_BUFFER(G_io_apdu_buffer + 100, 100);

					if ((infos & CX_ECCINFO_PARITY_ODD) != 0) {
						G_io_apdu_buffer[100] |= 0x01;
					}
					G_io_apdu_buffer[0] = 27 + 4 + (G_io_apdu_buffer[100] & 0x01);
					ecdsa_der_to_sig(G_io_apdu_buffer + 100, G_io_apdu_buffer + 1);
					TRACE_BUFFER(G_io_apdu_buffer, 65);

					if (check_canonical(G_io_apdu_buffer + 1)) {
						tx = 1 + 64;
						break;
					} else {
						TRACE("Try %d unsuccesfull! We will not get correct signature!!!!!!!!!!!!!!!!!!!!!!!!!", tries);
						tries++;
					}
				}
			}
			FINALLY {
				memset(&privateKey, 0, sizeof(privateKey));
			}
		}
		END_TRY;

		//We add hash to the response
		TRACE("ecdsa_der_to_sig_result:");
		TRACE_BUFFER(G_io_apdu_buffer, 65);
		memcpy(G_io_apdu_buffer + 65, txHashBuf, 32);
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

