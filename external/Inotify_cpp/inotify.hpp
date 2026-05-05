// ============================================================================
// inotify.hpp
// What:  Consolidated header for all Inotify_cpp components
// Why:   Simplifies includes - one header instead of 5+ separate ones
// How:   Include this single file to get access to all inotify classes
// ============================================================================

#ifndef __INOTIFY_HPP__
#define __INOTIFY_HPP__

#include "inotify/InotifyCommon.h"
#include "inotify/InotifyException.h"
#include "inotify/InotifyEvent.h"
#include "inotify/InotifyEventHandler.h"
#include "inotify/InotifyWatch.h"
#include "inotify/InotifyManager.h"

#endif // __INOTIFY_HPP__
