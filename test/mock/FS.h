// Mock de un filesystem Arduino (FS.h) para tests nativos: un FS en RAM
// (std::map<path,content>) que emula lo que ConfigStore usa de SPIFFS/SD.
#pragma once

#include <Arduino.h>
#include <map>
#include <string>

#define FILE_READ   "r"
#define FILE_WRITE  "w"
#define FILE_APPEND "a"

class MockFS;

// File mock: soporta operator bool, readString(), print() y close().
// Params como const std::string& para aceptar tanto literales const char* como
// el String (std::string) que pasa ConfigStore — igual que los overloads del FS real.
class MockFile {
public:
    MockFile() : _fs(nullptr), _ok(false) {}
    MockFile(MockFS* fs, const std::string& path, bool ok) : _fs(fs), _path(path), _ok(ok) {}

    explicit operator bool() const { return _ok; }   // habilita `if (f)` / `if (!f)`
    String readString();
    size_t print(const String& s);
    void close() { _ok = false; }

private:
    MockFS* _fs;
    std::string _path;
    bool _ok;
};

using File = MockFile;

class MockFS {
public:
    bool begin(bool format = false) { (void)format; return true; }

    bool exists(const std::string& path) { return _files.count(path) > 0; }

    MockFile open(const std::string& path, const char* mode) {
        const bool writing = (mode && mode[0] == 'w');
        if (writing) _files[path] = "";                 // crear/truncar
        const bool ok = writing ? true : (_files.count(path) > 0);
        return MockFile(this, path, ok);
    }

    bool remove(const std::string& path) { return _files.erase(path) > 0; }

    bool rename(const std::string& from, const std::string& to) {
        auto it = _files.find(from);
        if (it == _files.end()) return false;
        _files[to] = it->second;
        _files.erase(it);
        return true;
    }

    // ---- helpers SOLO para los tests (manipular/inspeccionar el FS) ----
    void mockClear() { _files.clear(); }
    void mockSet(const std::string& path, const std::string& content) { _files[path] = content; }
    bool mockHas(const std::string& path) const { return _files.count(path) > 0; }
    std::string mockGet(const std::string& path) const {
        auto it = _files.find(path);
        return it == _files.end() ? std::string() : it->second;
    }
    size_t mockCount() const { return _files.size(); }

private:
    friend class MockFile;
    std::map<std::string, std::string> _files;
};

inline String MockFile::readString() {
    if (!_fs || _fs->_files.count(_path) == 0) return String();
    return _fs->_files[_path];
}
inline size_t MockFile::print(const String& s) {
    if (!_fs) return 0;
    _fs->_files[_path] += s;       // append (open(FILE_WRITE) ya truncó)
    return s.length();
}
