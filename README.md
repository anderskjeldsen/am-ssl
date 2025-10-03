# am-ssl

SSL/TLS library for the Am programming language.

## Overview

`am-ssl` is a library that provides SSL/TLS socket stream functionality for applications written in the Am programming language. It wraps OpenSSL (or AmiSSL on AmigaOS) to provide secure network communication capabilities through the `SslSocketStream` class.

## Language

The source code is written in the Am programming language and is compiled using [@anderskjeldsen/am-lang-compiler](https://github.com/anderskjeldsen/am-lang-compiler).

## Features

- SSL/TLS socket stream implementation
- Secure HTTP/HTTPS communication support
- Integration with the Am I/O stream hierarchy
- Multi-platform support (Linux, macOS, AmigaOS, MorphOS, AROS)
- Certificate verification with trusted CA certificates

## Dependencies

This library requires the following dependencies:

- [am-lang-core](https://github.com/anderskjeldsen/am-lang-core) - Core language features
- [am-net](https://github.com/anderskjeldsen/am-net) - Networking primitives
- OpenSSL 3.x (or AmiSSL on AmigaOS platforms)

## Installation

Add this library as a dependency in your `package.yml`:

```yaml
dependencies:
  - id: am-ssl
    realm: github
    type: git-repo
    tag: latest
    url: https://github.com/anderskjeldsen/am-ssl.git
```

## Usage

Here's a simple example that connects to a server over HTTPS:

```am
namespace SslTest {
    class Program {
        import Am.Lang
        import Am.IO
        import Am.Net
        import Am.Net.Ssl

        static fun main() {
            "connect to github".println()
            var s = Socket.create(AddressFamily.inet, SocketType.stream, ProtocolFamily.unspecified)
            s.connect("github.com", 443)
            "connected".println()

            var sslStream = new SslSocketStream(s, "github.com")
            "ssl stream created".println()

            var ts = new TextStream(sslStream)
            ts.writeString("GET / HTTP/1.1\r\nHost:github.com\r\n\r\n")

            var rs = ts.readString()
            rs.println()        
            s.close()    
        }
    }
}
```

For a complete working example, see the [ssl-test example](examples/ssl-test).

## Supported Platforms

- **Linux x64** - Using OpenSSL
- **macOS** (Intel and Apple Silicon) - Using OpenSSL
- **AmigaOS** - Using AmiSSL
- **MorphOS PPC** - Cross-compiled support
- **AROS x86-64** - Cross-compiled support

## Building

### Requirements

- Am language compiler (amlc.jar)
- Platform-specific C compiler (gcc, m68k-amigaos-gcc, etc.)
- OpenSSL development libraries (for Linux/macOS)

### macOS Setup

On macOS, you need to create a symbolic link to your OpenSSL installation:

```bash
# For Intel Macs
cd c-libs
./create_openssl_link_mac.sh

# For Apple Silicon Macs
cd c-libs
./create_openssl_link_mac_silicon.sh
```

### Building from Examples

```bash
cd examples/ssl-test
make build
```

Or run tests:

```bash
make test
```

## API

### SslSocketStream

The main class providing SSL/TLS functionality:

```am
class SslSocketStream(socket: Socket, hostName: String) : Stream
```

**Methods:**
- `read(buffer: Byte[], offset: Long, length: Int): Int` - Read data from the SSL stream
- `write(buffer: Byte[], offset: Long, length: Int)` - Write data to the SSL stream

The `SslSocketStream` extends the `Stream` class, making it compatible with other Am I/O classes like `TextStream`.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

## Contributing

This project is part of the Am programming language ecosystem. For issues or contributions, please refer to the main Am language repositories.
