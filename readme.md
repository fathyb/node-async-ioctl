# node-async-ioctl

Asynchronous `ioctl` for Node.js. Used for [`node-nbd-client`](https://github.com/fathyb/node-nbd-client), UNIX-only.

## Usage

-   `ioctl(fd: number | bigint, request: number | bigint, value: number | bigint | Buffer): Promise<number>`
    -   meant for `ioctl` calls returning immediately. Uses the libuv thread pool.
-   `ioctl.batch(batch: [fd: number | bigint, request: number | bigint, value: number | bigint | Buffer][]): Promise<number[]>`
    -   same as `ioctl(fd, request, value)` except requests can be submitted in batches.
-   `ioctl.blocking(fd: number | bigint, request: number | bigint, value: number | bigint | Buffer): Promise<number>`
    -   meant for `ioctl` calls blocking until something happens. Uses a dedicated thread so more memory and higher latency.

```js
import { open } from 'fs'
import { ioctl } from 'async-ioctl'

// Get number of rows for current terminal
const rows = await ioctl(process.stdout.fd, 0x2000ab00)

async function getBlockDeviceSize(path: string) {
    const device = await open(path)
    const buffer = Buffer.alloc(8)

    await ioctl(device.fd, 0x2000ab00, buffer)

    return buffer
}

// Buffer argument
const result = await ioctl(device.fd, 0x2000ab00, Buffer.from('test'))
// Buffer argument
const result = await ioctl(device.fd, 0x2000ab00, BigInt('13343554'))
```
