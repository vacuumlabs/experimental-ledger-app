// import {expect} from "chai"
// import { HARDENED } from "../../src/types/public"
// import type Fio from "../../src/fio"
// import {getFio} from "../test_utils"
// import {ENCODING_STRING, ENCODING_UINT64} from "../../src/utils/parse"

// const path = [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0]

// describe("initHash", async () => {
//     let fio: Fio = {} as Fio

//     beforeEach(async () => {
//         fio = await getFio()
//     })

//     afterEach(async () => {
//         await (fio as any).t.close()
//     })

//     it("Should correctly start the tx hash and integrity hash", async () => {
//         const response = await fio.initHash()
//         await fio.sendDataNoDisplay("Actor", "anyone", ENCODING_STRING)
//         await fio.sendDataDisplay("Amount", "29374657", ENCODING_UINT64)
//         await fio.endHash(path)
//         expect(response.ret.length).to.equal(0)
//     })
// })