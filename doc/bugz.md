Unlinked bugs
=============

@bug Floating SIGSEGV in libpthread

Reported by: [Stanislav Ivochkin](https://github.com/ivochkin)

Occur only in valgrind builds.
Build examples:
* https://travis-ci.org/ivochkin/dfk/builds/140336836


@bug TCP socket error reporting to read/write functions

Reported by: [Stanislav Ivochkin](https://github.com/ivochkin)

dfk_tcp_socket_{write,read}v? functions should return -1 and set dfk.dfk_errno in the case of error
