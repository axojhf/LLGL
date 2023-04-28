/*
 * Log.cpp
 * 
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include <LLGL/Log.h>
#include <mutex>
#include <string>


namespace LLGL
{

namespace Log
{


struct LogState
{
    std::mutex      reportMutex;
    ReportCallback  reportCallback  = nullptr;
    std::ostream*   outputStream    = nullptr;
    void*           userData        = nullptr;
    std::size_t     limit           = 0;
    std::size_t     counter         = 0;
};

static LogState g_logState;


/* ----- Functions ----- */

LLGL_EXPORT void PostReport(ReportType type, const StringView& message, const StringView& contextInfo)
{
    ReportCallback  callback;
    void*           userData    = nullptr;
    bool            ignore      = false;

    /* Get callback and user data with a lock guard */
    {
        std::lock_guard<std::mutex> guard{ g_logState.reportMutex };

        /* Get callback and user data */
        callback = g_logState.reportCallback;
        userData = g_logState.userData;

        /* Increase report counter and check if the report must be ignored */
        g_logState.counter++;
        if (g_logState.limit > 0 && g_logState.counter > g_logState.limit)
            ignore = true;
    }

    /* Post report to callback */
    if (!ignore && callback != nullptr)
        callback(type, message, contextInfo, userData);
}

LLGL_EXPORT void SetReportCallback(const ReportCallback& callback, void* userData)
{
    std::lock_guard<std::mutex> guard{ g_logState.reportMutex };
    g_logState.reportCallback   = callback;
    g_logState.userData         = userData;
}

LLGL_EXPORT void SetReportCallbackStd(std::ostream* stream)
{
    std::lock_guard<std::mutex> guard{ g_logState.reportMutex };
    if (stream != nullptr)
    {
        g_logState.outputStream     = stream;
        g_logState.reportCallback   = [](ReportType type, const StringView& message, const StringView& contextInfo, void* userData)
        {
            if (!contextInfo.empty())
                (*g_logState.outputStream) << std::string(contextInfo.begin(), contextInfo.end()) << ": ";
            (*g_logState.outputStream) << std::string(message.begin(), message.end()) << std::endl;
        };
    }
    else
    {
        g_logState.outputStream     = nullptr;
        g_logState.reportCallback   = nullptr;
    }
    g_logState.userData = nullptr;
}

LLGL_EXPORT void SetReportLimit(std::size_t maxCount)
{
    std::lock_guard<std::mutex> guard{ g_logState.reportMutex };
    g_logState.limit = maxCount;
}


} // /namespace Log

} // /namespace LLGL



// ================================================================================
