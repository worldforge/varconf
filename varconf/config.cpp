/*
 *  config.cpp - implementation of the main configuration class.
 *  Copyright (C) 2001, Stefanus Du Toit, Joseph Zupko
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact:  Joseph Zupko
 *            jaz147@psu.edu
 *
 *            189 Reese St.
 *            Old Forge, PA 18518
 */
 
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include <varconf/Config.h>

#ifdef __WIN32__
#include <tchar.h>
#define snprintf _snprintf
#endif

extern char **environ;

using namespace SigC;

namespace {
  enum state_t {
    S_EXPECT_NAME, // Expect the start of a name/section/comment
    S_SECTION, // Parsing a section name
    S_NAME, // Parsing an item name
    S_COMMENT, // Parsing a comment
    S_EXPECT_EQ, // Expect an equal sign
    S_EXPECT_VALUE, // Expect the start of a value
    S_VALUE, // Parsing a value
    S_QUOTED_VALUE, // Parsing a "quoted" value
    S_EXPECT_EOL // Expect the end of the line
  };

  enum ctype_t {
    C_SPACE, // Whitespace
    C_NUMERIC, // 0-9
    C_ALPHA, // a-z, A-Z
    C_DASH, // '-' and '_'
    C_EQ, // '='
    C_QUOTE, // '"'
    C_SQUARE_OPEN, // '['
    C_SQUARE_CLOSE, // ']'
    C_HASH, // '#'
    C_ESCAPE, // '\' (an "escape")
    C_EOL, // End of the line
    C_OTHER // Anything else
  };

  ctype_t ctype( char c)
  {
    if ( c=='\n') return C_EOL;
    if ( isspace(c)) return C_SPACE;
    if ( ( (c >= 'a') && ( c <= 'z') || ( c >= 'A') && ( c <= 'Z'))) 
      return C_ALPHA;
    if ( isdigit( c)) return C_NUMERIC;
    if ( ( c == '-') || ( c == '_')) return C_DASH;
    if ( c == '=') return C_EQ;
    if ( c == '"') return C_QUOTE;
    if ( c == '[') return C_SQUARE_OPEN;
    if ( c == ']') return C_SQUARE_CLOSE;
    if ( c == '#') return C_HASH;
    if ( c == '\\') return C_ESCAPE;
    return C_OTHER;
  }
}

namespace varconf {

Config* Config::m_instance;

Config* Config::inst()
{
  if ( m_instance == NULL) 
    m_instance = new Config;
  
  return m_instance;
}

Config::Config( const Config& conf)
{
  m_conf = conf.m_conf;
  m_par_lookup = conf.m_par_lookup;
}

std::ostream& operator <<( std::ostream& out, Config& conf)
{
  if ( !conf.writeToStream( out))
    conf.sige.emit( "\nVarconf Error: error while trying to write "
                    "configuration data to output stream.\n");
 
  return out;
}

std::istream& operator >>( std::istream& in, Config& conf)
{
  try {
    conf.parseStream( in);
  }
  catch ( ParseError p) {
    char buf[1024];
    std::string p_str = p;
    snprintf( buf, 1024, "\nVarconf Error: parser exception throw while "
                         "parsing input stream.\n%s", p_str.c_str());   
    conf.sige.emit( buf);
  }

  return in;
} 

bool operator ==( const Config& one, const Config& two)
{
  if ( one.m_conf == two.m_conf && one.m_par_lookup == two.m_par_lookup)
    return true;
  else
    return false;
}

void Config::clean( std::string& str)
{
  ctype_t c;

  for ( size_t i = 0; i < str.size(); i++) {
    c = ctype( str[i]);

    if ( c != C_NUMERIC && c != C_ALPHA && c != C_DASH) 
      str[i] = '_';
    else 
      str[i] = (char) tolower( str[i]);
  } 
}

bool Config::erase( const std::string& section, const std::string& key)
{
  if ( find( section)) {
    if ( key == "") {
      m_conf.erase( section);
      return true;
    }
    else if ( find( section, key)) {
      m_conf[section].erase( key);
      return true;
    }
  }
 
  return false;
}

bool Config::find( const std::string& section, const std::string& key)
{
  if ( m_conf.count( section)) {
    if ( key == "") 
      return true;
    else if ( m_conf[section].count( key))
      return true;
  }

  return false;
}

bool Config::findItem( const std::string& section, const std::string& key)
{
  return find( section, key);
}

void Config::getCmdline( int argc, char** argv)
{
  std::string section = "", name = "", value = "", arg;
  bool fnd_sec = false, fnd_nam = false;
  size_t mark = 2;

  for ( size_t i = 1; i < argc; i++) {
    if ( argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] != '\0') {
      arg = argv[i];
      fnd_sec = fnd_nam = false;
      section = name = value = "";
      mark = 2;
       
      for ( size_t j = 2; j < arg.size(); j++) {
        if ( arg[j] == ':' && arg[j+1] != '\0' && !fnd_sec && !fnd_nam) {
          section = arg.substr( mark, ( j - mark));
          fnd_sec = true;
          mark = j + 1;
        }
        else if ( arg[j] == '=' && ( j - mark) > 1) {
          name = arg.substr( mark, ( j - mark));
          fnd_nam = true;
          value = arg.substr( ( j + 1), ( arg.size() - ( j + 1)));
          break;
        }
      }

      if ( !fnd_nam && ( arg.size() - mark) > 0) 
        name = arg.substr( mark, ( arg.size() - mark));

    } // long argument 
    else if( argv[i][0] == '-' && argv[i][1] != '-' && argv[i][1] != '\0') {
      parameter_map::iterator I = m_par_lookup.find( argv[i][1]);

      section = name = value = "";

      if ( I != m_par_lookup.end()) {
        name = ( ( *I).second).first;
        bool needs_value = ( ( *I).second).second;

        if ( needs_value && argv[i+1] != NULL && argv[i+1][0] != '-'
                         && argv[i+1] != "") {
          value = argv[++i];
        } 
        else {
          char buf[1024];
          snprintf( buf, 1024, "\nVarconf Warning: short argument \"%s\""
                               " given on command-line expects a value"
                               " but none was given.\n", argv[i]); 
          sige.emit( buf);
        }
      }
      else {
        char buf[1024];
        snprintf( buf, 1024, "\nVarconf Warning: short argument \"%s\""
                             " given on command-line does not exist in"
                             " the lookup table.\n", argv[i]);
        sige.emit( buf);
      }
    } // short argument

    if ( !name.empty()) 
      setItem( section, name, value);
  }
}

