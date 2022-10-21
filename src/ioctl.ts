import native from '../native/build/Release/ioctl.node'

/**
 * Perform an `ioctl` syscall using the libuv thread pool.
 * Meant for immediate `ioctl` calls.
 **/
export async function ioctl(
    fd: number,
    request: number | bigint,
    data?: number | bigint | Buffer,
) {
    const [result] = await ioctl.batch([fd, request, data])

    return result
}

const BaseError = Error

export namespace ioctl {
    export class Error extends BaseError {
        public readonly fd: number
        public readonly code: number
        public readonly request: bigint

        constructor({
            fd,
            code,
            request,
        }: {
            fd: number
            code: number
            request: number | bigint
        }) {
            super(`error ${code} running ioctl(${fd}, ${request})`)

            this.fd = fd
            this.code = code
            this.request = BigInt(request)
        }
    }

    export async function batch(
        ...calls: [
            fd: number,
            request: number | bigint,
            data?: number | bigint | Buffer,
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
                    (results: [code: number, result: number][]) => {
                        try {
                            resolve(
                                results.map(([code, result], index) => {
                                    if (code != 0 || result == -1) {
                                        const [fd, request] = calls[index]

                                        throw new Error({ fd, code, request })
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
        fd: number,
        request: number | bigint,
        data?: number | bigint | Buffer,
    ) {
        return await new Promise<number>((resolve, reject) => {
            try {
                const requestInt = BigInt(request)

                native.blocking(
                    BigInt(fd),
                    requestInt,
                    pointer(data),
                    (code: number, result: number) => {
                        if (code != 0 || result == -1) {
                            reject(new Error({ fd, code, request: requestInt }))
                        } else {
                            resolve(result)
                        }
                    },
                )
            } catch (error) {
                reject(error)
            }
        })
    }
}

function pointer(ptr: undefined | number | bigint | Buffer) {
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
