Staticlibs WebSocket library
============================

[![travis](https://travis-ci.org/staticlibs/staticlib_websocket.svg?branch=master)](https://travis-ci.org/staticlibs/staticlib_websocket)
[![appveyor](https://ci.appveyor.com/api/projects/status/github/staticlibs/staticlib_websocket?svg=true)](https://ci.appveyor.com/project/staticlibs/staticlib-websocket)

This project is a part of [Staticlibs](http://staticlibs.net/).

This library implements primitives (frame and handshake) for [RFC 6455 - The WebSocket Protocol](https://tools.ietf.org/html/rfc6455).

This library is header-only and depends on [staticlib_crypto](https://github.com/staticlibs/staticlib_crypto)
(that required OpenSSL) and [staticlib_endian](https://github.com/staticlibs/staticlib_endian).

Link to the [API documentation](http://staticlibs.github.io/staticlib_websocket/docs/html/namespacestaticlib_1_1websocket.html).

License information
-------------------

This project is released under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).

Changelog
---------

**2018-06-21**

 * version 1.0.1
 * windows build fix

**2018-06-20**

 * version 1.0.0
 * initial public version
