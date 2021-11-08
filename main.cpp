#include <fstream>
#include <iostream>

#include "ir/ir.h"

#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"

#include "lib/nullstream.h"

#include "ftt_midend/ftt_midend.h"

namespace P4HLS {

class FTTOptions : public CompilerOptions {
 public:
    cstring outputFile = nullptr;
    FTTOptions() {
      registerOption("-o", "outfile",
              [this](const char* arg) { this->outputFile = arg; return true; },
              "Write output to outfile");
     }
};

using FTTContext = P4CContextWithOptions<FTTOptions>;

void run_hls_backend(const FTTOptions& options, const IR::ToplevelBlock* toplevel,
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

    Target* target = new fpgaTarget();
    CodeBuilder c(target); // HLS kernel code
    CodeBuilder h(target); // HLS kernel header

    // build HLS program object
    // An HLSProgram constains the blocks and type map
    // of the input app, and emits output HLS code
    EBPFTypeFactory::createFactory(typeMap);
    auto prog = new HLSProgram(options, toplevel->getProgram(), refMap, typeMap, toplevel);
    if (!prog->build())
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

    // generate header and kernel code
    ebpfprog->emitH(&h, hfile);
    ebpfprog->emitC(&c, hfile);
    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

int main(int argc, char *const argv[]) {
    AutoCompileContext autoFTTContext(new FTTContext);
    auto& options = FTTContext::get().options();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = "0.0.1";

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    auto program = P4::parseP4File(options);

    if (program == nullptr || ::errorCount() > 0) {
        std::cerr << "Can't parse P4 file " << options.file << std::endl;
        return 1;
    }

    try {
        P4::FrontEnd fe;
        program = fe.run(options, program);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    if (program == nullptr || ::errorCount() > 0) {
        std::cerr << "Can't process P4 file " << options.file << " by frontend"<< std::endl;
        return 1;
    }

    FttMidEnd midEnd(options);

    const IR::ToplevelBlock *top = nullptr;
    try {
        top = midEnd.process(program);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    std::cout << top << std::endl;
    std::cout << std::endl << std::endl;
    std::cout << program << std::endl;

    if (options.dumpJsonFile) {
        JSONGenerator(*openFile(options.dumpJsonFile, true), true) << top << std::endl;
    }

    // Backend HLS code generator
    try {
        run_hls_backend(options, top, &midEnd.refMap, &midEnd.typeMap);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }

    return ::errorCount() > 0;
}

} // namespace P4HLS
