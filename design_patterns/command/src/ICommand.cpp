#include "ICommand.hpp"

namespace hrd41
{
    ICommand::ICommand(CMDPriority priority_)
    : m_priority(priority_)
    {

    }

    ICommand::ICommand(const ICommand& o_)
    : m_priority(o_.m_priority)
    {
    }

    ICommand& ICommand::operator=(const ICommand &o_)
    {
        m_priority = o_.m_priority;
        
        return (*this);
    }

    bool ICommand::operator<(const ICommand &rhs_) const 
    {
        return m_priority < rhs_.m_priority;
    }

}