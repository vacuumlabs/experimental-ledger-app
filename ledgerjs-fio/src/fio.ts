/********************************************************************************
 *   Ledger Node JS API
 *   (c) 2016-2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/
import type Transport from "@ledgerhq/hw-transport"

import {DeviceStatusCodes, DeviceStatusError} from './errors'
import {InvalidDataReason} from "./errors"
import type {Interaction, SendParams} from './interactions/common/types'
import {getPublicKey} from "./interactions/getPublicKey"
import {getSerial} from "./interactions/getSerial"
import {getCompatibility, getVersion} from "./interactions/getVersion"
import {runTests} from "./interactions/runTests"
import {signTransaction} from "./interactions/signTransaction"
import {initHash} from "./interactions/initHash"
import {endHash} from "./interactions/endHash"
import {sendData} from "./interactions/sendData"
import {startCountedSection} from "./interactions/startCountedSection"
import {endCountedSection} from "./interactions/endCountedSection"
import {startFor} from "./interactions/startFor"
import {endFor} from "./interactions/endFor"
import {startIteration} from "./interactions/startIteration"
import {endIteration} from "./interactions/endIteration"
import type {
    HexString,
    ParsedAction,
    ParsedActionAuthorisation,
    ParsedTransaction,
    ParsedTransferFIOTokensData,
    ValidBIP32Path,
} from './types/internal'
import type {
    Action, ActionAuthorisation, BIP32Path, DeviceCompatibility, PublicKey,
    Serial, Init, End, StartCountedSection, EndCountedSection, SendData, StartFor, EndFor,
    StartIteration, EndIteration,
    SignedTransactionData, Transaction, TransferFIOTokensData, Version,
} from './types/public'
import utils from "./utils"
import {assert} from './utils/assert'
import {
    isHexString,
    isValidPath,
    parseAuthorization,
    parseBIP32Path,
    parseContractAccountName,
    parseNameString, parseTransaction,
    parseUint16_t,
    parseUint32_t,
    parseUint64_str,
    validate,
} from './utils/parse'

export * from './errors'
export * from './types/public'

const CLA = 0xd7

function wrapConvertDeviceStatusError<T extends Function>(fn: T): T {
    // @ts-ignore
    return async (...args) => {
        try {
            return await fn(...args)
        } catch (e: any) {
            if (e && e.statusCode) {
                throw new DeviceStatusError(e.statusCode)
            }
            throw e
        }
    }
}

/**
 * FIO API
 *
 * @example
 * import Fio from "@fioprotocol/fiojs";
 * const fio = new Fio(transport);
 */

/** @ignore */
export type SendFn = (params: SendParams) => Promise<Buffer>;

// It can happen that we try to send a message to the device
// when the device thinks it is still in a middle of previous ADPU stream.
// This happens mostly if host does abort communication for some reason
// leaving ledger mid-call.
// In this case Ledger will respond by ERR_STILL_IN_CALL *and* resetting its state to
// default. We can therefore transparently retry the request.

// Note though that only the *first* request in an multi-APDU exchange should be retried.
function wrapRetryStillInCall<T extends Function>(fn: T): T {
    // @ts-ignore
    return async (...args: any) => {
        try {
            return await fn(...args)
        } catch (e: any) {
            if (
                e &&
                e.statusCode &&
                e.statusCode === DeviceStatusCodes.ERR_STILL_IN_CALL
            ) {
                // Do the retry
                return await fn(...args)
            }
            throw e
        }
    }
}


async function interact<T>(
    interaction: Interaction<T>,
    send: SendFn,
): Promise<T> {
    let cursor = interaction.next()
    let first = true
    while (!cursor.done) {
        const apdu = cursor.value
        const res = first
            ? await wrapRetryStillInCall(send)(apdu)
            : await send(apdu)
        first = false
        cursor = interaction.next(res)
    }
    return cursor.value
}

/**
 * Main API endpoint
 * @category Main
 */
export class Fio {
    /** @ignore */
    transport: Transport<string>;
    /** @ignore */
    _send: SendFn;

