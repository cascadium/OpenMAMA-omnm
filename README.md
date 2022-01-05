# OpenMAMA Native Message (OMNM)

[![Join the chat at https://gitter.im/fquinner/OpenMAMA-omnm](https://badges.gitter.im/fquinner/OpenMAMA-omnm.svg)](https://gitter.im/fquinner/OpenMAMA-omnm?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

![Build and Test](https://github.com/cascadium/OpenMAMA-omnm/actions/workflows/main.yml/badge.svg)

## Overview

Pronounced *ominum* (because it's fun to say), this is the OpenMAMA Native
Messaging bridge. 

This project provides a dependency-free payload implementation for OpenMAMA so that it
can be used with middlewares which don't have their own payload format (e.g. ZeroMQ,
Nanomsg etc).

The payload implementation is high performance and self describing so it is quite flexible.
It also supports every interface which is supported by OpenMAMA, and passes every OpenMAMA
unit test currently available.

## Functionality

The payload is considered feature complete and includes:

* Serialization / Deserialization functions
* Apply methods
* Iterators (both callback based and iterator based)
* Scalar data type accessor and modification methods for both field and message
* Sub messages
* All vector types
* Binary and opaque data types
* String serialization functionality
* It may be **extended** to form a suitable base for other payload implementations

## Extending this Bridge

It is possible to use this payload as a base bridge simply by implementing:

* `<yourpayload>msgPayload_init`
* `<yourpayload>msgPayload_getType`
* `<yourpayload>msgPayload_create`
* `<yourpayload>msgPayload_destroy`
* `<yourpayload>msgPayload_copy`
* `<yourpayload>msgPayload_serialize`
* `<yourpayload>msgPayload_unSerialize`
* `<yourpayload>msgPayload_getByteBuffer`
* `<yourpayload>msgPayload_getByteSize`
* `<yourpayload>msgPayload_setByteBuffer`
* `<yourpayload>msgPayload_createFromByteBuffer`
* `<yourpayload>msgPayload_addVectorMsg`
* `<yourpayload>msgPayload_updateVectorMsg`

And passing through the API call to omnmsg for all other methods. As a working example, see  [our msgpack payload](https://github.com/cascadium/OpenMAMA-msgpack).

## Build Instructions

### Supported Platforms

We support all of the platforms currently supported by OpenMAMA. [The complete list can be found here](https://openmama.finos.org/openmama_supported_platforms.html).

### Dependencies

The bridge depends on:

* OpenMAMA 2.4.0+
* CMake 3.11+

### Building

The software can be build in the standard cmake way

    mkdir build
    cd build
    cmake .. -DMAMA_ROOT=/path/to/openmama
    make install

Which will install the bridge to the OpenMAMA root directory. If building on windows, instead of `make`, run `cmake --build . --target install`.

## Usage Instructions

Add the
directory containing this library to your `LD_LIBRARY_PATH` and set the
following property in your `mama.properties` configuration file:

    mama.payload.default=omnmmsg

## Related Projects

* [OpenMAMA](http://openmama.org)
* [ZeroMQ](http://zeromq.org)
* [OpenMAMA Msgpack](https://github.com/cascadium/OpenMAMA-msgpack)
* [OpenMAMA OZ ZeroMQ Middleware Bridge](https://github.com/nyfix/OZ)
* [OpenMAMA ZeroMQ Middleware Bridge (archived)](https://github.com/fquinner/OpenMAMA-zmq)

## More from Cascadium

If you want to see what else we're up to, you can head on over to [cascadium.io](https://cascadium.io). We offer professional support for all of our projects as well as bespoke development for your in-house needs.
