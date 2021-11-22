import type {SendDataNoDisplay} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"
import {uint8_to_buf} from "../utils/serialize"
import { ENCODING_STRING, ENCODING_UINT8 } from "../../src/utils/parse"
import { Uint8_t } from "types/internal"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})

export function* sendDataNoDisplay(header: String, body: String, encoding: number): Interaction<SendDataNoDisplay> {
    const P1_UNUSED = 0x00
    const P2_UNUSED = 0x00
    let headerLen = header.length
    let bodyLen = body.length
    let buf = null
    if(encoding == ENCODING_STRING) {
        buf = Buffer.concat([
            uint8_to_buf(encoding as Uint8_t),
            uint8_to_buf(headerLen as Uint8_t),
            Buffer.from(header),
            uint8_to_buf(0 as Uint8_t),
            uint8_to_buf(bodyLen as Uint8_t),
            Buffer.from(body),
            uint8_to_buf(0 as Uint8_t)
        ])
    } else {
        let bodyUint = Number(body);
        buf = Buffer.concat([
            uint8_to_buf(encoding as Uint8_t),
            uint8_to_buf(headerLen as Uint8_t),
            Buffer.from(header),
            uint8_to_buf(0 as Uint8_t),
            uint8_to_buf(1 as Uint8_t), // Uint8_t is 1 byte
            uint8_to_buf(bodyUint as Uint8_t),
            uint8_to_buf(0 as Uint8_t)
        ])
    }
    const response = yield send({
        p1: 0x07,
        p2: P2_UNUSED,
        data: buf,
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