    constructor(transport: Transport<string>, scrambleKey: string = "FIO") {
        this.transport = transport
        // Note: this is list of methods that should "lock" the transport to avoid concurrent use
        const methods = [
            "getVersion",
            "getSerial",
            "getPublicKey",
            "signTransaction",
            "initHash",
            "endHash",
            "sendData"
        ]
        this.transport.decorateAppAPIMethods(this, methods, scrambleKey)
        this._send = async (params: SendParams): Promise<Buffer> => {
            let response = await wrapConvertDeviceStatusError(this.transport.send)(
                CLA,
                params.ins,
                params.p1,
                params.p2,
                params.data,
            )
            response = utils.stripRetcodeFromResponse(response)

            if (params.expectedResponseLength != null) {
                assert(
                    response.length === params.expectedResponseLength,
                    `unexpected response length: ${response.length} instead of ${params.expectedResponseLength}`,
                )
            }

            return response
        }
    }

    /**
     * Returns an object containing the app version.
     *
     * @returns Result object containing the application version number.
     *
     * @example
     * const { major, minor, patch, flags } = await fio.getVersion();
     * console.log(`App version ${major}.${minor}.${patch}`);
     *
     */
    async getVersion(): Promise<GetVersionResponse> {
        const version = await interact(this._getVersion(), this._send)
        return {version, compatibility: getCompatibility(version)}
    }

    /** @ignore */
    * _getVersion(): Interaction<Version> {
        return yield* getVersion()
    }

    /**
     * Returns an object containing the device serial number.
     *
     * @returns Result object containing the device serial number.
     *
     * @example
     * const { serial } = await fio.getSerial();
     * console.log(`Serial number ${serial}`);
     *
     */
    async getSerial(): Promise<GetSerialResponse> {
        return interact(this._getSerial(), this._send)
    }

    /** @ignore */
    * _getSerial(): Interaction<GetSerialResponse> {
        const version = yield* getVersion()
        return yield* getSerial(version)
    }

    async initHash(chainId: HexString): Promise<InitHashResponse> {
        return interact(this._initHash(chainId), this._send)
    }

    * _initHash(chainId: HexString): Interaction<InitHashResponse> {
        return yield* initHash(chainId);
    }

    async endHash(path: BIP32Path): Promise<EndHashResponse> {
        const parsedPath = parseBIP32Path(path, InvalidDataReason.INVALID_PATH)
        return interact(this._endHash(parsedPath), this._send)
    }

    * _endHash(parsedPath: ValidBIP32Path): Interaction<EndHashResponse> {
        return yield* endHash(parsedPath)
    }

    async startCountedSection(sectionLength: number): Promise<StartCountedSectionResponse> {
        return interact(this._startCountedSection(sectionLength), this._send);
    }

    * _startCountedSection(sectionLength: number): Interaction<StartCountedSectionResponse> {
        return yield* startCountedSection(sectionLength);
    }

    async endCountedSection(): Promise<EndCountedSectionResponse> {
        return interact(this._endCountedSection(), this._send);
    }

    * _endCountedSection(): Interaction<EndCountedSectionResponse> {
        return yield* endCountedSection();
    }

    async sendData(header: string, body: string, encoding: number, display: boolean = false): Promise<SendDataResponse> {
        return interact(this._sendData(header, body, encoding, display), this._send);
    }

    *_sendData(header: string, body: string, encoding: number, display: boolean = false): Interaction<SendDataResponse> {
        return yield* sendData(header, body, encoding, display);
    }

    async startFor(minNumIterations: number, maxNumIterations: number, allowedIterHashes: string[]): Promise<StartForResponse> {
        return interact(this._startFor(minNumIterations, maxNumIterations, allowedIterHashes), this._send);
    }

    * _startFor(minNumIterations: number, maxNumIterations: number, allowedIterHashes: string[]): Interaction<StartForResponse> {
        return yield* startFor(minNumIterations, maxNumIterations, allowedIterHashes);
    }

    async endFor(): Promise<EndForResponse> {
        return interact(this._endFor(), this._send);
    }

    * _endFor(): Interaction<EndForResponse> {
        return yield* endFor();
    }

    async startIteration(): Promise<StartIterationResponse> {
        return interact(this._startIteration(), this._send);
    }

    * _startIteration(): Interaction<StartIterationResponse> {
        return yield* startIteration();
    }

    async endIteration(allowedIterHashes: string[]): Promise<EndIterationResponse> {
        return interact(this._endIteration(allowedIterHashes), this._send);
    }

