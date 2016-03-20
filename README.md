# OpenMAMA Native Message (OMNM)

## Overview

Pronounced *ominum* (because it's fun to say), this is the OpenMAMA Native
Messaging bridge. This project really only exists to provide a reasonably
efficient middleware which can be used for testing ZeroMQ. If you look at
the code you'll see I have tried my best to focus on methods which reduce
the number of lines of code which are required to maintain.

In future if people are using this, I may expand the functionality out some
more, but in the meantime, it focuses entirely on scalar data types.

*NB: This project is MIT Licensed and in no way affiliated with nor supported
by the OpenMAMA project or SR Labs - if you find any issues, please report to
this project via github.*

## Functionality

The payload functionality is currently limited to:

* Serialization / Deserialization functions
* Apply methods
* Iterators (both callback based and iterator based)
* Scalar data type accessor and modification methods for both field and message

There are several known limitations which exist. Some by design, some I'll
maybe correct later:

* Not network byte ordered so you can't cross CPU architectures from pub to sub
* Doesn't support any vector data types
* Doesn't support sub messages
* Doesn't support toString functionality
* Doesn't support binary / opaque data types

If you really want to overcome these limitations, let me know or feel free to
contribute an implementation yourself.

## Build Instructions

*NB: This is very much in development and I will always be developing on the
latest version of Fedora. If I have broken an OS that you use, please let me
know.*

### Supported Platforms

* RHEL / CentOS 5
* RHEL / CentOS 6
* Windows (coming soon)

### Dependencies

The bridge depends on:

* MAMA / OpenMAMA
* Scons

As of the latest version of OpenMAMA, there is no longer a requirement to
build this library off my own special fork of OpenMAMA. Instead thanks to
dynamic bridge loading support, you can now build this off:

* The next branch of OpenMAMA
* The OpenMAMA-2.4.0 branch of OpenMAMA

The master branch will also contain the correct changes once the next OpenMAMA
GA release is issued which is expected within the next couple of weeks.

### Building

If you have all the prerequisites, the build process should be pretty
straightforward:

    scons --with-mamasource=PATH --with-mamainstall=PATH --with-gtest=PATH

## Usage Instructions

After building, you will have a `libmamaomnmmsgimpl.so` library created. Add the
directory containing this library to your `LD_LIBRARY_PATH` and set the
following property in your `mama.properties` configuration file:

    mama.payload.default=omnmmsg

## Related Projects

* [OpenMAMA](http://openmama.org)
* [ZeroMQ](http://zeromq.org)
* [OpenMAMA ZeroMQ Middleware Bridge](https://github.com/fquinner/OpenMAMA-zmq)

## Blog

If you're interested in the thought process behind this or the ramblings of the
author, you can shoot on over to [my blog page](http://fquinner.github.io).
