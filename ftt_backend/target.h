#ifndef _BACKENDS_EBPF_TARGET_H_
#define _BACKENDS_EBPF_TARGET_H_

#include "lib/cstring.h"
#include "lib/sourceCodeBuilder.h"
#include "lib/exceptions.h"

// We are prepared to support code generation using multiple styles
// (e.g., using BCC or using CLANG).

namespace EBPF {

enum TableKind {
    TableHash,
    TableArray,
    TableLPMTrie  // longest prefix match trie
};

class Target {
 protected:
    explicit Target(cstring name) : name(name) {}
    Target() = delete;
    virtual ~Target() {}

 public:
    const cstring name;

    virtual void emitLicense(Util::SourceCodeBuilder* builder, cstring license) const = 0;
    virtual void emitCodeSection(Util::SourceCodeBuilder* builder, cstring sectionName) const = 0;
    virtual void emitIncludes(Util::SourceCodeBuilder* builder) const = 0;
    virtual void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                                 cstring key, cstring value) const = 0;
    virtual void emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                 cstring key, cstring value) const = 0;
    virtual void emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                     cstring key, cstring value) const = 0;
    virtual void emitTableDecl(Util::SourceCodeBuilder* builder,
                               cstring tblName, TableKind tableKind,
                               cstring keyType, cstring valueType, unsigned size) const = 0;
    virtual void emitMain(Util::SourceCodeBuilder* builder,
                          cstring functionName,
                          cstring argName) const = 0;
    virtual cstring dataOffset(cstring base) const = 0;
    virtual cstring dataEnd(cstring base) const = 0;
    virtual cstring forwardReturnCode() const = 0;
    virtual cstring dropReturnCode() const = 0;
    virtual cstring abortReturnCode() const = 0;
    // Path on /sys filesystem where maps are stored
    virtual cstring sysMapPath() const = 0;
};

// Represents a target that is compiled within the kernel
// source tree samples folder and which attaches to a socket
class KernelSamplesTarget : public Target {
 public:
    explicit KernelSamplesTarget(cstring name = "Linux kernel") : Target(name) {}
    void emitLicense(Util::SourceCodeBuilder* builder, cstring license) const override;
    void emitCodeSection(Util::SourceCodeBuilder* builder, cstring sectionName) const override;
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
    void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                             cstring key, cstring value) const override;
    void emitTableDecl(Util::SourceCodeBuilder* builder,
                       cstring tblName, TableKind tableKind,
                       cstring keyType, cstring valueType, unsigned size) const override;
    void emitMain(Util::SourceCodeBuilder* builder,
                  cstring functionName,
                  cstring argName) const override;
    cstring dataOffset(cstring base) const override
    { return cstring("((void*)(long)")+ base + "->data)"; }
    cstring dataEnd(cstring base) const override
    { return cstring("((void*)(long)")+ base + "->data_end)"; }
    cstring forwardReturnCode() const override { return "TC_ACT_OK"; }
    cstring dropReturnCode() const override { return "TC_ACT_SHOT"; }
    cstring abortReturnCode() const override { return "TC_ACT_SHOT"; }
    cstring sysMapPath() const override { return "/sys/fs/bpf/tc/globals"; }
};

// Represents a target compiled by bcc that uses the TC
class BccTarget : public Target {
 public:
    BccTarget() : Target("BCC") {}
    void emitLicense(Util::SourceCodeBuilder*, cstring) const override {};
    void emitCodeSection(Util::SourceCodeBuilder*, cstring) const override {}
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
    void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                             cstring key, cstring value) const override;
    void emitTableDecl(Util::SourceCodeBuilder* builder,
                       cstring tblName, TableKind tableKind,
                       cstring keyType, cstring valueType, unsigned size) const override;
    void emitMain(Util::SourceCodeBuilder* builder,
                  cstring functionName,
                  cstring argName) const override;
    cstring dataOffset(cstring base) const override { return base; }
    cstring dataEnd(cstring base) const override
    { return cstring("(") + base + " + " + base + "->len)"; }
    cstring forwardReturnCode() const override { return "0"; }
    cstring dropReturnCode() const override { return "1"; }
    cstring abortReturnCode() const override { return "1"; }
    cstring sysMapPath() const override { return "/sys/fs/bpf"; }
};

// A userspace test version with functionality equivalent to the kernel
// Compiles with gcc
class TestTarget : public EBPF::KernelSamplesTarget {
 public:
    TestTarget() : KernelSamplesTarget("Userspace Test") {}
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
    void emitTableDecl(Util::SourceCodeBuilder* builder,
                       cstring tblName, TableKind tableKind,
                       cstring keyType, cstring valueType, unsigned size) const override;
    cstring dataOffset(cstring base) const override
    { return cstring("((void*)(long)")+ base + "->data)"; }
    cstring dataEnd(cstring base) const override
    { return cstring("((void*)(long)(")+ base + "->data + "+ base +"->len))"; }
    cstring forwardReturnCode() const override { return "true"; }
    cstring dropReturnCode() const override { return "false"; }
    cstring abortReturnCode() const override { return "false"; }
    cstring sysMapPath() const override { return "/sys/fs/bpf"; }
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_TARGET_H_ */
