#pragma once


#include <vector>
#include <array>
#include <memory>


namespace canc {

struct CanMsg {
  uint32_t id;
  uint8_t data[64];
  uint8_t size;
  bool fdcan = false;
};


/// @brief Class to send structures over CAN bus
/// @tparam T the structure type to be sent over CAN
/// @note T must have static member functions.
/// if fra
/// it maximum size must be less then 1792 bytes
template <typename T, bool FdCan = false> class CanStructureSender {
  using Type = T;

public:
  CanStructureSender(uint32_t can_id) : _frame_id(can_id) {
    if constexpr(FdCan) {
      static_assert(sizeof(Type) <= 16128,
                    "CanStructureSender: Structure size exceeds maximum Extended CAN data size");
    } else {
      static_assert(sizeof(Type) <= 1792, "CanStructureSender: Structure size exceeds maximum CAN data size");
    }
  }
  ~CanStructureSender() = default;


  ///
  /// CAN DATA STRUCTURE
  /// [ can id ] - Frame ID
  ///
  ///  ...  other can slob.
  ///
  /// [  1 byte  ] - Segment Index  (max is 256 segments)
  /// [  N bytes ] - Structure Data Segment (max is 7 bytes for CAN and 63 bytes for FDCAN for each can frame)
  ///
  ///

  std::vector<CanMsg> pack(const Type &structure) {
    std::vector<CanMsg> messages;
    size_t total_size = sizeof(Type);
    size_t offset     = 0;
    size_t index      = 0;
    while(total_size > 0) {
      CanMsg msg;
      msg.id    = _frame_id;
      msg.fdcan = FdCan;
      msg.size  = frame_size + 1; // +1 for segment index size all the frames are -1 to fit the index data and
      uint8_t size_to_copy = (total_size > frame_size) ? frame_size : static_cast<uint8_t>(total_size);
      msg.data[0]          = static_cast<uint8_t>(index++);
      std::memcpy(msg.data + 1, reinterpret_cast<const uint8_t *>(&structure) + offset, size_to_copy);
      messages.push_back(msg);
      total_size -= size_to_copy;
      offset += size_to_copy;
    }
    return messages;
  }

  void start_unpacking() {
    std::memset(_rx_buffor, 0, sizeof(Type));
    std::memset(_decoded_parts, 0, sizeof(_decoded_parts));
  }

  /// @brief This will unpack the structure from the CAN message
  /// @param msg received CAN message don't have to be in order
  /// @return true if unpacking was finished false otherwise
  bool unpack(const CanMsg &msg) {
    size_t total_size     = sizeof(Type);
    size_t total_segments = total_size / frame_size + ((total_size % frame_size) ? 1 : 0);
    size_t segment_index  = msg.data[0];
    if(segment_index >= total_segments) {
      return false;
    }
    size_t size_to_copy =
    (segment_index < total_segments) ? frame_size : static_cast<size_t>(total_size - segment_index * frame_size);
    std::memcpy(_rx_buffor + segment_index * frame_size, msg.data + 1, size_to_copy);
    _decoded_parts[segment_index] = true;

    for(size_t i = 0; i < total_segments; ++i) {
      if(!_decoded_parts[i]) {
        return false;
      }
    }
    return true;
  }

  Type get_unpacked_structure() const {
    return *reinterpret_cast<const Type *>(_rx_buffor);
  }

private:
  const constexpr size_t frame_size = FdCan ? 63 : 7;
  uint8_t _rx_buffor[sizeof(Type)];
  bool _decoded_parts[sizeof(Type) / frame_size + 1]{ false };
  uint32_t _frame_id;
};


} // namespace canc