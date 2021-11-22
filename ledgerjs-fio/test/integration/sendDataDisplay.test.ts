import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"
import {ENCODING_STRING, ENCODING_UINT8} from "../../src/utils/parse";

describe("sendDataDisplay", async () => {
    let fio: Fio = {} as Fio

    beforeEach(async () => {
        fio = await getFio()
    })

    afterEach(async () => {
        await (fio as any).t.close()
    })

    it("Should send header and body and them on ledger", async () => {
        await fio.initHash()
        await fio.sendDataNoDisplay("Actor", "some_actor", ENCODING_STRING)
        const response = await fio.sendDataDisplay("Amount", "52", ENCODING_UINT8)
        await fio.endHash()
        expect(response.ret.length).to.equal(0)
    })
})