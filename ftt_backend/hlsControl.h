#ifndef _BACKENDS_P4HLS_P4HLSCONTROL_H_
#define _BACKENDS_P4HLS_P4HLSCONTROL_H_

#include "hlsObject.h"
#include "hlsTable.h"

namespace P4HLS {

class P4HLSControl;

class ControlBodyTranslator : public CodeGenInspector {
    const P4HLSControl* control;
    std::set<const IR::Parameter*> toDereference;
    std::vector<cstring> saveAction;
    P4::P4CoreLibrary& p4lib;
 public:
    explicit ControlBodyTranslator(const P4HLSControl* control);

    // handle the packet_out.emit method
    virtual void compileEmitField(const IR::Expression* expr, cstring field,
                                  unsigned alignment, P4HLSType* type);
    virtual void compileEmit(const IR::Vector<IR::Argument>* args);
    virtual void processMethod(const P4::ExternMethod* method);
    virtual void processApply(const P4::ApplyMethod* method);
    virtual void processFunction(const P4::ExternFunction* function);

    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::ExitStatement*) override;
    bool preorder(const IR::ReturnStatement*) override;
    bool preorder(const IR::IfStatement* statement) override;
    bool preorder(const IR::SwitchStatement* statement) override;
};

class P4HLSControl : public P4HLSObject {
 public:
    const P4HLSProgram*      program;
    const IR::ControlBlock* controlBlock;
    const IR::Parameter*    headers;
    const IR::Parameter*    accept;
    const IR::Parameter*    parserHeaders;
    // replace references to headers with references to parserHeaders
    cstring                 hitVariable;
    ControlBodyTranslator*  codeGen;

    std::set<const IR::Parameter*> toDereference;
    std::map<cstring, P4HLSTable*>  tables;
    std::map<cstring, P4HLSCounterTable*>  counters;

    P4HLSControl(const P4HLSProgram* program, const IR::ControlBlock* block,
                const IR::Parameter* parserHeaders);
    virtual void emit(CodeBuilder* builder);
    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl);
    void emitTableTypes(CodeBuilder* builder);
    void emitTableInitializers(CodeBuilder* builder);
    void emitTableInstances(CodeBuilder* builder);
    virtual bool build();
    P4HLSTable* getTable(cstring name) const {
        auto result = ::get(tables, name);
        BUG_CHECK(result != nullptr, "No table named %1%", name);
        return result; }
    P4HLSCounterTable* getCounter(cstring name) const {
        auto result = ::get(counters, name);
        BUG_CHECK(result != nullptr, "No counter named %1%", name);
        return result; }

 protected:
    void scanConstants();
};

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSCONTROL_H_ */
