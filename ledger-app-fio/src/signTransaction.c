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
		0x7a, 0xa5, 0x20, 0x07, 0x0f, 0xec, 0x40, 0x97,
		0xe8, 0x0c, 0x2e, 0x66, 0x41, 0xa7, 0x7c, 0x77,
		0x25, 0xba, 0xb4, 0x4d, 0x38, 0x55, 0x95, 0x19,
		0xef, 0x35, 0x12, 0xfc, 0x39, 0x1a, 0xfd, 0xf5
	},
	{
		0x4c, 0xb8, 0x4d, 0x9a, 0x2f, 0x9b, 0x0b, 0x67,
		0x47, 0x79, 0x44, 0xe9, 0xbb, 0x05, 0xa0, 0x60,
		0xd3, 0x14, 0xfc, 0xe4, 0x34, 0x45, 0xe0, 0xcd,
		0xcf, 0xc8, 0x1d, 0x74, 0x6d, 0x19, 0x4c, 0xf2
	},
	{
		0x52, 0x91, 0x2f, 0xc9, 0x49, 0x6b, 0xeb, 0xcf,
		0x7c, 0x60, 0x7a, 0xed, 0x2a, 0xf9, 0xca, 0xdb,
		0x7d, 0x9c, 0x5e, 0x10, 0xbc, 0xb6, 0xe4, 0xf1,
		0xe1, 0x3d, 0xc3, 0xf9, 0x19, 0x51, 0xb3, 0x32
	},
	{
		0xbf, 0xc4, 0xb0, 0x84, 0x9b, 0x45, 0xdc, 0x29,
		0x81, 0x5e, 0xa0, 0xc9, 0xa9, 0xb6, 0x04, 0xbc,
		0x98, 0x9c, 0xa3, 0xe7, 0xd5, 0xe9, 0xad, 0x72,
		0x4f, 0x31, 0x85, 0x91, 0x55, 0x8f, 0x35, 0xc4
	},
	{
		0xaa, 0x9b, 0x8e, 0xa1, 0x1e, 0xea, 0xe7, 0x65,
		0x22, 0xed, 0xeb, 0x92, 0xeb, 0xc4, 0x11, 0xf5,
		0xc7, 0x48, 0xea, 0xea, 0xc8, 0xd3, 0x5b, 0x34,
		0x7c, 0x3a, 0xf1, 0xeb, 0xb5, 0xfb, 0x2f, 0xe2
	},
	{
		0x55, 0x41, 0x27, 0x29, 0xfd, 0xc0, 0xa0, 0x63,
		0x05, 0x6a, 0x12, 0xc3, 0x62, 0x10, 0xeb, 0x53,
		0x28, 0x1b, 0xf0, 0x0d, 0xb6, 0x75, 0x00, 0x39,
		0x97, 0xb9, 0xbb, 0x31, 0x04, 0x8b, 0x77, 0xd2
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
};

static void signTx_handleInit_ui_runStep()
{
	// TRACE("UI step handleInit");
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleInitAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
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

		sha_256_init(&ctx->txHashContext);
		sha_256_append(&ctx->txHashContext, wireData->chainId, SIZEOF(wireData->chainId));

		uint8_t constants[] = {0x30, 0x01, SIZEOF(wireData->chainId)};

		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));

		ctx->network = getNetworkByChainId(wireData->chainId, SIZEOF(wireData->chainId));

		ctx->currRegisterIdx = NO_REGISTER;
	}

	signTx_handleInit_ui_runStep();
}

// ========================= INIT ACTION ============================

