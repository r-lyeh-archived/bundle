libzling
========

**fast and lightweight compression library and utility.**

Introduction
============

Libzling is an improved lightweight compression utility and library. libzling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it **3 times as fast as gzip on compressing, while still getting much better compression ratio and decompression speed**.

Simple benchmark with **enwik8**(100,000,000 bytes), also on [Large Text Compression Benchmark](http://mattmahoney.net/dc/text.html#2702) (thanks to Matt Mahoney)

Tool    | Compressed Size | Encode | Decode |
--------|-----------------|--------|--------|
gzip    | 36520KB         | 8.13s  | 1.47s  |
zling e0| 32378KB         | 2.42s  | 1.03s  |
zling e1| 31720KB         | 2.73s  | 1.02s  |
zling e2| 31341KB         | 3.14s  | 1.00s  |
zling e3| 30980KB         | 3.73s  | 0.99s  |
zling e4| 30707KB         | 4.36s  | 0.98s  |

Simple benchmark with **fp.log**(20,617,071 bytes)

Tool    | Compressed Size | Encode | Decode |
--------|-----------------|--------|--------|
gzip    | 1449KB          | 0.41s  | 0.14s  |
zling e0| 973KB           | 0.10s  | 0.07s  |
zling e1| 914KB           | 0.11s  | 0.07s  |
zling e2| 903KB           | 0.12s  | 0.07s  |
zling e3| 914KB           | 0.15s  | 0.07s  |
zling e4| 901KB           | 0.19s  | 0.07s  |

Libzling is very suitable for compressing bigdata with a lot of redundancy. here is a benchmark for `jx-se-click0-0.jx.baidu.com_20140601000000.log` (60,908,643 bytes), which is a sequence file containing web logging data like (unvisible characters are removed):

    182.202.13.193 01/Jun/2014:00:00:00 +0800 GET /w.gif?q=%D5%BE%C7%B0%B5%BD%B4%F3%CE%F7%BD%D6&fm=se&T=1401551999&y=776FFE5A&rsv_cache=0&rsv_sid=2889&cid=0&qid=deecd1b40006525b&t=1401552000884&rsv_mobile=1_0_2_0_0&path=http://www.baidu.com/s?ie=utf-8&bs=%E5%85%88%E5%A4%A9%E4%B8%8D%E8%B6%B3%E5%90%8E%E5%A4%A9%E5%BE%97%E8%A1%A5&dsp=ipad&f=8&rsv_bp=1&wd=%E7%AB%99%E5%89%8D%E5%88%B0%E5%A4%A7%E8%A5%BF%E8%A1%97&rsv_sugtime=1212&inputT=22595&rsv_sug3=32 HTTP/1.1 ? _ga=GA1.2.635472944.1400156199; BAIDU_WISE_UID=wapp_1391443584896_450; BAIDUID=6477DD622450D4E300E5BD0B0E9C1887:FG=1; H_PS_PSSID=2889; Hm_lvt_f4165db5a1ac36eadcfa02a10a6bd243=1394801121; USER_AGENT=Mozilla/5.0 (iPad; CPU OS 7_1_1 like Mac OS X) AppleWebKit/537.51.2 (KHTML, like Gecko) Version/7.0 Mobile/11D201 Safari/9537.5

Tool    | Compressed Size | Encode | Decode |
--------|-----------------|--------|--------|
lzop    | 30300KB         | 0.44s  | 0.30s  |
gzip    | 18764KB         | 3.82s  | 0.85s  |
zling e0| 14324KB         | 1.27s  | 0.56s  |
zling e1| 13762KB         | 1.43s  | 0.55s  |
zling e2| 13428KB         | 1.61s  | 0.54s  |
zling e3| 13237KB         | 1.86s  | 0.53s  |
zling e4| 13003KB         | 2.17s  | 0.52s  |
bzip2   | 12497KB         | 10.82s | 3.45s  |
lzma    |  9912KB         | 53.26s | 1.44s  |

Build & Install
===============

You can build and install libzling automatically by **cmake** with the following command:

    cd ./build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install
    make
    make install

Usage
=====

Libling provides simple and lightweight interface. here is a simple program showing the basic usage of libzling. (compiled with `g++ -Wall -O3 zling_sample.cpp -o zling_sample -lzling`)

```C++
#include "libzling/libzling.h"

int main() {
    // compress
    {
        const int level = 0;  // valid levels: 0, 1, 2, 3, 4
        FILE* fin = fopen("./1.txt", "rb");
        FILE* fout = fopen("./1.txt.zlng", "wb");

        baidu::zling::FileInputter  inputter(fin);
        baidu::zling::FileOutputter outputter(fout);

        baidu::zling::Encode(&inputter, &outputter, level);
        fclose(fin);
        fclose(fout);
    }

    // decompress
    {
        FILE* fin = fopen("./1.txt.zlng", "rb");
        FILE* fout = fopen("./2.txt", "wb");

        baidu::zling::FileInputter  inputter(fin);
        baidu::zling::FileOutputter outputter(fout);

        baidu::zling::Decode(&inputter, &outputter);
        fclose(fin);
        fclose(fout);
    }
    return 0;
}
```
However libzling supports more complicated interface, see **./demo/zling.cpp** for details.
