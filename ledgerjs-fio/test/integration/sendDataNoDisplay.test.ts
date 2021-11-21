import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"

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
        const response = await fio.sendDataNoDisplay("Actor", "some_actor")
        // Ledger ui will hang, because the hash has not ended (no ui_idle() call
        // happened)
        expect(response.ret.length).to.equal(0)
    })
})