static void signTx_handleInitAction_ui_runStep()
{
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleInitActionAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		struct {
			uint8_t registerIdx[1];
			uint8_t actionLength[9]; // 0-terminated
		}* wireData = (void*) wireDataBuffer;

		VALIDATE(SIZEOF(*wireData) == wireDataSize, ERR_INVALID_DATA);
		
		VALIDATE(wireData->registerIdx[0] != NO_REGISTER, ERR_INVALID_DATA);
		VALIDATE(0 <= wireData->registerIdx[0] && wireData->registerIdx[0] < NUM_REGISTERS, ERR_INVALID_DATA);
		VALIDATE(ctx->registers[wireData->registerIdx[0]] == 0, ERR_INVALID_DATA);

		ctx->currRegisterIdx = wireData->registerIdx[0];

		uint64_t parsedActionLength = (uint32_t)u8be_read(wireData->actionLength);
		ctx->registers[ctx->currRegisterIdx] = parsedActionLength;

		uint8_t constants[] = {0x30, 0x09};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));
		sha_256_append(&ctx->integrityHashContext, wireData->actionLength, SIZEOF(wireData->actionLength));
	}

	signTx_handleInitAction_ui_runStep();
}

// ========================= END ACTION ============================

static void signTx_handleEndAction_ui_runStep()
{
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleEndActionAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		struct {
			uint8_t registerIdx[1];
		}* wireData = (void*) wireDataBuffer;

		VALIDATE(SIZEOF(*wireData) == wireDataSize, ERR_INVALID_DATA);

		VALIDATE(wireData->registerIdx[0] == ctx->currRegisterIdx, ERR_INVALID_DATA);
		VALIDATE(ctx->currRegisterIdx != NO_REGISTER, ERR_ASSERT);
		VALIDATE(0 <= ctx->currRegisterIdx && ctx->currRegisterIdx < NUM_REGISTERS, ERR_INVALID_DATA);
		VALIDATE(ctx->registers[ctx->currRegisterIdx] == 0, ERR_ASSERT);

		uint8_t constants[] = {0x30, 0x0a};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));

		ctx->currRegisterIdx = NO_REGISTER;
	}

	signTx_handleEndAction_ui_runStep();
}

// ========================== SEND DATA =============================

enum {
	HANDLE_SEND_DATA_DISPLAY = 500,
	HANDLE_SEND_DATA_RESPOND,
	HANDLE_SEND_DATA_INVALID
};

static void signTx_handleSendData_ui_runStep()
{
	ui_callback_fn_t* this_fn = signTx_handleSendData_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step, this_fn);

	UI_STEP(HANDLE_SEND_DATA_DISPLAY) {
		if(ctx->encoding == ENCODING_STRING) {
			ui_displayPaginatedText(ctx->headerBuf, ctx->bodyBuf, this_fn);
		} else if(ENCODING_UINT8 <= ctx->encoding && ctx->encoding <= ENCODING_UINT64) {
			ui_displayUint64Screen(ctx->headerBuf, ctx->uint64Body, this_fn);
		} else if(ctx->encoding == ENCODING_HEX) {
			ui_displayHexBufferScreen(ctx->headerBuf, ctx->bodyBuf, sizeof(ctx->bodyBuf) , this_fn); // TODO test this
		} else {
			THROW(ERR_NOT_IMPLEMENTED);
		}
	}

	UI_STEP(HANDLE_SEND_DATA_RESPOND) {
		respondSuccessEmptyMsg();
	}

	UI_STEP_END(HANDLE_SEND_DATA_INVALID);
}

