import native from '../native/build/Release/ioctl.node'

export type MaybeBigInt = number | bigint
export type MaybeBufferPointer = Buffer | MaybeBigInt

/**
 * Perform an `ioctl` syscall using the libuv thread pool.
 * Meant for immediate `ioctl` calls.
 **/
export async function ioctl(
    fd: MaybeBigInt,
    request: MaybeBigInt,
    data?: MaybeBufferPointer,
) {
    const [result] = await ioctl.batch([fd, request, data])

    return result
}

export namespace ioctl {
    export async function batch(
        ...calls: [
            fd: MaybeBigInt,
            request: MaybeBigInt,
            data?: MaybeBufferPointer,
        ][]
    ) {
        return await new Promise<number[]>((resolve, reject) => {
            try {
                native.nonBlocking(
                    calls.map(([fd, request, data]) => [
                        BigInt(fd),
                        BigInt(request),
                        pointer(data),
                    ]),
                    (results: [error: number, result: number][]) => {
                        try {
                            resolve(
                                results.map(([error, result]) => {
                                    if (result == -1) {
                                        const err: any = new Error(
                                            `Error ${error} running ioctl`,
                                        )

                                        err.code = error

                                        throw err
                                    } else {
                                        return result
                                    }
                                }),
                            )
                        } catch (error) {
                            reject(error)
                        }
                    },
                )
            } catch (error) {
                reject(error)
            }
        })
    }

    /**
     * Perform an `ioctl` syscall using a dedicated thread.
     * Meant for blocking/long-running `ioctl` calls.
     **/
    export async function blocking(
        fd: MaybeBigInt,
        request: MaybeBigInt,
        data?: MaybeBufferPointer,
    ) {
        return await new Promise<number>((resolve, reject) => {
            try {
                native.blocking(
                    BigInt(fd),
                    BigInt(request),
                    pointer(data),
                    complete(resolve, reject),
                )
            } catch (error) {
                reject(error)
            }
        })
    }
}

function complete(
    resolve: (result: number) => void,
    reject: (error: Error) => void,
) {
    return (error: number, result: number) => {
        if (result == -1) {
            const err: any = new Error(`Error ${error} running ioctl`)

            err.code = error

            reject(err)
        } else {
            resolve(result)
        }
    }
}

function pointer(ptr: undefined | MaybeBufferPointer) {
    if (ptr === undefined || ptr === null) {
        return BigInt(0)
    }

    if (typeof ptr === 'bigint' || Buffer.isBuffer(ptr)) {
        return ptr
    }

    if (typeof ptr === 'number') {
        return BigInt(ptr)
    }

    throw new Error('Unsupported ioctl pointer value')
}
