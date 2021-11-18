// import chai, {expect} from "chai"
// import chaiAsPromised from 'chai-as-promised'

// import type Fio from "../../src/fio"
// import {DeviceStatusError} from "../../src/fio"
// import {str_to_path} from "../../src/utils/address"
// import {getFio} from "../test_utils"
// import type {TestCase} from "./__fixtures__/getPublicKey"
// import {testsPublicKey} from "./__fixtures__/getPublicKey"

// chai.use(chaiAsPromised)

// describe("getPublicKey", async () => {
//     let fio: Fio = {} as Fio

//     beforeEach(async () => {
//         fio = await getFio()
//     })

//     afterEach(async () => {
//         await (fio as any).t.close()
//     })

//     describe("Should successfully get a single extended public key", async () => {
//         const test = async (tests: TestCase[]) => {
//             for (const {path, expected} of tests) {
//                 const response = await fio.getPublicKey(
//                     {path: str_to_path(path), show_or_not: false}
//                 )

//                 expect(response.publicKeyHex).to.equal(expected.publicKey)
//             }
//         }

//         it('fio', async () => {
//             await test(testsPublicKey)
//         })
//     })

//     describe("Should successfully show a single extended public key", async () => {
//         const test = async (tests: TestCase[]) => {
//             for (const {path, expected} of tests) {
//                 const response = await fio.getPublicKey(
//                     {path: str_to_path(path), show_or_not: true}
//                 )

//                 expect(response.publicKeyHex).to.equal(expected.publicKey)
//             }
//         }

//         it('fio', async () => {
//             await test(testsPublicKey)
//         })
//     })

//     describe("Should reject invalid paths", () => {
//         it('path shorter than 5 indexes', async () => {
//             const promise = fio.getPublicKey({path: str_to_path("44'/235'/0'/0"), show_or_not: false})
//             await expect(promise).to.be.rejectedWith(DeviceStatusError, "Action rejected by Ledger's security policy")
//         })

//         it('path contains non-zero address', async () => {
//             const promise = fio.getPublicKey({path: str_to_path("44'/235'/1'/0/0"), show_or_not: false})
//             await expect(promise).to.be.rejected
//         })

//         it('path contains non-hardened address', async () => {
//             const promise = fio.getPublicKey({path: str_to_path("44'/235'/0/0/0"), show_or_not: false})
//             await expect(promise).to.be.rejected
//         })

//         it('path contains non-zero chain', async () => {
//             const promise = fio.getPublicKey({path: str_to_path("44'/235'/0'/1/0"), show_or_not: false})
//             await expect(promise).to.be.rejected
//         })

//         it('path too long', async () => {
//             const promise = fio.getPublicKey({path: str_to_path("44'/235'/0'/0/0/0"), show_or_not: false})
//             await expect(promise).to.be.rejected
//         })
//     })

// })