// wireDataBuffer format:
// SIZE_B 			MEANING
// -------------------------
// 1 				register_index -- decrement this register by body_length
// 1 				encoding
// 1 				header_length
// header_length	header
// 1				0 (trailing 0)
// 1 				body_length
// body_length 		body
// 1				0 (trailing 0)
__noinline_due_to_stack__
void signTx_handleSendDataAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	struct {
		uint8_t registerIdx[1];
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

	VALIDATE(wireData1->registerIdx[0] == ctx->currRegisterIdx, ERR_INVALID_DATA);

	if(ctx->currRegisterIdx == NO_REGISTER) {
		uint8_t noRegisterBuf[] = {0};
		sha_256_append(&ctx->integrityHashContext, noRegisterBuf, SIZEOF(noRegisterBuf));
	} else {
		uint8_t registerExistsBuf[] = {1};
		sha_256_append(&ctx->integrityHashContext, registerExistsBuf, SIZEOF(registerExistsBuf));
		VALIDATE(0 <= ctx->currRegisterIdx && ctx->currRegisterIdx < NUM_REGISTERS, ERR_INVALID_DATA);
		VALIDATE(wireData2->bodyLength[0] <= ctx->registers[ctx->currRegisterIdx], ERR_INVALID_DATA);
		ctx->registers[ctx->currRegisterIdx] -= wireData2->bodyLength[0];
		VALIDATE(ctx->registers[ctx->currRegisterIdx] >= 0, ERR_INVALID_DATA);
	}

	ctx->headerBuf = (char*)wireData1->header;
	ctx->encoding = u1be_read(wireData1->encoding);
	ctx->bodyBuf = (char*)wireData2->body;

	if(ctx->encoding == ENCODING_STRING) {
		str_validateNullTerminatedTextBuffer(wireData2->body, wireData2->bodyLength[0]);
	}

	VALIDATE(ctx->encoding >= ENCODING_STRING && ctx->encoding <= ENCODING_HEX, ERR_INVALID_DATA);

	// Set correct body and add to tx
	// Always add numbers to ctx->uint64Body for displaying it
	switch(ctx->encoding) {
		case ENCODING_UINT8:
			ctx->uint8Body = u1be_read(wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint8Body;
			sha_256_append(&ctx->txHashContext, &ctx->uint8Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT16:
			ctx->uint16Body = u2be_read(wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint16Body;
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint16Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT32:
			ctx->uint32Body = u4be_read(wireData2->body);
			ctx->uint64Body = (uint64_t)ctx->uint32Body;
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint32Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_UINT64:
			ctx->uint64Body = u8be_read(wireData2->body);
			sha_256_append(&ctx->txHashContext, (uint8_t*)&ctx->uint64Body, wireData2->bodyLength[0]);
			break;
		case ENCODING_HEX:
		case ENCODING_STRING:
			sha_256_append(&ctx->txHashContext, ctx->bodyBuf, wireData2->bodyLength[0]);
			break;
		default:
			THROW(ERR_NOT_IMPLEMENTED);
			break;
	}

	{
		// Add to integrity hash
		uint8_t constants[] = {0x30, 0x07, p2, ctx->encoding, wireData2->bodyLength[0]};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));
		sha_256_append(&ctx->integrityHashContext, ctx->headerBuf, wireData1->headerLength[0]);
	}

	security_policy_t policy = policyForSendData(p2);
	ENSURE_NOT_DENIED(policy);

	{
		// select UI steps
		switch (policy) {
#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
			CASE(POLICY_PROMPT_BEFORE_RESPONSE, HANDLE_SEND_DATA_DISPLAY);
			CASE(POLICY_ALLOW_WITHOUT_PROMPT, HANDLE_SEND_DATA_RESPOND);
#	undef   CASE
		default:
			THROW(ERR_NOT_IMPLEMENTED);
		}
	}

	signTx_handleSendData_ui_runStep();
}

// ============================ START FOR ===============================
static void signTx_handleStartFor_ui_runStep()
{
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleStartForAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		struct {
			uint8_t numIterations[1];
			uint8_t allowedIterationHashesHash[32];
		}* wireData = (void*) wireDataBuffer;

		// TRACE("wireDataSize: %d", wireDataSize);
		// TRACE("sizeof wireData: %d", SIZEOF(*wireData));
		// VALIDATE(SIZEOF(*wireData) == wireDataSize, ERR_INVALID_DATA);
		VALIDATE(ctx->forLevel < MAX_FOR_DEPTH, ERR_TOO_MANY_NESTED_FOR_BLOCKS);

		uint8_t constants[] = {0x30, 0x0b};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));

		sha_256_finalize(&ctx->integrityHashContext, ctx->intHashes[ctx->forLevel], 32);
		sha_256_init(&ctx->integrityHashContext);

		ctx->forLevel++;

		// TODO this is probably not a correct way to copy an array
		// This will be used at the end of each iteration to check whether the received list is still the same
		// &ctx->allowedIterationHashesHash[ctx->forLevel] = (uint8_t*)wireData->allowedIterationHashesHash;
		memcpy(ctx->allowedIterationHashesHash[ctx->forLevel], wireData->allowedIterationHashesHash, 32);

		TRACE("START FOR: allowedIterationHashesHash: ");
		TRACE_BUFFER(&ctx->allowedIterationHashesHash, 32);

		// Will be decreased in each iteration
		ctx->forRegisters[ctx->forLevel] = wireData->numIterations[0];
	}

	signTx_handleStartFor_ui_runStep();
}

