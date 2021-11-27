import type {SendDataDisplay} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"
import {uint8_to_buf, uint16_to_buf, uint32_to_buf, uint64_to_buf} from "../utils/serialize"
import {Uint8_t, Uint16_t, Uint32_t, _Uint64_num, Uint64_str} from "types/internal"
import {
    ENCODING_STRING,
    ENCODING_UINT8,
    ENCODING_UINT16,
    ENCODING_UINT32,
    ENCODING_UINT64
} from "../../src/utils/parse"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})

export function* sendDataDisplay(header: String, body: String, encoding: number): Interaction<SendDataDisplay> {
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
            uint8_to_buf(0 as Uint8_t), // Trailing 0
            uint8_to_buf(bodyLen as Uint8_t),
            Buffer.from(body),
            uint8_to_buf(0 as Uint8_t)
        ])
    } else {
        let commonAttrsPrefix = [
            uint8_to_buf(encoding as Uint8_t),
            uint8_to_buf(headerLen as Uint8_t),
            Buffer.from(header),
            uint8_to_buf(0 as Uint8_t), // Trailing 0
        ]
        let commonAttrsSuffix = [
            uint8_to_buf(0 as Uint8_t)
        ]
        let bodyUint = Number(body);
        if(encoding == ENCODING_UINT8) {
            buf = Buffer.concat([
                ...commonAttrsPrefix, uint8_to_buf(1 as Uint8_t), uint8_to_buf(bodyUint as Uint8_t), ...commonAttrsSuffix
            ])
        } else if(encoding == ENCODING_UINT16) {
            buf = Buffer.concat([
                ...commonAttrsPrefix, uint8_to_buf(2 as Uint8_t), uint16_to_buf(bodyUint as Uint16_t), ...commonAttrsSuffix
            ])
        } else if(encoding == ENCODING_UINT32) {
            buf = Buffer.concat([
                ...commonAttrsPrefix, uint8_to_buf(4 as Uint8_t), uint32_to_buf(bodyUint as Uint32_t), ...commonAttrsSuffix
            ])
        } else if(encoding == ENCODING_UINT64) {
            buf = Buffer.concat([
                ...commonAttrsPrefix, uint8_to_buf(8 as Uint8_t), uint64_to_buf(body as Uint64_str), ...commonAttrsSuffix
            ])
        } else {
            throw Error("Invalid encoding");
        }
    }
    const response = yield send({
        p1: 0x08,
        p2: P2_UNUSED,
        data: buf,
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
