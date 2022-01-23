import { Uint32_t, Uint8_t } from "types/internal"
import { uint32_to_buf, uint8_to_buf } from "../utils/serialize"
import type {EndCountedSection} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})


export function* endCountedSection(): Interaction<EndCountedSection> {
    const P2_UNUSED = 0x00
    const response = yield send({
        p1: 0x0a,
        p2: P2_UNUSED,
        data: Buffer.from(''),
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
