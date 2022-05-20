import { resolve } from "path";
import { buf_to_hex } from "utils/serialize";
import { Fio } from "../fio";
import { ENCODING_STRING, ENCODING_HEX } from "utils/parse";

export default class Interpreter {
  appInstance: Fio;
  path: number[];

  constructor(appInstance: Fio, path: number[]) {
    this.appInstance = appInstance;
    this.path = path;
  }

  async interpret(template: any, values: any, level: number = 0) {
    for (let i = 0; i < template.instructions.length; i++) {
      const ins = template.instructions[i];
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
