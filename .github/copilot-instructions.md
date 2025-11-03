# AmLang SSL/TLS Library (am-ssl)

This repository contains the SSL/TLS (Secure Sockets Layer/Transport Layer Security) library for the AmLang programming language ecosystem. AmLang is a modern object-oriented programming language designed for systems programming with cross-platform compatibility, particularly targeting niche platforms like AmigaOS, MorphOS, and legacy systems.

## Repository Overview

**am-ssl** provides secure network communication capabilities for AmLang applications through SSL/TLS encryption. The library wraps platform-specific SSL implementations (OpenSSL on modern systems, AmiSSL on AmigaOS) to provide encrypted socket streams that integrate seamlessly with AmLang's I/O system.

### Key Features

- **SSL/TLS Encryption**: Secure network communication using industry-standard protocols
- **Stream Integration**: `SslSocketStream` extends AmLang's base `Stream` class
- **Certificate Verification**: Support for trusted CA certificate validation
- **HTTPS Support**: Enables secure HTTP communication for web clients and servers
- **Cross-Platform**: OpenSSL on Linux/macOS, AmiSSL on Amiga platforms
- **Memory Efficient**: Designed for embedded and resource-constrained environments

### Core Classes

- **`SslSocketStream`**: Main SSL socket stream class for encrypted I/O operations

## API Usage Patterns

### Basic HTTPS Client

```amlang
import Am.Lang
import Am.IO
import Am.Net
import Am.Net.Ssl

// Create socket and connect to HTTPS server
var socket = Socket.create(AddressFamily.inet, SocketType.stream, ProtocolFamily.tcp)
socket.connect("github.com", 443)

// Create SSL stream with hostname for certificate verification
var sslStream = new SslSocketStream(socket, "github.com")

// Use TextStream for easy text I/O over SSL
var textStream = new TextStream(sslStream)

// Send HTTP request
textStream.writeString("GET / HTTP/1.1\r\n")
textStream.writeString("Host: github.com\r\n")
textStream.writeString("Connection: close\r\n")
textStream.writeString("\r\n")

// Read response
var response = textStream.readString()
response.println()

socket.close()
```

### HTTPS POST Request

```amlang
import Am.Json
import Am.Util.Parsers.Text

// Create SSL connection
var socket = Socket.create(AddressFamily.inet, SocketType.stream, ProtocolFamily.tcp)
socket.connect("api.example.com", 443)
var sslStream = new SslSocketStream(socket, "api.example.com")
var textStream = new TextStream(sslStream)

// Prepare JSON payload
var jsonData = "{\"message\":\"Hello, secure world!\"}"

// Send HTTPS POST request
textStream.writeString("POST /api/data HTTP/1.1\r\n")
textStream.writeString("Host: api.example.com\r\n")
textStream.writeString("Content-Type: application/json\r\n")
textStream.writeString("Content-Length: ${jsonData.length()}\r\n")
textStream.writeString("\r\n")
textStream.writeString(jsonData)

// Read response
var response = textStream.readString()
response.println()

socket.close()
```

### Secure API Client

```amlang
class SecureApiClient {
    import Am.Net
    import Am.Net.Ssl
    import Am.IO
    import Am.Lang
    
    private var hostname: String
    private var port: Int
    
    fun makeSecureRequest(path: String): String {
        var socket = Socket.create(AddressFamily.inet, SocketType.stream, ProtocolFamily.tcp)
        
        try {
            socket.connect(this.hostname, this.port)
            var sslStream = new SslSocketStream(socket, this.hostname)
            var textStream = new TextStream(sslStream)
            
            // Send request
            textStream.writeString("GET ${path} HTTP/1.1\r\n")
            textStream.writeString("Host: ${this.hostname}\r\n")
            textStream.writeString("User-Agent: AmLang-SSL-Client/1.0\r\n")
            textStream.writeString("\r\n")
            
            // Read response
            return textStream.readString()
            
        } finally {
            socket.close()
        }
    }
}
```

## Project Structure

