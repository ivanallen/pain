#include "deva/namespace.h"
#include <pain/base/plog.h>
#include <pain/base/scope_exit.h>
#include "common/txn_store.h"

namespace pain::deva {

Namespace::Namespace(common::StorePtr store) : _store(store) {
    _root = UUID::from_str_or_die("00000000-0000-0000-0000-000000000000");
}

Status Namespace::load() {
    return Status::OK();
}

Status Namespace::create(const UUID& parent, const std::string& name, FileType type, const UUID& inode) {
    auto txn = _store->begin_txn();
    if (!txn) {
        return Status(EIO, "Failed to begin transaction");
    }
    auto rollback = make_scope_exit([&txn]() {
        auto status = txn->rollback();
        if (!status.ok()) {
            PLOG_ERROR(("desc", "Failed to rollback")("status", status));
        }
    });
    // find parent
    std::string dentries_str;
    proto::DirEntries dentries;
    auto status = txn->hget(_dentry_key, parent.str(), &dentries_str);
    if (status.ok()) {
        if (!dentries.ParseFromString(dentries_str)) {
            return Status(EBADMSG, "Failed to parse dentries");
        }
    } else if (status.error_code() != ENOENT) {
        // something wrong
        return status;
    }
    // check if name or inode already exists
    for (const auto& dentry : dentries.entries()) {
        if (dentry.name() == name) {
            return Status(EEXIST, "File name already exists");
        }
        if (dentry.file_id().high() == inode.high() && dentry.file_id().low() == inode.low()) {
            return Status(EEXIST, "File inode already exists");
        }
    }
    // create dentry
    auto dentry = dentries.add_entries();
    dentry->set_name(name);
    dentry->mutable_file_id()->set_high(inode.high());
    dentry->mutable_file_id()->set_low(inode.low());
    dentry->mutable_parent_file_id()->set_high(parent.high());
    dentry->mutable_parent_file_id()->set_low(parent.low());
    dentry->set_type(static_cast<proto::FileType>(type));
    dentries_str.clear();
    if (!dentries.SerializeToString(&dentries_str)) {
        return Status(EBADMSG, "Failed to serialize dentries");
    }
    // create dentry
    txn->hset(_dentry_key, parent.str(), dentries_str);

    // insert file info
    proto::FileInfo file_info;
    file_info.mutable_file_id()->set_high(inode.high());
    file_info.mutable_file_id()->set_low(inode.low());
    file_info.set_type(static_cast<proto::FileType>(type));
    file_info.set_size(0);
    file_info.set_ctime(0);
    file_info.set_mtime(0);
    file_info.set_atime(0);
    file_info.set_uid(0);
    file_info.set_gid(0);
    std::string file_info_str;
    if (!file_info.SerializeToString(&file_info_str)) {
        return Status(EBADMSG, "Failed to serialize file info");
    }
    txn->hset(_inode_key, inode.str(), file_info_str);

    status = txn->commit();
    if (!status.ok()) {
        return status;
    }
    rollback.release();
    return Status::OK();
}

Status Namespace::remove(const UUID& parent, const std::string& name) {
    // find parent
    auto txn = _store->begin_txn();
    if (!txn) {
        return Status(EIO, "Failed to begin transaction");
    }
    auto rollback = make_scope_exit([&txn]() {
        auto status = txn->rollback();
        if (!status.ok()) {
            PLOG_ERROR(("desc", "Failed to rollback")("status", status));
        }
    });
    // get dentries
    std::string dentries_str;
    auto status = txn->hget(_dentry_key, parent.str(), &dentries_str);
    if (!status.ok()) {
        return status;
    }
    proto::DirEntries dentries;
    if (!dentries.ParseFromString(dentries_str)) {
        return Status(EBADMSG, "Failed to parse dentries");
    }
    // remove dentry
    UUID file_id;
    auto entry =
        std::find_if(dentries.entries().begin(), dentries.entries().end(), [&name](const proto::DirEntry& dentry) {
            return dentry.name() == name;
        });
    if (entry == dentries.entries().end()) {
        return Status(ENOENT, "No such file or directory");
    }
    file_id = UUID(entry->file_id().high(), entry->file_id().low());
    dentries.mutable_entries()->erase(entry);
    // set dentries
    if (!dentries.SerializeToString(&dentries_str)) {
        return Status(EBADMSG, "Failed to serialize dentries");
    }
    txn->hset(_dentry_key, parent.str(), dentries_str);

    // remove file info
    txn->hdel(_inode_key, file_id.str());

    status = txn->commit();
    if (!status.ok()) {
        return status;
    }
    rollback.release();
    return Status::OK();
}

void Namespace::list(const UUID& parent, std::list<DirEntry>* entries) const {
    entries->clear();
    std::string dentries_str;
    auto status = _store->hget(_dentry_key, parent.str(), &dentries_str);
    if (!status.ok()) {
        return;
    }
    proto::DirEntries dentries;
    if (!dentries.ParseFromString(dentries_str)) {
        return;
    }
    for (const auto& dentry : dentries.entries()) {
        entries->emplace_back(
            UUID(dentry.file_id().high(), dentry.file_id().low()), dentry.name(), static_cast<FileType>(dentry.type()));
    }
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

Status Namespace::lookup(const char* path, UUID* inode, FileType* file_type) const {
    std::list<std::string_view> components;
    auto status = parse_path(path, &components);
    if (!status.ok()) {
        return status;
    }

    UUID parent = _root;
    *file_type = FileType::kDirectory;

    for (const auto& component : components) {
        std::string dentries_str;
        auto status = _store->hget(_dentry_key, parent.str(), &dentries_str);
        if (!status.ok()) {
            return status;
        }
        proto::DirEntries dentries;
        if (!dentries.ParseFromString(dentries_str)) {
            return Status(EBADMSG, fmt::format("Failed to parse dentries: {}", component));
        }
        auto entry = std::find_if(
            dentries.entries().begin(), dentries.entries().end(), [&component](const proto::DirEntry& dentry) {
                return dentry.name() == component;
            });
        if (entry == dentries.entries().end()) {
            return Status(ENOENT, fmt::format("No such file or directory: {}", component));
        }
        if (entry->type() == static_cast<proto::FileType>(FileType::kFile) && component != components.back()) {
            return Status(ENOENT, fmt::format("Not a directory: {}", component));
        }
        parent = UUID(entry->file_id().high(), entry->file_id().low());
        *file_type = static_cast<FileType>(entry->type());
    }
    *inode = parent;
    return Status::OK();
}

} // namespace pain::deva
