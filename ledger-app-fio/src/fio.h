#ifndef H_FIO_APP_FIO
#define H_FIO_APP_FIO

#include "common.h"

typedef enum  {
	NETWORK_MAINNET = 0,
	NETWORK_TESTNET = 1,
	NETWORK_UNKNOWN = 2,
} network_type_t;

#define CHAIN_ID_LENGTH 32
network_type_t getNetworkByChainId(uint8_t *chainId, size_t length);

typedef enum {
	ACTION_TYPE_UNKNOWN = 0,
	ACTION_TYPE_TRNSFIOPUBKY = 1,
} action_type_t;

#define CONTRACT_ACCOUNT_NAME_LENGTH 16
action_type_t getActionTypeByContractAccountName(network_type_t network, uint8_t * contractAccountName, size_t length);

//name compressed to 8 bytes, uncompresed up to 13 bytes, last byte for 0
#define NAME_VAR_LENGTH 8
typedef uint64_t name_t;
#define NAME_STRING_MAX_LENGTH 14
void name_to_string(name_t value, char *out, size_t size);
void uint8array_name_to_string(uint8_t *value, size_t valueSize, char *out, size_t outSize);

#define MAX_PUB_KEY_LENGTH 61
#define MAX_TPID_LENGTH 21

#define MAX_SINGLE_BYTE_LENGTH 127

#define MAX_BODY_LENGTH 200

#define MAX_NESTED_COUNTED_SECTIONS 8
#define NO_REGISTER 200

#define MAX_FOR_DEPTH 5

#define MAX_NUM_ALLOWED_ITER_HASHES 6

#endif // H_FIO_APP_FIO