    * _endIteration(allowedIterHashes: string[]): Interaction<EndIterationResponse> {
        return yield* endIteration(allowedIterHashes);
    }

    /**
     * Get public key for the specified BIP 32 path.
     *
     * @param path The path. A path must begin with `44'/235'/0'/0/i`.
     * @returns The public key (i.e. with chaincode).
     *
     * @example
     * ```
     * const publicKey = await fio.getPublicKey[[ HARDENED + 44, HARDENED + 235, HARDENED + 0, 0, 0 ]);
     * console.log(publicKey);
     * ```
     */
    async getPublicKey(
        {path, show_or_not}: GetPublicKeyRequest
    ): Promise<GetPublicKeyResponse> {
        // validate the input
        validate(isValidPath(path), InvalidDataReason.GET_EXT_PUB_KEY_PATHS_NOT_ARRAY)
        const parsedPath = parseBIP32Path(path, InvalidDataReason.INVALID_PATH)

        return interact(this._getPublicKey(parsedPath, show_or_not), this._send)
    }

    /** @ignore */
    * _getPublicKey(path: ValidBIP32Path, show_or_not: boolean) {
        const version = yield* getVersion()
        return yield* getPublicKey(version, path, show_or_not)
    }

    /**
     * Sign transaction.
     *
     * @param path The path. A path must begin with `44'/235'/0'/0/i`.
     * @returns The public key (i.e. with chaincode).
     *
     * @example
     * ```
     * TODO
     * const  = await fio.getPublicKey[[ HARDENED + 44, HARDENED + 235, HARDENED + 0, 0, 0 ]); TODO
     * console.log(sign);
     * ```
     */
    async signTransaction({path, chainId, tx}: SignTransactionRequest): Promise<SignTransactionResponse> {
        validate(isHexString(chainId), InvalidDataReason.INVALID_CHAIN_ID)

        const parsedPath = parseBIP32Path(path, InvalidDataReason.INVALID_PATH)
        const parsedTx = parseTransaction(chainId, tx)

        return interact(this._signTransaction(parsedPath, chainId as HexString, parsedTx), this._send)
    }

    /** @ignore */
    * _signTransaction(parsedPath: ValidBIP32Path, chainId: HexString, tx: ParsedTransaction) {
        const version = yield* getVersion()
        return yield* signTransaction(version, parsedPath, chainId, tx)
    }

    /**
     * Runs unit tests on the device (DEVEL app build only)
     */
    async runTests(): Promise<void> {
        return interact(this._runTests(), this._send)
    }

    /** @ignore */
    * _runTests(): Interaction<void> {
        const version = yield* getVersion()
        return yield* runTests(version)
    }

}

/**
 * Get FIO app version [[Fio.getVersion]] response data
 * @category Main
 */
export type GetVersionResponse = {
    version: Version
    compatibility: DeviceCompatibility
}

/**
 * Get device serial number ([[Fio.getSerial]]) response data
 * @category Main
 */
export type GetSerialResponse = Serial

export type InitHashResponse = Init

export type EndHashResponse = End

export type StartCountedSectionResponse = StartCountedSection

export type EndCountedSectionResponse = EndCountedSection

export type SendDataResponse = SendData

export type StartForResponse = StartFor

export type EndForResponse = EndFor

export type StartIterationResponse = StartIteration

export type EndIterationResponse = EndIteration

/**
 * Get public key ([[Fio.getPublicKey]]) request data
 * @category Main
 * @see [[GetPublicKeyResponse]]
 */
export type GetPublicKeyRequest = {
    /** Paths to public keys which should be derived by the device */
    show_or_not: boolean, 
    path: BIP32Path
}

/**
 * Get public key ([[Fio.getPublicKey]]) response data
 * @category Main
 * @see [[GetPublicKeyRequest]]
 */
export type GetPublicKeyResponse = PublicKey

/**
 * Sign transaction ([[Fio.signTransaction]]) request data
 * @category Main
 * @see [[SignTransactionResponse]]
 */
export type SignTransactionRequest = {
    path: BIP32Path,
    chainId: string,
    tx: Transaction,
}

/**
 * Sign transaction ([[Fio.signTransaction]]) response data
 * @category Main
 * @see [[SignTransactionRequest]]
 */
export type SignTransactionResponse = SignedTransactionData

export default Fio
