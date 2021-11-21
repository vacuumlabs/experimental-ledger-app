import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"

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
        const response = await fio.sendDataDisplay("Amount", "52")
        await fio.endHash()
        expect(response.ret.length).to.equal(0)
    })
})