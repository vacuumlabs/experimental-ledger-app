import { resolve } from "path";
import { buf_to_hex } from "utils/serialize";
import { Fio } from "../fio";
import { ENCODING_STRING, ENCODING_HEX } from "utils/parse";

const crypto = require("crypto");

// uint8_t prevHash[32]; // Finalized hash from the previous instruction
// uint8_t sectionLevel; // The current nesting level of counted sections, 0 means no counted section
// uint64_t expectedSectionLength[MAX_NESTED_COUNTED_SECTIONS + 1];
// uint8_t forMinIterations[MAX_FOR_DEPTH + 1]; // Min allowed number of iterations
// uint8_t forMaxIterations[MAX_FOR_DEPTH + 1]; // Max allowed number of iterations
// uint8_t forLevel;							 // Depth of the current for (nested), 0 if we are not inside a for
// uint8_t intHashes[MAX_FOR_DEPTH][32]; // List of integrity hashes of fors with lower level
// uint8_t allowedIterationHashesHash[MAX_FOR_DEPTH][32];
type Context = {
  prevHash: string;
  sectionLevel: number;
  expectedSectionLength: number[];
  forMinIterations: number[];
  forMaxIterations: number[];
  forLevel: number;
  intHashes: string[][];
  allowedIterationHashesHash: string[][];
};

export default class Interpreter {
  appInstance: Fio;
  path: number[];

  constructor(appInstance: Fio, path: number[]) {
    this.appInstance = appInstance;
    this.path = path;
  }

  // TODO BE vs LE
  intToBuf(x: number, numBytes: number) {
    const buf = Buffer.allocUnsafe(numBytes);
    switch (numBytes) {
      case 1:
        buf.writeUInt8(x);
        break;
      case 2:
        buf.writeUInt16BE(x);
        break;
      case 4:
        buf.writeUInt32BE(x);
        break;
      case 8:
        buf.writeBigUInt64BE(BigInt(x));
        break;
      default:
        throw new Error(`Number of bytes not allowed: ${numBytes}`);
    }
    return buf;
  }

  // calcTemplateHash(template: any, ctx: Context) {
  //   for (let i = 0; i < template.instructions.length; i++) {
  //     const ins = template.instructions[i];
  //     if (ins.name == "INIT_HASH") {
  //       const msg = Buffer.from("3001", "hex"); // INIT_HASH id
  //       ctx.prevHash = crypto.createHash("sha256").update(msg).digest("hex");
  //     } else if (ins.name == "SEND_DATA") {
  //       const display =
  //         ins.params.display == undefined ? false : ins.params.display;
  //       const storageAction = 0; // TODO get from JSON
  //       const msg = Buffer.concat([
  //         Buffer.from("3007", "hex"),
  //         Buffer.from(`${display ? "01" : "00"}`, "hex"),
  //         this.intToBuf(ins.params.encoding, 1),
  //         this.intToBuf(),
  //       ]);
  //     }
  //   }
  // }

  async interpret(template: any, values: any, level: number = 0) {
    for (let i = 0; i < template.instructions.length; i++) {
      const ins = template.instructions[i];
      console.log(ins);
      let res;
      if (ins.name == "INIT_HASH") {
        res = await this.appInstance.initHash();
      } else if (ins.name == "SEND_DATA") {
        const value =
          ins.params.value == undefined
            ? values[ins.params.header]
            : ins.params.value;
        const display =
          ins.params.display == undefined ? false : ins.params.display;
        const storageAction = 0; // TODO get from JSON
        res = await this.appInstance.sendData(
          ins.params.header,
          value,
          ins.params.encoding,
          display,
          storageAction
        );
      } else if (ins.name == "FOR") {
        const key = "iterations#" + ins.id;
        res = await this.appInstance.startFor(
          ins.params.min_iterations,
          ins.params.max_iterations,
          values[key].allowed_iter_hashes
        );
        for (let j = 0; j < ins.iterations.length; j++) {
          res = await this.appInstance.startIteration();
          const it = ins.iterations[j];
          const name = it.name;
          await this.interpret(
            { instructions: it.instructions },
            values[key][name],
            level + 1
          );
          res = await this.appInstance.endIteration(
            values[key].allowed_iter_hashes
          );
        }
        await this.appInstance.endFor();
      } else if (ins.name == "START_COUNTED_SECTION") {
        let expected_length: string = ins.params.expected_length;
        if (expected_length == undefined) {
          const key = "expected_length#" + ins.id;
          expected_length = values[key];
        }
        res = await this.appInstance.startCountedSection(
          Number(expected_length)
        );
      } else if (ins.name == "END_COUNTED_SECTION") {
        res = await this.appInstance.endCountedSection();
      } else if (ins.name == "END_HASH") {
        res = await this.appInstance.endHash(this.path);
        return res;
      } else {
        throw new Error("Unknown instruction name in the template");
      }
    }
  }
}
