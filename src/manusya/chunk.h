#pragma once

#include <boost/intrusive_ptr.hpp>
#include <butil/iobuf.h>
#include <butil/status.h>
#include <uuid_v4/uuid_v4.h>

namespace pain::manusya {

class Chunk;
using ChunkPtr = boost::intrusive_ptr<Chunk>;
class Chunk {
public:
  Chunk() = default;
  ~Chunk() = default;

  static ChunkPtr create();

  const UUIDv4::UUID &uuid() const { return _uuid; }
  butil::Status append(butil::IOBuf &buf, uint64_t offset);
  butil::Status seal();
  butil::Status read(uint64_t offset, uint64_t size, butil::IOBuf *buf) const;

private:
  friend void intrusive_ptr_add_ref(Chunk *chunk) { chunk->_use_count++; }

  friend void intrusive_ptr_release(Chunk *chunk) {
    if (--chunk->_use_count == 0) {
      delete chunk;
    }
  }

  UUIDv4::UUID _uuid;
  uint64_t _size = 0;
  int _use_count = 0;

  butil::IOBuf _data;
};

} // namespace pain::manusya