import {expect} from "chai"
import {parseTransaction} from "../../src/utils/parse"
import {Transaction} from "../../src/types/public"
import {InvalidDataReason} from "../../src/errors"

const chainId = "" //XXX

const validTx: Transaction = {
    expiration: "2021-08-28T12:50:36.686",
    ref_block_num: "0x1122",
    ref_block_prefix: "0x33445566",
    context_free_actions: [],
    actions: [{
        account: "fio.token",
        name: "trnsfiopubky",
        authorization: [{
            actor: "aftyershcu22",
            permission: "active",
        }],
        data: {
            payee_public_key: "FIO8PRe4WRZJj5mkem6qVGKyvNFgPsNnjNN6kPhh6EaCpzCVin5Jj",
            amount: "20",
            max_fee: "0x11223344",
            tpid: "rewards@wallet",
            actor: "aftyershcu22",
        },
    }],
    transaction_extensions: null,
}

describe("parse", () => {
    describe("parseTransaction", () => {
        it("successfully parse valid transaction", () => {
            expect(() => parseTransaction(chainId, validTx)).to.not.throw()
        })

        it("fail to parse incorrect expiration", () => {
            const invalidExpiration = JSON.parse(JSON.stringify(validTx))
            invalidExpiration.expiration = null
            expect(() => parseTransaction(chainId, invalidExpiration))
                .to.throw(InvalidDataReason.INVALID_EXPIRATION)
        })

        it("fail to parse incorrect ref_block_num", () => {
            const invalidRefBlockNum = JSON.parse(JSON.stringify(validTx))
            invalidRefBlockNum.ref_block_num = null
            expect(() => parseTransaction(chainId, invalidRefBlockNum))
                .to.throw(InvalidDataReason.INVALID_REF_BLOCK_NUM)
        })

        it("fail to parse incorrect ref_block_prefix", () => {
            const invalidRefBlockPrefix = JSON.parse(JSON.stringify(validTx))
            invalidRefBlockPrefix.ref_block_prefix = null
            expect(() => parseTransaction(chainId, invalidRefBlockPrefix))
                .to.throw(InvalidDataReason.INVALID_REF_BLOCK_PREFIX)
        })

        it("fail to parse when there are context free actions", () => {
            const someContextFreeActions = JSON.parse(JSON.stringify(validTx))
            someContextFreeActions.context_free_actions = ["A", "B"]
            expect(() => parseTransaction(chainId, someContextFreeActions))
                .to.throw(InvalidDataReason.CONTEXT_FREE_ACTIONS_NOT_SUPPORTED)
        })

        it("fail to parse when there are no actions", () => {
            const noActions = JSON.parse(JSON.stringify(validTx))
            noActions.actions = []
            expect(() => parseTransaction(chainId, noActions))
                .to.throw(InvalidDataReason.MULTIPLE_ACTIONS_NOT_SUPPORTED)
        })

        it("fail to parse when there is more than one action", () => {
            const moreThanOneAction = JSON.parse(JSON.stringify(validTx))
            moreThanOneAction.actions.push(moreThanOneAction.actions[0])
            expect(() => parseTransaction(chainId, moreThanOneAction))
                .to.throw(InvalidDataReason.MULTIPLE_ACTIONS_NOT_SUPPORTED)
        })

        it("fail to parse invalid account", () => {
            const invalidAccount = JSON.parse(JSON.stringify(validTx))
            invalidAccount.actions[0].account = null
            expect(() => parseTransaction(chainId, invalidAccount))
                .to.throw(InvalidDataReason.INVALID_ACCOUNT)
        })

        it("fail to parse invalid name", () => {
            const invalidName = JSON.parse(JSON.stringify(validTx))
            invalidName.actions[0].name = null
            expect(() => parseTransaction(chainId, invalidName))
                .to.throw(InvalidDataReason.INVALID_NAME)
        })

        it("fail to parse when there is no authorization", () => {
            const noAuthorization = JSON.parse(JSON.stringify(validTx))
            noAuthorization.actions[0].authorization = []
            expect(() => parseTransaction(chainId, noAuthorization))
                .to.throw(InvalidDataReason.MULTIPLE_AUTHORIZATION_NOT_SUPPORTED)
        })

        it("fail to parse when there is more than one authorization", () => {
            const moreThanOneAuthorization = JSON.parse(JSON.stringify(validTx))
            moreThanOneAuthorization.actions[0].authorization
                .push(moreThanOneAuthorization.actions[0].authorization[0])
            expect(() => parseTransaction(chainId, moreThanOneAuthorization))
                .to.throw(InvalidDataReason.MULTIPLE_AUTHORIZATION_NOT_SUPPORTED)
        })

        it("fail to parse invalid actor", () => {
            const invalidActor = JSON.parse(JSON.stringify(validTx))
            invalidActor.actions[0].authorization[0].actor = null
            expect(() => parseTransaction(chainId, invalidActor))
                .to.throw(InvalidDataReason.INVALID_ACTOR)
        })

        it("fail to parse invalid permission", () => {
            const invalidPermission = JSON.parse(JSON.stringify(validTx))
            invalidPermission.actions[0].authorization[0].permission = null
            expect(() => parseTransaction(chainId, invalidPermission))
                .to.throw(InvalidDataReason.INVALID_PERMISSION)
        })

        it("fail to parse invalid payee public key", () => {
            const invalidPayeePublicKey = JSON.parse(JSON.stringify(validTx))
            invalidPayeePublicKey.actions[0].data.payee_public_key = null
            expect(() => parseTransaction(chainId, invalidPayeePublicKey))
                .to.throw(InvalidDataReason.INVALID_PAYEE_PUBKEY)
        })

        it("fail to parse invalid amount", () => {
            const invalidAmount = JSON.parse(JSON.stringify(validTx))
            invalidAmount.actions[0].data.amount = null
            expect(() => parseTransaction(chainId, invalidAmount))
                .to.throw(InvalidDataReason.INVALID_AMOUNT)
        })

        it("fail to parse invalid max fee", () => {
            const invalidMaxFee = JSON.parse(JSON.stringify(validTx))
            invalidMaxFee.actions[0].data.max_fee = null
            expect(() => parseTransaction(chainId, invalidMaxFee))
                .to.throw(InvalidDataReason.INVALID_MAX_FEE)
        })

        it("fail to parse invalid tpid", () => {
            const invalidTpid = JSON.parse(JSON.stringify(validTx))
            invalidTpid.actions[0].data.tpid = null
            expect(() => parseTransaction(chainId, invalidTpid))
                .to.throw(InvalidDataReason.INVALID_TPID)
        })

        it("fail to parse invalid actor in action data", () => {
            const invalidActionDataActor = JSON.parse(JSON.stringify(validTx))
            invalidActionDataActor.actions[0].data.actor = null
            expect(() => parseTransaction(chainId, invalidActionDataActor))
                .to.throw(InvalidDataReason.INVALID_ACTOR)
        })
    })
})
