/*-----------------------------------------------------------*/
/* Block Sorting, Lossless Data Compression Library.         */
/* Statistical data compression model for QLFC               */
/*-----------------------------------------------------------*/

/*--

This file is a part of bsc and/or libbsc, a program and a library for
lossless, block-sorting data compression.

   Copyright (c) 2009-2012 Ilya Grebnov <ilya.grebnov@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Please see the file LICENSE for full copyright information and file AUTHORS
for full list of contributors.

See also the bsc and libbsc web site:
  http://libbsc.com/ for more information.

--*/

#include <stdlib.h>
#include <memory.h>

#include "qlfc_model.h"

#include "../../libbsc.h"
#include "../../platform/platform.h"

QlfcStatisticalModel g_QlfcStatisticalModel;

void bsc_qlfc_memset_2048(void * dst, int size)
{
    for (int i = 0; i < size / 2; ++i) ((short *)dst)[i] = 2048;
}

int bsc_qlfc_init_static_model()
{
    for (int mixer = 0; mixer < ALPHABET_SIZE; ++mixer)
    {
        g_QlfcStatisticalModel.mixerOfRank[mixer].Init();
        g_QlfcStatisticalModel.mixerOfRankEscape[mixer].Init();
        g_QlfcStatisticalModel.mixerOfRun[mixer].Init();
    }
    for (int bit = 0; bit < 8; ++bit)
    {
        g_QlfcStatisticalModel.mixerOfRankMantissa[bit].Init();
        for (int context = 0; context < 8; ++context)
            g_QlfcStatisticalModel.mixerOfRankExponent[context][bit].Init();
    }
    for (int bit = 0; bit < 32; ++bit)
    {
        g_QlfcStatisticalModel.mixerOfRunMantissa[bit].Init();
        for (int context = 0; context < 32; ++context)
            g_QlfcStatisticalModel.mixerOfRunExponent[context][bit].Init();
    }

    bsc_qlfc_memset_2048(&g_QlfcStatisticalModel.Rank, sizeof(g_QlfcStatisticalModel.Rank));
    bsc_qlfc_memset_2048(&g_QlfcStatisticalModel.Run, sizeof(g_QlfcStatisticalModel.Run));

    return LIBBSC_NO_ERROR;
}

void bsc_qlfc_init_model(QlfcStatisticalModel * model)
{
    memcpy(model, &g_QlfcStatisticalModel, sizeof(QlfcStatisticalModel));
}

/*-----------------------------------------------------------*/
/* End                                        qlfc_model.cpp */
/*-----------------------------------------------------------*/