void Config::getEnv( const std::string& prefix)
{
  std::string name = "", value = "", section = "", env = "";
  size_t eq_pos = 0;

  for ( size_t i = 0; environ[i] != NULL; i++) {
    env = environ[i];

    if ( env.substr( 0, prefix.size()) == prefix) {
      eq_pos = env.find( '='); 

      if ( eq_pos != std::string::npos) {
        name = env.substr( prefix.size(), ( eq_pos - prefix.size()));
        value = env.substr( ( eq_pos + 1), ( env.size() - ( eq_pos + 1)));
      }
      else {
        name = env.substr( prefix.size(), ( env.size() - prefix.size()));
        value = "";
      }   
      
      setItem( section, name, value);
    }
  }
}

Variable Config::getItem( const std::string& section, const std::string& key)
{
  return ( m_conf[section])[key];
}

void Config::parseStream( std::istream& in) throw ( ParseError)
{
  char c; 
  bool escaped = false;
  size_t line = 1, col = 0;
  std::string name = "", value = "", section = "";
  state_t state = S_EXPECT_NAME;

  while ( in.get( c)) {
    col++;
    switch ( state) {
      case S_EXPECT_NAME : 
	switch ( ctype( c)) {
	  case C_ALPHA:
	  case C_NUMERIC:
	  case C_DASH:
	    state = S_NAME;
	    name = c;
	    break;
	  case C_SQUARE_OPEN:
            section = "";
	    state = S_SECTION;
	    break;
	  case C_SPACE:
	  case C_EOL:
	    break;
	  case C_HASH:
	    state = S_COMMENT;
	    break;
	  default:
	    throw ParseError( "item name", (int) line, (int) col);
	    break;
	}
	break;
      case S_SECTION :
	switch ( ctype( c)) {
	  case C_ALPHA:
	  case C_NUMERIC:
	    section += c;
	    break;
	  case C_SQUARE_CLOSE:
	    state = S_EXPECT_EOL;
	    break;
	  default:
	    throw ParseError( "']'", (int) line, (int) col);
	    break;
	}
        break;
      case S_NAME :
	switch ( ctype( c)) {
	  case C_ALPHA:
	  case C_NUMERIC:
	  case C_DASH:
	    name += c;
	    break;
	  case C_EQ:
	    state = S_EXPECT_VALUE;
	    break;
	  case C_SPACE:
	    state = S_EXPECT_EQ;
	    break;
	  default:
	    throw ParseError( "'='", (int) line, (int) col);
	    break;
	}
	break;
      case S_COMMENT :
	switch ( ctype( c)) {
	  case C_EOL:
	    state = S_EXPECT_NAME;
	    break;
	  default:
	    break;
        }
	break;
      case S_EXPECT_EQ:
	switch ( ctype( c)) {
	  case C_SPACE:
	    break;
	  case C_EQ:
	    state = S_EXPECT_VALUE;
	    break;
	  default:
	    throw ParseError( "'='", (int) line, (int) col);
	    break;
	}
	break;
      case S_EXPECT_VALUE:
	switch ( ctype( c)) {
	  case C_ALPHA:
	  case C_NUMERIC:
	  case C_DASH:
	    state = S_VALUE;
	    value = c;
	    break;
	  case C_QUOTE:
            value = "";
	    state = S_QUOTED_VALUE;
	    break;
	  case C_SPACE:
	    break;
	  default:
	    throw ParseError( "value", (int) line, (int) col);
	    break;
	}
	break;
      case S_VALUE:
	switch ( ctype( c)) {
	  case C_QUOTE:
	    throw ParseError( "value", (int) line, (int) col);
	  case C_SPACE:
	    state = S_EXPECT_EOL;
	    setItem( section, name, value);
	    break;
	  case C_EOL:
	    state = S_EXPECT_NAME;
	    setItem( section, name, value);
	    break;
	  case C_HASH:
	    state = S_COMMENT;
	    setItem( section, name, value);
	    break;
	  default:
	    value += c;
	    break;
	}
	break;
      case S_QUOTED_VALUE:
        if ( escaped) {
          value += c;
	  escaped = false;
        } else {
	  switch ( ctype( c)) {
	    case C_QUOTE:
	      state = S_EXPECT_EOL;
	      setItem( section, name, value);
	      break;
	    case C_ESCAPE:
	      escaped = true;
	      break;
	    default:
	      value += c;
	      break;
	  }
	}
	break;
      case S_EXPECT_EOL:
	switch ( ctype( c)) {
	  case C_HASH:
	    state = S_COMMENT;
	    break;
	  case C_EOL:
	    state = S_EXPECT_NAME;
	    break;
	  case C_SPACE:
	    break;
	  default:
	    throw ParseError( "end of line", (int) line, (int) col);
            break;
	}
	break;
      default:
        break;
    }
    if ( c == '\n') {
      line++;
      col = 0;
    }
  } // while ( in.get( c))

  if ( state == S_QUOTED_VALUE) {
    throw ParseError( "\"", (int) line, (int) col);
  }

  if ( state == S_VALUE) {
    setItem( section, name, value);
  }
}

