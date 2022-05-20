import { expect } from "chai";
import { log } from "console";
import { type } from "os";
import type { HexString, Uint64_str } from "types/internal";
import { assert } from "utils/assert";

import type { Fio } from "../../src/fio";
import { DeviceStatusError, HARDENED } from "../../src/fio";
import type { Transaction } from "../../src/types/public";
import { hex_to_buf, uint64_to_buf } from "../../src/utils/serialize";
import { getFio } from "../test_utils";
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
} from "../../src/utils/parse";
import { InvalidDataReason } from "../../src/errors/invalidDataReason";
import Interpreter from "../../src/interpreter/interpreter";
import trnsfiopubkyTemplate from "../../src/tx-templates/trnsfiopubky";

// We initialize constants
const path = [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0];
const privateKeyDHex =
  "4d597899db76e87933e7c6841c2d661810f070bad20487ef20eb84e182695a3a" as HexString;
const PrivateKey = require("@fioprotocol/fiojs/dist/ecc/key_private");
const privateKey = PrivateKey(hex_to_buf(privateKeyDHex));
const publicKey = privateKey.toPublic();

const otherPath = [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 1];
const otherPrivateKeyDHex =
  "90835ae980cd10e9ca7df05d0e3b3c22e0aed0e75527511337f7c53a9d0c6c69" as HexString;
const otherPrivateKey = PrivateKey(hex_to_buf(otherPrivateKeyDHex));
const otherPublicKey = otherPrivateKey.toPublic();

const { TextEncoder, TextDecoder } = require("text-encoding");
const fetch = require("node-fetch");
const {
  base64ToBinary,
  arrayToHex,
} = require("@fioprotocol/fiojs/dist/chain-numeric");
const Signature = require("@fioprotocol/fiojs/dist/ecc/signature");
const crypto = require("crypto");
var ser = require("@fioprotocol/fiojs/dist/chain-serialize");
const textDecoder = new TextDecoder();
const textEncoder = new TextEncoder();
const { JsSignatureProvider } = require("@fioprotocol/fiojs/dist/chain-jssig");

// To do this we must perform asynchronous calls, initialized in before method
const httpEndpointTestnet = "http://testnet.fioprotocol.io";
const httpEndpointMainnet = "https://fio.greymass.com";
var networkInfo: any;

// Serializes and signs transaction using fiojs
async function buildTxAndSignatureFioJs(
  network: string,
  tx: Transaction,
  pubkey: any
) {
  // We serialize the transaction
  // Get the addaddress action type
  const actionAddaddress =
    networkInfo[network].typesFioAddress.get("trnsfiopubky");

  // Serialize the actions[] "data" field (This example assumes a single action, though transactions may hold an array of actions.)
  const buffer = new ser.SerialBuffer({ textEncoder, textDecoder });
  actionAddaddress.serialize(buffer, tx.actions[0].data);
  const serializedData = arrayToHex(buffer.asUint8Array());

  // Get the actions parameter from the transaction and replace the data field with the serialized data field
  var serializedAction = tx.actions[0] as any;
  serializedAction = {
    ...serializedAction,
    data: serializedData,
  };

  const rawTransaction = {
    ...tx,
    max_net_usage_words: 0x00,
    max_cpu_usage_ms: 0x00,
    delay_sec: 0x00,
    context_free_actions: [],
    actions: [serializedAction], //Actions have to be an array
    transaction_extensions: [],
  };

  // Get the transaction action type
  const txnaction = networkInfo[network].typesTransaction.get("transaction");

  // Serialize the transaction
  const buffer2 = new ser.SerialBuffer({ textEncoder, textDecoder });
  txnaction.serialize(buffer2, rawTransaction);
  const serializedTransaction = buffer2.asUint8Array();

  //Lets compute hash in using Signature.sign
  const msg = Buffer.concat([
    Buffer.from(networkInfo[network].chainId, "hex"),
    serializedTransaction,
    Buffer.allocUnsafe(32).fill(0),
  ]);
  const hash = crypto.createHash("sha256").update(msg).digest("hex");

  //Now using signatureProvider.sign
  const signatureProvider = new JsSignatureProvider([
    PrivateKey.fromHex(privateKeyDHex).toString(),
    PrivateKey.fromHex(otherPrivateKeyDHex).toString(),
  ]);
  const requiredKeys = [pubkey.toString()];
  const serializedContextFreeData = null;

  const signedTxn = await signatureProvider.sign({
    chainId: networkInfo[network].chainId,
    requiredKeys: requiredKeys,
    serializedTransaction: serializedTransaction,
    serializedContextFreeData: serializedContextFreeData,
  });

  return {
    serializedTx: serializedTransaction,
    fullMsg: msg,
    hash: crypto.createHash("sha256").update(msg).digest("hex"),
    signature: signedTxn.signatures[0],
  };
}

