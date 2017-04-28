#ifndef EGALITO_GENERATE_SECTION_H
#define EGALITO_GENERATE_SECTION_H

#include <algorithm>  // for std::find
#include <string>
#include <vector>
#include <map>
#include <elf.h>
#include "elf/elfxx.h"
#include "types.h"

class Section {
private:
    std::string data;
    std::string name;
    address_t address;
    size_t offset;
    bool withShdr;
    ElfXX_Word shdrType;
    ElfXX_Xword shdrFlags;
    Section *sectionLink;
    size_t shdrIndex;
public:
    Section(std::string name) : name(name), address(0), offset(0),
        withShdr(false), shdrType(SHT_NULL), shdrFlags(0),
        sectionLink(nullptr), shdrIndex(static_cast<size_t>(-1)) {}
    Section(std::string name, ElfXX_Word type, ElfXX_Xword flags = 0)
        : name(name), address(0), offset(0),
        withShdr(true), shdrType(type), shdrFlags(flags),
        sectionLink(nullptr), shdrIndex(static_cast<size_t>(-1)) {}
    virtual ~Section() {}
    Section *with(const void *data, size_t size)
        { add(data, size); return this; }
    Section *with(const char *data, size_t size)
        { add(data, size); return this; }
public:
    std::string getData() const { return data; }
    std::string getName() const { return name; }
    address_t getAddress() const { return address; }
    size_t getOffset() const { return offset; }
    virtual size_t getSize() const { return data.size(); }
    bool hasShdr() const { return withShdr; }
    Section *getSectionLink() const { return sectionLink; }
    size_t getShdrIndex() const { return shdrIndex; }
public:
    void setAddress(address_t addr) { address = addr; }
    void setOffset(size_t off) { offset = off; }
    void setSectionLink(Section *link) { sectionLink = link; }
    virtual void commitValues() {}
public:
    friend std::ostream& operator<<(std::ostream &stream, Section &rhs);
    size_t add(const void *data, size_t size);
    size_t add(const char *data, size_t size);
    size_t add(const std::string &string, bool withNull = false);
    void addNullBytes(size_t size);
    void setShdrFlags(ElfXX_Xword flags) { shdrFlags = flags; }
    virtual ElfXX_Shdr *makeShdr(size_t index, size_t nameStrIndex);
    template<typename ElfStructType> ElfStructType *castAs()
        { return (ElfStructType *)(data.data()); }
    template<typename ElfStructType> size_t getKeyCount()
        { return data.size() / sizeof(ElfStructType); }
};

/** Stores intermediate data generated from KeyType objects, which will
    be serialized into ValueType objects (e.g. Symbols -> ElfXX_Sym).
    This stores a map as well as vector.
*/
template <typename KeyType, typename ValueType>
class DeferredSection : public Section {
private:
    typedef std::map<KeyType *, ValueType *> ValueMapType;
    ValueMapType valueMap;

    typedef std::vector<KeyType *> KeyListType;
    KeyListType keyList;

    bool committed;
public:
    DeferredSection(std::string name)
        : Section(name), committed(false) {}
    DeferredSection(std::string name, ElfXX_Word type, ElfXX_Xword flags = 0)
        : Section(name, type, flags), committed(false) {}

    ~DeferredSection();

    virtual size_t getValueSize() const { return sizeof(ValueType); }
    virtual size_t getSize() const
        { return keyList.size() * getValueSize(); }
    size_t getCount() const { return keyList.size(); }
protected:
    void addKeyValue(KeyType *key, ValueType *value)
        { valueMap[key] = value; keyList.push_back(key); }
    void lowLevelAdd(ValueType *&value)
        { add(static_cast<void *>(value), getValueSize()); }
public:
    ValueType *&findValue(KeyType *key);
    size_t findIndex(KeyType *key);

    // returns keys in arbitrary order, not index-sorted
    ValueMapType &getValueMap() { return valueMap; }
    KeyListType &getKeyList() { return keyList; }

    // Create an iterator
    typename KeyListType::iterator begin() { return keyList.begin(); }
    typename KeyListType::iterator end() { return keyList.end(); }
    void insert(typename KeyListType::iterator it, KeyType *key, ValueType *value)
        { valueMap[key] = value; keyList.insert(it, key); }

    void commitValues();
};

template <typename KeyType, typename ValueType>
DeferredSection<KeyType, ValueType>::~DeferredSection() {
    for(auto value : valueMap) {
        delete value.second;
    }
}

template <typename KeyType, typename ValueType>
ValueType *&DeferredSection<KeyType, ValueType>
    ::findValue(KeyType *key) {

    return (*valueMap.find(key)).second;
}

template <typename KeyType, typename ValueType>
size_t DeferredSection<KeyType, ValueType>
    ::findIndex(KeyType *key) {

    auto it = std::find(keyList.begin(), keyList.end(), key);
    return (it != keyList.end()
        ? std::distance(keyList.begin(), it)
        : static_cast<size_t>(0));
}

template <typename KeyType, typename ValueType>
void DeferredSection<KeyType, ValueType>::commitValues() {
    if(committed) return;
    for(auto key : keyList) {
        auto &value = valueMap[key];
        lowLevelAdd(value);
    }
    committed = true;
}

/** Stores intermediate data generated from KeyType objects, which will
    be serialized into ValueType objects (e.g. just a list of ElfXX_Sym).
*/
template <typename ValueType>
class SimpleDeferredSection : public Section {
private:
    typedef std::vector<ValueType *> ValueListType;
    ValueListType valueList;

    bool committed;
public:
    SimpleDeferredSection(std::string name)
        : Section(name), committed(false) {}
    SimpleDeferredSection(std::string name, ElfXX_Word type, ElfXX_Xword flags = 0)
        : Section(name, type, flags), committed(false) {}

    ~SimpleDeferredSection();

    virtual size_t getValueSize() const { return sizeof(ValueType); }
    virtual size_t getSize() const
        { return valueList.size() * getValueSize(); }
    size_t getCount() const { return valueList.size(); }
protected:
    void addValue(ValueType *value)
        { valueList.push_back(value); }
    virtual void lowLevelAdd(ValueType *&value)
        { add(static_cast<void *>(value), getValueSize()); }
public:
    ValueListType &getValueList() { return valueList; }
    typename ValueListType::iterator begin() { return valueList.begin(); }
    typename ValueListType::iterator end() { return valueList.end(); }
    void insert(typename ValueListType::iterator it, ValueType *value)
        { valueList.insert(it, value); }

    void commitValues();
};

template <typename ValueType>
SimpleDeferredSection<ValueType>::~SimpleDeferredSection() {
    for(auto value : valueList) {
        delete value;
    }
}

template <typename ValueType>
void SimpleDeferredSection<ValueType>::commitValues() {
    if(committed) return;
    for(auto value : valueList) {
        lowLevelAdd(value);
    }
    committed = true;
}

#include "concretesection.h"

#endif
