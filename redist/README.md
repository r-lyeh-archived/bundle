bundle/redist
=============

- This optional folder is used to regenerate the amalgamated distribution. Do not include it into your project.
- Regenerate the distribution by typing the following lines:
```
move /y bundle.h ..
deps\amalgamate -i deps\easylzma\src -i deps\brotli -w "*.c*;*.h*;*.inc" redist.cpp ..\bundle.cpp
deps\fart.exe -- ..\bundle.cpp "#line" "//#line"
deps\fart.exe -- ..\bundle.cpp "#pragma once" "//#pragma once"
copy /y ..\bundle.h
```
