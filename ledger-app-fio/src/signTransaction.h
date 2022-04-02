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

	uint8_t prevHash[32]; // Finalized hash from the previous instruction

	int ui_step;

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

	uint8_t forIterationsCnt[MAX_FOR_DEPTH + 1]; // Current number of for iterations
	uint8_t forMinIterations[MAX_FOR_DEPTH + 1]; // Min allowed number of iterations
	uint8_t forMaxIterations[MAX_FOR_DEPTH + 1]; // Max allowed number of iterations
	uint8_t forLevel;							 // Depth of the current for (nested), 0 if we are not inside a for

	uint8_t intHashes[MAX_FOR_DEPTH][32]; // List of integrity hashes of fors with lower level
	uint8_t allowedIterationHashesHash[MAX_FOR_DEPTH][32];

	uint8_t storageBuffer[64];
	// uint8_t storageBuffer[1300];

	bip44_path_t wittnessPath;
	public_key_t wittnessPathPubkey;

} ins_sign_transaction_context_t;

#endif // H_FIO_APP_SIGN_TRANSACTION
