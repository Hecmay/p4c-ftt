#ifndef _BACKENDS_P4HLS_P4HLSPARSER_H_
#define _BACKENDS_P4HLS_P4HLSPARSER_H_

#include "ir/ir.h"
#include "hlsObject.h"
#include "hlsProgram.h"

namespace P4HLS {

class P4HLSParser;

class P4HLSParserState : public P4HLSObject {
 public:
    const IR::ParserState* state;
    const P4HLSParser* parser;

    P4HLSParserState(const IR::ParserState* state, P4HLSParser* parser) :
            state(state), parser(parser) {}
    void emit(CodeBuilder* builder);
};

class P4HLSParser : public P4HLSObject {
 public:
    const P4HLSProgram*            program;
    const P4::TypeMap*            typeMap;
    const IR::ParserBlock*        parserBlock;
    std::vector<P4HLSParserState*> states;
    const IR::Parameter*          packet;
    const IR::Parameter*          headers;
    P4HLSType*                     headerType;

    explicit P4HLSParser(const P4HLSProgram* program, const IR::ParserBlock* block,
                        const P4::TypeMap* typeMap);
    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl);
    void emit(CodeBuilder* builder);
    bool build();
};

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSPARSER_H_ */