// ============================== END FOR =================================
static void signTx_handleEndFor_ui_runStep()
{
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleEndForAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		// Is there a for to end? 
		VALIDATE(ctx->forLevel > 0, ERR_CANT_END_FOR);
		VALIDATE(ctx->forRegisters[ctx->forLevel] == 0, ERR_CANT_END_FOR);

		uint8_t constants[] = {0x30, 0x0c};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));
		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(&ctx->integrityHashContext, ctx->intHashes[ctx->forLevel], 32);
		sha_256_append(&ctx->integrityHashContext, ctx->allowedIterationHashesHash[ctx->forLevel], 32);
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));

		explicit_bzero(ctx->allowedIterationHashesHash, 32);
		explicit_bzero(ctx->intHashes[ctx->forLevel], 32);

		ctx->forLevel--;
	}

	signTx_handleEndFor_ui_runStep();
}

// ============================ START ITERATION ===============================
static void signTx_handleStartIteration_ui_runStep()
{
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleStartIterationAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		VALIDATE(ctx->forLevel > 0, ERR_NOT_IN_FOR);
		VALIDATE(ctx->forRegisters[ctx->forLevel] > 0, ERR_UNEXPECTED_INS);

		ctx->forRegisters[ctx->forLevel]--;

		// An empty hash is already started from endIteration or startFor
		uint8_t constants[] = {0x30, 0x0d};
		sha_256_append(&ctx->integrityHashContext, ctx->intHashes[ctx->forLevel], 32);
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));
	}

	signTx_handleStartIteration_ui_runStep();
}

// ============================ END ITERATION ===============================
static void signTx_handleEndIteration_ui_runStep()
{
	respondSuccessEmptyMsg();
}

__noinline_due_to_stack__
void signTx_handleEndIterationAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	{
		struct {
			uint8_t numAllowedHashes[1];
			uint8_t allowedIterationHashes[32 * MAX_NUM_ALLOWED_ITER_HASHES];
		}* wireData = (void*) wireDataBuffer;

		// VALIDATE(SIZEOF(*wireData) == wireDataSize, ERR_INVALID_DATA);
		VALIDATE(wireData->numAllowedHashes[0] <= MAX_NUM_ALLOWED_ITER_HASHES, ERR_INVALID_DATA);
		wireData->allowedIterationHashes[wireData->numAllowedHashes[0] * 32] = 0; // zero terminate them
		TRACE("Allowed iteration hashes: ");
		TRACE_BUFFER(wireData->allowedIterationHashes, 32 * MAX_NUM_ALLOWED_ITER_HASHES);

		uint8_t numCharsInAllowedHashes = 0;
		for(uint8_t i = 0; i < wireData->numAllowedHashes[0] * 32 && wireData->allowedIterationHashes[i]; i++) {
			numCharsInAllowedHashes++;
		}

		TRACE("numCharsInAllowedHashes: %d", numCharsInAllowedHashes);

		uint8_t constants[] = {0x30, 0x0e};
		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));

		// Finalize iteration integrity hash
		uint8_t tmpHashBuf[32];
		sha_256_finalize(&ctx->integrityHashContext, tmpHashBuf, 32);

		TRACE("Iteration integrity hash:");
		TRACE_BUFFER(tmpHashBuf, SIZEOF(tmpHashBuf));

		// Check whether iteration integrity hash is from the list of allowed iteration hashes
		uint8_t iterationIsValid = 0;
		for(uint8_t i = 0; i < wireData->numAllowedHashes[0]; i++) {
			uint8_t matches = 1;
			for(uint8_t j = 0; j < 32; j++) {
				if(wireData->allowedIterationHashes[i * 32 + j] != tmpHashBuf[j]) {
					matches = 0; // Current hash is not matching our iteration integrity hash
					break;
				}
				if(matches) {
					iterationIsValid = 1;
					break;
				}
			}
		}
		VALIDATE(iterationIsValid, ERR_INVALID_ITERATION);

		// Check whether the list of allowed iteration hashes is the same as the one from startFor
		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(
			&ctx->integrityHashContext,
			wireData->allowedIterationHashes,
			wireData->numAllowedHashes[0] * 32
		);
		sha_256_finalize(&ctx->integrityHashContext, tmpHashBuf, 32);
		TRACE("AllowedIterationHashesHash hashed by C:");
		TRACE_BUFFER(tmpHashBuf, 32);
		TRACE("ctx->allowedIterationHashesHash[ctx->forLevel]: ");
		TRACE_BUFFER(ctx->allowedIterationHashesHash[ctx->forLevel], 32);
		uint8_t allowedHashesListValid = 1;
		for(uint8_t i = 0; i < 32; i++) {
			if(tmpHashBuf[i] != ctx->allowedIterationHashesHash[ctx->forLevel][i]) {
				allowedHashesListValid = 0;
				break;
			}
		}
		VALIDATE(allowedHashesListValid, ERR_INVALID_DATA);

		// Number of remaining iterations is decreased in startFor, not here

		sha_256_init(&ctx->integrityHashContext);
	}

	signTx_handleEndIteration_ui_runStep();
}