```
src/am-lang/Am/Net/Ssl/
└── SslSocketStream.aml      # Main SSL socket stream implementation

src/native-c/
├── libc/Am/Net/Ssl/        # Common OpenSSL implementation
├── linux-x64/Am/Net/Ssl/   # Linux-specific SSL code
├── macos/Am/Net/Ssl/       # macOS-specific SSL code
├── amigaos/Am/Net/Ssl/     # AmigaOS AmiSSL implementation
└── morphos-ppc/Am/Net/Ssl/ # MorphOS-specific SSL code

c-libs/
├── openssl/               # OpenSSL libraries (macOS/Linux)
└── amissl/               # AmiSSL libraries (AmigaOS)

examples/
└── ssl-test/             # Complete SSL client example
    ├── src/am-lang/SslTest/Program.aml
    ├── Makefile
    └── README.md
```

## Building and Testing

### Prerequisites

- AmLang compiler (`amlc`) v0.6.1 or later
- AmLang core library (`am-lang-core`) as dependency
- AmLang networking library (`am-net`) as dependency
- Platform-specific SSL libraries:
  - **Linux**: OpenSSL development packages (`libssl-dev`)
  - **macOS**: OpenSSL via Homebrew or system
  - **AmigaOS**: AmiSSL libraries
  - **MorphOS**: Cross-compilation with SSL support

### macOS Setup

```bash
# For Intel Macs
cd c-libs
./create_openssl_link_mac.sh

# For Apple Silicon Macs  
cd c-libs
./create_openssl_link_mac_silicon.sh
```

### Build Commands

```bash
# Build for current platform
java -jar amlc.jar build . -bt linux-x64

# Build for AmigaOS (requires Docker and AmiSSL)
java -jar amlc.jar build . -bt amigaos_docker

# Build for MorphOS (requires Docker)
java -jar amlc.jar build . -bt morphos-ppc_docker

# Build examples
cd examples/ssl-test
make build
```

### Testing

```bash
# Test SSL functionality
cd examples/ssl-test
make test

# Manual SSL test
cd examples/ssl-test/builds/bin/linux-x64
./app
```

## Development Guidelines

### Critical SSL Usage Patterns

1. **Always Verify Hostname**:
   ```amlang
   // CORRECT: Provide hostname for certificate verification
   var sslStream = new SslSocketStream(socket, "api.example.com")
   
   // RISKY: No hostname verification
   var sslStream = new SslSocketStream(socket, "")
   ```

2. **Resource Management**:
   ```amlang
   var socket = Socket.create(...)
   var sslStream: SslSocketStream? = null
   
   try {
       socket.connect("secure.example.com", 443)
       sslStream = new SslSocketStream(socket, "secure.example.com")
       // Use sslStream...
   } finally {
       socket.close()  // Always close underlying socket
   }
   ```

3. **Error Handling**:
   ```amlang
   try {
       var sslStream = new SslSocketStream(socket, hostname)
       // SSL operations...
   } catch (e: Exception) {
       "SSL error: ${e.message}".println()
       socket.close()
   }
   ```

### Platform-Specific SSL Libraries

- **Linux**: Uses system OpenSSL (`-lssl -lcrypto`)
- **macOS**: Uses Homebrew OpenSSL or system libraries
- **AmigaOS**: Uses AmiSSL (`-lamisslstubs -lamisslauto`)
- **MorphOS**: Cross-compiled with SSL support
- **AROS**: Limited SSL support

### Certificate Verification

```amlang
// The hostname parameter enables automatic certificate verification
var sslStream = new SslSocketStream(socket, "trusted.example.com")

// For development/testing only - avoid in production
var sslStream = new SslSocketStream(socket, "")  // Skips verification
```

## Platform Support

