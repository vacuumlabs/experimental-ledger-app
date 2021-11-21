#ifndef H_FIO_APP_SIGN_TRANSACTION
#define H_FIO_APP_SIGN_TRANSACTION

#include "handlers.h"
#include "hash.h"
#include "fio.h"
#include "keyDerivation.h"

handler_fn_t signTransaction_handleAPDU;

typedef struct {
	sha_256_context_t hashContext;
	sha_256_context_t integrityHashContext;

	network_type_t network;
	char actionValidationActor[NAME_STRING_MAX_LENGTH];

	int ui_step;

	//The following data is not needed at once.
	//to be used in HEADER step
	uint32_t expiration;
	uint16_t refBlockNum;
	uint32_t refBlockPrefix;

	//only used in ACTION HEADER step
	action_type_t action_type;

	//only used in ACTION_AUTHORIZATION step
	char actionValidationPermission[NAME_STRING_MAX_LENGTH];

	//only used in ACTION_DATA step
	char *pubkey;
	uint64_t amount;
	uint64_t maxFee;
	char actionDataActor[NAME_STRING_MAX_LENGTH];
	char *tpid;

	//only used in WITNESS step
	bip44_path_t wittnessPath;
	public_key_t wittnessPathPubkey;

	char* textToDisplayBuf;
	char* valueBuf;
	uint8_t encoding;

} ins_sign_transaction_context_t;

#endif // H_FIO_APP_SIGN_TRANSACTION
