#ifndef _BACKENDS_P4HLS_P4HLSTYPE_H_
#define _BACKENDS_P4HLS_P4HLSTYPE_H_

#include "lib/algorithm.h"
#include "lib/sourceCodeBuilder.h"
#include "ebpfObject.h"
#include "ir/ir.h"

namespace P4HLS {

// Base class for P4HLS types
class P4HLSType : public P4HLSObject {
 protected:
    explicit P4HLSType(const IR::Type* type) : type(type) {}
 public:
    const IR::Type* type;
    virtual void emit(CodeBuilder* builder) = 0;
    virtual void declare(CodeBuilder* builder, cstring id, bool asPointer) = 0;
    virtual void emitInitializer(CodeBuilder* builder) = 0;
    virtual void declareArray(CodeBuilder* /*builder*/, cstring /*id*/, unsigned /*size*/)
    { BUG("%1%: unsupported array", type); }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
    template<typename T> T *to() { return dynamic_cast<T*>(this); }
};

class IHasWidth {
 public:
    virtual ~IHasWidth() {}
    // P4 width
    virtual unsigned widthInBits() = 0;
    // Width in the target implementation.
    // Currently a multiple of 8.
    virtual unsigned implementationWidthInBits() = 0;
};

class P4HLSTypeFactory {
 protected:
    const P4::TypeMap* typeMap;
    explicit P4HLSTypeFactory(const P4::TypeMap* typeMap) :
            typeMap(typeMap) { CHECK_NULL(typeMap); }
 public:
    static P4HLSTypeFactory* instance;
    static void createFactory(const P4::TypeMap* typeMap)
    { P4HLSTypeFactory::instance = new P4HLSTypeFactory(typeMap); }
    virtual P4HLSType* create(const IR::Type* type);
};

class P4HLSBoolType : public P4HLSType, public IHasWidth {
 public:
    P4HLSBoolType() : P4HLSType(IR::Type_Boolean::get()) {}
    void emit(CodeBuilder* builder) override
    { builder->append("u8"); }
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override
    { builder->append("0"); }
    unsigned widthInBits() override { return 1; }
    unsigned implementationWidthInBits() override { return 8; }
};

class P4HLSStackType : public P4HLSType, public IHasWidth {
    P4HLSType* elementType;
    unsigned  size;
 public:
    P4HLSStackType(const IR::Type_Stack* type, P4HLSType* elementType) :
            P4HLSType(type), elementType(elementType), size(type->getSize()) {
        CHECK_NULL(type); CHECK_NULL(elementType);
        BUG_CHECK(elementType->is<IHasWidth>(), "Unexpected element type %1%", elementType);
    }
    void emit(CodeBuilder*) override {}
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override;
    unsigned widthInBits() override;
    unsigned implementationWidthInBits() override;
};

class P4HLSScalarType : public P4HLSType, public IHasWidth {
 public:
    const unsigned width;
    const bool     isSigned;
    explicit P4HLSScalarType(const IR::Type_Bits* bits) :
            P4HLSType(bits), width(bits->size), isSigned(bits->isSigned) {}
    unsigned bytesRequired() const { return ROUNDUP(width, 8); }
    unsigned alignment() const;
    void emit(CodeBuilder* builder) override;
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override
    { builder->append("0"); }
    unsigned widthInBits() override { return width; }
    unsigned implementationWidthInBits() override { return bytesRequired() * 8; }
    // True if this width is small enough to store in a machine scalar
    static bool generatesScalar(unsigned width)
    { return width <= 64; }
};

// This should not always implement IHasWidth, but it may...
class P4HLSTypeName : public P4HLSType, public IHasWidth {
    const IR::Type_Name* type;
    P4HLSType* canonical;
 public:
    P4HLSTypeName(const IR::Type_Name* type, P4HLSType* canonical) :
            P4HLSType(type), type(type), canonical(canonical) {}
    void emit(CodeBuilder* builder) override { canonical->emit(builder); }
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override;
    unsigned widthInBits() override;
    unsigned implementationWidthInBits() override;
    void declareArray(CodeBuilder* builder, cstring id, unsigned size) override;
};

// Also represents headers and unions
class P4HLSStructType : public P4HLSType, public IHasWidth {
    class P4HLSField {
     public:
        cstring comment;
        P4HLSType* type;
        const IR::StructField* field;

        P4HLSField(P4HLSType* type, const IR::StructField* field, cstring comment = nullptr) :
            comment(comment), type(type), field(field) {}
    };

 public:
    cstring  kind;
    cstring  name;
    std::vector<P4HLSField*>  fields;
    unsigned width;
    unsigned implWidth;

    explicit P4HLSStructType(const IR::Type_StructLike* strct);
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override;
    unsigned widthInBits() override { return width; }
    unsigned implementationWidthInBits() override { return implWidth; }
    void emit(CodeBuilder* builder) override;
    void declareArray(CodeBuilder* builder, cstring id, unsigned size) override;
};

class P4HLSEnumType : public P4HLSType, public P4HLS::IHasWidth {
 public:
    explicit P4HLSEnumType(const IR::Type_Enum* type) : P4HLSType(type) {}
    void emit(CodeBuilder* builder) override;
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override
    { builder->append("0"); }
    unsigned widthInBits() override { return 32; }
    unsigned implementationWidthInBits() override { return 32; }
    const IR::Type_Enum* getType() const { return type->to<IR::Type_Enum>(); }
};

}  // namespace P4HLS

#endif /* _BACKENDS_P4HLS_P4HLSTYPE_H_ */
