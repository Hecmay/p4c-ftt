#ifndef _BACKENDS_P4HLS_P4HLSTABLE_H_
#define _BACKENDS_P4HLS_P4HLSTABLE_H_

#include "hlsObject.h"
#include "hlsProgram.h"
#include "frontends/p4/methodInstance.h"

namespace P4HLS {
// Also used to represent counters
class P4HLSTableBase : public P4HLSObject {
 public:
    const P4HLSProgram* program;

    cstring instanceName;
    cstring keyTypeName;
    cstring valueTypeName;
    cstring dataMapName;
    CodeGenInspector* codeGen;

 protected:
    P4HLSTableBase(const P4HLSProgram* program, cstring instanceName,
                  CodeGenInspector* codeGen) :
            program(program), instanceName(instanceName), codeGen(codeGen) {
        CHECK_NULL(codeGen); CHECK_NULL(program);
        keyTypeName = program->refMap->newName(instanceName + "_key");
        valueTypeName = program->refMap->newName(instanceName + "_value");
        dataMapName = instanceName;
    }
};

class P4HLSTable final : public P4HLSTableBase {
 public:
    const IR::Key*            keyGenerator;
    const IR::ActionList*     actionList;
    const IR::TableBlock*    table;
    cstring               defaultActionMapName;
    cstring               actionEnumName;
    std::map<const IR::KeyElement*, cstring> keyFieldNames;
    std::map<const IR::KeyElement*, P4HLSType*> keyTypes;

    P4HLSTable(const P4HLSProgram* program, const IR::TableBlock* table, CodeGenInspector* codeGen);
    void emitTypes(CodeBuilder* builder);
    void emitInstance(CodeBuilder* builder);
    void emitActionArguments(CodeBuilder* builder, const IR::P4Action* action, cstring name);
    void emitKeyType(CodeBuilder* builder);
    void emitValueType(CodeBuilder* builder);
    void emitKey(CodeBuilder* builder, cstring keyName);
    void emitAction(CodeBuilder* builder, cstring valueName);
    void emitInitializer(CodeBuilder* builder);
};

class P4HLSCounterTable final : public P4HLSTableBase {
    size_t    size;
    bool      isHash;
 public:
    P4HLSCounterTable(const P4HLSProgram* program, const IR::ExternBlock* block,
                     cstring name, CodeGenInspector* codeGen);
    void emitTypes(CodeBuilder*);
    void emitInstance(CodeBuilder* builder);
    void emitCounterIncrement(CodeBuilder* builder, const IR::MethodCallExpression* expression);
    void emitMethodInvocation(CodeBuilder* builder, const P4::ExternMethod* method);
};

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSTABLE_H_ */
