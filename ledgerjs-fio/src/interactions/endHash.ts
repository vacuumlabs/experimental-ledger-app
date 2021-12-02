import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"
import {HARDENED, End} from "../types/public"
import {assert} from "../utils/assert"
import {chunkBy} from "../utils/ioHelpers"
import type {ValidBIP32Path} from "../types/internal"
import { path_to_buf } from "../utils/serialize"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})


export function* endHash(parsedPath: ValidBIP32Path): Interaction<End> {
    const P1_UNUSED = 0x00
    const P2_UNUSED = 0x00
    const response = yield send({
        p1: 0x06,
        p2: P2_UNUSED,
        data: path_to_buf(parsedPath),
        expectedResponseLength: 65 + 32,  // Expect 65 (witness) + 32 (txHash) bytes in response
    })

    const [witnessSignature, hash, rest] = chunkBy(response, [65, 32])
    assert(rest.length === 0, "invalid response length")

    return {
        txHashHex: utils.buf_to_hex(hash),
        witness: {
            path: [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0],
            witnessSignatureHex: utils.buf_to_hex(witnessSignature),
        },
    }
}
