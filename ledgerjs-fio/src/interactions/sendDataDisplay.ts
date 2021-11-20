import type {SendDataDisplay} from "../types/public"
import utils from "../utils"
import {INS} from "./common/ins"
import type {Interaction, SendParams} from "./common/types"
import { uint8_to_buf } from "utils/serialize"
import { Uint8_t } from "types/internal"

const send = (params: {
    p1: number,
    p2: number,
    data: Buffer,
    expectedResponseLength?: number
}): SendParams => ({ins: INS.SIGN_TX, ...params})

function num_to_uint8_buf(value: number): Buffer {
    const data = Buffer.alloc(1)
    data.writeUInt8(value, 0)
    return data
}

export function* sendDataDisplay(headerText: String, bodyText: String): Interaction<SendDataDisplay> {
    const P1_UNUSED = 0x00
    const P2_UNUSED = 0x00
    let headerLen   = headerText.length
    let bodyLen     = bodyText.length

    console.log("headerLen: ", headerLen);
    console.log("bodyLen: ", bodyLen);
    console.log("headerText: ", headerText);
    console.log("bodyText: ", bodyText);

    let buf         =  Buffer.concat(
        [
            num_to_uint8_buf(headerLen), 
            Buffer.from(headerText),
            num_to_uint8_buf(bodyLen),
            Buffer.from(bodyText),
            num_to_uint8_buf(1)
        ]
    )
    const response = yield send({
        p1: 0x08,
        p2: P2_UNUSED,
        data: buf,
        // data: Buffer.from("\bNadpis: \bhodnotax"),
        expectedResponseLength: 0,  // Expect 0 bytes in response
    })

    const ret = utils.buf_to_hex(response)
    return {ret}
}
