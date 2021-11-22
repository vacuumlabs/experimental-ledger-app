import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"
import {ENCODING_STRING, ENCODING_UINT8} from "../../src/utils/parse"

describe("initHash", async () => {
    let fio: Fio = {} as Fio

    beforeEach(async () => {
        fio = await getFio()
    })

    afterEach(async () => {
        await (fio as any).t.close()
    })

    it("Should correctly start the tx hash and integrity hash", async () => {
        const response = await fio.initHash()
        await fio.sendDataNoDisplay("Actor", "anyone", ENCODING_STRING)
        await fio.sendDataDisplay("Amount", "31", ENCODING_UINT8)
        await fio.endHash()
        expect(response.ret.length).to.equal(0)
    })
})