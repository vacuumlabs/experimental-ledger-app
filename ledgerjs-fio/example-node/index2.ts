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
  parseContractAccountName
} from "../src/utils/parse";
import { Fio, HARDENED, GetPublicKeyRequest, SignTransactionRequest, TransactionEnc2 } from "../src/fio";
import {InvalidDataReason} from "../src/errors/invalidDataReason"

const path = [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0]

const fetch = require('node-fetch')
const readline = require('readline');

const wait = () => {
  const inputReader = require('wait-console-input')
  inputReader.wait('');
}

const getLine = (header: String) => {
  const inputReader = require('wait-console-input')
  let response: String = inputReader.readLine(header)
  return response
}

async function example() {
  const infoTestnet = await (await fetch('http://testnet.fioprotocol.io/v1/chain/get_info')).json()

  console.log("Running FIO examples");
  const transport = await TransportNodeHid.create(5000)
  const appFio = new Fio(transport);

  console.log("\n".repeat(3));
  console.log("Input: initHash");
  // wait()
  console.log("Response:");
  console.log(await appFio.initHash(infoTestnet.chain_id));
  // wait()
  console.log("\n".repeat(3));

  const basicTx: TransactionEnc2 = {
      expiration: "2021-08-28T12:50:36.686",
      ref_block_num: "4386",
      ref_block_prefix: "860116326",
      context_free_actions: [],
      actions: [{
          account: "fio.token",
          name: "trnsfiopubky",
          authorization: [{
              actor: "aftyershcu22",
              permission: "active",
          }],
          data: {
              payee_public_key: "FIO8PRe4WRZJj5mkem6qVGKyvNFgPsNnjNN6kPhh6EaCpzCVin5Jj",
              amount: "20",
              max_fee: "287454020",
              tpid: "rewards@wallet",
              actor: "aftyershcu22",
          },
      }],
      transaction_extensions: null,
    }

    console.log("Send expiration")
    console.log(await appFio.sendDataNoDisplay('expiration', basicTx['expiration'], ENCODING_DATETIME));

    console.log("Send ref_block_num")
    console.log(await appFio.sendDataNoDisplay('ref_block_num', basicTx['ref_block_num'], ENCODING_UINT16));

    console.log("Send ref_block_prefix")
    console.log(await appFio.sendDataNoDisplay('ref_block_prefix', basicTx['ref_block_prefix'], ENCODING_UINT32));

    console.log("Send max_net_usage_words")
    console.log(await appFio.sendDataNoDisplay('mx_net_words', '0', ENCODING_UINT8));

    console.log("Send max_cpu_usage_ms")
    console.log(await appFio.sendDataNoDisplay('mx_cpu_ms', '0', ENCODING_UINT8));

    console.log("Send delay_sec")
    console.log(await appFio.sendDataNoDisplay('delay_sec', '0', ENCODING_UINT8));

    console.log("Send context_free_actions_len")
    console.log(await appFio.sendDataNoDisplay('cf_act_amt', basicTx['context_free_actions'].length.toString(), ENCODING_UINT8));

    console.log("Send actions_len")
    console.log(await appFio.sendDataNoDisplay('act_amt', basicTx['actions'].length.toString(), ENCODING_UINT8));

    console.log("Send contract, account, name");
    console.log(await appFio.sendDataNoDisplay(
      "contract_account_name",
      parseContractAccountName(infoTestnet.chain_id, basicTx['actions'][0]['account'], basicTx['actions'][0]['name'], InvalidDataReason.ACTION_NOT_SUPPORTED),
      ENCODING_HEX
    ));

    console.log("Send number_of_authorizations")
    console.log(await appFio.sendDataNoDisplay('num_auths', basicTx['actions'][0]['authorization'].length.toString(), ENCODING_UINT8));

    console.log("Send actor")
    console.log(await appFio.sendDataNoDisplay('actor', parseNameString(basicTx['actions'][0]['authorization'][0]['actor'], InvalidDataReason.INVALID_ACTOR), ENCODING_HEX));

    console.log("Send permissions")
    console.log(await appFio.sendDataNoDisplay('permission', parseNameString(basicTx['actions'][0]['authorization'][0]['permission'], InvalidDataReason.INVALID_PERMISSION), ENCODING_HEX));

    // Data length
    const SIMPLE_LENGTH_VARIABLE_LENGTH = 1
    const AMOUNT_TYPE_LENGTH = 8
    const NAME_VARIABLE_LENGTH = 8
    let dataLength = SIMPLE_LENGTH_VARIABLE_LENGTH + basicTx['actions'][0]['data']['payee_public_key'].length + // pubkey_length, pubkey
                     2 * AMOUNT_TYPE_LENGTH + NAME_VARIABLE_LENGTH + // amount, max_fee, actor
                     + SIMPLE_LENGTH_VARIABLE_LENGTH + basicTx['actions'][0]['data']['tpid'].length; // tpid_length, tpid
    console.log("Send data_length")
    console.log(await appFio.sendDataNoDisplay('data_len', dataLength.toString(), ENCODING_UINT8));

    console.log("Send pubkey_length")
    console.log(await appFio.sendDataNoDisplay(
      'pk_len', basicTx['actions'][0]['data']['payee_public_key'].length.toString(), ENCODING_UINT8)
    );

    console.log("Send pubkey")
    console.log(await appFio.sendDataDisplay(
      'pubkey', basicTx['actions'][0]['data']['payee_public_key'], ENCODING_STRING)
    );

    console.log("Send amount")
    console.log(await appFio.sendDataDisplay(
      'amount', basicTx['actions'][0]['data']['amount'], ENCODING_UINT64)
    );

    console.log("Send max_fee")
    console.log(await appFio.sendDataDisplay(
      'max_fee', basicTx['actions'][0]['data']['max_fee'], ENCODING_UINT64)
    );

    console.log("Send actor")
    console.log(await appFio.sendDataNoDisplay(
      'actor', parseNameString(basicTx['actions'][0]['data']['actor'], InvalidDataReason.INVALID_ACTOR), ENCODING_HEX)
    );

    console.log("Send tpid_length")
    console.log(await appFio.sendDataNoDisplay(
      'tpid_len', basicTx['actions'][0]['data']['tpid'].length.toString(), ENCODING_UINT8)
    );
    
    console.log("Send tpid")
    console.log(await appFio.sendDataNoDisplay(
      'tpid', basicTx['actions'][0]['data']['tpid'], ENCODING_STRING)
    );

    // We omit extension points here

    console.log("\n".repeat(3));
    console.log("Input: endHash");
    // wait()
    console.log("Response:");
    console.log(await appFio.endHash(path));
    // wait()
    console.log("\n".repeat(3));


    // const signTransactionRequest: SignTransactionRequest = {
    //   path: [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0],
    //   chainId: infoTestnet.chain_id,
    //   tx: basicTx
    // };
    // console.log("Input: signTransaction");
    // console.log(signTransactionRequest);
    // console.log(signTransactionRequest.tx.actions);
    // wait()
    // console.log("Response:");
    // console.log(await appFio.signTransaction(signTransactionRequest));
    // wait()
    // console.log("\n".repeat(3));






//   console.log("Input: getSerial");
//   wait()
//   console.log("Response:");
//   console.log(await appFio.getSerial());
//   wait()
//   console.log("\n".repeat(3));

//   const getPublicKeyRequest1: GetPublicKeyRequest = {path: [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0], show_or_not: false};
//   console.log("Input: getPublicKey");
//   console.log(getPublicKeyRequest1);
//   wait()
//   console.log("Response:");
//   console.log(await appFio.getPublicKey(getPublicKeyRequest1));
//   wait()
//   console.log("\n".repeat(3));

//   const getPublicKeyRequest2: GetPublicKeyRequest = {path: [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 2000], show_or_not: false};
//   console.log("Input: getPublicKey");
//   console.log(getPublicKeyRequest2);
//   wait()
//   console.log("Response:");
//   console.log(await appFio.getPublicKey(getPublicKeyRequest2));
//   wait()
//   console.log("\n".repeat(3));

//   const basicTx: Transaction = {
//     expiration: "2021-08-28T12:50:36.686",
//     ref_block_num: 0x1122,
//     ref_block_prefix: 0x33445566,
//     context_free_actions: [],
//     actions: [{
//         account: "fio.token",
//         name: "trnsfiopubky",
//         authorization: [{
//             actor: "aftyershcu22",
//             permission: "active",
//         }],
//         data: {
//             payee_public_key: "FIO8PRe4WRZJj5mkem6qVGKyvNFgPsNnjNN6kPhh6EaCpzCVin5Jj",
//             amount: "2000",
//             max_fee: 0x11223344,
//             tpid: "rewards@wallet",
//             actor: "aftyershcu22",
//         },
//     }],
//     transaction_extensions: null,
//   }
//   const infoTestnet = await (await fetch('http://testnet.fioprotocol.io/v1/chain/get_info')).json()
//   const signTransactionRequest: SignTransactionRequest = {
//     path: [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0],
//     chainId: infoTestnet.chain_id,
//     tx: basicTx
//   };
//   console.log("Input: signTransaction");
//   console.log(signTransactionRequest);
//   console.log(signTransactionRequest.tx.actions);
//   wait()
//   console.log("Response:");
//   console.log(await appFio.signTransaction(signTransactionRequest));
//   wait()
  console.log("\n".repeat(3));
}

example()
