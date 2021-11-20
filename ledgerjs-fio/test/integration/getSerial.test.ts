// import {expect} from "chai"

// import type Fio from "../../src/fio"
// import {getFio} from "../test_utils"

// describe("getSerial", async () => {
//     let fio: Fio = {} as Fio

//     beforeEach(async () => {
//         fio = await getFio()
//     })

//     afterEach(async () => {
//         await (fio as any).t.close()
//     })

//     it("Should correctly get the serial number of the device", async () => {
//         const response = await fio.getSerial()
//         expect(response.serial.length).to.equal(14)
//     })
// })
