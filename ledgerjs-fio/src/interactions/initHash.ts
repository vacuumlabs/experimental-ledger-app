import { HexString } from "types/internal"
import type {Init} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})


export function* initHash(chainId: HexString): Interaction<Init> {
    const P1_UNUSED = 0x00
    const P2_UNUSED = 0x00
    const response = yield send({
        p1: 0x01,
        p2: P2_UNUSED,
        data: Buffer.from(chainId, "hex"),
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
