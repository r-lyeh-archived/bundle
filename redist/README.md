bundle/redist
=============

- This optional folder is used to regenerate the amalgamated distribution. Do not include it into your project.
- Regenerate the distribution by typing the following lines:
```
deps\amalgamate -w "*.*pp;*.c;*.h" redist.cpp ..\bundle.cpp
deps\amalgamate -w "*.*pp;*.c;*.h" redist.hpp ..\bundle.hpp
```
