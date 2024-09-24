#include "deva/namespace.h"
#include <cerrno>

namespace pain::deva {

Namespace& Namespace::instance() {
    static Namespace ns;
    return ns;
}

Namespace::Namespace() {
    _root = UUID::from_str_or_die("00000000-0000-0000-0000-000000000000");
}

Status Namespace::load() {
    return Status::OK();
}

void Namespace::create(const UUID& parent, const std::string& name, FileType type, const UUID& inode) {
    std::unique_lock guard(_mutex);
    auto it = _entries.find(parent);
    if (it == _entries.end()) {
        _entries[parent] = {};
    }
    _entries[parent].push_back({inode, name, type});
}

void Namespace::remove(const UUID& parent, const std::string& name) {
    std::unique_lock guard(_mutex);
    auto it = _entries.find(parent);
    if (it == _entries.end()) {
        return;
    }
    auto& entries = it->second;
    entries.remove_if([&name](const DirEntry& entry) { return entry.name == name; });
}

void Namespace::list(const UUID& parent, std::list<DirEntry>* entries) const {
    std::unique_lock guard(_mutex);
    auto it = _entries.find(parent);
    if (it == _entries.end()) {
        return;
    }
    *entries = it->second;
}

// parse path such as /a/b/c to ["a", "b", "c"]
Status Namespace::parse_path(const char* path, std::list<std::string_view>* components) const {
    const char* p = path;
    if (*p != '/') {
        return Status(EINVAL, "Invalid path");
    }

    while (*p != '\0') {
        if (*p == '/') {
            p++;
            continue;
        }
        const char* q = p;
        while (*q != '\0' && *q != '/') {
            q++;
        }
        components->emplace_back(p, q - p);
        p = q;
    }
    return Status::OK();
}

Status Namespace::lookup(const char* path, UUID* inode) const {
    std::list<std::string_view> components;
    auto status = parse_path(path, &components);
    if (!status.ok()) {
        return status;
    }

    std::unique_lock guard(_mutex);
    UUID parent = _root;
    for (const auto& component : components) {
        auto it = _entries.find(parent);
        if (it == _entries.end()) {
            return Status(ENOENT, "No such file or directory");
        }

        const auto& entries = it->second;
        auto entry = std::find_if(
            entries.begin(), entries.end(), [&component](const DirEntry& entry) { return entry.name == component; });
        if (entry == entries.end()) {
            return Status(ENOENT, "No such file or directory");
        }

        if (entry->type == FileType::FILE && &component != &components.back()) {
            return Status(ENOENT, "Not a directory");
        }
        parent = entry->inode;
    }
    *inode = parent;
    return Status::OK();
}

} // namespace pain::deva