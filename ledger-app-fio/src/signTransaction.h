#ifndef H_FIO_APP_SIGN_TRANSACTION
#define H_FIO_APP_SIGN_TRANSACTION

#include "handlers.h"
#include "hash.h"
#include "fio.h"
#include "keyDerivation.h"

handler_fn_t signTransaction_handleAPDU;

enum
{
	DISPLAY = 1,
	DONT_DISPLAY
};

typedef struct
{
	sha_256_context_t txHashContext;
	sha_256_context_t integrityHashContext;

	network_type_t network;
	char actionValidationActor[NAME_STRING_MAX_LENGTH];

	int ui_step;

	// The following data is not needed at once.
	// to be used in HEADER step
	uint32_t expiration;
	uint16_t refBlockNum;
	uint32_t refBlockPrefix;

	// only used in ACTION HEADER step
	action_type_t action_type;

	// only used in ACTION_AUTHORIZATION steps
	char actionValidationPermission[NAME_STRING_MAX_LENGTH];

	// only used in ACTION_DATA step
	char *pubkey;
	uint64_t amount;
	uint64_t maxFee;
	char actionDataActor[NAME_STRING_MAX_LENGTH];
	char *tpid;

	// only used in WITNESS step
	bip44_path_t wittnessPath;
	public_key_t wittnessPathPubkey;

	char *headerBuf;
	char *bodyBuf;
	uint8_t uint8Body;
	uint16_t uint16Body;
	uint32_t uint32Body;
	uint64_t uint64Body;
	uint8_t encoding;

	uint8_t sectionLevel; // The current nesting level of counted sections, 0 means no counted section
	uint64_t expectedSectionLength[MAX_NESTED_COUNTED_SECTIONS + 1];
	uint64_t currSectionLength[MAX_NESTED_COUNTED_SECTIONS + 1]; // Will be increased by SEND_DATA

	uint8_t forIterationsCnt[MAX_FOR_DEPTH + 1]; // Current number for iterations
	uint8_t forMinIterations[MAX_FOR_DEPTH + 1]; // Min allowed number of iterations
	uint8_t forMaxIterations[MAX_FOR_DEPTH + 1]; // Max allowed number of iterations

	uint8_t forLevel;					  // Depth of the current for (nested), 0 if we are not inside a for
	uint8_t intHashes[MAX_FOR_DEPTH][32]; // List of integrity hashes of fors with lower level
	uint8_t allowedIterationHashesHash[MAX_FOR_DEPTH][32];

	uint8_t storageBuffer[64];

} ins_sign_transaction_context_t;

#endif // H_FIO_APP_SIGN_TRANSACTION
