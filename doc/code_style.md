# Code Style
1. no include guards, just `#pragma once`
2. All API functions should:
  - start with `dfk_` prefix
  - be listed in the dfk.exports file
3. Internal functions should:
  - start with `dfk__` prefix
4. Containers' iterators should have `_it` suffix
5. Containers' reverse iterators should have `_rit` suffix
6. Include order:
  - standard C library headers
  - thirdparty libraries
  - thirdparty libraries from `dfk/thirdparty`
  - dfk headers starting from most generic (like `list.h`) up to most concrete
    (like `middleware/mysql/client.h`)
  - internal dfk headers from src/include
7. Objects' `_free` methods should populate object with DEADBEEF data if
  `DFK_DEBUG` is enabled.
8. Public data members of structs should be declared *prior* to private to
  preserve binary compatibility when private members are
  added/removed/rearranged.
9. If more than 50% of object's methods need a pointer to `dfk_t`, then `dfk_t*`
  should be added to the object's data members.
