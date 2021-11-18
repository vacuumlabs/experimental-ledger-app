import {expect} from "chai"

import type Fio from "../../src/fio"
import {getFio} from "../test_utils"

describe("endHash", async () => {
    let fio: Fio = {} as Fio

    beforeEach(async () => {
        fio = await getFio()
    })

    afterEach(async () => {
        await (fio as any).t.close()
    })

    it("Should correctly end the tx hash and integrity hash and return tx hash", async () => {
        const init_response = await fio.initHash()
        const response = await fio.endHash()
        expect(response.ret.length).to.equal(2 * 32)
    })
})