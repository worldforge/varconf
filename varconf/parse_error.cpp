/*
 *  parse_error.cpp - implementation of the parse error handling class
 *  Copyright (C) 2000, Stefanus Du Toit, Joseph Zupko
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  For more information on the GPL, please go to: 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 *      Contact:  Joseph Zupko
 *                jaz147@psu.edu
 *
 *                189 Reese St.
 *                Old Forge, PA 18518
 */

#include <cstdio>
#include <iostream>
#include "parse_error.h"

using namespace std;

namespace varconf {

ParseError::ParseError( const ParseError& p)
{
  m_exp  = p.m_exp;
  m_line = p.m_line;
  m_col  = p.m_col;
}

ParseError::ParseError( const string& exp, int line, int col)
{
  m_exp  = exp;
  m_line = line;
  m_col  = col;
}

ostream& operator<<( ostream& os, const ParseError& p)
{
  return ( os << "ParseError: Expected " << p.m_exp << " at line " << p.m_line
              << ", column " << p.m_col << "." << endl);
}

ParseError::operator string() 
{
  char buf[1024];
  snprintf( buf, 1024, "ParseError: Expected %s at line %d, column %d.",
            m_exp.c_str(), m_line, m_col);
  return string( buf);
}
 
} // namespace varconf
