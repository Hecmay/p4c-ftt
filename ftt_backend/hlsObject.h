#ifndef _BACKENDS_P4HLS_P4HLSOBJECT_H_
#define _BACKENDS_P4HLS_P4HLSOBJECT_H_

#include "target.h"
#include "ebpfModel.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "codeGen.h"

namespace P4HLS {

// Base class for P4HLS objects
class P4HLSObject {
 public:
    virtual ~P4HLSObject() {}
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const {
        return dynamic_cast<const T*>(this); }
    template<typename T> T* to() {
        return dynamic_cast<T*>(this); }

    static cstring externalName(const IR::IDeclaration* declaration) {
        cstring name = declaration->externalName();
        return name.replace('.', '_');
    }
};

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSOBJECT_H_ */
