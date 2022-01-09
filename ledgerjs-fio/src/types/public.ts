/**
 * Type for 64-bit integers.
 *
 * We accept either
 * - `Number` (if it is less than Number.MAX_SAFE_INTEGER)
 * - `String` (representing 64-bit number)
 * - `BigInt` (if platform supports it natively)
 * @category Basic types
 */
export type bigint_like = number | bigint | string

// Blockchain-defined constants

/**
 * Hardened derivation
 *
 * @example
 * ```
 * const accountId = 0 + HARDENED
 * ```
 *
 * @see [[BIP32Path]]
 */
export const HARDENED = 0x80000000

// Our types

/**
 * Device app flags
 * @category Basic types
 * @see [[Version]]
 */
export type Flags = {
    isDebug: boolean
}

export type DeviceCompatibility = {
    /** Overall compatibility.
     * - true if SDK supports the device with given firmware version (to the
     *   extent of the features supported by the firmware itself)
     * - false if SDK refuses to communicate with current device version
     */
    isCompatible: boolean
    /**
     * In case there are some compatibility problem, SDK recommended version.
     * Clients of SDK should check whether this is null and if not, urge users to upgrade.
     */
    recommendedVersion: string | null
}

/**
 * Device app version
 * @category Basic types
 * @see [[Fio.getVersion]]
 * @see [[DeviceCompatibility]]
 */
export type Version = {
    major: number
    minor: number
    patch: number
    flags: Flags
};

/**
 * Response to [[Fio.getSerial]] call
 * @category Basic types
 */
export type Serial = {
    /**
     * Serial is a Ledger device identifier.
     * It is 7 bytes long, which is represented here as 14-character hex string
     */
    serial: string
};

export type Init = {
    ret: string
};

export type End = SignedTransactionData

export type InitAction = {
    ret: string
}

export type EndAction = {
    ret: string
}

export type SendData= {
    ret: string
}

export type StartFor = {
    ret: string
}

export type EndFor = {
    ret: string
}

/**
 * Represents BIP 32 path.
 *
 * @example
 * ```
 *  const HD = HARDENED
 *  const Address = [44 + HD, 1815 + HD, 0 + HD, 0, 0];
 * ```
 *
 * @see [[HARDENED]]
 * @category Basic types
 */
export type BIP32Path = Array<number>

/**
 * Derived  public key
 * @category Basic types
 * @see [[Fio.getPublicKey]]
 */
export type PublicKey = {
    publicKeyHex: string
}


/**
 * Transaction witness.
 * @see [[SignedTransactionData]]
 * @category Basic types
 */
export type Witness = {
    /**
     * Witnessed path
     */
    path: BIP32Path
    /**
     * Note: this is *only* a signature.
     * You need to add proper extended public key to form a full witness
     */
    witnessSignatureHex: string
};

/**
 * Result of signing a transaction.
 * @category Basic types
 * @see [[Fio.signTransaction]]
 */
export type SignedTransactionData = {
    /**
     * Hash of signed transaction. Callers should check that they serialize tx the same way
     */
    txHashHex: string
    /**
     * List of witnesses. Caller should assemble full transaction to be submitted to the network.
     */
    witness: Witness
};


/**
 * Represents Transfer FIO Tokens trnsfiopubkey data.
 * @category Basic types
 * @see [[Action]]
 */
export type TransferFIOTokensData = {
    payee_public_key: string
    amount: string
    max_fee: string
    tpid: string
    actor: string
}


/**
 * Represents authorisation in transaction Actions.
 * @category Basic types
 * @see [[Action]]
 */
export type ActionAuthorisation = {
    actor: string
    permission: string
}


/**
 * Represents actions in the transaction.
 * @category Basic types
 * @see [[Transaction]]
 */
export type Action = {
    account: string
    name: string
    authorization: Array<ActionAuthorisation>
    data: | TransferFIOTokensData
}


/**
 * Represents transaction to be signed by the device.
 * Note that this represents a *superset* of what Ledger can sign due to certain hardware app/security limitations.
 * @category Basic types
 * @see [[Fio.signTransaction]]
 */
export type Transaction = {
    expiration: string
    ref_block_num: string
    ref_block_prefix: string
    context_free_actions: Array<Action>
    actions: Array<Action>
    transaction_extensions: null
}
