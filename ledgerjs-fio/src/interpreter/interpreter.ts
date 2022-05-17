import { Fio } from "../fio";

export default class Interpreter {
  appInstance: Fio;
  path: number[];

  constructor(appInstance: Fio, path: number[]) {
    this.appInstance = appInstance;
    this.path = path;
  }

  async interpret(template: any, values: any, level: number = 0) {
    for (let i = 0; i < template.instructions.length; i++) {
      console.log("iteration");
      const ins = template.instructions[i];
      console.log("ins: ", ins);
      let res;
      console.log(ins.name);
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
        console.log(res);
        console.log(
          "GOING TO DO THIS MANY ITERATIONS: ",
          ins.iterations.length
        );
        for (let j = 0; j < ins.iterations.length; j++) {
          console.log("STARTING ITERATION ", j);
          res = await this.appInstance.startIteration();
          console.log(res);
          const it = ins.iterations[j];
          const name = it.name;
          await this.interpret(
            { instructions: it.instructions },
            values[key][name],
            level + 1
          );
          console.log("NOW:");
          res = await this.appInstance.endIteration(
            values[key].allowed_iter_hashes
          );
          console.log(res);
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
      } else {
        throw new Error("Unknown instruction name in the template");
      }
      console.log("Instruction response: ", res);
    }
    if (level == 0) {
      const endRes = await this.appInstance.endHash(this.path);
      return endRes;
    }
  }
}
