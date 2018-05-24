// Module:  Log4CPLUS
// File:    loglevel.cxx
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright 2001-2015 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/loglevel.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/internal/internal.h>
#include <algorithm>


namespace log4cplus
{


namespace
{

static tstring const ALL_STRING (LOG4CPLUS_TEXT("ALL"));
static tstring const TRACE_STRING (LOG4CPLUS_TEXT("TRACE"));
static tstring const DEBUG_STRING (LOG4CPLUS_TEXT("DEBUG"));
static tstring const INFO_STRING (LOG4CPLUS_TEXT("INFO"));
static tstring const WARN_STRING (LOG4CPLUS_TEXT("WARN"));
static tstring const ERROR_STRING (LOG4CPLUS_TEXT("ERROR"));
static tstring const FATAL_STRING (LOG4CPLUS_TEXT("FATAL"));
static tstring const OFF_STRING (LOG4CPLUS_TEXT("OFF"));
static tstring const NOTSET_STRING (LOG4CPLUS_TEXT("NOTSET"));
static tstring const UNKNOWN_STRING (LOG4CPLUS_TEXT("UNKNOWN"));


static
tstring const &
defaultLogLevelToStringMethod(LogLevel ll)
{
    switch(ll) {
        case OFF_LOG_LEVEL:     return OFF_STRING;
        case FATAL_LOG_LEVEL:   return FATAL_STRING;
        case ERROR_LOG_LEVEL:   return ERROR_STRING;
        case WARN_LOG_LEVEL:    return WARN_STRING;
        case INFO_LOG_LEVEL:    return INFO_STRING;
        case DEBUG_LOG_LEVEL:   return DEBUG_STRING;
        case TRACE_LOG_LEVEL:   return TRACE_STRING;
        //case ALL_LOG_LEVEL:     return ALL_STRING;
        case NOT_SET_LOG_LEVEL: return NOTSET_STRING;
    };

    return internal::empty_str;
}


static
LogLevel
defaultStringToLogLevelMethod(const tstring& s)
{
#if __cplusplus < 201103L
    if (s.empty ())
        return NOT_SET_LOG_LEVEL;

    // The above check is only necessary prior to C++11 standard. Since C++11,
    // accessing str[0] is always safe as it returns '\0' for empty string.
#endif

    switch (s[0])
    {
#define DEF_LLMATCH(_chr, _logLevel)                 \
        case _chr:                                   \
            if (s == _logLevel ## _STRING)           \
                return _logLevel ## _LOG_LEVEL;      \
            else                                     \
                break;

        DEF_LLMATCH ('O', OFF);
        DEF_LLMATCH ('F', FATAL);
        DEF_LLMATCH ('E', ERROR);
        DEF_LLMATCH ('W', WARN);
        DEF_LLMATCH ('I', INFO);
        DEF_LLMATCH ('D', DEBUG);
        DEF_LLMATCH ('T', TRACE);
        DEF_LLMATCH ('A', ALL);

#undef DEF_LLMATCH
    }

    return NOT_SET_LOG_LEVEL;
}

} // namespace


//////////////////////////////////////////////////////////////////////////////
// LogLevelManager ctors and dtor
//////////////////////////////////////////////////////////////////////////////

LogLevelManager::LogLevelManager()
{
    pushToStringMethod (defaultLogLevelToStringMethod);

    pushFromStringMethod (defaultStringToLogLevelMethod);
}


LogLevelManager::~LogLevelManager()
{ }



//////////////////////////////////////////////////////////////////////////////
// LogLevelManager public methods
//////////////////////////////////////////////////////////////////////////////

tstring const &
LogLevelManager::toString(LogLevel ll) const
{
    tstring const * ret;
    for (LogLevelToStringMethodList::const_iterator it
        = toStringMethods.begin (); it != toStringMethods.end (); ++it)
    {
        LogLevelToStringMethodRec const & rec = *it;
        if (rec.use_1_0)
        {
            // Use TLS to store the result to allow us to return
            // a reference.
            tstring & ll_str = internal::get_ptd ()->ll_str;
            rec.func_1_0 (ll).swap (ll_str);
            ret = &ll_str;
        }
        else
            ret = &rec.func (ll);

        if (! ret->empty ())
            return *ret;
    }

    return UNKNOWN_STRING;
}


LogLevel
LogLevelManager::fromString(const tstring& arg) const
{
    tstring s = helpers::toUpper(arg);

    for (StringToLogLevelMethodList::const_iterator it
        = fromStringMethods.begin (); it != fromStringMethods.end (); ++it)
    {
        LogLevel ret = (*it) (s);
        if (ret != NOT_SET_LOG_LEVEL)
            return ret;
    }

    helpers::getLogLog ().error (
        LOG4CPLUS_TEXT ("Unrecognized log level: ")
        + arg);

    return NOT_SET_LOG_LEVEL;
}


void
LogLevelManager::pushToStringMethod(LogLevelToStringMethod newToString)
{
    LogLevelToStringMethodRec rec;
    rec.func = newToString;
    rec.use_1_0 = false;
    toStringMethods.insert (toStringMethods.begin (), rec);
}


void
LogLevelManager::pushToStringMethod(LogLevelToStringMethod_1_0 newToString)
{
    LogLevelToStringMethodRec rec;
    rec.func_1_0 = newToString;
    rec.use_1_0 = true;
    toStringMethods.insert (toStringMethods.begin (), rec);
}


void
LogLevelManager::pushFromStringMethod(StringToLogLevelMethod newFromString)
{
    fromStringMethods.insert (fromStringMethods.begin (), newFromString);
}


} // namespace log4cplus
