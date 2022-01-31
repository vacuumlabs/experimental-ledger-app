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
import { Fio, HARDENED, GetPublicKeyRequest, SignTransactionRequest, Transaction } from "../src/fio";
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

  const basicTx: Transaction = {
      expiration: "2021-08-28T12:50:36.686",
      ref_block_num: "4386",
      ref_block_prefix: "860116326",
      context_free_actions: [],
      actions: [
        {
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
        },
        {
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
        },
        {
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
        }
      ],
      transaction_extensions: null,
    }

    const allowedIterHashes: string[][] = [
      ["7abb808e3cc2539ca34b519625c0516e17da445dc585f4b3c3130b6f283e0918"],
    ];

    console.log("Send expiration")
    console.log(await appFio.sendData('expiration', basicTx['expiration'], ENCODING_DATETIME));

    console.log("Send ref_block_num")
    console.log(await appFio.sendData('ref_block_num', basicTx['ref_block_num'], ENCODING_UINT16));

    console.log("Send ref_block_prefix")
    console.log(await appFio.sendData('ref_block_prefix', basicTx['ref_block_prefix'], ENCODING_UINT32));

    console.log("Send max_net_usage_words")
    console.log(await appFio.sendData('mx_net_words', '0', ENCODING_UINT8));

    console.log("Send max_cpu_usage_ms")
    console.log(await appFio.sendData('mx_cpu_ms', '0', ENCODING_UINT8));

    console.log("Send delay_sec")
    console.log(await appFio.sendData('delay_sec', '0', ENCODING_UINT8));

    console.log("Send context_free_actions_len")
    console.log(await appFio.sendData('cf_act_amt', basicTx['context_free_actions'].length.toString(), ENCODING_UINT8));

    console.log("Send actions_len")
    console.log(await appFio.sendData('act_amt', basicTx['actions'].length.toString(), ENCODING_UINT8));

    // basicTx['actions'].forEach(async (action) => {
    console.log('Start for');
    console.log(await appFio.startFor(1, 10, allowedIterHashes[0]));

    for(let i = 0; i < basicTx['actions'].length; i++) {

      console.log('Start iteration');
      console.log(await appFio.startIteration());

      console.log("Init first counted section");
      console.log(await appFio.startCountedSection(127));

      console.log("Send contract, account, name");
      console.log(await appFio.sendData(
        "contract_account_name",
        parseContractAccountName(infoTestnet.chain_id, basicTx['actions'][i]['account'], basicTx['actions'][i]['name'], InvalidDataReason.ACTION_NOT_SUPPORTED),
        ENCODING_HEX
      ));

      console.log("Send number_of_authorizations")
      console.log(await appFio.sendData('num_auths', basicTx['actions'][i]['authorization'].length.toString(), ENCODING_UINT8));

      console.log("Send actor")
      console.log(await appFio.sendData(
        'actor',
        parseNameString(basicTx['actions'][i]['authorization'][0]['actor'],
        InvalidDataReason.INVALID_ACTOR),
        ENCODING_HEX
      ));

      console.log("Send permissions")
      console.log(await appFio.sendData(
        'permission',
        parseNameString(basicTx['actions'][i]['authorization'][0]['permission'],
        InvalidDataReason.INVALID_PERMISSION),
        ENCODING_HEX
      ));

      // Data length
      const SIMPLE_LENGTH_VARIABLE_LENGTH = 1
      const AMOUNT_TYPE_LENGTH = 8
      const NAME_VARIABLE_LENGTH = 8
      let dataLength = SIMPLE_LENGTH_VARIABLE_LENGTH + basicTx['actions'][i]['data']['payee_public_key'].length + // pubkey_length, pubkey
                      2 * AMOUNT_TYPE_LENGTH + NAME_VARIABLE_LENGTH + // amount, max_fee, actor
                      + SIMPLE_LENGTH_VARIABLE_LENGTH + basicTx['actions'][i]['data']['tpid'].length; // tpid_length, tpid
      console.log("Send data_length")
      console.log(await appFio.sendData('data_len', dataLength.toString(), ENCODING_UINT8));

      console.log("Send pubkey_length")
      console.log(await appFio.sendData(
        'pk_len', basicTx['actions'][i]['data']['payee_public_key'].length.toString(), ENCODING_UINT8)
      );

      console.log("Send pubkey")
      console.log(await appFio.sendData(
        'pubkey', basicTx['actions'][i]['data']['payee_public_key'], ENCODING_STRING, true)
      );

      console.log("Send amount")
      console.log(await appFio.sendData(
        'amount', basicTx['actions'][i]['data']['amount'], ENCODING_UINT64, true)
      );

      console.log("Send max_fee")
      console.log(await appFio.sendData(
        'max_fee', basicTx['actions'][i]['data']['max_fee'], ENCODING_UINT64, true)
      );

      console.log("Send actor")
      console.log(await appFio.sendData(
        'actor', parseNameString(basicTx['actions'][i]['data']['actor'], InvalidDataReason.INVALID_ACTOR), ENCODING_HEX)
      );

      console.log("Send tpid_length")
      console.log(await appFio.sendData(
        'tpid_len', basicTx['actions'][i]['data']['tpid'].length.toString(), ENCODING_UINT8)
      );
      
      console.log("Send tpid")
      console.log(await appFio.sendData(
        'tpid', basicTx['actions'][i]['data']['tpid'], ENCODING_STRING)
      );

      console.log("End first counted section")
      console.log(await appFio.endCountedSection());

      console.log('End iteration');
      console.log(await appFio.endIteration(allowedIterHashes[0]));

    }

    console.log('endFor');
    console.log(await appFio.endFor());
    // });

    // console.log('Start for');
    // console.log(await appFio.startFor(1, 3, allowedIterHashes[0]));

    //   console.log('Start iteration');
    //   console.log(await appFio.startIteration());

    //     console.log('Start for2');
    //     console.log(await appFio.startFor(0, 1, allowedIterHashes[1]));

    //       console.log('Start iteration2');
    //       console.log(await appFio.startIteration());

    //         console.log('Start second counted section');
    //         console.log(await appFio.startCountedSection(9));

    //           console.log('Send data inside iteration');
    //           console.log(await appFio.sendData("SomeHeader", "aaaaaaaaa", ENCODING_STRING));

    //           console.log('Start for3');
    //           console.log(await appFio.startFor(0, 2, allowedIterHashes[2]));

    //           console.log('End for3');
    //           console.log(await appFio.endFor());

    //         console.log('End second counted section');
    //         console.log(await appFio.endCountedSection());

    //       console.log('End iteration2');
    //       console.log(await appFio.endIteration(allowedIterHashes[1]));

    //     console.log('End for2');
    //     console.log(await appFio.endFor());
      
    //   console.log('End iteration');
    //   console.log(await appFio.endIteration(allowedIterHashes[0]));

    // console.log('End for');
    // console.log(await appFio.endFor());

    // We omit extension points here

    console.log("\n".repeat(3));
    console.log("Input: endHash");
    console.log(await appFio.endHash(path));
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