// =============================== END ==================================

enum {
	HANDLE_END_STEP_CONFIRM = 400,
	HANDLE_END_STEP_RESPOND,
	HANDLE_END_STEP_HASH_NOT_ALLOWED,
} ;


static void signTx_handleEnd_ui_runStep()
{
	ui_callback_fn_t* this_fn = signTx_handleEnd_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step, this_fn);

	UI_STEP(HANDLE_END_STEP_CONFIRM) {
		ui_displayPrompt(
		        "Sign",
		        "transaction?",
		        this_fn,
		        respond_with_user_reject
		);
	}

	UI_STEP(HANDLE_END_STEP_RESPOND) {
		io_send_buf(SUCCESS, G_io_apdu_buffer, 65 + 32);
		ui_displayBusy(); // needs to happen after I/O
		ui_idle();
	}

	UI_STEP_END(HANDLE_INIT_STEP_INVALID);
}

__noinline_due_to_stack__
void signTx_handleEndAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	{
		// sanity checks
		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
	}

	explicit_bzero(&ctx->wittnessPath, SIZEOF(ctx->wittnessPath));
	{
		// parse
		size_t parsedSize = bip44_parseFromWire(&ctx->wittnessPath, wireDataBuffer, wireDataSize);
		VALIDATE(parsedSize == wireDataSize, ERR_INVALID_DATA);
	}

	{
		uint8_t constants[] = {0x30, 0x06, wireDataSize};

		sha_256_append(&ctx->integrityHashContext, constants, SIZEOF(constants));

		//Extension points
		uint8_t epBuf[1];
		explicit_bzero(epBuf, SIZEOF(epBuf));
		sha_256_append(&ctx->txHashContext, epBuf, SIZEOF(epBuf));

		//We finish the tx hash appending a 32-byte empty buffer
		uint8_t txHashBuf[32];
		explicit_bzero(txHashBuf, SIZEOF(txHashBuf));
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
					ctx->ui_step = HANDLE_END_STEP_CONFIRM;
					break;
			}
		}

		if(ctx->ui_step == HANDLE_END_STEP_HASH_NOT_ALLOWED) {
			TRACE("Hash NOT ALLOWED");
			THROW(ERR_HASH_NOT_ALLOWED);
		}
		else {
			TRACE("Hash ALLOWED");
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
		CASE(0x06, signTx_handleEndAPDU);
		CASE(0x07, signTx_handleSendDataAPDU);
		CASE(0x09, signTx_handleInitActionAPDU);
		CASE(0x0a, signTx_handleEndActionAPDU);
		CASE(0x0b, signTx_handleStartForAPDU);
		CASE(0x0c, signTx_handleEndForAPDU);
		CASE(0x0d, signTx_handleStartIterationAPDU);
		CASE(0x0e, signTx_handleEndIterationAPDU);
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

