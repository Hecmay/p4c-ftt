#ifndef _BACKENDS_P4HLS_P4HLSBACKEND_H_
#define _BACKENDS_P4HLS_P4HLSBACKEND_H_

#include "ebpfOptions.h"
#include "ebpfObject.h"
#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace P4HLS {

void run_hls_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                      P4::ReferenceMap* refMap, P4::TypeMap* typeMap);

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSBACKEND_H_ */
