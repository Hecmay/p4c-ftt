#include "lib/error.h"
#include "lib/nullstream.h"
#include "frontends/p4/evaluator/evaluator.h"

#include "hlsBackend.h"
#include "target.h"
#include "hlsType.h"
#include "hlsProgram.h"

namespace P4HLS {

void run_ebpf_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                      P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
    if (toplevel == nullptr)
        return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::warning(ErrorType::WARN_MISSING,
                  "Could not locate top-level block; is there a %1% module?",
                  IR::P4Program::main);
        return;
    }

    Target* target;
    if (options.target.isNullOrEmpty() || options.target == "kernel") {
        target = new KernelSamplesTarget();
    } else if (options.target == "bcc") {
        target = new BccTarget();
    } else if (options.target == "test") {
        target = new TestTarget();
    } else {
        ::error("Unknown target %s; legal choices are 'bcc', 'kernel', and test", options.target);
        return;
    }

    CodeBuilder c(target);
    CodeBuilder h(target);

    P4HLSTypeFactory::createFactory(typeMap);
    auto ebpfprog = new P4HLSProgram(options, toplevel->getProgram(), refMap, typeMap, toplevel);
    if (!ebpfprog->build())
        return;

    if (options.outputFile.isNullOrEmpty())
        return;

    cstring cfile = options.outputFile;
    auto cstream = openFile(cfile, false);
    if (cstream == nullptr)
        return;

    cstring hfile;
    const char* dot = cfile.findlast('.');
    if (dot == nullptr)
        hfile = cfile + ".h";
    else
        hfile = cfile.before(dot) + ".h";
    auto hstream = openFile(hfile, false);
    if (hstream == nullptr)
        return;

    ebpfprog->emitH(&h, hfile);
    ebpfprog->emitC(&c, hfile);
    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

}  // namespace P4HLS
