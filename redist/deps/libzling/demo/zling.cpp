/**
 * zling:
 *  light-weight lossless data compression utility.
 *
 * Copyright (C) 2012-2013 by Zhang Li <zhangli10 at baidu.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @author zhangli10<zhangli10@baidu.com>
 * @brief  zling demo.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define __STDC_FORMAT_MACROS

#ifdef _MSC_VER
#include "msinttypes/stdint.h"
#include "msinttypes/inttypes.h"

#elif HAS_CXX11_SUPPORT
#include <cstdint>
#include <cinttypes>
#else
#include <stdint.h>
#include <inttypes.h>
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <fcntl.h>  // setmode()
#include <io.h>
#endif

#include "libzling/libzling.h"

static inline uint32_t ComputeAdler32(unsigned char* data, size_t size) {
    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < size; i++) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

#define ENABLE_ADLER32_CHECKSUM 0

struct DemoActionHandler: baidu::zling::ActionHandler {
    DemoActionHandler() {
        m_clockstart = clock();
    }
    void OnInit() {
        m_inputter  = dynamic_cast<baidu::zling::FileInputter*>(GetInputter());
        m_outputter = dynamic_cast<baidu::zling::FileOutputter*>(GetOutputter());
    }

    void OnDone() {
        if (m_inputter->IsErr() || m_outputter->IsErr()) {
            fprintf(stderr, "I/O error during encode/decode.\n");
            fflush(stderr);
            return;
        }

        const char* encode_message;
        const char* encode_direction;
        uint64_t isize;
        uint64_t osize;
        double cost_seconds = double(clock() - m_clockstart) / CLOCKS_PER_SEC;

        if (IsEncode()) {
            encode_message = "encode";
            encode_direction = "=>";
            isize = m_inputter->GetInputSize();
            osize = m_outputter->GetOutputSize();
        } else {
            encode_message = "decode";
            encode_direction = "<=";
            isize = m_outputter->GetOutputSize();
            osize = m_inputter->GetInputSize();
        }
        fprintf(stderr, "%s: %"PRIu64" %s %"PRIu64", time=%.3f sec, speed=%.3f MB/sec\n",
                encode_message,
                isize,
                encode_direction,
                osize,
                cost_seconds,
                isize / cost_seconds / 1e6);
        fflush(stderr);
    }

    void OnProcess(unsigned char* orig_data, size_t orig_size) {
        const char* encode_direction;
        uint64_t isize;
        uint64_t osize;
        double cost_seconds = double(clock() - m_clockstart) / CLOCKS_PER_SEC;

        // adler32 checksum
#if ENABLE_ADLER32_CHECKSUM
        if (IsEncode()) {
            m_outputter->PutUInt32(ComputeAdler32(orig_data, orig_size));
        } else {
            if (m_inputter->GetUInt32() != ComputeAdler32(orig_data, orig_size)) {
                throw std::runtime_error("baidu::zling::Decode(): adler32 checksum not match.");
            }
        }
#endif

        if (IsEncode()) {
            encode_direction = "=>";
            isize = m_inputter->GetInputSize();
            osize = m_outputter->GetOutputSize();
        } else {
            encode_direction = "<=";
            isize = m_outputter->GetOutputSize();
            osize = m_inputter->GetInputSize();
        }
        fprintf(stderr, "%6.2f MB %s %6.2f MB %.2f%%, %.3f sec, speed=%.3f MB/sec\n",
                isize / 1e6,
                encode_direction,
                osize / 1e6,
                osize * 1e2 / isize,
                cost_seconds,
                isize / cost_seconds / 1e6);
        fflush(stderr);
    }

private:
    baidu::zling::FileInputter*  m_inputter;
    baidu::zling::FileOutputter* m_outputter;
    clock_t m_clockstart;
};

int main(int argc, char** argv) {
    baidu::zling::FileInputter  inputter(stdin);
    baidu::zling::FileOutputter outputter(stdout);
    DemoActionHandler demo_handler;

#if defined(__MINGW32__) || defined(__MINGW64__)
    setmode(fileno(stdin),  O_BINARY);  // set stdio to binary mode for windows
    setmode(fileno(stdout), O_BINARY);
#endif

    // welcome message
    fprintf(stderr, "zling:\n");
    fprintf(stderr, "   light-weight lossless data compression utility\n");
    fprintf(stderr, "   by Zhang Li <zhangli10 at baidu.com>\n");
    fprintf(stderr, "\n");

    // zling <e/d> __argv2__ __argv3__
    if (argc == 4) {
        if (freopen(argv[3], "wb", stdout) == NULL) {
            fprintf(stderr, "error: cannot open file '%s' for write.\n", argv[3]);
            return -1;
        }
        argc = 3;
    }

    // zling <e/d> __argv2__ (stdout)
    if (argc == 3) {
        if (freopen(argv[2], "rb", stdin) == NULL) {
            fprintf(stderr, "error: cannot open file '%s' for read.\n", argv[2]);
            return -1;
        }
        argc = 2;
    }

    // zling <e/d> (stdin) (stdout)
    try {
        if (argc == 2 && strcmp(argv[1], "e4") == 0) {
            return baidu::zling::Encode(&inputter, &outputter, &demo_handler, 4);
        }
        if (argc == 2 && strcmp(argv[1], "e3") == 0) {
            return baidu::zling::Encode(&inputter, &outputter, &demo_handler, 3);
        }
        if (argc == 2 && strcmp(argv[1], "e2") == 0) {
            return baidu::zling::Encode(&inputter, &outputter, &demo_handler, 2);
        }
        if (argc == 2 && strcmp(argv[1], "e1") == 0) {
            return baidu::zling::Encode(&inputter, &outputter, &demo_handler, 1);
        }
        if (argc == 2 && strcmp(argv[1], "e0") == 0) {
            return baidu::zling::Encode(&inputter, &outputter, &demo_handler, 0);
        }

        if (argc == 2 && strcmp(argv[1], "e") == 0) {
            return baidu::zling::Encode(&inputter, &outputter, &demo_handler, 0);
        }
        if (argc == 2 && strcmp(argv[1], "d") == 0) {
            return baidu::zling::Decode(&inputter, &outputter, &demo_handler);
        }

    } catch (const std::runtime_error& e) {
        fprintf(stderr, "zling: runtime error: %s\n", e.what());
        return -1;

    } catch (const std::bad_alloc& e) {
        fprintf(stderr, "zling: allocation failed.");
        return -1;
    }

    // help message
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "   zling e[N=0,1,2,3,4] source target\n");
    fprintf(stderr, "   zling d source target\n");
    fprintf(stderr, "    * source: (default: stdin)\n");
    fprintf(stderr, "    * target: (default: stdout)\n");
    fprintf(stderr, "    * N:      (default: 0) compression level, bigger level for better and slower compression.\n");
    return -1;
}
