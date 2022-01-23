import { HexString, Uint32_t, Uint64_str, Uint8_t } from "types/internal"
import { isBuffer } from "utils/parse"
import { hex_to_buf, uint32_to_buf, uint64_to_buf, uint8_to_buf } from "../../src/utils/serialize"
import type {StartFor} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"

const crypto = require("crypto");

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})


// allowedIterHashes is a list of allowed hashes in hex strings
export function* startFor(minNumIterations: number, maxNumIterations: number, allowedIterHashes: string[]): Interaction<StartFor> {
    const P2_UNUSED = 0x00

    // console.log('joined: ', allowedIterHashes.join());
    console.log('starFor hasheshas hashing this: ', hex_to_buf(allowedIterHashes.join('') as HexString));
    const allowedIterHashesHash: HexString =
        crypto.createHash('sha256')
        .update(hex_to_buf(allowedIterHashes.join('') as HexString))
        .digest('hex')

    const response = yield send({
        p1: 0x0b,
        p2: P2_UNUSED,
        data: Buffer.concat([
            uint8_to_buf(minNumIterations as Uint8_t),
            uint8_to_buf(maxNumIterations as Uint8_t),
            hex_to_buf(allowedIterHashesHash)
        ]),
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
