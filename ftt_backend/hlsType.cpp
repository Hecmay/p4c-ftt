#include "hlsType.h"

namespace P4HLS {

P4HLSTypeFactory* P4HLSTypeFactory::instance;

P4HLSType* P4HLSTypeFactory::create(const IR::Type* type) {
    CHECK_NULL(type);
    CHECK_NULL(typeMap);
    P4HLSType* result = nullptr;
    if (type->is<IR::Type_Boolean>()) {
        result = new P4HLSBoolType();
    } else if (auto bt = type->to<IR::Type_Bits>()) {
        result = new P4HLSScalarType(bt);
    } else if (auto st = type->to<IR::Type_StructLike>()) {
        result = new P4HLSStructType(st);
    } else if (auto tt = type->to<IR::Type_Typedef>()) {
        auto canon = typeMap->getTypeType(type, true);
        result = create(canon);
        auto path = new IR::Path(tt->name);
        result = new P4HLSTypeName(new IR::Type_Name(path), result);
    } else if (auto tn = type->to<IR::Type_Name>()) {
        auto canon = typeMap->getTypeType(type, true);
        result = create(canon);
        result = new P4HLSTypeName(tn, result);
    } else if (auto te = type->to<IR::Type_Enum>()) {
        result = new P4HLSEnumType(te);
    } else if (auto ts = type->to<IR::Type_Stack>()) {
        auto et = create(ts->elementType);
        if (et == nullptr)
            return nullptr;
        result = new P4HLSStackType(ts, et);
    } else {
        ::error("Type %1% not supported", type);
    }

    return result;
}

void
P4HLSBoolType::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    emit(builder);
    if (asPointer)
        builder->append("*");
    builder->appendFormat(" %s", id.c_str());
}

/////////////////////////////////////////////////////////////

void P4HLSStackType::declare(CodeBuilder* builder, cstring id, bool) {
    elementType->declareArray(builder, id, size);
}

void P4HLSStackType::emitInitializer(CodeBuilder* builder) {
    builder->append("{");
    for (unsigned i = 0; i < size; i++) {
        if (i > 0)
            builder->append(", ");
        elementType->emitInitializer(builder);
    }
    builder->append(" }");
}

unsigned P4HLSStackType::widthInBits() {
    return size * elementType->to<IHasWidth>()->widthInBits();
}

unsigned P4HLSStackType::implementationWidthInBits() {
    return size * elementType->to<IHasWidth>()->implementationWidthInBits();
}

/////////////////////////////////////////////////////////////

unsigned P4HLSScalarType::alignment() const {
    if (width <= 8)
        return 1;
    else if (width <= 16)
        return 2;
    else if (width <= 32)
        return 4;
    else if (width <= 64)
        return 8;
    else
        // compiled as u8*
        return 1;
}

void P4HLSScalarType::emit(CodeBuilder* builder) {
    auto prefix = isSigned ? "i" : "u";

    if (width <= 8)
        builder->appendFormat("%s8", prefix);
    else if (width <= 16)
        builder->appendFormat("%s16", prefix);
    else if (width <= 32)
        builder->appendFormat("%s32", prefix);
    else if (width <= 64)
        builder->appendFormat("%s64", prefix);
    else
        builder->appendFormat("u8*");
}

void
P4HLSScalarType::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    if (P4HLSScalarType::generatesScalar(width)) {
        emit(builder);
        if (asPointer)
            builder->append("*");
        builder->spc();
        builder->append(id);
    } else {
        if (asPointer)
            builder->append("u8*");
        else
            builder->appendFormat("u8 %s[%d]", id.c_str(), bytesRequired());
    }
}

//////////////////////////////////////////////////////////

