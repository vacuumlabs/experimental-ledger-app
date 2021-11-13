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

		uint8_t ins_code[1];
		ins_code[0] = 0x30;

		sha_256_init(&ctx->integrityHashContext);
		sha_256_append(&ctx->integrityHashContext, ins_code, 1); // TODO change this buffer, it is ugly

		ctx->network = getNetworkByChainId(wireData->chainId, SIZEOF(wireData->chainId));
		TRACE("Network %d:", ctx->network);
	}

	signTx_handleInit_ui_runStep();
}


// // ============================== WITNESS ==============================

// enum {
// 	HANDLE_WITNESS_STEP_DISPLAY_DETAILS = 1000,
// 	HANDLE_WITNESS_STEP_CONFIRM,
// 	HANDLE_WITNESS_STEP_RESPOND,
// 	HANDLE_WITNESS_STEP_INVALID,
// } ;

// static void signTx_handleWitness_ui_runStep()
// {
// 	TRACE("UI step %d", ctx->ui_step);
// 	TRACE_STACK_USAGE();
// 	ui_callback_fn_t* this_fn = signTx_handleWitness_ui_runStep;

// 	UI_STEP_BEGIN(ctx->ui_step, this_fn);

// 	UI_STEP(HANDLE_WITNESS_STEP_DISPLAY_DETAILS) {
// 		ui_displayPubkeyScreen("Sign with", &ctx->wittnessPathPubkey, this_fn);
// 	}

// 	UI_STEP(HANDLE_WITNESS_STEP_CONFIRM) {
// 		ui_displayPrompt(
// 		        "Sign",
// 		        "transaction?",
// 		        this_fn,
// 		        respond_with_user_reject
// 		);
// 	}

// 	UI_STEP(HANDLE_WITNESS_STEP_RESPOND) {
// 		io_send_buf(SUCCESS, G_io_apdu_buffer, 65 + 32);
// 		ui_displayBusy(); // needs to happen after I/O
// 		advanceStage();
// 	}

// 	UI_STEP_END(HANDLE_WITNESS_STEP_INVALID);
// }

// __noinline_due_to_stack__
// void signTx_handleWitnessAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
// {
// 	TRACE_STACK_USAGE();
// 	{
// 		// sanity checks
// 		CHECK_STAGE(SIGN_STAGE_WITNESS);
// 		VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
// 		ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);
// 	}

// 	explicit_bzero(&ctx->wittnessPath, SIZEOF(ctx->wittnessPath));

// 	{
// 		// parse
// 		TRACE_BUFFER(wireDataBuffer, wireDataSize);

// 		size_t parsedSize = bip44_parseFromWire(&ctx->wittnessPath, wireDataBuffer, wireDataSize);
// 		VALIDATE(parsedSize == wireDataSize, ERR_INVALID_DATA);
// 	}

// 	security_policy_t policy = POLICY_DENY;
// 	{
// 		// get policy
// 		policy = policyForSignTxWitness(&ctx->wittnessPath);
// 		TRACE("Policy: %d", (int) policy);
// 		ENSURE_NOT_DENIED(policy);
// 	}

// 	//Extension points
// 	uint8_t buf[1];
// 	explicit_bzero(buf, SIZEOF(buf));
// 	sha_256_append(&ctx->hashContext, buf, SIZEOF(buf));

// 	//We finish the hash appending a 32-byte empty buffer
// 	uint8_t hashBuf[32];
// 	explicit_bzero(hashBuf, SIZEOF(hashBuf));
// 	sha_256_append(&ctx->hashContext, hashBuf, SIZEOF(hashBuf));

// 	//we get the resulting hash
// 	sha_256_finalize(&ctx->hashContext, hashBuf, SIZEOF(hashBuf));
// 	TRACE("SHA_256_finalize, resulting hash:");
// 	TRACE_BUFFER(hashBuf, 32);

// 	//We derive the private key
// 	private_key_t privateKey;
// 	derivePrivateKey(&ctx->wittnessPath, &privateKey);
// 	TRACE("privateKey.d:");
// 	TRACE_BUFFER(privateKey.d, privateKey.d_len);

// 	//We want to show pubkey, thus we derive it
// 	derivePublicKey(&ctx->wittnessPath, &ctx->wittnessPathPubkey);
// 	TRACE_BUFFER(ctx->wittnessPathPubkey.W, SIZEOF(ctx->wittnessPathPubkey.W));

// 	//We sign the hash
// 	//Code producing signatures is taken from EOS app
// 	uint32_t tx = 0;
// 	uint8_t V[33];
// 	uint8_t K[32];
// 	int tries = 0;

// 	// Loop until a candidate matching the canonical signature is found
// 	// Taken from EOS app
// 	// We use G_io_apdu_buffer to save memory (and also to minimize changes to EOS code)
// 	// The code produces the signature right where we need it for the respons
// 	BEGIN_TRY {
// 		TRY {
// 			explicit_bzero(G_io_apdu_buffer, SIZEOF(G_io_apdu_buffer));
// 			for (;;)
// 			{
// 				if (tries == 0) {
// 					rng_rfc6979(G_io_apdu_buffer + 100, hashBuf, privateKey.d, privateKey.d_len, SECP256K1_N, 32, V, K);
// 				} else {
// 					rng_rfc6979(G_io_apdu_buffer + 100, hashBuf, NULL, 0, SECP256K1_N, 32, V, K);
// 				}
// 				uint32_t infos;
// 				tx = cx_ecdsa_sign(&privateKey, CX_NO_CANONICAL | CX_RND_PROVIDED | CX_LAST, CX_SHA256,
// 				                   hashBuf, 32,
// 				                   G_io_apdu_buffer + 100, 100,
// 				                   &infos);
// 				TRACE_BUFFER(G_io_apdu_buffer + 100, 100);

// 				if ((infos & CX_ECCINFO_PARITY_ODD) != 0) {
// 					G_io_apdu_buffer[100] |= 0x01;
// 				}
// 				G_io_apdu_buffer[0] = 27 + 4 + (G_io_apdu_buffer[100] & 0x01);
// 				ecdsa_der_to_sig(G_io_apdu_buffer + 100, G_io_apdu_buffer + 1);
// 				TRACE_BUFFER(G_io_apdu_buffer, 65);

// 				if (check_canonical(G_io_apdu_buffer + 1)) {
// 					tx = 1 + 64;
// 					break;
// 				} else {
// 					TRACE("Try %d unsuccesfull! We will not get correct signature!!!!!!!!!!!!!!!!!!!!!!!!!", tries);
// 					tries++;
// 				}
// 			}
// 		}
// 		FINALLY {
// 			memset(&privateKey, 0, sizeof(privateKey));
// 		}
// 	}
// 	END_TRY;

// 	//We add hash to the response
// 	TRACE("ecdsa_der_to_sig_result:");
// 	TRACE_BUFFER(G_io_apdu_buffer, 65);
// 	memcpy(G_io_apdu_buffer + 65, hashBuf, 32);

// 	{
// 		// select UI steps
// 		switch (policy) {
// #	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
// 			CASE(POLICY_PROMPT_BEFORE_RESPONSE, HANDLE_WITNESS_STEP_DISPLAY_DETAILS);
// #	undef   CASE
// 		default:
// 			THROW(ERR_NOT_IMPLEMENTED);
// 		}
// 	}

// 	signTx_handleWitness_ui_runStep();
// }


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
