import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"
import {ENCODING_STRING, ENCODING_UINT8} from "../../src/utils/parse"

describe("endHash", async () => {
    let fio: Fio = {} as Fio

    beforeEach(async () => {
        fio = await getFio()
    })

    afterEach(async () => {
        await (fio as any).t.close()
    })

    it("Should correctly end the tx hash and integrity hash and return tx hash", async () => {
        await fio.initHash()
        await fio.sendDataNoDisplay("Actor", "someone", ENCODING_STRING)
        await fio.sendDataDisplay("Amount", "76", ENCODING_UINT8)
        const response = await fio.endHash()
        expect(response.ret.length).to.equal(2 * 32)
    })
})