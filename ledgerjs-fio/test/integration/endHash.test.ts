// import { expect } from "chai";
// import { HARDENED } from "../../src/types/public";
// import type Fio from "../../src/fio";
// import { getFio } from "../test_utils";
// import { ENCODING_STRING, ENCODING_UINT64 } from "../../src/utils/parse";

// const path = [44 + HARDENED, 235 + HARDENED, 0 + HARDENED, 0, 0];

// describe("endHash", async () => {
//   let fio: Fio = {} as Fio;

//   beforeEach(async () => {
//     fio = await getFio();
//   });

//   afterEach(async () => {
//     await (fio as any).t.close();
//   });

//   it("Should correctly end the tx hash and integrity hash and return tx hash", async () => {
//     await fio.initHash();
//     await fio.sendDataNoDisplay("Actor", "someone", ENCODING_STRING);
//     await fio.sendDataDisplay("Amount", "76", ENCODING_UINT64);
//     const response = await fio.endHash(path);
//     expect(response.txHashHex.length).to.equal(2 * 32);
//     expect(response.witness.witnessSignatureHex.length).to.equal(2 * 65);
//   });
// });