| Platform | Status | SSL Library | Certificate Verification |
|----------|---------|-------------|-------------------------|
| Linux x64 | ✅ Full | OpenSSL | ✅ Full CA chain |
| macOS x64 | ✅ Full | OpenSSL | ✅ Full CA chain |
| macOS ARM64 | ✅ Full | OpenSSL | ✅ Full CA chain |
| AmigaOS 68k | ✅ Full | AmiSSL | ✅ AmiSSL CA support |
| MorphOS PPC | 🚧 Partial | Cross-compiled | 🚧 Limited |
| AROS x86-64 | 🚧 Partial | Cross-compiled | 🚧 Limited |
| Windows x64 | ❌ None | - | - |

## Dependencies

- **am-lang-core**: Core AmLang standard library
  - `Am.Lang.*`: Basic types and functionality
  - `Am.IO.Stream`: Base stream interface for SslSocketStream
  
- **am-net**: AmLang networking library
  - `Am.Net.Socket`: Underlying socket implementation
  - `Am.Net.AddressFamily`, `Am.Net.SocketType`: Socket configuration
  
- **Platform SSL Libraries**:
  - OpenSSL 3.x (Linux/macOS)
  - AmiSSL (AmigaOS)

## SSL/TLS API Reference

### SslSocketStream Constructor

```amlang
// Create SSL stream over existing socket
new SslSocketStream(socket: Socket, hostname: String)
```

**Parameters**:
- `socket`: Connected TCP socket from am-net
- `hostname`: Server hostname for certificate verification

### Stream Methods

```amlang
// Read encrypted data
read(buffer: Byte[], offset: Long, length: Int): Int

// Write encrypted data  
write(buffer: Byte[], offset: Long, length: Int)
```

### Integration with TextStream

```amlang
var sslStream = new SslSocketStream(socket, hostname)
var textStream = new TextStream(sslStream)

// Now use text-based I/O over SSL
textStream.writeString("Hello, secure world!")
var response = textStream.readString()
```

## Common Issues and Solutions

### SSL Handshake Failed
**Problem**: "SSL handshake failed" during connection
**Solution**: Verify hostname matches certificate, check SSL library installation

### Certificate Verification Failed
**Problem**: Certificate verification errors
**Solution**: Ensure correct hostname, check CA certificate bundle, verify server certificate

### OpenSSL Library Not Found (macOS)
**Problem**: "dylib not found" errors on macOS
**Solution**: Run appropriate `create_openssl_link_*.sh` script in `c-libs/`

### AmiSSL Not Available (AmigaOS)
**Problem**: AmiSSL functions not found
**Solution**: Install AmiSSL on target system, verify library linking

### Memory Issues with Large Transfers
**Problem**: High memory usage during large file transfers
**Solution**: Use smaller buffer sizes, implement streaming for large data

## Security Considerations

1. **Certificate Verification**: Always provide correct hostname for certificate verification
2. **Error Handling**: Don't expose SSL errors that might reveal system information
3. **Memory Management**: Clear sensitive data from memory when possible
4. **Protocol Versions**: Uses system SSL library default (typically TLS 1.2+)
5. **Cipher Suites**: Relies on system SSL configuration for secure defaults

## Examples and Learning Resources

1. **ssl-test Example**: Complete HTTPS client implementation
2. **SSL Stream Documentation**: Inline documentation in SslSocketStream.aml
3. **Integration Examples**: Using SSL with TextStream and JSON APIs

## Contributing Guidelines

1. **Security First**: Never compromise on certificate verification or encryption strength
2. **Cross-Platform Compatibility**: Test SSL functionality on multiple platforms  
3. **Memory Safety**: Ensure proper cleanup of SSL contexts and buffers
4. **Performance**: Consider SSL overhead on embedded/legacy platforms
5. **Documentation**: Update examples for SSL/TLS best practices

## Version Compatibility

- **AmLang Compiler**: Requires v0.6.1+ for proper native SSL support
- **am-lang-core**: Must be compatible version with Stream interface
- **am-net**: Requires compatible Socket implementation
- **SSL Libraries**: OpenSSL 1.1.1+ or AmiSSL 4.0+

## License

This library is licensed under the Apache License, Version 2.0, maintaining compatibility with OpenSSL licensing.

---

This library is part of the AmLang ecosystem. For broader context and cross-repository development, see the main AmLang workspace documentation.