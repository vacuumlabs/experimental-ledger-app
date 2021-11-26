import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"
import {ENCODING_STRING} from "../../src/utils/parse"

describe("sendDataNoDisplay", async () => {
    let fio: Fio = {} as Fio

    beforeEach(async () => {
        fio = await getFio()
    })

    afterEach(async () => {
        await (fio as any).t.close()
    })

    it("Should send data without displaying them on ledger", async () => {
        await fio.initHash()
        const response = await fio.sendDataNoDisplay("Actor", "some_actor", ENCODING_STRING)
        expect(response.ret.length).to.equal(0)
    })
})