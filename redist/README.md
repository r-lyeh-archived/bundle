bundle/redist
=============

- This optional folder is used to regenerate the amalgamated distribution. Do not include it into your project.
- Regenerate the distribution by typing the following lines:
```
move /y bundle.hpp ..
deps\amalgamate -i deps\easylzma\src -p bundle.hpp -w "*.*pp;*.c;*.h" redist.cpp ..\bundle.cpp
copy /y ..\bundle.hpp
```