const basicTx: Transaction = {
  expiration: "2021-08-28T12:50:36.686",
  ref_block_num: "4386",
  ref_block_prefix: "860116326",
  context_free_actions: [],
  actions: [
    {
      account: "fio.token",
      name: "trnsfiopubky",
      authorization: [
        {
          actor: "aftyershcu22",
          permission: "active",
        },
      ],
      data: {
        payee_public_key:
          "FIO8PRe4WRZJj5mkem6qVGKyvNFgPsNnjNN6kPhh6EaCpzCVin5Jj",
        amount: "20",
        max_fee: "287454020",
        tpid: "rewards@wallet",
        actor: "aftyershcu22",
      },
    },
  ],
  transaction_extensions: null,
};

describe("signTransaction", async () => {
  let fio: Fio = {} as Fio;

  before(async () => {
    const infoTestnet = await (
      await fetch(httpEndpointTestnet + "/v1/chain/get_info")
    ).json();
    const infoMainnet = await (
      await fetch(httpEndpointMainnet + "/v1/chain/get_info")
    ).json();

    const abiFioAddressTestnet = await (
      await fetch(httpEndpointTestnet + "/v1/chain/get_abi", {
        body: `{"account_name": "fio.token"}`,
        method: "POST",
      })
    ).json();
    const abiMsigTestnet = await (
      await fetch(httpEndpointTestnet + "/v1/chain/get_abi", {
        body: `{"account_name": "eosio.msig"}`,
        method: "POST",
      })
    ).json();
    const abiFioAddressMainnet = await (
      await fetch(httpEndpointMainnet + "/v1/chain/get_abi", {
        body: `{"account_name": "fio.token"}`,
        method: "POST",
      })
    ).json();
    const abiMsigMainnet = await (
      await fetch(httpEndpointMainnet + "/v1/chain/get_abi", {
        body: `{"account_name": "eosio.msig"}`,
        method: "POST",
      })
    ).json();

    // Get a Map of all the types from fio.address
    const typesFioAddressTestnet = ser.getTypesFromAbi(
      ser.createInitialTypes(),
      abiFioAddressTestnet.abi
    );
    const typesTransactionTestnet = ser.getTypesFromAbi(
      ser.createInitialTypes(),
      abiMsigTestnet.abi
    );
    const typesFioAddressMainnet = ser.getTypesFromAbi(
      ser.createInitialTypes(),
      abiFioAddressMainnet.abi
    );
    const typesTransactionMainnet = ser.getTypesFromAbi(
      ser.createInitialTypes(),
      abiMsigMainnet.abi
    );
    networkInfo = {
      TESTNET: {
        chainId: infoTestnet.chain_id,
        typesFioAddress: typesFioAddressTestnet,
        typesTransaction: typesTransactionTestnet,
      },
      MAINNET: {
        chainId: infoMainnet.chain_id,
        typesFioAddress: typesFioAddressMainnet,
        typesTransaction: typesTransactionMainnet,
      },
    };
  });

  beforeEach(async () => {
    fio = await getFio();
  });

  afterEach(async () => {
    await (fio as any).t.close();
  });

  it("Sign testnet transaction", async () => {
    const network = "TESTNET";

    const tx = basicTx;
    // Lets sign the transaction with fiojs
    const { serializedTx, fullMsg, hash, signature } =
      await buildTxAndSignatureFioJs(network, tx, publicKey);

    // Data length
    const SIMPLE_LENGTH_VARIABLE_LENGTH = 1;
    const AMOUNT_TYPE_LENGTH = 8;
    const NAME_VARIABLE_LENGTH = 8;
    let dataLength =
      SIMPLE_LENGTH_VARIABLE_LENGTH +
      basicTx["actions"][0]["data"]["payee_public_key"].length + // pubkey_length, pubkey
      2 * AMOUNT_TYPE_LENGTH +
      NAME_VARIABLE_LENGTH + // amount, max_fee, actor
      +SIMPLE_LENGTH_VARIABLE_LENGTH +
      basicTx["actions"][0]["data"]["tpid"].length; // tpid_length, tpid

    const vals = {
      chain_id: networkInfo[network].chainId,
      expiration: basicTx["expiration"],
      ref_block_num: basicTx["ref_block_num"],
      ref_block_prefix: basicTx["ref_block_prefix"],
      cf_act_amt: "0",
      act_amt: basicTx["actions"].length.toString(),
      "iterations#actions": {
        allowed_iter_hashes: [
          "b59afff68ef56bd5abb3f74d6047559aebcc1a1189803c57eb988b76a92f90e5",
        ],
        trnsfiopubky: {
          "expected_length#1": "127",
          contract_account_name: parseContractAccountName(
            networkInfo[network].chainId,
            basicTx["actions"][0]["account"],
            basicTx["actions"][0]["name"],
            InvalidDataReason.ACTION_NOT_SUPPORTED
          ),
          num_auths: "1",
          permission: parseNameString(
            "active",
            InvalidDataReason.INVALID_PERMISSION
          ),
          data_len: dataLength.toString(),
          pk_len:
            basicTx["actions"][0]["data"]["payee_public_key"].length.toString(),
          pubkey: basicTx["actions"][0]["data"]["payee_public_key"],
          amount: basicTx["actions"][0]["data"]["amount"],
          max_fee: basicTx["actions"][0]["data"]["max_fee"],
          actor: parseNameString(
            basicTx["actions"][0]["data"]["actor"],
            InvalidDataReason.INVALID_ACTOR
          ),
          tpid_len: basicTx["actions"][0]["data"]["tpid"].length.toString(),
          tpid: basicTx["actions"][0]["data"]["tpid"],
        },
      },
    };

    const interpreter = new Interpreter(fio, path);
    const ledgerResponse = await interpreter.interpret(
      trnsfiopubkyTemplate,
      vals
    );

    if (ledgerResponse == undefined) {
      throw new Error("NO_LEDGER_RESPONSE");
    }

    const signatureLedger = Signature.fromHex(
      ledgerResponse.witness.witnessSignatureHex
    );

    expect(ledgerResponse.txHashHex).to.be.equal(hash);
    expect(signatureLedger.verify(fullMsg, publicKey)).to.be.true;
    expect(signatureLedger.verify(fullMsg, otherPublicKey)).to.be.false;
  });

  it("Sign mainnet transaction", async () => {
    const network = "MAINNET";

    const tx = basicTx;
    // Lets sign the transaction with fiojs
    const { serializedTx, fullMsg, hash, signature } =
      await buildTxAndSignatureFioJs(network, tx, publicKey);

    // Data length
    const SIMPLE_LENGTH_VARIABLE_LENGTH = 1;
    const AMOUNT_TYPE_LENGTH = 8;
    const NAME_VARIABLE_LENGTH = 8;
    let dataLength =
      SIMPLE_LENGTH_VARIABLE_LENGTH +
      basicTx["actions"][0]["data"]["payee_public_key"].length + // pubkey_length, pubkey
      2 * AMOUNT_TYPE_LENGTH +
      NAME_VARIABLE_LENGTH + // amount, max_fee, actor
      +SIMPLE_LENGTH_VARIABLE_LENGTH +
      basicTx["actions"][0]["data"]["tpid"].length; // tpid_length, tpid

    const vals = {
      chain_id: networkInfo[network].chainId,
      expiration: basicTx["expiration"],
      ref_block_num: basicTx["ref_block_num"],
      ref_block_prefix: basicTx["ref_block_prefix"],
      cf_act_amt: "0",
      act_amt: basicTx["actions"].length.toString(),
      "iterations#actions": {
        allowed_iter_hashes: [
          "b59afff68ef56bd5abb3f74d6047559aebcc1a1189803c57eb988b76a92f90e5",
        ],
        trnsfiopubky: {
          "expected_length#1": "127",
          contract_account_name: parseContractAccountName(
            networkInfo[network].chainId,
            basicTx["actions"][0]["account"],
            basicTx["actions"][0]["name"],
            InvalidDataReason.ACTION_NOT_SUPPORTED
          ),
          num_auths: "1",
          permission: parseNameString(
            "active",
            InvalidDataReason.INVALID_PERMISSION
          ),
          data_len: dataLength.toString(),
          pk_len:
            basicTx["actions"][0]["data"]["payee_public_key"].length.toString(),
          pubkey: basicTx["actions"][0]["data"]["payee_public_key"],
          amount: basicTx["actions"][0]["data"]["amount"],
          max_fee: basicTx["actions"][0]["data"]["max_fee"],
          actor: parseNameString(
            basicTx["actions"][0]["data"]["actor"],
            InvalidDataReason.INVALID_ACTOR
          ),
          tpid_len: basicTx["actions"][0]["data"]["tpid"].length.toString(),
          tpid: basicTx["actions"][0]["data"]["tpid"],
        },
      },
    };

    const interpreter = new Interpreter(fio, path);
    const ledgerResponse = await interpreter.interpret(
      trnsfiopubkyTemplate,
      vals
    );

    if (ledgerResponse == undefined) {
      throw new Error("NO_LEDGER_RESPONSE");
    }

    const signatureLedger = Signature.fromHex(
      ledgerResponse.witness.witnessSignatureHex
    );

    expect(ledgerResponse.txHashHex).to.be.equal(hash);
    expect(signatureLedger.verify(fullMsg, publicKey)).to.be.true;
    expect(signatureLedger.verify(fullMsg, otherPublicKey)).to.be.false;
  });
});
