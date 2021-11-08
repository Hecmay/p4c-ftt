#ifndef _BACKENDS_P4HLS_P4HLSOPTIONS_H_
#define _BACKENDS_P4HLS_EBPFOPTIONS_H_

#include <getopt.h>
#include "frontends/common/options.h"


class HlsOptions : public CompilerOptions {
 public:
    // file to output to
    cstring outputFile = nullptr;
    // read from json
    bool loadIRFromJson = false;
    HlsOptions();
};

using HlsContext = P4CContextWithOptions<HlsOptions>;

#endif /* _BACKENDS_P4HLS_P4HLSOPTIONS_H_ */
