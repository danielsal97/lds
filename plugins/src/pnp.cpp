#include "pnp.hpp"
#include "singelton.hpp"

namespace hrd41
{

PNP::PNP(const std::string& dirPath)
    : m_dirMonitor(dirPath),
      m_soLoader(m_dirMonitor.GetDispatcher()),
      m_logger(Singleton<Logger>::GetInstance())
{
    m_logger->Write("PNP initialized with directory: " + dirPath, Logger::INFO);
}

PNP::~PNP()
{
    m_logger->Write("PNP shutting down", Logger::INFO);
}

} // namespace hrd41
