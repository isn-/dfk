# Code Style
* no include guards, just `#pragma once`
* All API functions should:
  - start with `dfk_` prefix
  - be listed in the dfk.exports file
* Internal functions should:
  - start with `dfk__` prefix
* Containers' iterators should have `_it` suffix
* Containers' reverse iterators should have `_rit` suffix
* Include order:
  - standard C library headers
  - thirdparty libraries
  - thirdparty libraries from `dfk/thirdparty`
  - dfk headers starting from most generic (like `list.h`) up to most concrete
    (like `middleware/mysql/client.h`)
* Objects' `_free` methods should populate object with DEADBEEF data if
  `DFK_DEBUG` is enabled.
* Public data members of structs should be declared *prior* to private to
  preserve binary compatibility when private members are
  added/removed/rearranged.