P4HLSStructType::P4HLSStructType(const IR::Type_StructLike* strct) :
        P4HLSType(strct) {
    if (strct->is<IR::Type_Struct>())
        kind = "struct";
    else if (strct->is<IR::Type_Header>())
        kind = "struct";
    else if (strct->is<IR::Type_HeaderUnion>())
        kind = "union";
    else
        BUG("Unexpected struct type %1%", strct);
    name = strct->name.name;
    width = 0;
    implWidth = 0;

    for (auto f : strct->fields) {
        auto type = P4HLSTypeFactory::instance->create(f->type);
        auto wt = dynamic_cast<IHasWidth*>(type);
        if (wt == nullptr) {
            ::error("P4HLS: Unsupported type in struct: %s", f->type);
        } else {
            width += wt->widthInBits();
            implWidth += wt->implementationWidthInBits();
        }
        fields.push_back(new P4HLSField(type, f));
    }
}

void
P4HLSStructType::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    builder->append(kind);
    if (asPointer)
        builder->append("*");
    builder->appendFormat(" %s %s", name.c_str(), id.c_str());
}

void P4HLSStructType::emitInitializer(CodeBuilder* builder) {
    builder->blockStart();
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_HeaderUnion>()) {
        for (auto f : fields) {
            builder->emitIndent();
            builder->appendFormat(".%s = ", f->field->name.name);
            f->type->emitInitializer(builder);
            builder->append(",");
            builder->newline();
        }
    } else if (type->is<IR::Type_Header>()) {
        builder->emitIndent();
        builder->appendLine(".ebpf_valid = 0");
    } else {
        BUG("Unexpected type %1%", type);
    }
    builder->blockEnd(false);
}

void P4HLSStructType::emit(CodeBuilder* builder) {
    builder->emitIndent();
    builder->append(kind);
    builder->spc();
    builder->append(name);
    builder->spc();
    builder->blockStart();

    for (auto f : fields) {
        auto type = f->type;
        builder->emitIndent();

        type->declare(builder, f->field->name, false);
        builder->append("; ");
        builder->append("/* ");
        builder->append(type->type->toString());
        if (f->comment != nullptr) {
            builder->append(" ");
            builder->append(f->comment);
        }
        builder->append(" */");
        builder->newline();
    }

    if (type->is<IR::Type_Header>()) {
        builder->emitIndent();
        auto type = P4HLSTypeFactory::instance->create(IR::Type_Boolean::get());
        if (type != nullptr) {
            type->declare(builder, "ebpf_valid", false);
            builder->endOfStatement(true);
        }
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void
P4HLSStructType::declareArray(CodeBuilder* builder, cstring id, unsigned size) {
    builder->appendFormat("%s %s[%d]", name.c_str(), id.c_str(), size);
}

///////////////////////////////////////////////////////////////

void P4HLSTypeName::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    if (canonical != nullptr)
        canonical->declare(builder, id, asPointer);
}

void P4HLSTypeName::emitInitializer(CodeBuilder* builder) {
    if (canonical != nullptr)
        canonical->emitInitializer(builder);
}

unsigned P4HLSTypeName::widthInBits() {
    auto wt = dynamic_cast<IHasWidth*>(canonical);
    if (wt == nullptr) {
        ::error("Type %1% does not have a fixed witdh", type);
        return 0;
    }
    return wt->widthInBits();
}

unsigned P4HLSTypeName::implementationWidthInBits() {
    auto wt = dynamic_cast<IHasWidth*>(canonical);
    if (wt == nullptr) {
        ::error("Type %1% does not have a fixed witdh", type);
        return 0;
    }
    return wt->implementationWidthInBits();
}

void
P4HLSTypeName::declareArray(CodeBuilder* builder, cstring id, unsigned size) {
    declare(builder, id, false);
    builder->appendFormat("[%d]", size);
}

////////////////////////////////////////////////////////////////

void P4HLSEnumType::declare(P4HLS::CodeBuilder* builder, cstring id, bool asPointer) {
    builder->append("enum ");
    builder->append(getType()->name);
    if (asPointer)
        builder->append("*");
    builder->append(" ");
    builder->append(id);
}

void P4HLSEnumType::emit(P4HLS::CodeBuilder* builder) {
    builder->append("enum ");
    auto et = getType();
    builder->append(et->name);
    builder->blockStart();
    for (auto m : et->members) {
        builder->append(m->name);
        builder->appendLine(",");
    }
    builder->blockEnd(true);
}

}  // namespace P4HLS
