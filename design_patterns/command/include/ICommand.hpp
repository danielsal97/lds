// ============================================================================
// ICommand.hpp
// What:  Command pattern - encapsulates a request as an object with priority
// Why:   Allows commands to be queued, prioritized, and executed asynchronously;
//        decouples command creation from execution
// How:   Subclass ICommand, implement Execute(), use priority levels
//        (Low/Med/High/Admin) for execution ordering
// ============================================================================

#ifndef __ILRD_ICOMMAND_HPP__
#define __ILRD_ICOMMAND_HPP__

#include <stdexcept> // runtime_error

namespace hrd41
{
class ICommand
{
public:
    //....
    enum CMDPriority{Low, Med, High, Admin};
    
    explicit ICommand(CMDPriority priority = Med);
    ICommand(const ICommand& o_);
    ICommand& operator=(const ICommand& o_);
    virtual ~ICommand() = default;

    bool operator<(const ICommand& rhs ) const;      
    virtual void Execute() = 0; // throw CommandError 
private:
    CMDPriority m_priority;
};


class CommandError: public std::runtime_error
{
    CommandError(const std::string& msg_);
    virtual ~CommandError() = 0;
};

} // namespace hrd41


#endif  //__ILRD_ICOMMAND_H__
