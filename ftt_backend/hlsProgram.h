#ifndef _BACKENDS_P4HLS_P4HLSPROGRAM_H_
#define _BACKENDS_P4HLS_P4HLSPROGRAM_H_

#include "target.h"
#include "ebpfModel.h"
#include "ebpfObject.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/options.h"
#include "codeGen.h"

namespace P4HLS {

class P4HLSProgram;
class P4HLSParser;
class P4HLSControl;
class P4HLSTable;
class P4HLSType;

class P4HLSProgram : public P4HLSObject {
 public:
    const CompilerOptions& options;
    const IR::P4Program* program;
    const IR::ToplevelBlock*  toplevel;
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;
    P4HLSParser*          parser;
    P4HLSControl*         control;
    P4HLSModel           &model;

    cstring endLabel, offsetVar, lengthVar;
    cstring zeroKey, functionName, errorVar;
    cstring packetStartVar, packetEndVar, byteVar;
    cstring errorEnum;
    cstring license = "GPL";  // TODO: this should be a compiler option probably
    cstring arrayIndexType = "u32";

    virtual bool build();  // return 'true' on success

    P4HLSProgram(const CompilerOptions &options, const IR::P4Program* program,
                P4::ReferenceMap* refMap, P4::TypeMap* typeMap, const IR::ToplevelBlock* toplevel) :
            options(options), program(program), toplevel(toplevel),
            refMap(refMap), typeMap(typeMap),
            parser(nullptr), control(nullptr), model(P4HLSModel::instance) {
        offsetVar = P4HLSModel::reserved("packetOffsetInBits");
        zeroKey = P4HLSModel::reserved("zero");
        functionName = P4HLSModel::reserved("filter");
        errorVar = P4HLSModel::reserved("errorCode");
        packetStartVar = P4HLSModel::reserved("packetStart");
        packetEndVar = P4HLSModel::reserved("packetEnd");
        byteVar = P4HLSModel::reserved("byte");
        endLabel = P4HLSModel::reserved("end");
        errorEnum = P4HLSModel::reserved("errorCodes");
    }

 protected:
    virtual void emitGeneratedComment(CodeBuilder* builder);
    virtual void emitPreamble(CodeBuilder* builder);
    virtual void emitTypes(CodeBuilder* builder);
    virtual void emitHeaderInstances(CodeBuilder* builder);
    virtual void emitLocalVariables(CodeBuilder* builder);
    virtual void emitPipeline(CodeBuilder* builder);

 public:
    virtual void emitH(CodeBuilder* builder, cstring headerFile);  // emits C headers
    virtual void emitC(CodeBuilder* builder, cstring headerFile);  // emits C program
};

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSPROGRAM_H_ */
