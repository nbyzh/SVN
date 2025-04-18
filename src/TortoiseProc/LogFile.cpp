﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2010-2011, 2014-2016, 2021, 2024 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "LogFile.h"
#include "PathUtils.h"

#include "Streams/MappedInFile.h"
#include "Streams/StreamException.h"

CLogFile::CLogFile()
{
    m_maxLines = CRegStdDWORD(L"Software\\TortoiseSVN\\MaxLinesInLogfile", 4000);
}

CLogFile::~CLogFile()
{
}

bool CLogFile::Open()
{
    CTSVNPath logfile = CTSVNPath(CPathUtils::GetLocalAppDataDirectory() + L"\\logfile.txt");
    return Open(logfile);
}

bool CLogFile::Open(const CTSVNPath& logfile)
{
    if (m_maxLines == 0)
        return false; // do nothing if no log lines should be used.
    m_newLines.clear();
    m_logFile = logfile;
    if (!logfile.Exists())
    {
        CPathUtils::MakeSureDirectoryPathExists(logfile.GetContainingDirectory().GetWinPath());
    }

    return true;
}

bool CLogFile::AddLine(const CString& line)
{
    m_newLines.push_back(line);
    return true;
}

bool CLogFile::Close()
{
    try
    {
        // limit log file growth

        size_t maxLines = static_cast<DWORD>(m_maxLines);
        size_t newLines = m_newLines.size();
        TrimFile(static_cast<DWORD>(max(maxLines, newLines) - newLines));

        // append new info

        CStdioFile file;

        int retryCounter = 10;
        // try to open the file for about two seconds - some other TSVN process might be
        // writing to the file and we just wait a little for this to finish
        while (!file.Open(m_logFile.GetWinPath(), CFile::typeText | CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate) && retryCounter)
        {
            retryCounter--;
            Sleep(200);
        }
        if (retryCounter == 0)
            return false;

        file.SeekToEnd();
        for (std::deque<CString>::const_iterator it = m_newLines.begin(); it != m_newLines.end(); ++it)
        {
            file.WriteString(*it);
            file.WriteString(L"\n");
        }
        file.Close();
    }
    catch (CFileException* pE)
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException saving log file\n");
        pE->Delete();
        return false;
    }
    return true;
}

bool CLogFile::AddTimeLine()
{
    CString sLine;
    // first add an empty line as a separator
    m_newLines.push_back(sLine);
    // now add the time string
    wchar_t dateBuf[4096] = {0};
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, dateBuf, 4096);
    sLine = dateBuf;
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, nullptr, nullptr, dateBuf, 4096);
    sLine += L" - ";
    sLine += dateBuf;
    m_newLines.push_back(sLine);
    return true;
}

void CLogFile::TrimFile(DWORD maxLines) const
{
    if (!PathFileExists(m_logFile.GetWinPath()))
        return;

    // find the start of the maxLines-th last line
    // (\n is always a new line - regardless of the encoding)

    try
    {
        CMappedInFile file(m_logFile.GetWinPath(), true);

        unsigned char* begin = file.GetWritableBuffer();
        unsigned char* end   = begin + file.GetSize();

        if (begin >= (end - 2))
            return;

        unsigned char* trimPos = begin;
        for (unsigned char* s = end; s != begin; --s)
        {
            if (*(s - 1) == '\n')
            {
                if (maxLines == 0)
                {
                    trimPos = s;
                    break;
                }
                else
                    --maxLines;
            }
        }

        // need to remove lines from the beginning of the file?

        if (trimPos == begin)
            return;

        // remove data

        size_t newSize = end - trimPos;
        memmove(begin, trimPos, newSize);
        file.Close(newSize);
    }
    catch (CStreamException&)
    {
        // can't trim the file right now
    }
}
