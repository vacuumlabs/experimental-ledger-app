import TransportNodeHid from "@ledgerhq/hw-transport-node-hid";
import {
  ENCODING_STRING,
  ENCODING_UINT8,
  ENCODING_UINT16,
  ENCODING_UINT32,
  ENCODING_UINT64,
  ENCODING_DATETIME,
  ENCODING_HEX,
  parseNameString,
  parseContractAccountName,
} from "../src/utils/parse";
import {
  Fio,
  HARDENED,
  GetPublicKeyRequest,
  SignTransactionRequest,
  Transaction,
} from "../src/fio";
import { InvalidDataReason } from "../src/errors/invalidDataReason";
import Interpreter from "../src/interpreter/interpreter";
import trnsfiopubkyTemplate from "../src/tx-templates/trnsfiopubky";

//  expiration: "2021-08-28T12:50:36.686",
//     ref_block_num: "4386",
//     ref_block_prefix: "860116326",
//     context_free_actions: [],
//     actions: [
//       {
//         account: "fio.token",
//         name: "trnsfiopubky",
//         authorization: [
//           {
//             actor: "aftyershcu22",
//             permission: "active",
//           },
//         ],
//         data: {
//           payee_public_key:
//             "FIO8PRe4WRZJj5mkem6qVGKyvNFgPsNnjNN6kPhh6EaCpzCVin5Jj",
//           amount: "20",
//           max_fee: "287454020",
//           tpid: "rewards@wallet",
//           actor: "aftyershcu22",
//         },
//       },

const vals = {
  chain_id: "b20901380af44ef59c5918439a1f9a41d83669020319a80574b804a5f95cbd7e",
  expiration: "2021-08-28T12:50:36.686",
  ref_block_num: "4386",
  ref_block_prefix: "860116326",
  cf_act_amt: "0",
  act_amt: "1",
  "iterations#actions": {
    allowed_iter_hashes: [
      "b59afff68ef56bd5abb3f74d6047559aebcc1a1189803c57eb988b76a92f90e5",
    ],
    trnsfiopubky: {
      "expected_length#1": "127",
      contract_account_name: parseContractAccountName(
        "b20901380af44ef59c5918439a1f9a41d83669020319a80574b804a5f95cbd7e",
        "fio.token",
        "trnsfiopubky",
        InvalidDataReason.ACTION_NOT_SUPPORTED
      ),
      num_auths: "1",
      permission: parseNameString(
        "active",
        InvalidDataReason.INVALID_PERMISSION
      ),
      data_len: "93",
      pk_len: "53",
      pubkey: "FIO8PRe4WRZJj5mkem6qVGKyvNFgPsNnjNN6kPhh6EaCpzCVin5Jj",
      amount: "20",
      max_fee: "287454020",
      actor: parseNameString("aftyershcu22", InvalidDataReason.INVALID_ACTOR),
      tpid_len: "14",
      tpid: "rewards@wallet",
    },
  },
};

const path = [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0];

const fetch = require("node-fetch");
const readline = require("readline");

async function example() {
  console.log("Running FIO examples");
  const transport = await TransportNodeHid.create(5000);
  const appFio = new Fio(transport);

  const interpreter = new Interpreter(appFio, path);
  await interpreter.interpret(trnsfiopubkyTemplate, vals);
}

example();
