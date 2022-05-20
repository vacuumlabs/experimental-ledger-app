import {
  ENCODING_STRING,
  ENCODING_UINT8,
  ENCODING_UINT16,
  ENCODING_UINT32,
  ENCODING_UINT64,
  ENCODING_DATETIME,
  ENCODING_HEX,
} from "../utils/parse";

const trnsfiopubkyTemplate = {
  instructions: [
    {
      name: "INIT_HASH",
      params: {},
    },
    {
      name: "SEND_DATA",
      params: {
        header: "chain_id",
        encoding: ENCODING_HEX,
        body_len: 32,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "expiration",
        encoding: ENCODING_DATETIME,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "ref_block_num",
        encoding: ENCODING_UINT16,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "ref_block_prefix",
        encoding: ENCODING_UINT32,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "mx_net_words",
        value: "0",
        encoding: ENCODING_UINT8,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "max_cpu_ms",
        value: "0",
        encoding: ENCODING_UINT8,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "delay_sec",
        value: "0",
        encoding: ENCODING_UINT8,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "cf_act_amt",
        encoding: ENCODING_UINT8,
      },
    },
    {
      name: "SEND_DATA",
      params: {
        header: "act_amt",
        encoding: ENCODING_UINT8,
      },
    },
    {
      name: "FOR",
      id: "actions",
      params: {
        min_iterations: 1,
        max_iterations: 10,
      },
      iterations: [
        {
          name: "trnsfiopubky",
          instructions: [
            {
              name: "START_COUNTED_SECTION",
              id: "1",
              params: {},
            },
            {
              name: "SEND_DATA",
              params: {
                header: "contract_account_name",
                encoding: ENCODING_HEX,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "num_auths",
                encoding: ENCODING_UINT8,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "actor",
                encoding: ENCODING_HEX,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "permission",
                encoding: ENCODING_HEX,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "data_len",
                encoding: ENCODING_UINT8,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "pk_len",
                encoding: ENCODING_UINT8,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "pubkey",
                encoding: ENCODING_STRING,
                display: true,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "amount",
                encoding: ENCODING_UINT64,
                display: true,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "max_fee",
                encoding: ENCODING_UINT64,
                display: true,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "actor",
                encoding: ENCODING_HEX,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "tpid_len",
                encoding: ENCODING_UINT8,
              },
            },
            {
              name: "SEND_DATA",
              params: {
                header: "tpid",
                encoding: ENCODING_STRING,
              },
            },
            {
              name: "END_COUNTED_SECTION",
            },
          ],
        },
      ],
    },
    {
      name: "END_HASH",
      params: {},
    },
  ],
};

export default trnsfiopubkyTemplate;
