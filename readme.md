# node-async-ioctl

Asynchronous `ioctl` for Node.js. Used for `node-nbd-client`, UNIX-only.

## Usage

Two APIs are provided:

-   `ioctl()`: meant for `ioctl` calls returning immediately. Uses the libuv thread pool.
-   `ioctl.blocking()`: meant for `ioctl` calls blocking until something happens. Uses a dedicated thread so more memory and higher latency.

```js
import { open } from 'fs'
import { ioctl } from 'async-ioctl'

const device = await open('/dev/nbd0')

// No argument
const result = await ioctl(device.fd, 0x2000ab00)
// Buffer argument
const result = await ioctl(device.fd, 0x2000ab00, Buffer.from('test'))
// Buffer argument
const result = await ioctl(device.fd, 0x2000ab00, BigInt('13343554'))
```
