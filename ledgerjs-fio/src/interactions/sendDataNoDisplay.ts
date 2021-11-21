import type {SendDataNoDisplay} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"
import {num_to_uint8_buf} from "../utils/serialize"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})

export function* sendDataNoDisplay(headerText: String, bodyText: String): Interaction<SendDataNoDisplay> {
    const P1_UNUSED = 0x00
    const P2_UNUSED = 0x00
    let headerLen = headerText.length
    let bodyLen = bodyText.length
    let buf = Buffer.concat([
        num_to_uint8_buf(150),
        num_to_uint8_buf(headerLen),
        Buffer.from(headerText),
        num_to_uint8_buf(0),
        num_to_uint8_buf(bodyLen),
        Buffer.from(bodyText),
        num_to_uint8_buf(0)
    ])
    const response = yield send({
        p1: 0x07,
        p2: P2_UNUSED,
        data: buf,
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