bool Config::readFromFile( const std::string& filename)
{
  std::ifstream fin( filename.c_str());
  
  if ( fin.fail()) {
    char buf[1024];
    snprintf( buf, 1024, "\nVarconf Error: could not open configuration file"
                         " \"%s\" for input.\n", filename.c_str());    
    sige.emit( buf);
    
    return false;
  }

  try {
    parseStream( fin);
  }
  catch ( ParseError p) {
    char buf[1024];
    std::string p_str = p;
    snprintf( buf, 1024, "\nVarconf Error: parsing exception thrown while "
                       "parsing \"%s\".\n%s", filename.c_str(), p_str.c_str());
    sige.emit( buf);
    return false;
  }
  
  return true;
}

void Config::setItem( const std::string& section, const std::string& key, const Variable& item)
{
  if ( key.empty()) {
    char buf[1024];
    snprintf( buf, 1024, "\nVarconf Warning: blank key under section \"%s\""
                         " sent to setItem() method.\n", section.c_str());
    sige.emit( buf);
  }
  else {
    std::string sec_clean = section; 
    std::string key_clean = key; 

    clean( sec_clean);
    clean( key_clean);

    ( m_conf[sec_clean])[key_clean] = item;
 
    sig.emit(); 
    sigv.emit( sec_clean, key_clean);
    sigsv.emit( sec_clean, key_clean, *this);
  }
}

void Config::setParameterLookup( char s_name, const std::string& l_name, bool value)
{
    m_par_lookup[s_name] = std::pair<std::string, bool>( l_name, value);  
}

bool Config::writeToFile( const std::string& filename)
{
  std::ofstream fout( filename.c_str());

  if ( fout.fail()) {
    char buf[1024];
    snprintf( buf, 1024, "\nVarconf Error: could not open configuration file"
                         " \"%s\" for output.\n", filename.c_str());
    sige.emit( buf);

    return false;
  }

  return writeToStream( fout);
}

bool Config::writeToStream( std::ostream& out)
{
  conf_map::iterator I;
  sec_map::iterator J;
 
  for ( I = m_conf.begin(); I != m_conf.end(); I++) {
    out << std::endl << "[" << ( *I).first << "]\n\n";
    
    for ( J = ( *I).second.begin(); J != ( *I).second.end(); J++) 
      out << ( *J).first << " = \"" << ( *J).second << "\"\n";
  }
  
  return true;
}

} // namespace varconf
