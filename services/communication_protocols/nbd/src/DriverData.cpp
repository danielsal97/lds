#include <vector>
#include "DriverData.hpp"


namespace hrd41
{
DriverData::DriverData(ActionType type_, size_t handle_, size_t offset_,
                       size_t nBytes_, StatusType status_)
    : m_type(type_), m_handle(handle_), m_offset(offset_), m_len(nBytes_),
      m_status(status_), m_buffer(std::vector<char>(nBytes_)), m_source_fd(-1)
{
}

} // namespace hrd41
