﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013, 2020 - TortoiseSVN

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
#pragma once

#ifndef __STREAMEXCEPTION_H__
#define __STREAMEXCEPTION_H__

class CStreamException : public std::exception
{
private:

    // store the message in this buffer

    enum {MAX_MESSAGE_LEN = 100};
    char message[MAX_MESSAGE_LEN+1];

public:

    // construction / destruction

    CStreamException(const char* message);
    virtual ~CStreamException();

    // implement message access

    virtual const char* what() const;
};

#endif
