/// Json-cpp amalgated source (http://jsoncpp.sourceforge.net/).
/// It is intented to be used with #include <json/json.h>

// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: LICENSE
// //////////////////////////////////////////////////////////////////////

/*
The JsonCpp library's source code, including accompanying documentation, 
tests and demonstration applications, are licensed under the following
conditions...

The author (Baptiste Lepilleur) explicitly disclaims copyright in all 
jurisdictions which recognize such a disclaimer. In such jurisdictions, 
this software is released into the Public Domain.

In jurisdictions which do not recognize Public Domain property (e.g. Germany as of
2010), this software is Copyright (c) 2007-2010 by Baptiste Lepilleur, and is
released under the terms of the MIT License (see below).

In jurisdictions which recognize Public Domain property, the user of this 
software may choose to accept it either as 1) Public Domain, 2) under the 
conditions of the MIT License (see below), or 3) under the terms of dual 
Public Domain/MIT License conditions described here, as they choose.

The MIT License is about as close to Public Domain as a license can get, and is
described in clear, concise terms at:

   http://en.wikipedia.org/wiki/MIT_License
   
The full text of the MIT License follows:

========================================================================
Copyright (c) 2007-2010 Baptiste Lepilleur

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
========================================================================
(END LICENSE TEXT)

The MIT license is compatible with both the GPL and commercial
software, affording one all of the rights of Public Domain with the
minor nuisance of being required to keep the above copyright notice
and license text in the source code. Note also that by accepting the
Public Domain "license" you can re-license your copy using whatever
license you like.

*/

// //////////////////////////////////////////////////////////////////////
// End of content of file: LICENSE
// //////////////////////////////////////////////////////////////////////






#include "json/json.h"
#include <math.h>


// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_tool.h
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef LIB_JSONCPP_JSON_TOOL_H_INCLUDED
# define LIB_JSONCPP_JSON_TOOL_H_INCLUDED

/* This header provides common string manipulation support, such as UTF-8,
 * portable conversion from/to string...
 *
 * It is an internal header that must not be exposed.
 */

bool g_compactCommonFloats_rightNow = true;
bool g_compactZeroVecs_rightNow = true;

namespace Json {

/// Converts a unicode code-point to UTF-8.
static inline std::string 
codePointToUTF8(unsigned int cp)
{
   std::string result;
   
   // based on description from http://en.wikipedia.org/wiki/UTF-8

   if (cp <= 0x7f) 
   {
      result.resize(1);
      result[0] = static_cast<char>(cp);
   } 
   else if (cp <= 0x7FF) 
   {
      result.resize(2);
      result[1] = static_cast<char>(0x80 | (0x3f & cp));
      result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
   } 
   else if (cp <= 0xFFFF) 
   {
      result.resize(3);
      result[2] = static_cast<char>(0x80 | (0x3f & cp));
      result[1] = 0x80 | static_cast<char>((0x3f & (cp >> 6)));
      result[0] = 0xE0 | static_cast<char>((0xf & (cp >> 12)));
   }
   else if (cp <= 0x10FFFF) 
   {
      result.resize(4);
      result[3] = static_cast<char>(0x80 | (0x3f & cp));
      result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
      result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
      result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
   }

   return result;
}


/// Returns true if ch is a control character (in range [0,32[).
static inline bool 
isControlCharacter(char ch)
{
   return ch > 0 && ch <= 0x1F;
}


enum { 
   /// Constant that specify the size of the buffer that must be passed to uintToString.
   uintToStringBufferSize = 3*sizeof(LargestUInt)+1 
};

// Defines a char buffer for use with uintToString().
typedef char UIntToStringBuffer[uintToStringBufferSize];


/** Converts an unsigned integer to string.
 * @param value Unsigned interger to convert to string
 * @param current Input/Output string buffer. 
 *        Must have at least uintToStringBufferSize chars free.
 */
static inline void 
uintToString( LargestUInt value, 
              char *&current )
{
   *--current = 0;
   do
   {
      *--current = char(value % 10) + '0';
      value /= 10;
   }
   while ( value != 0 );
}

} // namespace Json {

#endif // LIB_JSONCPP_JSON_TOOL_H_INCLUDED

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_tool.h
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_reader.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
# include <json/reader.h>
# include <json/value.h>
# include "json_tool.h"
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <utility>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>

#if _MSC_VER >= 1400 // VC++ 8.0
#pragma warning( disable : 4996 )   // disable warning about strdup being deprecated.
#endif

namespace Json {

// Implementation of class Features
// ////////////////////////////////

Features::Features()
   : allowComments_( true )
   , strictRoot_( false )
{
}


Features 
Features::all()
{
   return Features();
}


Features 
Features::strictMode()
{
   Features features;
   features.allowComments_ = false;
   features.strictRoot_ = true;
   return features;
}

// Implementation of class Reader
// ////////////////////////////////


static inline bool 
in( Reader::Char c, Reader::Char c1, Reader::Char c2, Reader::Char c3, Reader::Char c4 )
{
   return c == c1  ||  c == c2  ||  c == c3  ||  c == c4;
}

static inline bool 
in( Reader::Char c, Reader::Char c1, Reader::Char c2, Reader::Char c3, Reader::Char c4, Reader::Char c5 )
{
   return c == c1  ||  c == c2  ||  c == c3  ||  c == c4  ||  c == c5;
}


static bool 
containsNewLine( Reader::Location begin, 
                 Reader::Location end )
{
   for ( ;begin < end; ++begin )
      if ( *begin == '\n'  ||  *begin == '\r' )
         return true;
   return false;
}


// Class Reader
// //////////////////////////////////////////////////////////////////

Reader::Reader()
   : features_( Features::all() )
{
}


Reader::Reader( const Features &features )
   : features_( features )
{
}


bool
Reader::parse( const std::string &document, 
               Value &root,
               bool collectComments )
{
   document_ = document;
   const char *begin = document_.c_str();
   const char *end = begin + document_.length();
   return parse( begin, end, root, collectComments );
}


bool
Reader::parse( std::istream& sin,
               Value &root,
               bool collectComments )
{
   //std::istream_iterator<char> begin(sin);
   //std::istream_iterator<char> end;
   // Those would allow streamed input from a file, if parse() were a
   // template function.

   // Since std::string is reference-counted, this at least does not
   // create an extra copy.
   std::string doc;
   std::getline(sin, doc, (char)EOF);
   return parse( doc, root, collectComments );
}

bool 
Reader::parse( const char *beginDoc, const char *endDoc, 
               Value &root,
               bool collectComments )
{
   if ( !features_.allowComments_ )
   {
      collectComments = false;
   }

   begin_ = beginDoc;
   end_ = endDoc;
   collectComments_ = collectComments;
   current_ = begin_;
   lastValueEnd_ = 0;
   lastValue_ = 0;
   commentsBefore_ = "";
   errors_.clear();
   while ( !nodes_.empty() )
      nodes_.pop();
   nodes_.push( &root );
   
   bool successful = readValue();
   Token token;
   skipCommentTokens( token );
   if ( collectComments_  &&  !commentsBefore_.empty() )
      root.setComment( commentsBefore_, commentAfter );
   if ( features_.strictRoot_ )
   {
      if ( !root.isArray()  &&  !root.isObject() )
      {
         // Set error location to start of doc, ideally should be first token found in doc
         token.type_ = tokenError;
         token.start_ = beginDoc;
         token.end_ = endDoc;
         addError( "A valid JSON document must be either an array or an object value.",
                   token );
         return false;
      }
   }
   return successful;
}


bool
Reader::readValue()
{
   Token token;
   skipCommentTokens( token );
   bool successful = true;

   if ( collectComments_  &&  !commentsBefore_.empty() )
   {
      currentValue().setComment( commentsBefore_, commentBefore );
      commentsBefore_ = "";
   }


   switch ( token.type_ )
   {
   case tokenObjectBegin:
      successful = readObject( token );
      break;
   case tokenArrayBegin:
      successful = readArray( token );
      break;
   case tokenNumber:
      successful = decodeNumber( token );
      break;
   case tokenString:
      successful = decodeString( token );
      break;
   case tokenTrue:
      currentValue() = true;
      break;
   case tokenFalse:
      currentValue() = false;
      break;
   case tokenNull:
      currentValue() = Value();
      break;
   default:
      return addError( "Syntax error: value, object or array expected.", token );
   }

   if ( collectComments_ )
   {
      lastValueEnd_ = current_;
      lastValue_ = &currentValue();
   }

   return successful;
}


void 
Reader::skipCommentTokens( Token &token )
{
   if ( features_.allowComments_ )
   {
      do
      {
         readToken( token );
      }
      while ( token.type_ == tokenComment );
   }
   else
   {
      readToken( token );
   }
}


bool 
Reader::expectToken( TokenType type, Token &token, const char *message )
{
   readToken( token );
   if ( token.type_ != type )
      return addError( message, token );
   return true;
}


bool 
Reader::readToken( Token &token )
{
   skipSpaces();
   token.start_ = current_;
   Char c = getNextChar();
   bool ok = true;
   switch ( c )
   {
   case '{':
      token.type_ = tokenObjectBegin;
      break;
   case '}':
      token.type_ = tokenObjectEnd;
      break;
   case '[':
      token.type_ = tokenArrayBegin;
      break;
   case ']':
      token.type_ = tokenArrayEnd;
      break;
   case '"':
      token.type_ = tokenString;
      ok = readString();
      break;
   case '/':
      token.type_ = tokenComment;
      ok = readComment();
      break;
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
   case '-':
      token.type_ = tokenNumber;
      readNumber();
      break;
   case 't':
      token.type_ = tokenTrue;
      ok = match( "rue", 3 );
      break;
   case 'f':
      token.type_ = tokenFalse;
      ok = match( "alse", 4 );
      break;
   case 'n':
      token.type_ = tokenNull;
      ok = match( "ull", 3 );
      break;
   case ',':
      token.type_ = tokenArraySeparator;
      break;
   case ':':
      token.type_ = tokenMemberSeparator;
      break;
   case 0:
      token.type_ = tokenEndOfStream;
      break;
   default:
      ok = false;
      break;
   }
   if ( !ok )
      token.type_ = tokenError;
   token.end_ = current_;
   return true;
}


void 
Reader::skipSpaces()
{
   while ( current_ != end_ )
   {
      Char c = *current_;
      if ( c == ' '  ||  c == '\t'  ||  c == '\r'  ||  c == '\n' )
         ++current_;
      else
         break;
   }
}


bool 
Reader::match( Location pattern, 
               int patternLength )
{
   if ( end_ - current_ < patternLength )
      return false;
   int index = patternLength;
   while ( index-- )
      if ( current_[index] != pattern[index] )
         return false;
   current_ += patternLength;
   return true;
}


bool
Reader::readComment()
{
   Location commentBegin = current_ - 1;
   Char c = getNextChar();
   bool successful = false;
   if ( c == '*' )
      successful = readCStyleComment();
   else if ( c == '/' )
      successful = readCppStyleComment();
   if ( !successful )
      return false;

   if ( collectComments_ )
   {
      CommentPlacement placement = commentBefore;
      if ( lastValueEnd_  &&  !containsNewLine( lastValueEnd_, commentBegin ) )
      {
         if ( c != '*'  ||  !containsNewLine( commentBegin, current_ ) )
            placement = commentAfterOnSameLine;
      }

      addComment( commentBegin, current_, placement );
   }
   return true;
}


void 
Reader::addComment( Location begin, 
                    Location end, 
                    CommentPlacement placement )
{
   assert( collectComments_ );
   if ( placement == commentAfterOnSameLine )
   {
      assert( lastValue_ != 0 );
      lastValue_->setComment( std::string( begin, end ), placement );
   }
   else
   {
      if ( !commentsBefore_.empty() )
         commentsBefore_ += "\n";
      commentsBefore_ += std::string( begin, end );
   }
}


bool 
Reader::readCStyleComment()
{
   while ( current_ != end_ )
   {
      Char c = getNextChar();
      if ( c == '*'  &&  *current_ == '/' )
         break;
   }
   return getNextChar() == '/';
}


bool 
Reader::readCppStyleComment()
{
   while ( current_ != end_ )
   {
      Char c = getNextChar();
      if (  c == '\r'  ||  c == '\n' )
         break;
   }
   return true;
}


void 
Reader::readNumber()
{
   while ( current_ != end_ )
   {
      if ( !(*current_ >= '0'  &&  *current_ <= '9')  &&
           !in( *current_, '.', 'e', 'E', '+', '-' ) )
         break;
      ++current_;
   }
}

bool
Reader::readString()
{
   Char c = 0;
   while ( current_ != end_ )
   {
      c = getNextChar();
      if ( c == '\\' )
         getNextChar();
      else if ( c == '"' )
         break;
   }
   return c == '"';
}


bool 
Reader::readObject( Token &/*tokenStart*/ )
{
   Token tokenName;
   std::string name;
   currentValue() = Value( objectValue );
   while ( readToken( tokenName ) )
   {
      bool initialTokenOk = true;
      while ( tokenName.type_ == tokenComment  &&  initialTokenOk )
         initialTokenOk = readToken( tokenName );
      if  ( !initialTokenOk )
         break;
      if ( tokenName.type_ == tokenObjectEnd  &&  name.empty() )  // empty object
         return true;
      if ( tokenName.type_ != tokenString )
         break;
      
      name = "";
      if ( !decodeString( tokenName, name ) )
         return recoverFromError( tokenObjectEnd );

      Token colon;
      if ( !readToken( colon ) ||  colon.type_ != tokenMemberSeparator )
      {
         return addErrorAndRecover( "Missing ':' after object member name", 
                                    colon, 
                                    tokenObjectEnd );
      }
      Value &value = currentValue()[ name ];
      nodes_.push( &value );
      bool ok = readValue();
      nodes_.pop();
      if ( !ok ) // error already set
         return recoverFromError( tokenObjectEnd );

      Token comma;
      if ( !readToken( comma )
            ||  ( comma.type_ != tokenObjectEnd  &&  
                  comma.type_ != tokenArraySeparator &&
                  comma.type_ != tokenComment ) )
      {
         return addErrorAndRecover( "Missing ',' or '}' in object declaration", 
                                    comma, 
                                    tokenObjectEnd );
      }
      bool finalizeTokenOk = true;
      while ( comma.type_ == tokenComment &&
              finalizeTokenOk )
         finalizeTokenOk = readToken( comma );
      if ( comma.type_ == tokenObjectEnd )
         return true;
   }
   return addErrorAndRecover( "Missing '}' or object member name", 
                              tokenName, 
                              tokenObjectEnd );
}


bool 
Reader::readArray( Token &/*tokenStart*/ )
{
   currentValue() = Value( arrayValue );
   skipSpaces();
   if ( *current_ == ']' ) // empty array
   {
      Token endArray;
      readToken( endArray );
      return true;
   }
   int index = 0;
   for (;;)
   {
      Value &value = currentValue()[ index++ ];
      nodes_.push( &value );
      bool ok = readValue();
      nodes_.pop();
      if ( !ok ) // error already set
         return recoverFromError( tokenArrayEnd );

      Token token;
      // Accept Comment after last item in the array.
      ok = readToken( token );
      while ( token.type_ == tokenComment  &&  ok )
      {
         ok = readToken( token );
      }
      bool badTokenType = ( token.type_ != tokenArraySeparator  &&
                            token.type_ != tokenArrayEnd );
      if ( !ok  ||  badTokenType )
      {
         return addErrorAndRecover( "Missing ',' or ']' in array declaration", 
                                    token, 
                                    tokenArrayEnd );
      }
      if ( token.type_ == tokenArrayEnd )
         break;
   }
   return true;
}


bool 
Reader::decodeNumber( Token &token )
{
   bool isDouble = false;
   for ( Location inspect = token.start_; inspect != token.end_; ++inspect )
   {
      isDouble = isDouble  
                 ||  in( *inspect, '.', 'e', 'E', '+' )  
                 ||  ( *inspect == '-'  &&  inspect != token.start_ );
   }
   if ( isDouble )
      return decodeDouble( token );
   // Attempts to parse the number as an integer. If the number is
   // larger than the maximum supported value of an integer then
   // we decode the number as a double.
   Location current = token.start_;
   bool isNegative = *current == '-';
   if ( isNegative )
      ++current;
   Value::LargestUInt maxIntegerValue = isNegative ? Value::LargestUInt(-Value::minLargestInt) 
                                                   : Value::maxLargestUInt;
   Value::LargestUInt threshold = maxIntegerValue / 10;
   Value::UInt lastDigitThreshold = Value::UInt( maxIntegerValue % 10 );
   assert( /*lastDigitThreshold >=0  &&*/  lastDigitThreshold <= 9 );
   Value::LargestUInt value = 0;
   while ( current < token.end_ )
   {
      Char c = *current++;
      if ( c < '0'  ||  c > '9' )
         return addError( "'" + std::string( token.start_, token.end_ ) + "' is not a number.", token );
      Value::UInt digit(c - '0');
      if ( value >= threshold )
      {
         // If the current digit is not the last one, or if it is
         // greater than the last digit of the maximum integer value,
         // the parse the number as a double.
         if ( current != token.end_  ||  digit > lastDigitThreshold )
         {
            return decodeDouble( token );
         }
      }
      value = value * 10 + digit;
   }
   if ( isNegative )
      currentValue() = -Value::LargestInt( value );
   else if ( value <= Value::LargestUInt(Value::maxInt) )
      currentValue() = Value::LargestInt( value );
   else
      currentValue() = value;
   return true;
}


bool 
Reader::decodeDouble( Token &token )
{
   double value = 0;
   const int bufferSize = 32;
   int count;
   int length = int(token.end_ - token.start_);
   if ( length <= bufferSize )
   {
      Char buffer[bufferSize+1];
      memcpy( buffer, token.start_, length );
      buffer[length] = 0;
      count = sscanf( buffer, "%lf", &value );
   }
   else
   {
      std::string buffer( token.start_, token.end_ );
      count = sscanf( buffer.c_str(), "%lf", &value );
   }

   if ( count != 1 )
      return addError( "'" + std::string( token.start_, token.end_ ) + "' is not a number.", token );
   currentValue() = value;
   return true;
}


bool 
Reader::decodeString( Token &token )
{
   std::string decoded;
   if ( !decodeString( token, decoded ) )
      return false;
   currentValue() = decoded;
   return true;
}


bool 
Reader::decodeString( Token &token, std::string &decoded )
{
   decoded.reserve( token.end_ - token.start_ - 2 );
   Location current = token.start_ + 1; // skip '"'
   Location end = token.end_ - 1;      // do not include '"'
   while ( current != end )
   {
      Char c = *current++;
      if ( c == '"' )
         break;
      else if ( c == '\\' )
      {
         if ( current == end )
            return addError( "Empty escape sequence in string", token, current );
         Char escape = *current++;
         switch ( escape )
         {
         case '"': decoded += '"'; break;
         case '/': decoded += '/'; break;
         case '\\': decoded += '\\'; break;
         case 'b': decoded += '\b'; break;
         case 'f': decoded += '\f'; break;
         case 'n': decoded += '\n'; break;
         case 'r': decoded += '\r'; break;
         case 't': decoded += '\t'; break;
         case 'u':
            {
               unsigned int unicode;
               if ( !decodeUnicodeCodePoint( token, current, end, unicode ) )
                  return false;
               decoded += codePointToUTF8(unicode);
            }
            break;
         default:
            return addError( "Bad escape sequence in string", token, current );
         }
      }
      else
      {
         decoded += c;
      }
   }
   return true;
}

bool
Reader::decodeUnicodeCodePoint( Token &token, 
                                     Location &current, 
                                     Location end, 
                                     unsigned int &unicode )
{

   if ( !decodeUnicodeEscapeSequence( token, current, end, unicode ) )
      return false;
   if (unicode >= 0xD800 && unicode <= 0xDBFF)
   {
      // surrogate pairs
      if (end - current < 6)
         return addError( "additional six characters expected to parse unicode surrogate pair.", token, current );
      unsigned int surrogatePair;
      if (*(current++) == '\\' && *(current++)== 'u')
      {
         if (decodeUnicodeEscapeSequence( token, current, end, surrogatePair ))
         {
            unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
         } 
         else
            return false;
      } 
      else
         return addError( "expecting another \\u token to begin the second half of a unicode surrogate pair", token, current );
   }
   return true;
}

bool 
Reader::decodeUnicodeEscapeSequence( Token &token, 
                                     Location &current, 
                                     Location end, 
                                     unsigned int &unicode )
{
   if ( end - current < 4 )
      return addError( "Bad unicode escape sequence in string: four digits expected.", token, current );
   unicode = 0;
   for ( int index =0; index < 4; ++index )
   {
      Char c = *current++;
      unicode *= 16;
      if ( c >= '0'  &&  c <= '9' )
         unicode += c - '0';
      else if ( c >= 'a'  &&  c <= 'f' )
         unicode += c - 'a' + 10;
      else if ( c >= 'A'  &&  c <= 'F' )
         unicode += c - 'A' + 10;
      else
         return addError( "Bad unicode escape sequence in string: hexadecimal digit expected.", token, current );
   }
   return true;
}


bool 
Reader::addError( const std::string &message, 
                  Token &token,
                  Location extra )
{
   ErrorInfo info;
   info.token_ = token;
   info.message_ = message;
   info.extra_ = extra;
   errors_.push_back( info );
   return false;
}


bool 
Reader::recoverFromError( TokenType skipUntilToken )
{
   int errorCount = int(errors_.size());
   Token skip;
   for (;;)
   {
      if ( !readToken(skip) )
         errors_.resize( errorCount ); // discard errors caused by recovery
      if ( skip.type_ == skipUntilToken  ||  skip.type_ == tokenEndOfStream )
         break;
   }
   errors_.resize( errorCount );
   return false;
}


bool 
Reader::addErrorAndRecover( const std::string &message, 
                            Token &token,
                            TokenType skipUntilToken )
{
   addError( message, token );
   return recoverFromError( skipUntilToken );
}


Value &
Reader::currentValue()
{
   return *(nodes_.top());
}


Reader::Char 
Reader::getNextChar()
{
   if ( current_ == end_ )
      return 0;
   return *current_++;
}


void 
Reader::getLocationLineAndColumn( Location location,
                                  int &line,
                                  int &column ) const
{
   Location current = begin_;
   Location lastLineStart = current;
   line = 0;
   while ( current < location  &&  current != end_ )
   {
      Char c = *current++;
      if ( c == '\r' )
      {
         if ( *current == '\n' )
            ++current;
         lastLineStart = current;
         ++line;
      }
      else if ( c == '\n' )
      {
         lastLineStart = current;
         ++line;
      }
   }
   // column & line start at 1
   column = int(location - lastLineStart) + 1;
   ++line;
}


std::string
Reader::getLocationLineAndColumn( Location location ) const
{
   int line, column;
   getLocationLineAndColumn( location, line, column );
   char buffer[18+16+16+1];
   sprintf( buffer, "Line %d, Column %d", line, column );
   return buffer;
}


// Deprecated. Preserved for backward compatibility
std::string 
Reader::getFormatedErrorMessages() const
{
    return getFormattedErrorMessages();
}


std::string 
Reader::getFormattedErrorMessages() const
{
   std::string formattedMessage;
   for ( Errors::const_iterator itError = errors_.begin();
         itError != errors_.end();
         ++itError )
   {
      const ErrorInfo &error = *itError;
      formattedMessage += "* " + getLocationLineAndColumn( error.token_.start_ ) + "\n";
      formattedMessage += "  " + error.message_ + "\n";
      if ( error.extra_ )
         formattedMessage += "See " + getLocationLineAndColumn( error.extra_ ) + " for detail.\n";
   }
   return formattedMessage;
}


std::istream& operator>>( std::istream &sin, Value &root )
{
    Json::Reader reader;
    bool ok = reader.parse(sin, root, true);
    //JSON_ASSERT( ok );
    if (!ok) throw std::runtime_error(reader.getFormattedErrorMessages());
    return sin;
}


} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_reader.cpp
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_batchallocator.h
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef JSONCPP_BATCHALLOCATOR_H_INCLUDED
# define JSONCPP_BATCHALLOCATOR_H_INCLUDED

# include <stdlib.h>
# include <assert.h>

# ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION

namespace Json {

/* Fast memory allocator.
 *
 * This memory allocator allocates memory for a batch of object (specified by
 * the page size, the number of object in each page).
 *
 * It does not allow the destruction of a single object. All the allocated objects
 * can be destroyed at once. The memory can be either released or reused for future
 * allocation.
 * 
 * The in-place new operator must be used to construct the object using the pointer
 * returned by allocate.
 */
template<typename AllocatedType
        ,const unsigned int objectPerAllocation>
class BatchAllocator
{
public:
   typedef AllocatedType Type;

   BatchAllocator( unsigned int objectsPerPage = 255 )
      : freeHead_( 0 )
      , objectsPerPage_( objectsPerPage )
   {
//      printf( "Size: %d => %s\n", sizeof(AllocatedType), typeid(AllocatedType).name() );
      assert( sizeof(AllocatedType) * objectPerAllocation >= sizeof(AllocatedType *) ); // We must be able to store a slist in the object free space.
      assert( objectsPerPage >= 16 );
      batches_ = allocateBatch( 0 );   // allocated a dummy page
      currentBatch_ = batches_;
   }

   ~BatchAllocator()
   {
      for ( BatchInfo *batch = batches_; batch;  )
      {
         BatchInfo *nextBatch = batch->next_;
         free( batch );
         batch = nextBatch;
      }
   }

   /// allocate space for an array of objectPerAllocation object.
   /// @warning it is the responsability of the caller to call objects constructors.
   AllocatedType *allocate()
   {
      if ( freeHead_ ) // returns node from free list.
      {
         AllocatedType *object = freeHead_;
         freeHead_ = *(AllocatedType **)object;
         return object;
      }
      if ( currentBatch_->used_ == currentBatch_->end_ )
      {
         currentBatch_ = currentBatch_->next_;
         while ( currentBatch_  &&  currentBatch_->used_ == currentBatch_->end_ )
            currentBatch_ = currentBatch_->next_;

         if ( !currentBatch_  ) // no free batch found, allocate a new one
         { 
            currentBatch_ = allocateBatch( objectsPerPage_ );
            currentBatch_->next_ = batches_; // insert at the head of the list
            batches_ = currentBatch_;
         }
      }
      AllocatedType *allocated = currentBatch_->used_;
      currentBatch_->used_ += objectPerAllocation;
      return allocated;
   }

   /// Release the object.
   /// @warning it is the responsability of the caller to actually destruct the object.
   void release( AllocatedType *object )
   {
      assert( object != 0 );
      *(AllocatedType **)object = freeHead_;
      freeHead_ = object;
   }

private:
   struct BatchInfo
   {
      BatchInfo *next_;
      AllocatedType *used_;
      AllocatedType *end_;
      AllocatedType buffer_[objectPerAllocation];
   };

   // disabled copy constructor and assignement operator.
   BatchAllocator( const BatchAllocator & );
   void operator =( const BatchAllocator &);

   static BatchInfo *allocateBatch( unsigned int objectsPerPage )
   {
      const unsigned int mallocSize = sizeof(BatchInfo) - sizeof(AllocatedType)* objectPerAllocation
                                + sizeof(AllocatedType) * objectPerAllocation * objectsPerPage;
      BatchInfo *batch = static_cast<BatchInfo*>( malloc( mallocSize ) );
      batch->next_ = 0;
      batch->used_ = batch->buffer_;
      batch->end_ = batch->buffer_ + objectsPerPage;
      return batch;
   }

   BatchInfo *batches_;
   BatchInfo *currentBatch_;
   /// Head of a single linked list within the allocated space of freeed object
   AllocatedType *freeHead_;
   unsigned int objectsPerPage_;
};


} // namespace Json

# endif // ifndef JSONCPP_DOC_INCLUDE_IMPLEMENTATION

#endif // JSONCPP_BATCHALLOCATOR_H_INCLUDED


// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_batchallocator.h
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_valueiterator.inl
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

// included by json_value.cpp

namespace Json {

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class ValueIteratorBase
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

ValueIteratorBase::ValueIteratorBase()
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   : current_()
   , isNull_( true )
{
}
#else
   : isArray_( true )
   , isNull_( true )
{
   iterator_.array_ = ValueInternalArray::IteratorState();
}
#endif


#ifndef JSON_VALUE_USE_INTERNAL_MAP
ValueIteratorBase::ValueIteratorBase( const Value::ObjectValues::iterator &current )
   : current_( current )
   , isNull_( false )
{
}
#else
ValueIteratorBase::ValueIteratorBase( const ValueInternalArray::IteratorState &state )
   : isArray_( true )
{
   iterator_.array_ = state;
}


ValueIteratorBase::ValueIteratorBase( const ValueInternalMap::IteratorState &state )
   : isArray_( false )
{
   iterator_.map_ = state;
}
#endif

Value &
ValueIteratorBase::deref() const
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   return current_->second;
#else
   if ( isArray_ )
      return ValueInternalArray::dereference( iterator_.array_ );
   return ValueInternalMap::value( iterator_.map_ );
#endif
}


void 
ValueIteratorBase::increment()
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   ++current_;
#else
   if ( isArray_ )
      ValueInternalArray::increment( iterator_.array_ );
   ValueInternalMap::increment( iterator_.map_ );
#endif
}


void 
ValueIteratorBase::decrement()
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   --current_;
#else
   if ( isArray_ )
      ValueInternalArray::decrement( iterator_.array_ );
   ValueInternalMap::decrement( iterator_.map_ );
#endif
}


ValueIteratorBase::difference_type 
ValueIteratorBase::computeDistance( const SelfType &other ) const
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
# ifdef JSON_USE_CPPTL_SMALLMAP
   return current_ - other.current_;
# else
   // Iterator for null value are initialized using the default
   // constructor, which initialize current_ to the default
   // std::map::iterator. As begin() and end() are two instance 
   // of the default std::map::iterator, they can not be compared.
   // To allow this, we handle this comparison specifically.
   if ( isNull_  &&  other.isNull_ )
   {
      return 0;
   }


   // Usage of std::distance is not portable (does not compile with Sun Studio 12 RogueWave STL,
   // which is the one used by default).
   // Using a portable hand-made version for non random iterator instead:
   //   return difference_type( std::distance( current_, other.current_ ) );
   difference_type myDistance = 0;
   for ( Value::ObjectValues::iterator it = current_; it != other.current_; ++it )
   {
      ++myDistance;
   }
   return myDistance;
# endif
#else
   if ( isArray_ )
      return ValueInternalArray::distance( iterator_.array_, other.iterator_.array_ );
   return ValueInternalMap::distance( iterator_.map_, other.iterator_.map_ );
#endif
}


bool 
ValueIteratorBase::isEqual( const SelfType &other ) const
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   if ( isNull_ )
   {
      return other.isNull_;
   }
   return current_ == other.current_;
#else
   if ( isArray_ )
      return ValueInternalArray::equals( iterator_.array_, other.iterator_.array_ );
   return ValueInternalMap::equals( iterator_.map_, other.iterator_.map_ );
#endif
}


void 
ValueIteratorBase::copy( const SelfType &other )
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   current_ = other.current_;
#else
   if ( isArray_ )
      iterator_.array_ = other.iterator_.array_;
   iterator_.map_ = other.iterator_.map_;
#endif
}


Value 
ValueIteratorBase::key() const
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   const Value::CZString czstring = (*current_).first;
   if ( czstring.c_str() )
   {
      if ( czstring.isStaticString() )
         return Value( StaticString( czstring.c_str() ) );
      return Value( czstring.c_str() );
   }
   return Value( czstring.index() );
#else
   if ( isArray_ )
      return Value( ValueInternalArray::indexOf( iterator_.array_ ) );
   bool isStatic;
   const char *memberName = ValueInternalMap::key( iterator_.map_, isStatic );
   if ( isStatic )
      return Value( StaticString( memberName ) );
   return Value( memberName );
#endif
}


UInt 
ValueIteratorBase::index() const
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   const Value::CZString czstring = (*current_).first;
   if ( !czstring.c_str() )
      return czstring.index();
   return Value::UInt( -1 );
#else
   if ( isArray_ )
      return Value::UInt( ValueInternalArray::indexOf( iterator_.array_ ) );
   return Value::UInt( -1 );
#endif
}


const char *
ValueIteratorBase::memberName() const
{
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   const char *name = (*current_).first.c_str();
   return name ? name : "";
#else
   if ( !isArray_ )
      return ValueInternalMap::key( iterator_.map_ );
   return "";
#endif
}


// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class ValueConstIterator
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

ValueConstIterator::ValueConstIterator()
{
}


#ifndef JSON_VALUE_USE_INTERNAL_MAP
ValueConstIterator::ValueConstIterator( const Value::ObjectValues::iterator &current )
   : ValueIteratorBase( current )
{
}
#else
ValueConstIterator::ValueConstIterator( const ValueInternalArray::IteratorState &state )
   : ValueIteratorBase( state )
{
}

ValueConstIterator::ValueConstIterator( const ValueInternalMap::IteratorState &state )
   : ValueIteratorBase( state )
{
}
#endif

ValueConstIterator &
ValueConstIterator::operator =( const ValueIteratorBase &other )
{
   copy( other );
   return *this;
}


// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class ValueIterator
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

ValueIterator::ValueIterator()
{
}


#ifndef JSON_VALUE_USE_INTERNAL_MAP
ValueIterator::ValueIterator( const Value::ObjectValues::iterator &current )
   : ValueIteratorBase( current )
{
}
#else
ValueIterator::ValueIterator( const ValueInternalArray::IteratorState &state )
   : ValueIteratorBase( state )
{
}

ValueIterator::ValueIterator( const ValueInternalMap::IteratorState &state )
   : ValueIteratorBase( state )
{
}
#endif

ValueIterator::ValueIterator( const ValueConstIterator &other )
   : ValueIteratorBase( other )
{
}

ValueIterator::ValueIterator( const ValueIterator &other )
   : ValueIteratorBase( other )
{
}

ValueIterator &
ValueIterator::operator =( const SelfType &other )
{
   copy( other );
   return *this;
}

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_valueiterator.inl
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_value.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
# include <json/value.h>
# include <json/writer.h>
# ifndef JSON_USE_SIMPLE_INTERNAL_ALLOCATOR
#  include "json_batchallocator.h"
# endif // #ifndef JSON_USE_SIMPLE_INTERNAL_ALLOCATOR
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <iostream>
#include <utility>
#include <stdexcept>
#include <cstring>
#include <cassert>
#ifdef JSON_USE_CPPTL
# include <cpptl/conststring.h>
#endif
#include <cstddef>    // size_t

#define JSON_ASSERT_UNREACHABLE assert( false )
#define JSON_ASSERT( condition ) assert( condition );  // @todo <= change this into an exception throw
#define JSON_FAIL_MESSAGE( message ) throw std::runtime_error( message );
#define JSON_ASSERT_MESSAGE( condition, message ) if (!( condition )) JSON_FAIL_MESSAGE( message )

namespace Json {

const Value Value::null;
const Int Value::minInt = Int( ~(UInt(-1)/2) );
const Int Value::maxInt = Int( UInt(-1)/2 );
const UInt Value::maxUInt = UInt(-1);
const Int64 Value::minInt64 = Int64( ~(UInt64(-1)/2) );
const Int64 Value::maxInt64 = Int64( UInt64(-1)/2 );
const UInt64 Value::maxUInt64 = UInt64(-1);
const LargestInt Value::minLargestInt = LargestInt( ~(LargestUInt(-1)/2) );
const LargestInt Value::maxLargestInt = LargestInt( LargestUInt(-1)/2 );
const LargestUInt Value::maxLargestUInt = LargestUInt(-1);


/// Unknown size marker
static const unsigned int unknown = (unsigned)-1;


/** Duplicates the specified string value.
 * @param value Pointer to the string to duplicate. Must be zero-terminated if
 *              length is "unknown".
 * @param length Length of the value. if equals to unknown, then it will be
 *               computed using strlen(value).
 * @return Pointer on the duplicate instance of string.
 */
static inline char *
duplicateStringValue( const char *value, 
                      unsigned int length = unknown )
{
   if ( length == unknown )
      length = (unsigned int)strlen(value);
   char *newString = static_cast<char *>( malloc( length + 1 ) );
   JSON_ASSERT_MESSAGE( newString != 0, "Failed to allocate string value buffer" );
   memcpy( newString, value, length );
   newString[length] = 0;
   return newString;
}


/** Free the string duplicated by duplicateStringValue().
 */
static inline void 
releaseStringValue( char *value )
{
   if ( value )
      free( value );
}

} // namespace Json


// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// ValueInternals...
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
#if !defined(JSON_IS_AMALGAMATION)
# ifdef JSON_VALUE_USE_INTERNAL_MAP
#  include "json_internalarray.inl"
#  include "json_internalmap.inl"
# endif // JSON_VALUE_USE_INTERNAL_MAP

# include "json_valueiterator.inl"
#endif // if !defined(JSON_IS_AMALGAMATION)

namespace Json {

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::CommentInfo
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////


Value::CommentInfo::CommentInfo()
   : comment_( 0 )
{
}

Value::CommentInfo::~CommentInfo()
{
   if ( comment_ )
      releaseStringValue( comment_ );
}


void 
Value::CommentInfo::setComment( const char *text )
{
   if ( comment_ )
      releaseStringValue( comment_ );
   JSON_ASSERT( text != 0 );
   JSON_ASSERT_MESSAGE( text[0]=='\0' || text[0]=='/', "Comments must start with /");
   // It seems that /**/ style comments are acceptable as well.
   comment_ = duplicateStringValue( text );
}


// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::CZString
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
# ifndef JSON_VALUE_USE_INTERNAL_MAP

// Notes: index_ indicates if the string was allocated when
// a string is stored.

Value::CZString::CZString( ArrayIndex index )
   : cstr_( 0 )
   , index_( index )
{
}

Value::CZString::CZString( const char *cstr, DuplicationPolicy allocate )
   : cstr_( allocate == duplicate ? duplicateStringValue(cstr) 
                                  : cstr )
   , index_( allocate )
{
}

Value::CZString::CZString( const CZString &other )
: cstr_( other.index_ != noDuplication &&  other.cstr_ != 0
                ?  duplicateStringValue( other.cstr_ )
                : other.cstr_ )
   , index_( other.cstr_ ? (other.index_ == noDuplication ? noDuplication : duplicate)
                         : (DuplicationPolicy)other.index_ )
{
}

Value::CZString::~CZString()
{
   if ( cstr_  &&  index_ == duplicate )
      releaseStringValue( const_cast<char *>( cstr_ ) );
}

void 
Value::CZString::swap( CZString &other )
{
   std::swap( cstr_, other.cstr_ );
   std::swap( index_, other.index_ );
}

Value::CZString &
Value::CZString::operator =( const CZString &other )
{
   CZString temp( other );
   swap( temp );
   return *this;
}

bool 
Value::CZString::operator<( const CZString &other ) const 
{
   if ( cstr_ )
      return strcmp( cstr_, other.cstr_ ) < 0;
   return index_ < other.index_;
}

bool 
Value::CZString::operator==( const CZString &other ) const 
{
   if ( cstr_ )
      return strcmp( cstr_, other.cstr_ ) == 0;
   return index_ == other.index_;
}


ArrayIndex 
Value::CZString::index() const
{
   return index_;
}


const char *
Value::CZString::c_str() const
{
   return cstr_;
}

bool 
Value::CZString::isStaticString() const
{
   return index_ == noDuplication;
}

#endif // ifndef JSON_VALUE_USE_INTERNAL_MAP


// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::Value
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

/*! \internal Default constructor initialization must be equivalent to:
 * memset( this, 0, sizeof(Value) )
 * This optimization is used in ValueInternalMap fast allocator.
 */
Value::Value( ValueType type )
   : type_( type )
   , allocated_( 0 )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   switch ( type )
   {
   case nullValue:
      break;
   case intValue:
   case uintValue:
      value_.int_ = 0;
      break;
   case realValue:
      value_.real_ = 0.0;
      break;
   case stringValue:
      value_.string_ = 0;
      break;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
   case objectValue:
      value_.map_ = new ObjectValues();
      break;
#else
   case arrayValue:
      value_.array_ = arrayAllocator()->newArray();
      break;
   case objectValue:
      value_.map_ = mapAllocator()->newMap();
      break;
#endif
   case booleanValue:
      value_.bool_ = false;
      break;
   default:
      JSON_ASSERT_UNREACHABLE;
   }
}


#if defined(JSON_HAS_INT64)
Value::Value( UInt value )
   : type_( uintValue )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.uint_ = value;
}

Value::Value( Int value )
   : type_( intValue )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.int_ = value;
}

#endif // if defined(JSON_HAS_INT64)


Value::Value( Int64 value )
   : type_( intValue )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.int_ = value;
}


Value::Value( UInt64 value )
   : type_( uintValue )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.uint_ = value;
}

Value::Value( double value )
   : type_( realValue )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.real_ = value;
}

Value::Value( const char *value )
   : type_( stringValue )
   , allocated_( true )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.string_ = duplicateStringValue( value );
}


Value::Value( const char *beginValue, 
              const char *endValue )
   : type_( stringValue )
   , allocated_( true )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.string_ = duplicateStringValue( beginValue, 
                                          (unsigned int)(endValue - beginValue) );
}


Value::Value( const std::string &value )
   : type_( stringValue )
   , allocated_( true )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.string_ = duplicateStringValue( value.c_str(), 
                                          (unsigned int)value.length() );

}

Value::Value( const StaticString &value )
   : type_( stringValue )
   , allocated_( false )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.string_ = const_cast<char *>( value.c_str() );
}


# ifdef JSON_USE_CPPTL
Value::Value( const CppTL::ConstString &value )
   : type_( stringValue )
   , allocated_( true )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.string_ = duplicateStringValue( value, value.length() );
}
# endif

Value::Value( bool value )
   : type_( booleanValue )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   value_.bool_ = value;
}


Value::Value( const Value &other )
   : type_( other.type_ )
   , comments_( 0 )
# ifdef JSON_VALUE_USE_INTERNAL_MAP
   , itemIsUsed_( 0 )
#endif
{
   switch ( type_ )
   {
   case nullValue:
   case intValue:
   case uintValue:
   case realValue:
   case booleanValue:
      value_ = other.value_;
      break;
   case stringValue:
      if ( other.value_.string_ )
      {
         value_.string_ = duplicateStringValue( other.value_.string_ );
         allocated_ = true;
      }
      else
         value_.string_ = 0;
      break;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
   case objectValue:
      value_.map_ = new ObjectValues( *other.value_.map_ );
      break;
#else
   case arrayValue:
      value_.array_ = arrayAllocator()->newArrayCopy( *other.value_.array_ );
      break;
   case objectValue:
      value_.map_ = mapAllocator()->newMapCopy( *other.value_.map_ );
      break;
#endif
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   if ( other.comments_ )
   {
      comments_ = new CommentInfo[numberOfCommentPlacement];
      for ( int comment =0; comment < numberOfCommentPlacement; ++comment )
      {
         const CommentInfo &otherComment = other.comments_[comment];
         if ( otherComment.comment_ )
            comments_[comment].setComment( otherComment.comment_ );
      }
   }
}


Value::~Value()
{
   switch ( type_ )
   {
   case nullValue:
   case intValue:
   case uintValue:
   case realValue:
   case booleanValue:
      break;
   case stringValue:
      if ( allocated_ )
         releaseStringValue( value_.string_ );
      break;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
   case objectValue:
      delete value_.map_;
      break;
#else
   case arrayValue:
      arrayAllocator()->destructArray( value_.array_ );
      break;
   case objectValue:
      mapAllocator()->destructMap( value_.map_ );
      break;
#endif
   default:
      JSON_ASSERT_UNREACHABLE;
   }

   if ( comments_ )
      delete[] comments_;
}

Value &
Value::operator=( const Value &other )
{
   Value temp( other );
   swap( temp );
   return *this;
}

void 
Value::swap( Value &other )
{
   ValueType temp = type_;
   type_ = other.type_;
   other.type_ = temp;
   std::swap( value_, other.value_ );
   int temp2 = allocated_;
   allocated_ = other.allocated_;
   other.allocated_ = temp2;
}

ValueType 
Value::type() const
{
   return type_;
}


int 
Value::compare( const Value &other ) const
{
   if ( *this < other )
      return -1;
   if ( *this > other )
      return 1;
   return 0;
}


bool 
Value::operator <( const Value &other ) const
{
   int typeDelta = type_ - other.type_;
   if ( typeDelta )
      return typeDelta < 0 ? true : false;
   switch ( type_ )
   {
   case nullValue:
      return false;
   case intValue:
      return value_.int_ < other.value_.int_;
   case uintValue:
      return value_.uint_ < other.value_.uint_;
   case realValue:
      return value_.real_ < other.value_.real_;
   case booleanValue:
      return value_.bool_ < other.value_.bool_;
   case stringValue:
      return ( value_.string_ == 0  &&  other.value_.string_ )
             || ( other.value_.string_  
                  &&  value_.string_  
                  && strcmp( value_.string_, other.value_.string_ ) < 0 );
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
   case objectValue:
      {
         int delta = int( value_.map_->size() - other.value_.map_->size() );
         if ( delta )
            return delta < 0;
         return (*value_.map_) < (*other.value_.map_);
      }
#else
   case arrayValue:
      return value_.array_->compare( *(other.value_.array_) ) < 0;
   case objectValue:
      return value_.map_->compare( *(other.value_.map_) ) < 0;
#endif
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return false;  // unreachable
}

bool 
Value::operator <=( const Value &other ) const
{
   return !(other < *this);
}

bool 
Value::operator >=( const Value &other ) const
{
   return !(*this < other);
}

bool 
Value::operator >( const Value &other ) const
{
   return other < *this;
}

bool 
Value::operator ==( const Value &other ) const
{
   //if ( type_ != other.type_ )
   // GCC 2.95.3 says:
   // attempt to take address of bit-field structure member `Json::Value::type_'
   // Beats me, but a temp solves the problem.
   int temp = other.type_;
   if ( type_ != temp )
      return false;
   switch ( type_ )
   {
   case nullValue:
      return true;
   case intValue:
      return value_.int_ == other.value_.int_;
   case uintValue:
      return value_.uint_ == other.value_.uint_;
   case realValue:
      return value_.real_ == other.value_.real_;
   case booleanValue:
      return value_.bool_ == other.value_.bool_;
   case stringValue:
      return ( value_.string_ == other.value_.string_ )
             || ( other.value_.string_  
                  &&  value_.string_  
                  && strcmp( value_.string_, other.value_.string_ ) == 0 );
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
   case objectValue:
      return value_.map_->size() == other.value_.map_->size()
             && (*value_.map_) == (*other.value_.map_);
#else
   case arrayValue:
      return value_.array_->compare( *(other.value_.array_) ) == 0;
   case objectValue:
      return value_.map_->compare( *(other.value_.map_) ) == 0;
#endif
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return false;  // unreachable
}

bool 
Value::operator !=( const Value &other ) const
{
   return !( *this == other );
}

const char *
Value::asCString() const
{
   JSON_ASSERT( type_ == stringValue );
   return value_.string_;
}


std::string 
Value::asString() const
{
   switch ( type_ )
   {
   case nullValue:
      return "";
   case stringValue:
      return value_.string_ ? value_.string_ : "";
   case booleanValue:
      return value_.bool_ ? "true" : "false";
   case intValue:
   case uintValue:
   case realValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to string" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return ""; // unreachable
}

# ifdef JSON_USE_CPPTL
CppTL::ConstString 
Value::asConstString() const
{
   return CppTL::ConstString( asString().c_str() );
}
# endif


Value::Int 
Value::asInt() const
{
   switch ( type_ )
   {
   case nullValue:
      return 0;
   case intValue:
      JSON_ASSERT_MESSAGE( value_.int_ >= minInt  &&  value_.int_ <= maxInt, "unsigned integer out of signed int range" );
      return Int(value_.int_);
   case uintValue:
      JSON_ASSERT_MESSAGE( value_.uint_ <= UInt(maxInt), "unsigned integer out of signed int range" );
      return Int(value_.uint_);
   case realValue:
      JSON_ASSERT_MESSAGE( value_.real_ >= minInt  &&  value_.real_ <= maxInt, "Real out of signed integer range" );
      return Int( value_.real_ );
   case booleanValue:
      return value_.bool_ ? 1 : 0;
   case stringValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to int" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0; // unreachable;
}


Value::UInt 
Value::asUInt() const
{
   switch ( type_ )
   {
   case nullValue:
      return 0;
   case intValue:
      JSON_ASSERT_MESSAGE( value_.int_ >= 0, "Negative integer can not be converted to unsigned integer" );
      JSON_ASSERT_MESSAGE( value_.int_ <= maxUInt, "signed integer out of UInt range" );
      return UInt(value_.int_);
   case uintValue:
      JSON_ASSERT_MESSAGE( value_.uint_ <= maxUInt, "unsigned integer out of UInt range" );
      return UInt(value_.uint_);
   case realValue:
      JSON_ASSERT_MESSAGE( value_.real_ >= 0  &&  value_.real_ <= maxUInt,  "Real out of unsigned integer range" );
      return UInt( value_.real_ );
   case booleanValue:
      return value_.bool_ ? 1 : 0;
   case stringValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to uint" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0; // unreachable;
}


# if defined(JSON_HAS_INT64)

Value::Int64
Value::asInt64() const
{
   switch ( type_ )
   {
   case nullValue:
      return 0;
   case intValue:
      return value_.int_;
   case uintValue:
      JSON_ASSERT_MESSAGE( value_.uint_ <= UInt64(maxInt64), "unsigned integer out of Int64 range" );
      return value_.uint_;
   case realValue:
      JSON_ASSERT_MESSAGE( value_.real_ >= minInt64  &&  value_.real_ <= maxInt64, "Real out of Int64 range" );
      return Int( value_.real_ );
   case booleanValue:
      return value_.bool_ ? 1 : 0;
   case stringValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to Int64" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0; // unreachable;
}


Value::UInt64
Value::asUInt64() const
{
   switch ( type_ )
   {
   case nullValue:
      return 0;
   case intValue:
      JSON_ASSERT_MESSAGE( value_.int_ >= 0, "Negative integer can not be converted to UInt64" );
      return value_.int_;
   case uintValue:
      return value_.uint_;
   case realValue:
      JSON_ASSERT_MESSAGE( value_.real_ >= 0  &&  value_.real_ <= maxUInt64,  "Real out of UInt64 range" );
      return UInt( value_.real_ );
   case booleanValue:
      return value_.bool_ ? 1 : 0;
   case stringValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to UInt64" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0; // unreachable;
}
# endif // if defined(JSON_HAS_INT64)


LargestInt 
Value::asLargestInt() const
{
#if defined(JSON_NO_INT64)
    return asInt();
#else
    return asInt64();
#endif
}


LargestUInt 
Value::asLargestUInt() const
{
#if defined(JSON_NO_INT64)
    return asUInt();
#else
    return asUInt64();
#endif
}


double 
Value::asDouble() const
{
   switch ( type_ )
   {
   case nullValue:
      return 0.0;
   case intValue:
      return static_cast<double>( value_.int_ );
   case uintValue:
#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
      return static_cast<double>( value_.uint_ );
#else // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
      return static_cast<double>( Int(value_.uint_/2) ) * 2 + Int(value_.uint_ & 1);
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
   case realValue:
      return value_.real_;
   case booleanValue:
      return value_.bool_ ? 1.0 : 0.0;
   case stringValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to double" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0; // unreachable;
}

float
Value::asFloat() const
{
   switch ( type_ )
   {
   case nullValue:
      return 0.0f;
   case intValue:
      return static_cast<float>( value_.int_ );
   case uintValue:
#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
      return static_cast<float>( value_.uint_ );
#else // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
      return static_cast<float>( Int(value_.uint_/2) ) * 2 + Int(value_.uint_ & 1);
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
   case realValue:
      return static_cast<float>( value_.real_ );
   case booleanValue:
      return value_.bool_ ? 1.0f : 0.0f;
   case stringValue:
   case arrayValue:
   case objectValue:
      JSON_FAIL_MESSAGE( "Type is not convertible to float" );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0.0f; // unreachable;
}

bool 
Value::asBool() const
{
   switch ( type_ )
   {
   case nullValue:
      return false;
   case intValue:
   case uintValue:
      return value_.int_ != 0;
   case realValue:
      return value_.real_ != 0.0;
   case booleanValue:
      return value_.bool_;
   case stringValue:
      return value_.string_  &&  value_.string_[0] != 0;
   case arrayValue:
   case objectValue:
      return value_.map_->size() != 0;
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return false; // unreachable;
}


bool 
Value::isConvertibleTo( ValueType other ) const
{
   switch ( type_ )
   {
   case nullValue:
      return true;
   case intValue:
      return ( other == nullValue  &&  value_.int_ == 0 )
             || other == intValue
             || ( other == uintValue  && value_.int_ >= 0 )
             || other == realValue
             || other == stringValue
             || other == booleanValue;
   case uintValue:
      return ( other == nullValue  &&  value_.uint_ == 0 )
             || ( other == intValue  && value_.uint_ <= (unsigned)maxInt )
             || other == uintValue
             || other == realValue
             || other == stringValue
             || other == booleanValue;
   case realValue:
      return ( other == nullValue  &&  value_.real_ == 0.0 )
             || ( other == intValue  &&  value_.real_ >= minInt  &&  value_.real_ <= maxInt )
             || ( other == uintValue  &&  value_.real_ >= 0  &&  value_.real_ <= maxUInt )
             || other == realValue
             || other == stringValue
             || other == booleanValue;
   case booleanValue:
      return ( other == nullValue  &&  value_.bool_ == false )
             || other == intValue
             || other == uintValue
             || other == realValue
             || other == stringValue
             || other == booleanValue;
   case stringValue:
      return other == stringValue
             || ( other == nullValue  &&  (!value_.string_  ||  value_.string_[0] == 0) );
   case arrayValue:
      return other == arrayValue
             ||  ( other == nullValue  &&  value_.map_->size() == 0 );
   case objectValue:
      return other == objectValue
             ||  ( other == nullValue  &&  value_.map_->size() == 0 );
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return false; // unreachable;
}


/// Number of values in array or object
ArrayIndex 
Value::size() const
{
   switch ( type_ )
   {
   case nullValue:
   case intValue:
   case uintValue:
   case realValue:
   case booleanValue:
   case stringValue:
      return 0;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:  // size of the array is highest index + 1
      if ( !value_.map_->empty() )
      {
         ObjectValues::const_iterator itLast = value_.map_->end();
         --itLast;
         return (*itLast).first.index()+1;
      }
      return 0;
   case objectValue:
      return ArrayIndex( value_.map_->size() );
#else
   case arrayValue:
      return Int( value_.array_->size() );
   case objectValue:
      return Int( value_.map_->size() );
#endif
   default:
      JSON_ASSERT_UNREACHABLE;
   }
   return 0; // unreachable;
}


bool 
Value::empty() const
{
   if ( isNull() || isArray() || isObject() )
      return size() == 0u;
   else
      return false;
}


bool
Value::operator!() const
{
   return isNull();
}


void 
Value::clear()
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == arrayValue  || type_ == objectValue );

   switch ( type_ )
   {
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
   case objectValue:
      value_.map_->clear();
      break;
#else
   case arrayValue:
      value_.array_->clear();
      break;
   case objectValue:
      value_.map_->clear();
      break;
#endif
   default:
      break;
   }
}

void 
Value::resize( ArrayIndex newSize )
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == arrayValue );
   if ( type_ == nullValue )
      *this = Value( arrayValue );
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   ArrayIndex oldSize = size();
   if ( newSize == 0 )
      clear();
   else if ( newSize > oldSize )
      (*this)[ newSize - 1 ];
   else
   {
      for ( ArrayIndex index = newSize; index < oldSize; ++index )
      {
         value_.map_->erase( index );
      }
      assert( size() == newSize );
   }
#else
   value_.array_->resize( newSize );
#endif
}


Value &
Value::operator[]( ArrayIndex index )
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == arrayValue );
   if ( type_ == nullValue )
      *this = Value( arrayValue );
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   CZString key( index );
   ObjectValues::iterator it = value_.map_->lower_bound( key );
   if ( it != value_.map_->end()  &&  (*it).first == key )
      return (*it).second;

   ObjectValues::value_type defaultValue( key, null );
   it = value_.map_->insert( it, defaultValue );
   return (*it).second;
#else
   return value_.array_->resolveReference( index );
#endif
}


Value &
Value::operator[]( int index )
{
   JSON_ASSERT( index >= 0 );
   return (*this)[ ArrayIndex(index) ];
}


const Value &
Value::operator[]( ArrayIndex index ) const
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == arrayValue );
   if ( type_ == nullValue )
      return null;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   CZString key( index );
   ObjectValues::const_iterator it = value_.map_->find( key );
   if ( it == value_.map_->end() )
      return null;
   return (*it).second;
#else
   Value *value = value_.array_->find( index );
   return value ? *value : null;
#endif
}


const Value &
Value::operator[]( int index ) const
{
   JSON_ASSERT( index >= 0 );
   return (*this)[ ArrayIndex(index) ];
}


Value &
Value::operator[]( const char *key )
{
   return resolveReference( key, false );
}


Value &
Value::resolveReference( const char *key, 
                         bool isStatic )
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == objectValue );
   if ( type_ == nullValue )
      *this = Value( objectValue );
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   CZString actualKey( key, isStatic ? CZString::noDuplication 
                                     : CZString::duplicateOnCopy );
   ObjectValues::iterator it = value_.map_->lower_bound( actualKey );
   if ( it != value_.map_->end()  &&  (*it).first == actualKey )
      return (*it).second;

   ObjectValues::value_type defaultValue( actualKey, null );
   it = value_.map_->insert( it, defaultValue );
   Value &value = (*it).second;
   return value;
#else
   return value_.map_->resolveReference( key, isStatic );
#endif
}


Value 
Value::get( ArrayIndex index, 
            const Value &defaultValue ) const
{
   const Value *value = &((*this)[index]);
   return value == &null ? defaultValue : *value;
}


bool 
Value::isValidIndex( ArrayIndex index ) const
{
   return index < size();
}



const Value &
Value::operator[]( const char *key ) const
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == objectValue );
   if ( type_ == nullValue )
      return null;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   CZString actualKey( key, CZString::noDuplication );
   ObjectValues::const_iterator it = value_.map_->find( actualKey );
   if ( it == value_.map_->end() )
      return null;
   return (*it).second;
#else
   const Value *value = value_.map_->find( key );
   return value ? *value : null;
#endif
}


Value &
Value::operator[]( const std::string &key )
{
   return (*this)[ key.c_str() ];
}


const Value &
Value::operator[]( const std::string &key ) const
{
   return (*this)[ key.c_str() ];
}

Value &
Value::operator[]( const StaticString &key )
{
   return resolveReference( key, true );
}


# ifdef JSON_USE_CPPTL
Value &
Value::operator[]( const CppTL::ConstString &key )
{
   return (*this)[ key.c_str() ];
}


const Value &
Value::operator[]( const CppTL::ConstString &key ) const
{
   return (*this)[ key.c_str() ];
}
# endif


Value &
Value::append( const Value &value )
{
   return (*this)[size()] = value;
}


Value 
Value::get( const char *key, 
            const Value &defaultValue ) const
{
   const Value *value = &((*this)[key]);
   return value == &null ? defaultValue : *value;
}


Value 
Value::get( const std::string &key,
            const Value &defaultValue ) const
{
   return get( key.c_str(), defaultValue );
}

Value
Value::removeMember( const char* key )
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == objectValue );
   if ( type_ == nullValue )
      return null;
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   CZString actualKey( key, CZString::noDuplication );
   ObjectValues::iterator it = value_.map_->find( actualKey );
   if ( it == value_.map_->end() )
      return null;
   Value old(it->second);
   value_.map_->erase(it);
   return old;
#else
   Value *value = value_.map_->find( key );
   if (value){
      Value old(*value);
      value_.map_.remove( key );
      return old;
   } else {
      return null;
   }
#endif
}

Value
Value::removeMember( const std::string &key )
{
   return removeMember( key.c_str() );
}

# ifdef JSON_USE_CPPTL
Value 
Value::get( const CppTL::ConstString &key,
            const Value &defaultValue ) const
{
   return get( key.c_str(), defaultValue );
}
# endif

bool 
Value::isMember( const char *key ) const
{
   const Value *value = &((*this)[key]);
   return value != &null;
}


bool 
Value::isMember( const std::string &key ) const
{
   return isMember( key.c_str() );
}


# ifdef JSON_USE_CPPTL
bool 
Value::isMember( const CppTL::ConstString &key ) const
{
   return isMember( key.c_str() );
}
#endif

Value::Members 
Value::getMemberNames() const
{
   JSON_ASSERT( type_ == nullValue  ||  type_ == objectValue );
   if ( type_ == nullValue )
       return Value::Members();
   Members members;
   members.reserve( value_.map_->size() );
#ifndef JSON_VALUE_USE_INTERNAL_MAP
   ObjectValues::const_iterator it = value_.map_->begin();
   ObjectValues::const_iterator itEnd = value_.map_->end();
   for ( ; it != itEnd; ++it )
      members.push_back( std::string( (*it).first.c_str() ) );
#else
   ValueInternalMap::IteratorState it;
   ValueInternalMap::IteratorState itEnd;
   value_.map_->makeBeginIterator( it );
   value_.map_->makeEndIterator( itEnd );
   for ( ; !ValueInternalMap::equals( it, itEnd ); ValueInternalMap::increment(it) )
      members.push_back( std::string( ValueInternalMap::key( it ) ) );
#endif
   return members;
}
//
//# ifdef JSON_USE_CPPTL
//EnumMemberNames
//Value::enumMemberNames() const
//{
//   if ( type_ == objectValue )
//   {
//      return CppTL::Enum::any(  CppTL::Enum::transform(
//         CppTL::Enum::keys( *(value_.map_), CppTL::Type<const CZString &>() ),
//         MemberNamesTransform() ) );
//   }
//   return EnumMemberNames();
//}
//
//
//EnumValues 
//Value::enumValues() const
//{
//   if ( type_ == objectValue  ||  type_ == arrayValue )
//      return CppTL::Enum::anyValues( *(value_.map_), 
//                                     CppTL::Type<const Value &>() );
//   return EnumValues();
//}
//
//# endif


bool
Value::isNull() const
{
   return type_ == nullValue;
}


bool 
Value::isBool() const
{
   return type_ == booleanValue;
}


bool 
Value::isInt() const
{
   return type_ == intValue;
}


bool 
Value::isUInt() const
{
   return type_ == uintValue;
}


bool 
Value::isIntegral() const
{
   return type_ == intValue  
          ||  type_ == uintValue  
          ||  type_ == booleanValue;
}


bool 
Value::isDouble() const
{
   return type_ == realValue;
}


bool 
Value::isNumeric() const
{
   return isIntegral() || isDouble();
}


bool 
Value::isString() const
{
   return type_ == stringValue;
}


bool 
Value::isArray() const
{
   return type_ == nullValue  ||  type_ == arrayValue;
}


bool 
Value::isObject() const
{
   return type_ == nullValue  ||  type_ == objectValue;
}


void 
Value::setComment( const char *comment,
                   CommentPlacement placement )
{
   if ( !comments_ )
      comments_ = new CommentInfo[numberOfCommentPlacement];
   comments_[placement].setComment( comment );
}


void 
Value::setComment( const std::string &comment,
                   CommentPlacement placement )
{
   setComment( comment.c_str(), placement );
}


bool 
Value::hasComment( CommentPlacement placement ) const
{
   return comments_ != 0  &&  comments_[placement].comment_ != 0;
}

std::string 
Value::getComment( CommentPlacement placement ) const
{
   if ( hasComment(placement) )
      return comments_[placement].comment_;
   return "";
}


std::string 
Value::toStyledString() const
{
   StyledWriter writer;
   return writer.write( *this );
}


Value::const_iterator 
Value::begin() const
{
   switch ( type_ )
   {
#ifdef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
      if ( value_.array_ )
      {
         ValueInternalArray::IteratorState it;
         value_.array_->makeBeginIterator( it );
         return const_iterator( it );
      }
      break;
   case objectValue:
      if ( value_.map_ )
      {
         ValueInternalMap::IteratorState it;
         value_.map_->makeBeginIterator( it );
         return const_iterator( it );
      }
      break;
#else
   case arrayValue:
   case objectValue:
      if ( value_.map_ )
         return const_iterator( value_.map_->begin() );
      break;
#endif
   default:
      break;
   }
   return const_iterator();
}

Value::const_iterator 
Value::end() const
{
   switch ( type_ )
   {
#ifdef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
      if ( value_.array_ )
      {
         ValueInternalArray::IteratorState it;
         value_.array_->makeEndIterator( it );
         return const_iterator( it );
      }
      break;
   case objectValue:
      if ( value_.map_ )
      {
         ValueInternalMap::IteratorState it;
         value_.map_->makeEndIterator( it );
         return const_iterator( it );
      }
      break;
#else
   case arrayValue:
   case objectValue:
      if ( value_.map_ )
         return const_iterator( value_.map_->end() );
      break;
#endif
   default:
      break;
   }
   return const_iterator();
}


Value::iterator 
Value::begin()
{
   switch ( type_ )
   {
#ifdef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
      if ( value_.array_ )
      {
         ValueInternalArray::IteratorState it;
         value_.array_->makeBeginIterator( it );
         return iterator( it );
      }
      break;
   case objectValue:
      if ( value_.map_ )
      {
         ValueInternalMap::IteratorState it;
         value_.map_->makeBeginIterator( it );
         return iterator( it );
      }
      break;
#else
   case arrayValue:
   case objectValue:
      if ( value_.map_ )
         return iterator( value_.map_->begin() );
      break;
#endif
   default:
      break;
   }
   return iterator();
}

Value::iterator 
Value::end()
{
   switch ( type_ )
   {
#ifdef JSON_VALUE_USE_INTERNAL_MAP
   case arrayValue:
      if ( value_.array_ )
      {
         ValueInternalArray::IteratorState it;
         value_.array_->makeEndIterator( it );
         return iterator( it );
      }
      break;
   case objectValue:
      if ( value_.map_ )
      {
         ValueInternalMap::IteratorState it;
         value_.map_->makeEndIterator( it );
         return iterator( it );
      }
      break;
#else
   case arrayValue:
   case objectValue:
      if ( value_.map_ )
         return iterator( value_.map_->end() );
      break;
#endif
   default:
      break;
   }
   return iterator();
}


// class PathArgument
// //////////////////////////////////////////////////////////////////

PathArgument::PathArgument()
   : kind_( kindNone )
{
}


PathArgument::PathArgument( ArrayIndex index )
   : index_( index )
   , kind_( kindIndex )
{
}


PathArgument::PathArgument( const char *key )
   : key_( key )
   , kind_( kindKey )
{
}


PathArgument::PathArgument( const std::string &key )
   : key_( key.c_str() )
   , kind_( kindKey )
{
}

// class Path
// //////////////////////////////////////////////////////////////////

Path::Path( const std::string &path,
            const PathArgument &a1,
            const PathArgument &a2,
            const PathArgument &a3,
            const PathArgument &a4,
            const PathArgument &a5 )
{
   InArgs in;
   in.push_back( &a1 );
   in.push_back( &a2 );
   in.push_back( &a3 );
   in.push_back( &a4 );
   in.push_back( &a5 );
   makePath( path, in );
}


void 
Path::makePath( const std::string &path,
                const InArgs &in )
{
   const char *current = path.c_str();
   const char *end = current + path.length();
   InArgs::const_iterator itInArg = in.begin();
   while ( current != end )
   {
      if ( *current == '[' )
      {
         ++current;
         if ( *current == '%' )
            addPathInArg( path, in, itInArg, PathArgument::kindIndex );
         else
         {
            ArrayIndex index = 0;
            for ( ; current != end && *current >= '0'  &&  *current <= '9'; ++current )
               index = index * 10 + ArrayIndex(*current - '0');
            args_.push_back( index );
         }
         if ( current == end  ||  *current++ != ']' )
            invalidPath( path, int(current - path.c_str()) );
      }
      else if ( *current == '%' )
      {
         addPathInArg( path, in, itInArg, PathArgument::kindKey );
         ++current;
      }
      else if ( *current == '.' )
      {
         ++current;
      }
      else
      {
         const char *beginName = current;
         while ( current != end  &&  !strchr( "[.", *current ) )
            ++current;
         args_.push_back( std::string( beginName, current ) );
      }
   }
}


void 
Path::addPathInArg( const std::string &/*path*/,
                    const InArgs &in, 
                    InArgs::const_iterator &itInArg, 
                    PathArgument::Kind kind )
{
   if ( itInArg == in.end() )
   {
      // Error: missing argument %d
   }
   else if ( (*itInArg)->kind_ != kind )
   {
      // Error: bad argument type
   }
   else
   {
      args_.push_back( **itInArg );
   }
}


void 
Path::invalidPath( const std::string &/*path*/,
                   int /*location*/ )
{
   // Error: invalid path.
}


const Value &
Path::resolve( const Value &root ) const
{
   const Value *node = &root;
   for ( Args::const_iterator it = args_.begin(); it != args_.end(); ++it )
   {
      const PathArgument &arg = *it;
      if ( arg.kind_ == PathArgument::kindIndex )
      {
         if ( !node->isArray()  ||  node->isValidIndex( arg.index_ ) )
         {
            // Error: unable to resolve path (array value expected at position...
         }
         node = &((*node)[arg.index_]);
      }
      else if ( arg.kind_ == PathArgument::kindKey )
      {
         if ( !node->isObject() )
         {
            // Error: unable to resolve path (object value expected at position...)
         }
         node = &((*node)[arg.key_]);
         if ( node == &Value::null )
         {
            // Error: unable to resolve path (object has no member named '' at position...)
         }
      }
   }
   return *node;
}


Value 
Path::resolve( const Value &root, 
               const Value &defaultValue ) const
{
   const Value *node = &root;
   for ( Args::const_iterator it = args_.begin(); it != args_.end(); ++it )
   {
      const PathArgument &arg = *it;
      if ( arg.kind_ == PathArgument::kindIndex )
      {
         if ( !node->isArray()  ||  node->isValidIndex( arg.index_ ) )
            return defaultValue;
         node = &((*node)[arg.index_]);
      }
      else if ( arg.kind_ == PathArgument::kindKey )
      {
         if ( !node->isObject() )
            return defaultValue;
         node = &((*node)[arg.key_]);
         if ( node == &Value::null )
            return defaultValue;
      }
   }
   return *node;
}


Value &
Path::make( Value &root ) const
{
   Value *node = &root;
   for ( Args::const_iterator it = args_.begin(); it != args_.end(); ++it )
   {
      const PathArgument &arg = *it;
      if ( arg.kind_ == PathArgument::kindIndex )
      {
         if ( !node->isArray() )
         {
            // Error: node is not an array at position ...
         }
         node = &((*node)[arg.index_]);
      }
      else if ( arg.kind_ == PathArgument::kindKey )
      {
         if ( !node->isObject() )
         {
            // Error: node is not an object at position...
         }
         node = &((*node)[arg.key_]);
      }
   }
   return *node;
}


} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_value.cpp
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_writer.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
# include <json/writer.h>
# include "json_tool.h"
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <utility>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#if _MSC_VER >= 1400 // VC++ 8.0
#pragma warning( disable : 4996 )   // disable warning about strdup being deprecated.
#endif

namespace Json {

static bool containsControlCharacter( const char* str )
{
   while ( *str ) 
   {
      if ( isControlCharacter( *(str++) ) )
         return true;
   }
   return false;
}


std::string valueToString( LargestInt value )
{
   UIntToStringBuffer buffer;
   char *current = buffer + sizeof(buffer);
   bool isNegative = value < 0;
   if ( isNegative )
      value = -value;
   uintToString( LargestUInt(value), current );
   if ( isNegative )
      *--current = '-';
   assert( current >= buffer );
   return current;
}


std::string valueToString( LargestUInt value )
{
   UIntToStringBuffer buffer;
   char *current = buffer + sizeof(buffer);
   uintToString( value, current );
   assert( current >= buffer );
   return current;
}

#if defined(JSON_HAS_INT64)

std::string valueToString( Int value )
{
   return valueToString( LargestInt(value) );
}


std::string valueToString( UInt value )
{
   return valueToString( LargestUInt(value) );
}

#endif // # if defined(JSON_HAS_INT64)


std::string valueToString( double value )
{
#define CHECKCOMPACT(theVal) if ( fabs(theVal - value) < 0.000001 ) return #theVal;

    if ( g_compactCommonFloats_rightNow ) {

    //CHECKCOMPACT(0);

    CHECKCOMPACT(0.01);
    CHECKCOMPACT(0.02);
    CHECKCOMPACT(0.03);
    CHECKCOMPACT(0.04);
    CHECKCOMPACT(0.05);
    CHECKCOMPACT(0.06);
    CHECKCOMPACT(0.07);
    CHECKCOMPACT(0.08);
    CHECKCOMPACT(0.09);
    CHECKCOMPACT(0.1);
    CHECKCOMPACT(0.11);
    CHECKCOMPACT(0.12);
    CHECKCOMPACT(0.13);
    CHECKCOMPACT(0.14);
    CHECKCOMPACT(0.15);
    CHECKCOMPACT(0.16);
    CHECKCOMPACT(0.17);
    CHECKCOMPACT(0.18);
    CHECKCOMPACT(0.19);
    CHECKCOMPACT(0.2);
    CHECKCOMPACT(0.21);
    CHECKCOMPACT(0.22);
    CHECKCOMPACT(0.23);
    CHECKCOMPACT(0.24);
    CHECKCOMPACT(0.25);
    CHECKCOMPACT(0.26);
    CHECKCOMPACT(0.27);
    CHECKCOMPACT(0.28);
    CHECKCOMPACT(0.29);
    CHECKCOMPACT(0.3);
    CHECKCOMPACT(0.31);
    CHECKCOMPACT(0.32);
    CHECKCOMPACT(0.33);
    CHECKCOMPACT(0.34);
    CHECKCOMPACT(0.35);
    CHECKCOMPACT(0.36);
    CHECKCOMPACT(0.37);
    CHECKCOMPACT(0.38);
    CHECKCOMPACT(0.39);
    CHECKCOMPACT(0.4);
    CHECKCOMPACT(0.41);
    CHECKCOMPACT(0.42);
    CHECKCOMPACT(0.43);
    CHECKCOMPACT(0.44);
    CHECKCOMPACT(0.45);
    CHECKCOMPACT(0.46);
    CHECKCOMPACT(0.47);
    CHECKCOMPACT(0.48);
    CHECKCOMPACT(0.49);
    CHECKCOMPACT(0.5);
    CHECKCOMPACT(0.51);
    CHECKCOMPACT(0.52);
    CHECKCOMPACT(0.53);
    CHECKCOMPACT(0.54);
    CHECKCOMPACT(0.55);
    CHECKCOMPACT(0.56);
    CHECKCOMPACT(0.57);
    CHECKCOMPACT(0.58);
    CHECKCOMPACT(0.59);
    CHECKCOMPACT(0.6);
    CHECKCOMPACT(0.61);
    CHECKCOMPACT(0.62);
    CHECKCOMPACT(0.63);
    CHECKCOMPACT(0.64);
    CHECKCOMPACT(0.65);
    CHECKCOMPACT(0.66);
    CHECKCOMPACT(0.67);
    CHECKCOMPACT(0.68);
    CHECKCOMPACT(0.69);
    CHECKCOMPACT(0.7);
    CHECKCOMPACT(0.71);
    CHECKCOMPACT(0.72);
    CHECKCOMPACT(0.73);
    CHECKCOMPACT(0.74);
    CHECKCOMPACT(0.75);
    CHECKCOMPACT(0.76);
    CHECKCOMPACT(0.77);
    CHECKCOMPACT(0.78);
    CHECKCOMPACT(0.79);
    CHECKCOMPACT(0.8);
    CHECKCOMPACT(0.81);
    CHECKCOMPACT(0.82);
    CHECKCOMPACT(0.83);
    CHECKCOMPACT(0.84);
    CHECKCOMPACT(0.85);
    CHECKCOMPACT(0.86);
    CHECKCOMPACT(0.87);
    CHECKCOMPACT(0.88);
    CHECKCOMPACT(0.89);
    CHECKCOMPACT(0.9);
    CHECKCOMPACT(0.91);
    CHECKCOMPACT(0.92);
    CHECKCOMPACT(0.93);
    CHECKCOMPACT(0.94);
    CHECKCOMPACT(0.95);
    CHECKCOMPACT(0.96);
    CHECKCOMPACT(0.97);
    CHECKCOMPACT(0.98);
    CHECKCOMPACT(0.99);
    CHECKCOMPACT(1);
    CHECKCOMPACT(1.01);
    CHECKCOMPACT(1.02);
    CHECKCOMPACT(1.03);
    CHECKCOMPACT(1.04);
    CHECKCOMPACT(1.05);
    CHECKCOMPACT(1.06);
    CHECKCOMPACT(1.07);
    CHECKCOMPACT(1.08);
    CHECKCOMPACT(1.09);
    CHECKCOMPACT(1.1);
    CHECKCOMPACT(1.11);
    CHECKCOMPACT(1.12);
    CHECKCOMPACT(1.13);
    CHECKCOMPACT(1.14);
    CHECKCOMPACT(1.15);
    CHECKCOMPACT(1.16);
    CHECKCOMPACT(1.17);
    CHECKCOMPACT(1.18);
    CHECKCOMPACT(1.19);
    CHECKCOMPACT(1.2);
    CHECKCOMPACT(1.21);
    CHECKCOMPACT(1.22);
    CHECKCOMPACT(1.23);
    CHECKCOMPACT(1.24);
    CHECKCOMPACT(1.25);
    CHECKCOMPACT(1.26);
    CHECKCOMPACT(1.27);
    CHECKCOMPACT(1.28);
    CHECKCOMPACT(1.29);
    CHECKCOMPACT(1.3);
    CHECKCOMPACT(1.31);
    CHECKCOMPACT(1.32);
    CHECKCOMPACT(1.33);
    CHECKCOMPACT(1.34);
    CHECKCOMPACT(1.35);
    CHECKCOMPACT(1.36);
    CHECKCOMPACT(1.37);
    CHECKCOMPACT(1.38);
    CHECKCOMPACT(1.39);
    CHECKCOMPACT(1.4);
    CHECKCOMPACT(1.41);
    CHECKCOMPACT(1.42);
    CHECKCOMPACT(1.43);
    CHECKCOMPACT(1.44);
    CHECKCOMPACT(1.45);
    CHECKCOMPACT(1.46);
    CHECKCOMPACT(1.47);
    CHECKCOMPACT(1.48);
    CHECKCOMPACT(1.49);
    CHECKCOMPACT(1.5);
    CHECKCOMPACT(1.51);
    CHECKCOMPACT(1.52);
    CHECKCOMPACT(1.53);
    CHECKCOMPACT(1.54);
    CHECKCOMPACT(1.55);
    CHECKCOMPACT(1.56);
    CHECKCOMPACT(1.57);
    CHECKCOMPACT(1.58);
    CHECKCOMPACT(1.59);
    CHECKCOMPACT(1.6);
    CHECKCOMPACT(1.61);
    CHECKCOMPACT(1.62);
    CHECKCOMPACT(1.63);
    CHECKCOMPACT(1.64);
    CHECKCOMPACT(1.65);
    CHECKCOMPACT(1.66);
    CHECKCOMPACT(1.67);
    CHECKCOMPACT(1.68);
    CHECKCOMPACT(1.69);
    CHECKCOMPACT(1.7);
    CHECKCOMPACT(1.71);
    CHECKCOMPACT(1.72);
    CHECKCOMPACT(1.73);
    CHECKCOMPACT(1.74);
    CHECKCOMPACT(1.75);
    CHECKCOMPACT(1.76);
    CHECKCOMPACT(1.77);
    CHECKCOMPACT(1.78);
    CHECKCOMPACT(1.79);
    CHECKCOMPACT(1.8);
    CHECKCOMPACT(1.81);
    CHECKCOMPACT(1.82);
    CHECKCOMPACT(1.83);
    CHECKCOMPACT(1.84);
    CHECKCOMPACT(1.85);
    CHECKCOMPACT(1.86);
    CHECKCOMPACT(1.87);
    CHECKCOMPACT(1.88);
    CHECKCOMPACT(1.89);
    CHECKCOMPACT(1.9);
    CHECKCOMPACT(1.91);
    CHECKCOMPACT(1.92);
    CHECKCOMPACT(1.93);
    CHECKCOMPACT(1.94);
    CHECKCOMPACT(1.95);
    CHECKCOMPACT(1.96);
    CHECKCOMPACT(1.97);
    CHECKCOMPACT(1.98);
    CHECKCOMPACT(1.99);
    CHECKCOMPACT(2);
    CHECKCOMPACT(2.01);
    CHECKCOMPACT(2.02);
    CHECKCOMPACT(2.03);
    CHECKCOMPACT(2.04);
    CHECKCOMPACT(2.05);
    CHECKCOMPACT(2.06);
    CHECKCOMPACT(2.07);
    CHECKCOMPACT(2.08);
    CHECKCOMPACT(2.09);
    CHECKCOMPACT(2.1);
    CHECKCOMPACT(2.11);
    CHECKCOMPACT(2.12);
    CHECKCOMPACT(2.13);
    CHECKCOMPACT(2.14);
    CHECKCOMPACT(2.15);
    CHECKCOMPACT(2.16);
    CHECKCOMPACT(2.17);
    CHECKCOMPACT(2.18);
    CHECKCOMPACT(2.19);
    CHECKCOMPACT(2.2);
    CHECKCOMPACT(2.21);
    CHECKCOMPACT(2.22);
    CHECKCOMPACT(2.23);
    CHECKCOMPACT(2.24);
    CHECKCOMPACT(2.25);
    CHECKCOMPACT(2.26);
    CHECKCOMPACT(2.27);
    CHECKCOMPACT(2.28);
    CHECKCOMPACT(2.29);
    CHECKCOMPACT(2.3);
    CHECKCOMPACT(2.31);
    CHECKCOMPACT(2.32);
    CHECKCOMPACT(2.33);
    CHECKCOMPACT(2.34);
    CHECKCOMPACT(2.35);
    CHECKCOMPACT(2.36);
    CHECKCOMPACT(2.37);
    CHECKCOMPACT(2.38);
    CHECKCOMPACT(2.39);
    CHECKCOMPACT(2.4);
    CHECKCOMPACT(2.41);
    CHECKCOMPACT(2.42);
    CHECKCOMPACT(2.43);
    CHECKCOMPACT(2.44);
    CHECKCOMPACT(2.45);
    CHECKCOMPACT(2.46);
    CHECKCOMPACT(2.47);
    CHECKCOMPACT(2.48);
    CHECKCOMPACT(2.49);
    CHECKCOMPACT(2.5);
    CHECKCOMPACT(2.51);
    CHECKCOMPACT(2.52);
    CHECKCOMPACT(2.53);
    CHECKCOMPACT(2.54);
    CHECKCOMPACT(2.55);
    CHECKCOMPACT(2.56);
    CHECKCOMPACT(2.57);
    CHECKCOMPACT(2.58);
    CHECKCOMPACT(2.59);
    CHECKCOMPACT(2.6);
    CHECKCOMPACT(2.61);
    CHECKCOMPACT(2.62);
    CHECKCOMPACT(2.63);
    CHECKCOMPACT(2.64);
    CHECKCOMPACT(2.65);
    CHECKCOMPACT(2.66);
    CHECKCOMPACT(2.67);
    CHECKCOMPACT(2.68);
    CHECKCOMPACT(2.69);
    CHECKCOMPACT(2.7);
    CHECKCOMPACT(2.71);
    CHECKCOMPACT(2.72);
    CHECKCOMPACT(2.73);
    CHECKCOMPACT(2.74);
    CHECKCOMPACT(2.75);
    CHECKCOMPACT(2.76);
    CHECKCOMPACT(2.77);
    CHECKCOMPACT(2.78);
    CHECKCOMPACT(2.79);
    CHECKCOMPACT(2.8);
    CHECKCOMPACT(2.81);
    CHECKCOMPACT(2.82);
    CHECKCOMPACT(2.83);
    CHECKCOMPACT(2.84);
    CHECKCOMPACT(2.85);
    CHECKCOMPACT(2.86);
    CHECKCOMPACT(2.87);
    CHECKCOMPACT(2.88);
    CHECKCOMPACT(2.89);
    CHECKCOMPACT(2.9);
    CHECKCOMPACT(2.91);
    CHECKCOMPACT(2.92);
    CHECKCOMPACT(2.93);
    CHECKCOMPACT(2.94);
    CHECKCOMPACT(2.95);
    CHECKCOMPACT(2.96);
    CHECKCOMPACT(2.97);
    CHECKCOMPACT(2.98);
    CHECKCOMPACT(2.99);
    CHECKCOMPACT(3);
    CHECKCOMPACT(3.01);
    CHECKCOMPACT(3.02);
    CHECKCOMPACT(3.03);
    CHECKCOMPACT(3.04);
    CHECKCOMPACT(3.05);
    CHECKCOMPACT(3.06);
    CHECKCOMPACT(3.07);
    CHECKCOMPACT(3.08);
    CHECKCOMPACT(3.09);
    CHECKCOMPACT(3.1);
    CHECKCOMPACT(3.11);
    CHECKCOMPACT(3.12);
    CHECKCOMPACT(3.13);
    CHECKCOMPACT(3.14);
    CHECKCOMPACT(3.15);
    CHECKCOMPACT(3.16);
    CHECKCOMPACT(3.17);
    CHECKCOMPACT(3.18);
    CHECKCOMPACT(3.19);
    CHECKCOMPACT(3.2);
    CHECKCOMPACT(3.21);
    CHECKCOMPACT(3.22);
    CHECKCOMPACT(3.23);
    CHECKCOMPACT(3.24);
    CHECKCOMPACT(3.25);
    CHECKCOMPACT(3.26);
    CHECKCOMPACT(3.27);
    CHECKCOMPACT(3.28);
    CHECKCOMPACT(3.29);
    CHECKCOMPACT(3.3);
    CHECKCOMPACT(3.31);
    CHECKCOMPACT(3.32);
    CHECKCOMPACT(3.33);
    CHECKCOMPACT(3.34);
    CHECKCOMPACT(3.35);
    CHECKCOMPACT(3.36);
    CHECKCOMPACT(3.37);
    CHECKCOMPACT(3.38);
    CHECKCOMPACT(3.39);
    CHECKCOMPACT(3.4);
    CHECKCOMPACT(3.41);
    CHECKCOMPACT(3.42);
    CHECKCOMPACT(3.43);
    CHECKCOMPACT(3.44);
    CHECKCOMPACT(3.45);
    CHECKCOMPACT(3.46);
    CHECKCOMPACT(3.47);
    CHECKCOMPACT(3.48);
    CHECKCOMPACT(3.49);
    CHECKCOMPACT(3.5);
    CHECKCOMPACT(3.51);
    CHECKCOMPACT(3.52);
    CHECKCOMPACT(3.53);
    CHECKCOMPACT(3.54);
    CHECKCOMPACT(3.55);
    CHECKCOMPACT(3.56);
    CHECKCOMPACT(3.57);
    CHECKCOMPACT(3.58);
    CHECKCOMPACT(3.59);
    CHECKCOMPACT(3.6);
    CHECKCOMPACT(3.61);
    CHECKCOMPACT(3.62);
    CHECKCOMPACT(3.63);
    CHECKCOMPACT(3.64);
    CHECKCOMPACT(3.65);
    CHECKCOMPACT(3.66);
    CHECKCOMPACT(3.67);
    CHECKCOMPACT(3.68);
    CHECKCOMPACT(3.69);
    CHECKCOMPACT(3.7);
    CHECKCOMPACT(3.71);
    CHECKCOMPACT(3.72);
    CHECKCOMPACT(3.73);
    CHECKCOMPACT(3.74);
    CHECKCOMPACT(3.75);
    CHECKCOMPACT(3.76);
    CHECKCOMPACT(3.77);
    CHECKCOMPACT(3.78);
    CHECKCOMPACT(3.79);
    CHECKCOMPACT(3.8);
    CHECKCOMPACT(3.81);
    CHECKCOMPACT(3.82);
    CHECKCOMPACT(3.83);
    CHECKCOMPACT(3.84);
    CHECKCOMPACT(3.85);
    CHECKCOMPACT(3.86);
    CHECKCOMPACT(3.87);
    CHECKCOMPACT(3.88);
    CHECKCOMPACT(3.89);
    CHECKCOMPACT(3.9);
    CHECKCOMPACT(3.91);
    CHECKCOMPACT(3.92);
    CHECKCOMPACT(3.93);
    CHECKCOMPACT(3.94);
    CHECKCOMPACT(3.95);
    CHECKCOMPACT(3.96);
    CHECKCOMPACT(3.97);
    CHECKCOMPACT(3.98);
    CHECKCOMPACT(3.99);
    CHECKCOMPACT(4);
    CHECKCOMPACT(4.01);
    CHECKCOMPACT(4.02);
    CHECKCOMPACT(4.03);
    CHECKCOMPACT(4.04);
    CHECKCOMPACT(4.05);
    CHECKCOMPACT(4.06);
    CHECKCOMPACT(4.07);
    CHECKCOMPACT(4.08);
    CHECKCOMPACT(4.09);
    CHECKCOMPACT(4.1);
    CHECKCOMPACT(4.11);
    CHECKCOMPACT(4.12);
    CHECKCOMPACT(4.13);
    CHECKCOMPACT(4.14);
    CHECKCOMPACT(4.15);
    CHECKCOMPACT(4.16);
    CHECKCOMPACT(4.17);
    CHECKCOMPACT(4.18);
    CHECKCOMPACT(4.19);
    CHECKCOMPACT(4.2);
    CHECKCOMPACT(4.21);
    CHECKCOMPACT(4.22);
    CHECKCOMPACT(4.23);
    CHECKCOMPACT(4.24);
    CHECKCOMPACT(4.25);
    CHECKCOMPACT(4.26);
    CHECKCOMPACT(4.27);
    CHECKCOMPACT(4.28);
    CHECKCOMPACT(4.29);
    CHECKCOMPACT(4.3);
    CHECKCOMPACT(4.31);
    CHECKCOMPACT(4.32);
    CHECKCOMPACT(4.33);
    CHECKCOMPACT(4.34);
    CHECKCOMPACT(4.35);
    CHECKCOMPACT(4.36);
    CHECKCOMPACT(4.37);
    CHECKCOMPACT(4.38);
    CHECKCOMPACT(4.39);
    CHECKCOMPACT(4.4);
    CHECKCOMPACT(4.41);
    CHECKCOMPACT(4.42);
    CHECKCOMPACT(4.43);
    CHECKCOMPACT(4.44);
    CHECKCOMPACT(4.45);
    CHECKCOMPACT(4.46);
    CHECKCOMPACT(4.47);
    CHECKCOMPACT(4.48);
    CHECKCOMPACT(4.49);
    CHECKCOMPACT(4.5);
    CHECKCOMPACT(4.51);
    CHECKCOMPACT(4.52);
    CHECKCOMPACT(4.53);
    CHECKCOMPACT(4.54);
    CHECKCOMPACT(4.55);
    CHECKCOMPACT(4.56);
    CHECKCOMPACT(4.57);
    CHECKCOMPACT(4.58);
    CHECKCOMPACT(4.59);
    CHECKCOMPACT(4.6);
    CHECKCOMPACT(4.61);
    CHECKCOMPACT(4.62);
    CHECKCOMPACT(4.63);
    CHECKCOMPACT(4.64);
    CHECKCOMPACT(4.65);
    CHECKCOMPACT(4.66);
    CHECKCOMPACT(4.67);
    CHECKCOMPACT(4.68);
    CHECKCOMPACT(4.69);
    CHECKCOMPACT(4.7);
    CHECKCOMPACT(4.71);
    CHECKCOMPACT(4.72);
    CHECKCOMPACT(4.73);
    CHECKCOMPACT(4.74);
    CHECKCOMPACT(4.75);
    CHECKCOMPACT(4.76);
    CHECKCOMPACT(4.77);
    CHECKCOMPACT(4.78);
    CHECKCOMPACT(4.79);
    CHECKCOMPACT(4.8);
    CHECKCOMPACT(4.81);
    CHECKCOMPACT(4.82);
    CHECKCOMPACT(4.83);
    CHECKCOMPACT(4.84);
    CHECKCOMPACT(4.85);
    CHECKCOMPACT(4.86);
    CHECKCOMPACT(4.87);
    CHECKCOMPACT(4.88);
    CHECKCOMPACT(4.89);
    CHECKCOMPACT(4.9);
    CHECKCOMPACT(4.91);
    CHECKCOMPACT(4.92);
    CHECKCOMPACT(4.93);
    CHECKCOMPACT(4.94);
    CHECKCOMPACT(4.95);
    CHECKCOMPACT(4.96);
    CHECKCOMPACT(4.97);
    CHECKCOMPACT(4.98);
    CHECKCOMPACT(4.99);
    CHECKCOMPACT(5);
    CHECKCOMPACT(5.01);
    CHECKCOMPACT(5.02);
    CHECKCOMPACT(5.03);
    CHECKCOMPACT(5.04);
    CHECKCOMPACT(5.05);
    CHECKCOMPACT(5.06);
    CHECKCOMPACT(5.07);
    CHECKCOMPACT(5.08);
    CHECKCOMPACT(5.09);
    CHECKCOMPACT(5.1);
    CHECKCOMPACT(5.11);
    CHECKCOMPACT(5.12);
    CHECKCOMPACT(5.13);
    CHECKCOMPACT(5.14);
    CHECKCOMPACT(5.15);
    CHECKCOMPACT(5.16);
    CHECKCOMPACT(5.17);
    CHECKCOMPACT(5.18);
    CHECKCOMPACT(5.19);
    CHECKCOMPACT(5.2);
    CHECKCOMPACT(5.21);
    CHECKCOMPACT(5.22);
    CHECKCOMPACT(5.23);
    CHECKCOMPACT(5.24);
    CHECKCOMPACT(5.25);
    CHECKCOMPACT(5.26);
    CHECKCOMPACT(5.27);
    CHECKCOMPACT(5.28);
    CHECKCOMPACT(5.29);
    CHECKCOMPACT(5.3);
    CHECKCOMPACT(5.31);
    CHECKCOMPACT(5.32);
    CHECKCOMPACT(5.33);
    CHECKCOMPACT(5.34);
    CHECKCOMPACT(5.35);
    CHECKCOMPACT(5.36);
    CHECKCOMPACT(5.37);
    CHECKCOMPACT(5.38);
    CHECKCOMPACT(5.39);
    CHECKCOMPACT(5.4);
    CHECKCOMPACT(5.41);
    CHECKCOMPACT(5.42);
    CHECKCOMPACT(5.43);
    CHECKCOMPACT(5.44);
    CHECKCOMPACT(5.45);
    CHECKCOMPACT(5.46);
    CHECKCOMPACT(5.47);
    CHECKCOMPACT(5.48);
    CHECKCOMPACT(5.49);
    CHECKCOMPACT(5.5);
    CHECKCOMPACT(5.51);
    CHECKCOMPACT(5.52);
    CHECKCOMPACT(5.53);
    CHECKCOMPACT(5.54);
    CHECKCOMPACT(5.55);
    CHECKCOMPACT(5.56);
    CHECKCOMPACT(5.57);
    CHECKCOMPACT(5.58);
    CHECKCOMPACT(5.59);
    CHECKCOMPACT(5.6);
    CHECKCOMPACT(5.61);
    CHECKCOMPACT(5.62);
    CHECKCOMPACT(5.63);
    CHECKCOMPACT(5.64);
    CHECKCOMPACT(5.65);
    CHECKCOMPACT(5.66);
    CHECKCOMPACT(5.67);
    CHECKCOMPACT(5.68);
    CHECKCOMPACT(5.69);
    CHECKCOMPACT(5.7);
    CHECKCOMPACT(5.71);
    CHECKCOMPACT(5.72);
    CHECKCOMPACT(5.73);
    CHECKCOMPACT(5.74);
    CHECKCOMPACT(5.75);
    CHECKCOMPACT(5.76);
    CHECKCOMPACT(5.77);
    CHECKCOMPACT(5.78);
    CHECKCOMPACT(5.79);
    CHECKCOMPACT(5.8);
    CHECKCOMPACT(5.81);
    CHECKCOMPACT(5.82);
    CHECKCOMPACT(5.83);
    CHECKCOMPACT(5.84);
    CHECKCOMPACT(5.85);
    CHECKCOMPACT(5.86);
    CHECKCOMPACT(5.87);
    CHECKCOMPACT(5.88);
    CHECKCOMPACT(5.89);
    CHECKCOMPACT(5.9);
    CHECKCOMPACT(5.91);
    CHECKCOMPACT(5.92);
    CHECKCOMPACT(5.93);
    CHECKCOMPACT(5.94);
    CHECKCOMPACT(5.95);
    CHECKCOMPACT(5.96);
    CHECKCOMPACT(5.97);
    CHECKCOMPACT(5.98);
    CHECKCOMPACT(5.99);
    CHECKCOMPACT(6);
    CHECKCOMPACT(6.01);
    CHECKCOMPACT(6.02);
    CHECKCOMPACT(6.03);
    CHECKCOMPACT(6.04);
    CHECKCOMPACT(6.05);
    CHECKCOMPACT(6.06);
    CHECKCOMPACT(6.07);
    CHECKCOMPACT(6.08);
    CHECKCOMPACT(6.09);
    CHECKCOMPACT(6.1);
    CHECKCOMPACT(6.11);
    CHECKCOMPACT(6.12);
    CHECKCOMPACT(6.13);
    CHECKCOMPACT(6.14);
    CHECKCOMPACT(6.15);
    CHECKCOMPACT(6.16);
    CHECKCOMPACT(6.17);
    CHECKCOMPACT(6.18);
    CHECKCOMPACT(6.19);
    CHECKCOMPACT(6.2);
    CHECKCOMPACT(6.21);
    CHECKCOMPACT(6.22);
    CHECKCOMPACT(6.23);
    CHECKCOMPACT(6.24);
    CHECKCOMPACT(6.25);
    CHECKCOMPACT(6.26);
    CHECKCOMPACT(6.27);
    CHECKCOMPACT(6.28);
    CHECKCOMPACT(6.29);
    CHECKCOMPACT(6.3);
    CHECKCOMPACT(6.31);
    CHECKCOMPACT(6.32);
    CHECKCOMPACT(6.33);
    CHECKCOMPACT(6.34);
    CHECKCOMPACT(6.35);
    CHECKCOMPACT(6.36);
    CHECKCOMPACT(6.37);
    CHECKCOMPACT(6.38);
    CHECKCOMPACT(6.39);
    CHECKCOMPACT(6.4);
    CHECKCOMPACT(6.41);
    CHECKCOMPACT(6.42);
    CHECKCOMPACT(6.43);
    CHECKCOMPACT(6.44);
    CHECKCOMPACT(6.45);
    CHECKCOMPACT(6.46);
    CHECKCOMPACT(6.47);
    CHECKCOMPACT(6.48);
    CHECKCOMPACT(6.49);
    CHECKCOMPACT(6.5);
    CHECKCOMPACT(6.51);
    CHECKCOMPACT(6.52);
    CHECKCOMPACT(6.53);
    CHECKCOMPACT(6.54);
    CHECKCOMPACT(6.55);
    CHECKCOMPACT(6.56);
    CHECKCOMPACT(6.57);
    CHECKCOMPACT(6.58);
    CHECKCOMPACT(6.59);
    CHECKCOMPACT(6.6);
    CHECKCOMPACT(6.61);
    CHECKCOMPACT(6.62);
    CHECKCOMPACT(6.63);
    CHECKCOMPACT(6.64);
    CHECKCOMPACT(6.65);
    CHECKCOMPACT(6.66);
    CHECKCOMPACT(6.67);
    CHECKCOMPACT(6.68);
    CHECKCOMPACT(6.69);
    CHECKCOMPACT(6.7);
    CHECKCOMPACT(6.71);
    CHECKCOMPACT(6.72);
    CHECKCOMPACT(6.73);
    CHECKCOMPACT(6.74);
    CHECKCOMPACT(6.75);
    CHECKCOMPACT(6.76);
    CHECKCOMPACT(6.77);
    CHECKCOMPACT(6.78);
    CHECKCOMPACT(6.79);
    CHECKCOMPACT(6.8);
    CHECKCOMPACT(6.81);
    CHECKCOMPACT(6.82);
    CHECKCOMPACT(6.83);
    CHECKCOMPACT(6.84);
    CHECKCOMPACT(6.85);
    CHECKCOMPACT(6.86);
    CHECKCOMPACT(6.87);
    CHECKCOMPACT(6.88);
    CHECKCOMPACT(6.89);
    CHECKCOMPACT(6.9);
    CHECKCOMPACT(6.91);
    CHECKCOMPACT(6.92);
    CHECKCOMPACT(6.93);
    CHECKCOMPACT(6.94);
    CHECKCOMPACT(6.95);
    CHECKCOMPACT(6.96);
    CHECKCOMPACT(6.97);
    CHECKCOMPACT(6.98);
    CHECKCOMPACT(6.99);
    CHECKCOMPACT(7);
    CHECKCOMPACT(7.01);
    CHECKCOMPACT(7.02);
    CHECKCOMPACT(7.03);
    CHECKCOMPACT(7.04);
    CHECKCOMPACT(7.05);
    CHECKCOMPACT(7.06);
    CHECKCOMPACT(7.07);
    CHECKCOMPACT(7.08);
    CHECKCOMPACT(7.09);
    CHECKCOMPACT(7.1);
    CHECKCOMPACT(7.11);
    CHECKCOMPACT(7.12);
    CHECKCOMPACT(7.13);
    CHECKCOMPACT(7.14);
    CHECKCOMPACT(7.15);
    CHECKCOMPACT(7.16);
    CHECKCOMPACT(7.17);
    CHECKCOMPACT(7.18);
    CHECKCOMPACT(7.19);
    CHECKCOMPACT(7.2);
    CHECKCOMPACT(7.21);
    CHECKCOMPACT(7.22);
    CHECKCOMPACT(7.23);
    CHECKCOMPACT(7.24);
    CHECKCOMPACT(7.25);
    CHECKCOMPACT(7.26);
    CHECKCOMPACT(7.27);
    CHECKCOMPACT(7.28);
    CHECKCOMPACT(7.29);
    CHECKCOMPACT(7.3);
    CHECKCOMPACT(7.31);
    CHECKCOMPACT(7.32);
    CHECKCOMPACT(7.33);
    CHECKCOMPACT(7.34);
    CHECKCOMPACT(7.35);
    CHECKCOMPACT(7.36);
    CHECKCOMPACT(7.37);
    CHECKCOMPACT(7.38);
    CHECKCOMPACT(7.39);
    CHECKCOMPACT(7.4);
    CHECKCOMPACT(7.41);
    CHECKCOMPACT(7.42);
    CHECKCOMPACT(7.43);
    CHECKCOMPACT(7.44);
    CHECKCOMPACT(7.45);
    CHECKCOMPACT(7.46);
    CHECKCOMPACT(7.47);
    CHECKCOMPACT(7.48);
    CHECKCOMPACT(7.49);
    CHECKCOMPACT(7.5);
    CHECKCOMPACT(7.51);
    CHECKCOMPACT(7.52);
    CHECKCOMPACT(7.53);
    CHECKCOMPACT(7.54);
    CHECKCOMPACT(7.55);
    CHECKCOMPACT(7.56);
    CHECKCOMPACT(7.57);
    CHECKCOMPACT(7.58);
    CHECKCOMPACT(7.59);
    CHECKCOMPACT(7.6);
    CHECKCOMPACT(7.61);
    CHECKCOMPACT(7.62);
    CHECKCOMPACT(7.63);
    CHECKCOMPACT(7.64);
    CHECKCOMPACT(7.65);
    CHECKCOMPACT(7.66);
    CHECKCOMPACT(7.67);
    CHECKCOMPACT(7.68);
    CHECKCOMPACT(7.69);
    CHECKCOMPACT(7.7);
    CHECKCOMPACT(7.71);
    CHECKCOMPACT(7.72);
    CHECKCOMPACT(7.73);
    CHECKCOMPACT(7.74);
    CHECKCOMPACT(7.75);
    CHECKCOMPACT(7.76);
    CHECKCOMPACT(7.77);
    CHECKCOMPACT(7.78);
    CHECKCOMPACT(7.79);
    CHECKCOMPACT(7.8);
    CHECKCOMPACT(7.81);
    CHECKCOMPACT(7.82);
    CHECKCOMPACT(7.83);
    CHECKCOMPACT(7.84);
    CHECKCOMPACT(7.85);
    CHECKCOMPACT(7.86);
    CHECKCOMPACT(7.87);
    CHECKCOMPACT(7.88);
    CHECKCOMPACT(7.89);
    CHECKCOMPACT(7.9);
    CHECKCOMPACT(7.91);
    CHECKCOMPACT(7.92);
    CHECKCOMPACT(7.93);
    CHECKCOMPACT(7.94);
    CHECKCOMPACT(7.95);
    CHECKCOMPACT(7.96);
    CHECKCOMPACT(7.97);
    CHECKCOMPACT(7.98);
    CHECKCOMPACT(7.99);
    CHECKCOMPACT(8);
    CHECKCOMPACT(8.01);
    CHECKCOMPACT(8.02);
    CHECKCOMPACT(8.03);
    CHECKCOMPACT(8.04);
    CHECKCOMPACT(8.05);
    CHECKCOMPACT(8.06);
    CHECKCOMPACT(8.07);
    CHECKCOMPACT(8.08);
    CHECKCOMPACT(8.09);
    CHECKCOMPACT(8.1);
    CHECKCOMPACT(8.11);
    CHECKCOMPACT(8.12);
    CHECKCOMPACT(8.13);
    CHECKCOMPACT(8.14);
    CHECKCOMPACT(8.15);
    CHECKCOMPACT(8.16);
    CHECKCOMPACT(8.17);
    CHECKCOMPACT(8.18);
    CHECKCOMPACT(8.19);
    CHECKCOMPACT(8.2);
    CHECKCOMPACT(8.21);
    CHECKCOMPACT(8.22);
    CHECKCOMPACT(8.23);
    CHECKCOMPACT(8.24);
    CHECKCOMPACT(8.25);
    CHECKCOMPACT(8.26);
    CHECKCOMPACT(8.27);
    CHECKCOMPACT(8.28);
    CHECKCOMPACT(8.29);
    CHECKCOMPACT(8.3);
    CHECKCOMPACT(8.31);
    CHECKCOMPACT(8.32);
    CHECKCOMPACT(8.33);
    CHECKCOMPACT(8.34);
    CHECKCOMPACT(8.35);
    CHECKCOMPACT(8.36);
    CHECKCOMPACT(8.37);
    CHECKCOMPACT(8.38);
    CHECKCOMPACT(8.39);
    CHECKCOMPACT(8.4);
    CHECKCOMPACT(8.41);
    CHECKCOMPACT(8.42);
    CHECKCOMPACT(8.43);
    CHECKCOMPACT(8.44);
    CHECKCOMPACT(8.45);
    CHECKCOMPACT(8.46);
    CHECKCOMPACT(8.47);
    CHECKCOMPACT(8.48);
    CHECKCOMPACT(8.49);
    CHECKCOMPACT(8.5);
    CHECKCOMPACT(8.51);
    CHECKCOMPACT(8.52);
    CHECKCOMPACT(8.53);
    CHECKCOMPACT(8.54);
    CHECKCOMPACT(8.55);
    CHECKCOMPACT(8.56);
    CHECKCOMPACT(8.57);
    CHECKCOMPACT(8.58);
    CHECKCOMPACT(8.59);
    CHECKCOMPACT(8.6);
    CHECKCOMPACT(8.61);
    CHECKCOMPACT(8.62);
    CHECKCOMPACT(8.63);
    CHECKCOMPACT(8.64);
    CHECKCOMPACT(8.65);
    CHECKCOMPACT(8.66);
    CHECKCOMPACT(8.67);
    CHECKCOMPACT(8.68);
    CHECKCOMPACT(8.69);
    CHECKCOMPACT(8.7);
    CHECKCOMPACT(8.71);
    CHECKCOMPACT(8.72);
    CHECKCOMPACT(8.73);
    CHECKCOMPACT(8.74);
    CHECKCOMPACT(8.75);
    CHECKCOMPACT(8.76);
    CHECKCOMPACT(8.77);
    CHECKCOMPACT(8.78);
    CHECKCOMPACT(8.79);
    CHECKCOMPACT(8.8);
    CHECKCOMPACT(8.81);
    CHECKCOMPACT(8.82);
    CHECKCOMPACT(8.83);
    CHECKCOMPACT(8.84);
    CHECKCOMPACT(8.85);
    CHECKCOMPACT(8.86);
    CHECKCOMPACT(8.87);
    CHECKCOMPACT(8.88);
    CHECKCOMPACT(8.89);
    CHECKCOMPACT(8.9);
    CHECKCOMPACT(8.91);
    CHECKCOMPACT(8.92);
    CHECKCOMPACT(8.93);
    CHECKCOMPACT(8.94);
    CHECKCOMPACT(8.95);
    CHECKCOMPACT(8.96);
    CHECKCOMPACT(8.97);
    CHECKCOMPACT(8.98);
    CHECKCOMPACT(8.99);
    CHECKCOMPACT(9);
    CHECKCOMPACT(9.01);
    CHECKCOMPACT(9.02);
    CHECKCOMPACT(9.03);
    CHECKCOMPACT(9.04);
    CHECKCOMPACT(9.05);
    CHECKCOMPACT(9.06);
    CHECKCOMPACT(9.07);
    CHECKCOMPACT(9.08);
    CHECKCOMPACT(9.09);
    CHECKCOMPACT(9.1);
    CHECKCOMPACT(9.11);
    CHECKCOMPACT(9.12);
    CHECKCOMPACT(9.13);
    CHECKCOMPACT(9.14);
    CHECKCOMPACT(9.15);
    CHECKCOMPACT(9.16);
    CHECKCOMPACT(9.17);
    CHECKCOMPACT(9.18);
    CHECKCOMPACT(9.19);
    CHECKCOMPACT(9.2);
    CHECKCOMPACT(9.21);
    CHECKCOMPACT(9.22);
    CHECKCOMPACT(9.23);
    CHECKCOMPACT(9.24);
    CHECKCOMPACT(9.25);
    CHECKCOMPACT(9.26);
    CHECKCOMPACT(9.27);
    CHECKCOMPACT(9.28);
    CHECKCOMPACT(9.29);
    CHECKCOMPACT(9.3);
    CHECKCOMPACT(9.31);
    CHECKCOMPACT(9.32);
    CHECKCOMPACT(9.33);
    CHECKCOMPACT(9.34);
    CHECKCOMPACT(9.35);
    CHECKCOMPACT(9.36);
    CHECKCOMPACT(9.37);
    CHECKCOMPACT(9.38);
    CHECKCOMPACT(9.39);
    CHECKCOMPACT(9.4);
    CHECKCOMPACT(9.41);
    CHECKCOMPACT(9.42);
    CHECKCOMPACT(9.43);
    CHECKCOMPACT(9.44);
    CHECKCOMPACT(9.45);
    CHECKCOMPACT(9.46);
    CHECKCOMPACT(9.47);
    CHECKCOMPACT(9.48);
    CHECKCOMPACT(9.49);
    CHECKCOMPACT(9.5);
    CHECKCOMPACT(9.51);
    CHECKCOMPACT(9.52);
    CHECKCOMPACT(9.53);
    CHECKCOMPACT(9.54);
    CHECKCOMPACT(9.55);
    CHECKCOMPACT(9.56);
    CHECKCOMPACT(9.57);
    CHECKCOMPACT(9.58);
    CHECKCOMPACT(9.59);
    CHECKCOMPACT(9.6);
    CHECKCOMPACT(9.61);
    CHECKCOMPACT(9.62);
    CHECKCOMPACT(9.63);
    CHECKCOMPACT(9.64);
    CHECKCOMPACT(9.65);
    CHECKCOMPACT(9.66);
    CHECKCOMPACT(9.67);
    CHECKCOMPACT(9.68);
    CHECKCOMPACT(9.69);
    CHECKCOMPACT(9.7);
    CHECKCOMPACT(9.71);
    CHECKCOMPACT(9.72);
    CHECKCOMPACT(9.73);
    CHECKCOMPACT(9.74);
    CHECKCOMPACT(9.75);
    CHECKCOMPACT(9.76);
    CHECKCOMPACT(9.77);
    CHECKCOMPACT(9.78);
    CHECKCOMPACT(9.79);
    CHECKCOMPACT(9.8);
    CHECKCOMPACT(9.81);
    CHECKCOMPACT(9.82);
    CHECKCOMPACT(9.83);
    CHECKCOMPACT(9.84);
    CHECKCOMPACT(9.85);
    CHECKCOMPACT(9.86);
    CHECKCOMPACT(9.87);
    CHECKCOMPACT(9.88);
    CHECKCOMPACT(9.89);
    CHECKCOMPACT(9.9);
    CHECKCOMPACT(9.91);
    CHECKCOMPACT(9.92);
    CHECKCOMPACT(9.93);
    CHECKCOMPACT(9.94);
    CHECKCOMPACT(9.95);
    CHECKCOMPACT(9.96);
    CHECKCOMPACT(9.97);
    CHECKCOMPACT(9.98);
    CHECKCOMPACT(9.99);
    CHECKCOMPACT(10);

    //////////////////////////////////////////

    CHECKCOMPACT(-0.01);
    CHECKCOMPACT(-0.02);
    CHECKCOMPACT(-0.03);
    CHECKCOMPACT(-0.04);
    CHECKCOMPACT(-0.05);
    CHECKCOMPACT(-0.06);
    CHECKCOMPACT(-0.07);
    CHECKCOMPACT(-0.08);
    CHECKCOMPACT(-0.09);
    CHECKCOMPACT(-0.1);
    CHECKCOMPACT(-0.11);
    CHECKCOMPACT(-0.12);
    CHECKCOMPACT(-0.13);
    CHECKCOMPACT(-0.14);
    CHECKCOMPACT(-0.15);
    CHECKCOMPACT(-0.16);
    CHECKCOMPACT(-0.17);
    CHECKCOMPACT(-0.18);
    CHECKCOMPACT(-0.19);
    CHECKCOMPACT(-0.2);
    CHECKCOMPACT(-0.21);
    CHECKCOMPACT(-0.22);
    CHECKCOMPACT(-0.23);
    CHECKCOMPACT(-0.24);
    CHECKCOMPACT(-0.25);
    CHECKCOMPACT(-0.26);
    CHECKCOMPACT(-0.27);
    CHECKCOMPACT(-0.28);
    CHECKCOMPACT(-0.29);
    CHECKCOMPACT(-0.3);
    CHECKCOMPACT(-0.31);
    CHECKCOMPACT(-0.32);
    CHECKCOMPACT(-0.33);
    CHECKCOMPACT(-0.34);
    CHECKCOMPACT(-0.35);
    CHECKCOMPACT(-0.36);
    CHECKCOMPACT(-0.37);
    CHECKCOMPACT(-0.38);
    CHECKCOMPACT(-0.39);
    CHECKCOMPACT(-0.4);
    CHECKCOMPACT(-0.41);
    CHECKCOMPACT(-0.42);
    CHECKCOMPACT(-0.43);
    CHECKCOMPACT(-0.44);
    CHECKCOMPACT(-0.45);
    CHECKCOMPACT(-0.46);
    CHECKCOMPACT(-0.47);
    CHECKCOMPACT(-0.48);
    CHECKCOMPACT(-0.49);
    CHECKCOMPACT(-0.5);
    CHECKCOMPACT(-0.51);
    CHECKCOMPACT(-0.52);
    CHECKCOMPACT(-0.53);
    CHECKCOMPACT(-0.54);
    CHECKCOMPACT(-0.55);
    CHECKCOMPACT(-0.56);
    CHECKCOMPACT(-0.57);
    CHECKCOMPACT(-0.58);
    CHECKCOMPACT(-0.59);
    CHECKCOMPACT(-0.6);
    CHECKCOMPACT(-0.61);
    CHECKCOMPACT(-0.62);
    CHECKCOMPACT(-0.63);
    CHECKCOMPACT(-0.64);
    CHECKCOMPACT(-0.65);
    CHECKCOMPACT(-0.66);
    CHECKCOMPACT(-0.67);
    CHECKCOMPACT(-0.68);
    CHECKCOMPACT(-0.69);
    CHECKCOMPACT(-0.7);
    CHECKCOMPACT(-0.71);
    CHECKCOMPACT(-0.72);
    CHECKCOMPACT(-0.73);
    CHECKCOMPACT(-0.74);
    CHECKCOMPACT(-0.75);
    CHECKCOMPACT(-0.76);
    CHECKCOMPACT(-0.77);
    CHECKCOMPACT(-0.78);
    CHECKCOMPACT(-0.79);
    CHECKCOMPACT(-0.8);
    CHECKCOMPACT(-0.81);
    CHECKCOMPACT(-0.82);
    CHECKCOMPACT(-0.83);
    CHECKCOMPACT(-0.84);
    CHECKCOMPACT(-0.85);
    CHECKCOMPACT(-0.86);
    CHECKCOMPACT(-0.87);
    CHECKCOMPACT(-0.88);
    CHECKCOMPACT(-0.89);
    CHECKCOMPACT(-0.9);
    CHECKCOMPACT(-0.91);
    CHECKCOMPACT(-0.92);
    CHECKCOMPACT(-0.93);
    CHECKCOMPACT(-0.94);
    CHECKCOMPACT(-0.95);
    CHECKCOMPACT(-0.96);
    CHECKCOMPACT(-0.97);
    CHECKCOMPACT(-0.98);
    CHECKCOMPACT(-0.99);
    CHECKCOMPACT(-1);
    CHECKCOMPACT(-1.01);
    CHECKCOMPACT(-1.02);
    CHECKCOMPACT(-1.03);
    CHECKCOMPACT(-1.04);
    CHECKCOMPACT(-1.05);
    CHECKCOMPACT(-1.06);
    CHECKCOMPACT(-1.07);
    CHECKCOMPACT(-1.08);
    CHECKCOMPACT(-1.09);
    CHECKCOMPACT(-1.1);
    CHECKCOMPACT(-1.11);
    CHECKCOMPACT(-1.12);
    CHECKCOMPACT(-1.13);
    CHECKCOMPACT(-1.14);
    CHECKCOMPACT(-1.15);
    CHECKCOMPACT(-1.16);
    CHECKCOMPACT(-1.17);
    CHECKCOMPACT(-1.18);
    CHECKCOMPACT(-1.19);
    CHECKCOMPACT(-1.2);
    CHECKCOMPACT(-1.21);
    CHECKCOMPACT(-1.22);
    CHECKCOMPACT(-1.23);
    CHECKCOMPACT(-1.24);
    CHECKCOMPACT(-1.25);
    CHECKCOMPACT(-1.26);
    CHECKCOMPACT(-1.27);
    CHECKCOMPACT(-1.28);
    CHECKCOMPACT(-1.29);
    CHECKCOMPACT(-1.3);
    CHECKCOMPACT(-1.31);
    CHECKCOMPACT(-1.32);
    CHECKCOMPACT(-1.33);
    CHECKCOMPACT(-1.34);
    CHECKCOMPACT(-1.35);
    CHECKCOMPACT(-1.36);
    CHECKCOMPACT(-1.37);
    CHECKCOMPACT(-1.38);
    CHECKCOMPACT(-1.39);
    CHECKCOMPACT(-1.4);
    CHECKCOMPACT(-1.41);
    CHECKCOMPACT(-1.42);
    CHECKCOMPACT(-1.43);
    CHECKCOMPACT(-1.44);
    CHECKCOMPACT(-1.45);
    CHECKCOMPACT(-1.46);
    CHECKCOMPACT(-1.47);
    CHECKCOMPACT(-1.48);
    CHECKCOMPACT(-1.49);
    CHECKCOMPACT(-1.5);
    CHECKCOMPACT(-1.51);
    CHECKCOMPACT(-1.52);
    CHECKCOMPACT(-1.53);
    CHECKCOMPACT(-1.54);
    CHECKCOMPACT(-1.55);
    CHECKCOMPACT(-1.56);
    CHECKCOMPACT(-1.57);
    CHECKCOMPACT(-1.58);
    CHECKCOMPACT(-1.59);
    CHECKCOMPACT(-1.6);
    CHECKCOMPACT(-1.61);
    CHECKCOMPACT(-1.62);
    CHECKCOMPACT(-1.63);
    CHECKCOMPACT(-1.64);
    CHECKCOMPACT(-1.65);
    CHECKCOMPACT(-1.66);
    CHECKCOMPACT(-1.67);
    CHECKCOMPACT(-1.68);
    CHECKCOMPACT(-1.69);
    CHECKCOMPACT(-1.7);
    CHECKCOMPACT(-1.71);
    CHECKCOMPACT(-1.72);
    CHECKCOMPACT(-1.73);
    CHECKCOMPACT(-1.74);
    CHECKCOMPACT(-1.75);
    CHECKCOMPACT(-1.76);
    CHECKCOMPACT(-1.77);
    CHECKCOMPACT(-1.78);
    CHECKCOMPACT(-1.79);
    CHECKCOMPACT(-1.8);
    CHECKCOMPACT(-1.81);
    CHECKCOMPACT(-1.82);
    CHECKCOMPACT(-1.83);
    CHECKCOMPACT(-1.84);
    CHECKCOMPACT(-1.85);
    CHECKCOMPACT(-1.86);
    CHECKCOMPACT(-1.87);
    CHECKCOMPACT(-1.88);
    CHECKCOMPACT(-1.89);
    CHECKCOMPACT(-1.9);
    CHECKCOMPACT(-1.91);
    CHECKCOMPACT(-1.92);
    CHECKCOMPACT(-1.93);
    CHECKCOMPACT(-1.94);
    CHECKCOMPACT(-1.95);
    CHECKCOMPACT(-1.96);
    CHECKCOMPACT(-1.97);
    CHECKCOMPACT(-1.98);
    CHECKCOMPACT(-1.99);
    CHECKCOMPACT(-2);
    CHECKCOMPACT(-2.01);
    CHECKCOMPACT(-2.02);
    CHECKCOMPACT(-2.03);
    CHECKCOMPACT(-2.04);
    CHECKCOMPACT(-2.05);
    CHECKCOMPACT(-2.06);
    CHECKCOMPACT(-2.07);
    CHECKCOMPACT(-2.08);
    CHECKCOMPACT(-2.09);
    CHECKCOMPACT(-2.1);
    CHECKCOMPACT(-2.11);
    CHECKCOMPACT(-2.12);
    CHECKCOMPACT(-2.13);
    CHECKCOMPACT(-2.14);
    CHECKCOMPACT(-2.15);
    CHECKCOMPACT(-2.16);
    CHECKCOMPACT(-2.17);
    CHECKCOMPACT(-2.18);
    CHECKCOMPACT(-2.19);
    CHECKCOMPACT(-2.2);
    CHECKCOMPACT(-2.21);
    CHECKCOMPACT(-2.22);
    CHECKCOMPACT(-2.23);
    CHECKCOMPACT(-2.24);
    CHECKCOMPACT(-2.25);
    CHECKCOMPACT(-2.26);
    CHECKCOMPACT(-2.27);
    CHECKCOMPACT(-2.28);
    CHECKCOMPACT(-2.29);
    CHECKCOMPACT(-2.3);
    CHECKCOMPACT(-2.31);
    CHECKCOMPACT(-2.32);
    CHECKCOMPACT(-2.33);
    CHECKCOMPACT(-2.34);
    CHECKCOMPACT(-2.35);
    CHECKCOMPACT(-2.36);
    CHECKCOMPACT(-2.37);
    CHECKCOMPACT(-2.38);
    CHECKCOMPACT(-2.39);
    CHECKCOMPACT(-2.4);
    CHECKCOMPACT(-2.41);
    CHECKCOMPACT(-2.42);
    CHECKCOMPACT(-2.43);
    CHECKCOMPACT(-2.44);
    CHECKCOMPACT(-2.45);
    CHECKCOMPACT(-2.46);
    CHECKCOMPACT(-2.47);
    CHECKCOMPACT(-2.48);
    CHECKCOMPACT(-2.49);
    CHECKCOMPACT(-2.5);
    CHECKCOMPACT(-2.51);
    CHECKCOMPACT(-2.52);
    CHECKCOMPACT(-2.53);
    CHECKCOMPACT(-2.54);
    CHECKCOMPACT(-2.55);
    CHECKCOMPACT(-2.56);
    CHECKCOMPACT(-2.57);
    CHECKCOMPACT(-2.58);
    CHECKCOMPACT(-2.59);
    CHECKCOMPACT(-2.6);
    CHECKCOMPACT(-2.61);
    CHECKCOMPACT(-2.62);
    CHECKCOMPACT(-2.63);
    CHECKCOMPACT(-2.64);
    CHECKCOMPACT(-2.65);
    CHECKCOMPACT(-2.66);
    CHECKCOMPACT(-2.67);
    CHECKCOMPACT(-2.68);
    CHECKCOMPACT(-2.69);
    CHECKCOMPACT(-2.7);
    CHECKCOMPACT(-2.71);
    CHECKCOMPACT(-2.72);
    CHECKCOMPACT(-2.73);
    CHECKCOMPACT(-2.74);
    CHECKCOMPACT(-2.75);
    CHECKCOMPACT(-2.76);
    CHECKCOMPACT(-2.77);
    CHECKCOMPACT(-2.78);
    CHECKCOMPACT(-2.79);
    CHECKCOMPACT(-2.8);
    CHECKCOMPACT(-2.81);
    CHECKCOMPACT(-2.82);
    CHECKCOMPACT(-2.83);
    CHECKCOMPACT(-2.84);
    CHECKCOMPACT(-2.85);
    CHECKCOMPACT(-2.86);
    CHECKCOMPACT(-2.87);
    CHECKCOMPACT(-2.88);
    CHECKCOMPACT(-2.89);
    CHECKCOMPACT(-2.9);
    CHECKCOMPACT(-2.91);
    CHECKCOMPACT(-2.92);
    CHECKCOMPACT(-2.93);
    CHECKCOMPACT(-2.94);
    CHECKCOMPACT(-2.95);
    CHECKCOMPACT(-2.96);
    CHECKCOMPACT(-2.97);
    CHECKCOMPACT(-2.98);
    CHECKCOMPACT(-2.99);
    CHECKCOMPACT(-3);
    CHECKCOMPACT(-3.01);
    CHECKCOMPACT(-3.02);
    CHECKCOMPACT(-3.03);
    CHECKCOMPACT(-3.04);
    CHECKCOMPACT(-3.05);
    CHECKCOMPACT(-3.06);
    CHECKCOMPACT(-3.07);
    CHECKCOMPACT(-3.08);
    CHECKCOMPACT(-3.09);
    CHECKCOMPACT(-3.1);
    CHECKCOMPACT(-3.11);
    CHECKCOMPACT(-3.12);
    CHECKCOMPACT(-3.13);
    CHECKCOMPACT(-3.14);
    CHECKCOMPACT(-3.15);
    CHECKCOMPACT(-3.16);
    CHECKCOMPACT(-3.17);
    CHECKCOMPACT(-3.18);
    CHECKCOMPACT(-3.19);
    CHECKCOMPACT(-3.2);
    CHECKCOMPACT(-3.21);
    CHECKCOMPACT(-3.22);
    CHECKCOMPACT(-3.23);
    CHECKCOMPACT(-3.24);
    CHECKCOMPACT(-3.25);
    CHECKCOMPACT(-3.26);
    CHECKCOMPACT(-3.27);
    CHECKCOMPACT(-3.28);
    CHECKCOMPACT(-3.29);
    CHECKCOMPACT(-3.3);
    CHECKCOMPACT(-3.31);
    CHECKCOMPACT(-3.32);
    CHECKCOMPACT(-3.33);
    CHECKCOMPACT(-3.34);
    CHECKCOMPACT(-3.35);
    CHECKCOMPACT(-3.36);
    CHECKCOMPACT(-3.37);
    CHECKCOMPACT(-3.38);
    CHECKCOMPACT(-3.39);
    CHECKCOMPACT(-3.4);
    CHECKCOMPACT(-3.41);
    CHECKCOMPACT(-3.42);
    CHECKCOMPACT(-3.43);
    CHECKCOMPACT(-3.44);
    CHECKCOMPACT(-3.45);
    CHECKCOMPACT(-3.46);
    CHECKCOMPACT(-3.47);
    CHECKCOMPACT(-3.48);
    CHECKCOMPACT(-3.49);
    CHECKCOMPACT(-3.5);
    CHECKCOMPACT(-3.51);
    CHECKCOMPACT(-3.52);
    CHECKCOMPACT(-3.53);
    CHECKCOMPACT(-3.54);
    CHECKCOMPACT(-3.55);
    CHECKCOMPACT(-3.56);
    CHECKCOMPACT(-3.57);
    CHECKCOMPACT(-3.58);
    CHECKCOMPACT(-3.59);
    CHECKCOMPACT(-3.6);
    CHECKCOMPACT(-3.61);
    CHECKCOMPACT(-3.62);
    CHECKCOMPACT(-3.63);
    CHECKCOMPACT(-3.64);
    CHECKCOMPACT(-3.65);
    CHECKCOMPACT(-3.66);
    CHECKCOMPACT(-3.67);
    CHECKCOMPACT(-3.68);
    CHECKCOMPACT(-3.69);
    CHECKCOMPACT(-3.7);
    CHECKCOMPACT(-3.71);
    CHECKCOMPACT(-3.72);
    CHECKCOMPACT(-3.73);
    CHECKCOMPACT(-3.74);
    CHECKCOMPACT(-3.75);
    CHECKCOMPACT(-3.76);
    CHECKCOMPACT(-3.77);
    CHECKCOMPACT(-3.78);
    CHECKCOMPACT(-3.79);
    CHECKCOMPACT(-3.8);
    CHECKCOMPACT(-3.81);
    CHECKCOMPACT(-3.82);
    CHECKCOMPACT(-3.83);
    CHECKCOMPACT(-3.84);
    CHECKCOMPACT(-3.85);
    CHECKCOMPACT(-3.86);
    CHECKCOMPACT(-3.87);
    CHECKCOMPACT(-3.88);
    CHECKCOMPACT(-3.89);
    CHECKCOMPACT(-3.9);
    CHECKCOMPACT(-3.91);
    CHECKCOMPACT(-3.92);
    CHECKCOMPACT(-3.93);
    CHECKCOMPACT(-3.94);
    CHECKCOMPACT(-3.95);
    CHECKCOMPACT(-3.96);
    CHECKCOMPACT(-3.97);
    CHECKCOMPACT(-3.98);
    CHECKCOMPACT(-3.99);
    CHECKCOMPACT(-4);
    CHECKCOMPACT(-4.01);
    CHECKCOMPACT(-4.02);
    CHECKCOMPACT(-4.03);
    CHECKCOMPACT(-4.04);
    CHECKCOMPACT(-4.05);
    CHECKCOMPACT(-4.06);
    CHECKCOMPACT(-4.07);
    CHECKCOMPACT(-4.08);
    CHECKCOMPACT(-4.09);
    CHECKCOMPACT(-4.1);
    CHECKCOMPACT(-4.11);
    CHECKCOMPACT(-4.12);
    CHECKCOMPACT(-4.13);
    CHECKCOMPACT(-4.14);
    CHECKCOMPACT(-4.15);
    CHECKCOMPACT(-4.16);
    CHECKCOMPACT(-4.17);
    CHECKCOMPACT(-4.18);
    CHECKCOMPACT(-4.19);
    CHECKCOMPACT(-4.2);
    CHECKCOMPACT(-4.21);
    CHECKCOMPACT(-4.22);
    CHECKCOMPACT(-4.23);
    CHECKCOMPACT(-4.24);
    CHECKCOMPACT(-4.25);
    CHECKCOMPACT(-4.26);
    CHECKCOMPACT(-4.27);
    CHECKCOMPACT(-4.28);
    CHECKCOMPACT(-4.29);
    CHECKCOMPACT(-4.3);
    CHECKCOMPACT(-4.31);
    CHECKCOMPACT(-4.32);
    CHECKCOMPACT(-4.33);
    CHECKCOMPACT(-4.34);
    CHECKCOMPACT(-4.35);
    CHECKCOMPACT(-4.36);
    CHECKCOMPACT(-4.37);
    CHECKCOMPACT(-4.38);
    CHECKCOMPACT(-4.39);
    CHECKCOMPACT(-4.4);
    CHECKCOMPACT(-4.41);
    CHECKCOMPACT(-4.42);
    CHECKCOMPACT(-4.43);
    CHECKCOMPACT(-4.44);
    CHECKCOMPACT(-4.45);
    CHECKCOMPACT(-4.46);
    CHECKCOMPACT(-4.47);
    CHECKCOMPACT(-4.48);
    CHECKCOMPACT(-4.49);
    CHECKCOMPACT(-4.5);
    CHECKCOMPACT(-4.51);
    CHECKCOMPACT(-4.52);
    CHECKCOMPACT(-4.53);
    CHECKCOMPACT(-4.54);
    CHECKCOMPACT(-4.55);
    CHECKCOMPACT(-4.56);
    CHECKCOMPACT(-4.57);
    CHECKCOMPACT(-4.58);
    CHECKCOMPACT(-4.59);
    CHECKCOMPACT(-4.6);
    CHECKCOMPACT(-4.61);
    CHECKCOMPACT(-4.62);
    CHECKCOMPACT(-4.63);
    CHECKCOMPACT(-4.64);
    CHECKCOMPACT(-4.65);
    CHECKCOMPACT(-4.66);
    CHECKCOMPACT(-4.67);
    CHECKCOMPACT(-4.68);
    CHECKCOMPACT(-4.69);
    CHECKCOMPACT(-4.7);
    CHECKCOMPACT(-4.71);
    CHECKCOMPACT(-4.72);
    CHECKCOMPACT(-4.73);
    CHECKCOMPACT(-4.74);
    CHECKCOMPACT(-4.75);
    CHECKCOMPACT(-4.76);
    CHECKCOMPACT(-4.77);
    CHECKCOMPACT(-4.78);
    CHECKCOMPACT(-4.79);
    CHECKCOMPACT(-4.8);
    CHECKCOMPACT(-4.81);
    CHECKCOMPACT(-4.82);
    CHECKCOMPACT(-4.83);
    CHECKCOMPACT(-4.84);
    CHECKCOMPACT(-4.85);
    CHECKCOMPACT(-4.86);
    CHECKCOMPACT(-4.87);
    CHECKCOMPACT(-4.88);
    CHECKCOMPACT(-4.89);
    CHECKCOMPACT(-4.9);
    CHECKCOMPACT(-4.91);
    CHECKCOMPACT(-4.92);
    CHECKCOMPACT(-4.93);
    CHECKCOMPACT(-4.94);
    CHECKCOMPACT(-4.95);
    CHECKCOMPACT(-4.96);
    CHECKCOMPACT(-4.97);
    CHECKCOMPACT(-4.98);
    CHECKCOMPACT(-4.99);
    CHECKCOMPACT(-5);
    CHECKCOMPACT(-5.01);
    CHECKCOMPACT(-5.02);
    CHECKCOMPACT(-5.03);
    CHECKCOMPACT(-5.04);
    CHECKCOMPACT(-5.05);
    CHECKCOMPACT(-5.06);
    CHECKCOMPACT(-5.07);
    CHECKCOMPACT(-5.08);
    CHECKCOMPACT(-5.09);
    CHECKCOMPACT(-5.1);
    CHECKCOMPACT(-5.11);
    CHECKCOMPACT(-5.12);
    CHECKCOMPACT(-5.13);
    CHECKCOMPACT(-5.14);
    CHECKCOMPACT(-5.15);
    CHECKCOMPACT(-5.16);
    CHECKCOMPACT(-5.17);
    CHECKCOMPACT(-5.18);
    CHECKCOMPACT(-5.19);
    CHECKCOMPACT(-5.2);
    CHECKCOMPACT(-5.21);
    CHECKCOMPACT(-5.22);
    CHECKCOMPACT(-5.23);
    CHECKCOMPACT(-5.24);
    CHECKCOMPACT(-5.25);
    CHECKCOMPACT(-5.26);
    CHECKCOMPACT(-5.27);
    CHECKCOMPACT(-5.28);
    CHECKCOMPACT(-5.29);
    CHECKCOMPACT(-5.3);
    CHECKCOMPACT(-5.31);
    CHECKCOMPACT(-5.32);
    CHECKCOMPACT(-5.33);
    CHECKCOMPACT(-5.34);
    CHECKCOMPACT(-5.35);
    CHECKCOMPACT(-5.36);
    CHECKCOMPACT(-5.37);
    CHECKCOMPACT(-5.38);
    CHECKCOMPACT(-5.39);
    CHECKCOMPACT(-5.4);
    CHECKCOMPACT(-5.41);
    CHECKCOMPACT(-5.42);
    CHECKCOMPACT(-5.43);
    CHECKCOMPACT(-5.44);
    CHECKCOMPACT(-5.45);
    CHECKCOMPACT(-5.46);
    CHECKCOMPACT(-5.47);
    CHECKCOMPACT(-5.48);
    CHECKCOMPACT(-5.49);
    CHECKCOMPACT(-5.5);
    CHECKCOMPACT(-5.51);
    CHECKCOMPACT(-5.52);
    CHECKCOMPACT(-5.53);
    CHECKCOMPACT(-5.54);
    CHECKCOMPACT(-5.55);
    CHECKCOMPACT(-5.56);
    CHECKCOMPACT(-5.57);
    CHECKCOMPACT(-5.58);
    CHECKCOMPACT(-5.59);
    CHECKCOMPACT(-5.6);
    CHECKCOMPACT(-5.61);
    CHECKCOMPACT(-5.62);
    CHECKCOMPACT(-5.63);
    CHECKCOMPACT(-5.64);
    CHECKCOMPACT(-5.65);
    CHECKCOMPACT(-5.66);
    CHECKCOMPACT(-5.67);
    CHECKCOMPACT(-5.68);
    CHECKCOMPACT(-5.69);
    CHECKCOMPACT(-5.7);
    CHECKCOMPACT(-5.71);
    CHECKCOMPACT(-5.72);
    CHECKCOMPACT(-5.73);
    CHECKCOMPACT(-5.74);
    CHECKCOMPACT(-5.75);
    CHECKCOMPACT(-5.76);
    CHECKCOMPACT(-5.77);
    CHECKCOMPACT(-5.78);
    CHECKCOMPACT(-5.79);
    CHECKCOMPACT(-5.8);
    CHECKCOMPACT(-5.81);
    CHECKCOMPACT(-5.82);
    CHECKCOMPACT(-5.83);
    CHECKCOMPACT(-5.84);
    CHECKCOMPACT(-5.85);
    CHECKCOMPACT(-5.86);
    CHECKCOMPACT(-5.87);
    CHECKCOMPACT(-5.88);
    CHECKCOMPACT(-5.89);
    CHECKCOMPACT(-5.9);
    CHECKCOMPACT(-5.91);
    CHECKCOMPACT(-5.92);
    CHECKCOMPACT(-5.93);
    CHECKCOMPACT(-5.94);
    CHECKCOMPACT(-5.95);
    CHECKCOMPACT(-5.96);
    CHECKCOMPACT(-5.97);
    CHECKCOMPACT(-5.98);
    CHECKCOMPACT(-5.99);
    CHECKCOMPACT(-6);
    CHECKCOMPACT(-6.01);
    CHECKCOMPACT(-6.02);
    CHECKCOMPACT(-6.03);
    CHECKCOMPACT(-6.04);
    CHECKCOMPACT(-6.05);
    CHECKCOMPACT(-6.06);
    CHECKCOMPACT(-6.07);
    CHECKCOMPACT(-6.08);
    CHECKCOMPACT(-6.09);
    CHECKCOMPACT(-6.1);
    CHECKCOMPACT(-6.11);
    CHECKCOMPACT(-6.12);
    CHECKCOMPACT(-6.13);
    CHECKCOMPACT(-6.14);
    CHECKCOMPACT(-6.15);
    CHECKCOMPACT(-6.16);
    CHECKCOMPACT(-6.17);
    CHECKCOMPACT(-6.18);
    CHECKCOMPACT(-6.19);
    CHECKCOMPACT(-6.2);
    CHECKCOMPACT(-6.21);
    CHECKCOMPACT(-6.22);
    CHECKCOMPACT(-6.23);
    CHECKCOMPACT(-6.24);
    CHECKCOMPACT(-6.25);
    CHECKCOMPACT(-6.26);
    CHECKCOMPACT(-6.27);
    CHECKCOMPACT(-6.28);
    CHECKCOMPACT(-6.29);
    CHECKCOMPACT(-6.3);
    CHECKCOMPACT(-6.31);
    CHECKCOMPACT(-6.32);
    CHECKCOMPACT(-6.33);
    CHECKCOMPACT(-6.34);
    CHECKCOMPACT(-6.35);
    CHECKCOMPACT(-6.36);
    CHECKCOMPACT(-6.37);
    CHECKCOMPACT(-6.38);
    CHECKCOMPACT(-6.39);
    CHECKCOMPACT(-6.4);
    CHECKCOMPACT(-6.41);
    CHECKCOMPACT(-6.42);
    CHECKCOMPACT(-6.43);
    CHECKCOMPACT(-6.44);
    CHECKCOMPACT(-6.45);
    CHECKCOMPACT(-6.46);
    CHECKCOMPACT(-6.47);
    CHECKCOMPACT(-6.48);
    CHECKCOMPACT(-6.49);
    CHECKCOMPACT(-6.5);
    CHECKCOMPACT(-6.51);
    CHECKCOMPACT(-6.52);
    CHECKCOMPACT(-6.53);
    CHECKCOMPACT(-6.54);
    CHECKCOMPACT(-6.55);
    CHECKCOMPACT(-6.56);
    CHECKCOMPACT(-6.57);
    CHECKCOMPACT(-6.58);
    CHECKCOMPACT(-6.59);
    CHECKCOMPACT(-6.6);
    CHECKCOMPACT(-6.61);
    CHECKCOMPACT(-6.62);
    CHECKCOMPACT(-6.63);
    CHECKCOMPACT(-6.64);
    CHECKCOMPACT(-6.65);
    CHECKCOMPACT(-6.66);
    CHECKCOMPACT(-6.67);
    CHECKCOMPACT(-6.68);
    CHECKCOMPACT(-6.69);
    CHECKCOMPACT(-6.7);
    CHECKCOMPACT(-6.71);
    CHECKCOMPACT(-6.72);
    CHECKCOMPACT(-6.73);
    CHECKCOMPACT(-6.74);
    CHECKCOMPACT(-6.75);
    CHECKCOMPACT(-6.76);
    CHECKCOMPACT(-6.77);
    CHECKCOMPACT(-6.78);
    CHECKCOMPACT(-6.79);
    CHECKCOMPACT(-6.8);
    CHECKCOMPACT(-6.81);
    CHECKCOMPACT(-6.82);
    CHECKCOMPACT(-6.83);
    CHECKCOMPACT(-6.84);
    CHECKCOMPACT(-6.85);
    CHECKCOMPACT(-6.86);
    CHECKCOMPACT(-6.87);
    CHECKCOMPACT(-6.88);
    CHECKCOMPACT(-6.89);
    CHECKCOMPACT(-6.9);
    CHECKCOMPACT(-6.91);
    CHECKCOMPACT(-6.92);
    CHECKCOMPACT(-6.93);
    CHECKCOMPACT(-6.94);
    CHECKCOMPACT(-6.95);
    CHECKCOMPACT(-6.96);
    CHECKCOMPACT(-6.97);
    CHECKCOMPACT(-6.98);
    CHECKCOMPACT(-6.99);
    CHECKCOMPACT(-7);
    CHECKCOMPACT(-7.01);
    CHECKCOMPACT(-7.02);
    CHECKCOMPACT(-7.03);
    CHECKCOMPACT(-7.04);
    CHECKCOMPACT(-7.05);
    CHECKCOMPACT(-7.06);
    CHECKCOMPACT(-7.07);
    CHECKCOMPACT(-7.08);
    CHECKCOMPACT(-7.09);
    CHECKCOMPACT(-7.1);
    CHECKCOMPACT(-7.11);
    CHECKCOMPACT(-7.12);
    CHECKCOMPACT(-7.13);
    CHECKCOMPACT(-7.14);
    CHECKCOMPACT(-7.15);
    CHECKCOMPACT(-7.16);
    CHECKCOMPACT(-7.17);
    CHECKCOMPACT(-7.18);
    CHECKCOMPACT(-7.19);
    CHECKCOMPACT(-7.2);
    CHECKCOMPACT(-7.21);
    CHECKCOMPACT(-7.22);
    CHECKCOMPACT(-7.23);
    CHECKCOMPACT(-7.24);
    CHECKCOMPACT(-7.25);
    CHECKCOMPACT(-7.26);
    CHECKCOMPACT(-7.27);
    CHECKCOMPACT(-7.28);
    CHECKCOMPACT(-7.29);
    CHECKCOMPACT(-7.3);
    CHECKCOMPACT(-7.31);
    CHECKCOMPACT(-7.32);
    CHECKCOMPACT(-7.33);
    CHECKCOMPACT(-7.34);
    CHECKCOMPACT(-7.35);
    CHECKCOMPACT(-7.36);
    CHECKCOMPACT(-7.37);
    CHECKCOMPACT(-7.38);
    CHECKCOMPACT(-7.39);
    CHECKCOMPACT(-7.4);
    CHECKCOMPACT(-7.41);
    CHECKCOMPACT(-7.42);
    CHECKCOMPACT(-7.43);
    CHECKCOMPACT(-7.44);
    CHECKCOMPACT(-7.45);
    CHECKCOMPACT(-7.46);
    CHECKCOMPACT(-7.47);
    CHECKCOMPACT(-7.48);
    CHECKCOMPACT(-7.49);
    CHECKCOMPACT(-7.5);
    CHECKCOMPACT(-7.51);
    CHECKCOMPACT(-7.52);
    CHECKCOMPACT(-7.53);
    CHECKCOMPACT(-7.54);
    CHECKCOMPACT(-7.55);
    CHECKCOMPACT(-7.56);
    CHECKCOMPACT(-7.57);
    CHECKCOMPACT(-7.58);
    CHECKCOMPACT(-7.59);
    CHECKCOMPACT(-7.6);
    CHECKCOMPACT(-7.61);
    CHECKCOMPACT(-7.62);
    CHECKCOMPACT(-7.63);
    CHECKCOMPACT(-7.64);
    CHECKCOMPACT(-7.65);
    CHECKCOMPACT(-7.66);
    CHECKCOMPACT(-7.67);
    CHECKCOMPACT(-7.68);
    CHECKCOMPACT(-7.69);
    CHECKCOMPACT(-7.7);
    CHECKCOMPACT(-7.71);
    CHECKCOMPACT(-7.72);
    CHECKCOMPACT(-7.73);
    CHECKCOMPACT(-7.74);
    CHECKCOMPACT(-7.75);
    CHECKCOMPACT(-7.76);
    CHECKCOMPACT(-7.77);
    CHECKCOMPACT(-7.78);
    CHECKCOMPACT(-7.79);
    CHECKCOMPACT(-7.8);
    CHECKCOMPACT(-7.81);
    CHECKCOMPACT(-7.82);
    CHECKCOMPACT(-7.83);
    CHECKCOMPACT(-7.84);
    CHECKCOMPACT(-7.85);
    CHECKCOMPACT(-7.86);
    CHECKCOMPACT(-7.87);
    CHECKCOMPACT(-7.88);
    CHECKCOMPACT(-7.89);
    CHECKCOMPACT(-7.9);
    CHECKCOMPACT(-7.91);
    CHECKCOMPACT(-7.92);
    CHECKCOMPACT(-7.93);
    CHECKCOMPACT(-7.94);
    CHECKCOMPACT(-7.95);
    CHECKCOMPACT(-7.96);
    CHECKCOMPACT(-7.97);
    CHECKCOMPACT(-7.98);
    CHECKCOMPACT(-7.99);
    CHECKCOMPACT(-8);
    CHECKCOMPACT(-8.01);
    CHECKCOMPACT(-8.02);
    CHECKCOMPACT(-8.03);
    CHECKCOMPACT(-8.04);
    CHECKCOMPACT(-8.05);
    CHECKCOMPACT(-8.06);
    CHECKCOMPACT(-8.07);
    CHECKCOMPACT(-8.08);
    CHECKCOMPACT(-8.09);
    CHECKCOMPACT(-8.1);
    CHECKCOMPACT(-8.11);
    CHECKCOMPACT(-8.12);
    CHECKCOMPACT(-8.13);
    CHECKCOMPACT(-8.14);
    CHECKCOMPACT(-8.15);
    CHECKCOMPACT(-8.16);
    CHECKCOMPACT(-8.17);
    CHECKCOMPACT(-8.18);
    CHECKCOMPACT(-8.19);
    CHECKCOMPACT(-8.2);
    CHECKCOMPACT(-8.21);
    CHECKCOMPACT(-8.22);
    CHECKCOMPACT(-8.23);
    CHECKCOMPACT(-8.24);
    CHECKCOMPACT(-8.25);
    CHECKCOMPACT(-8.26);
    CHECKCOMPACT(-8.27);
    CHECKCOMPACT(-8.28);
    CHECKCOMPACT(-8.29);
    CHECKCOMPACT(-8.3);
    CHECKCOMPACT(-8.31);
    CHECKCOMPACT(-8.32);
    CHECKCOMPACT(-8.33);
    CHECKCOMPACT(-8.34);
    CHECKCOMPACT(-8.35);
    CHECKCOMPACT(-8.36);
    CHECKCOMPACT(-8.37);
    CHECKCOMPACT(-8.38);
    CHECKCOMPACT(-8.39);
    CHECKCOMPACT(-8.4);
    CHECKCOMPACT(-8.41);
    CHECKCOMPACT(-8.42);
    CHECKCOMPACT(-8.43);
    CHECKCOMPACT(-8.44);
    CHECKCOMPACT(-8.45);
    CHECKCOMPACT(-8.46);
    CHECKCOMPACT(-8.47);
    CHECKCOMPACT(-8.48);
    CHECKCOMPACT(-8.49);
    CHECKCOMPACT(-8.5);
    CHECKCOMPACT(-8.51);
    CHECKCOMPACT(-8.52);
    CHECKCOMPACT(-8.53);
    CHECKCOMPACT(-8.54);
    CHECKCOMPACT(-8.55);
    CHECKCOMPACT(-8.56);
    CHECKCOMPACT(-8.57);
    CHECKCOMPACT(-8.58);
    CHECKCOMPACT(-8.59);
    CHECKCOMPACT(-8.6);
    CHECKCOMPACT(-8.61);
    CHECKCOMPACT(-8.62);
    CHECKCOMPACT(-8.63);
    CHECKCOMPACT(-8.64);
    CHECKCOMPACT(-8.65);
    CHECKCOMPACT(-8.66);
    CHECKCOMPACT(-8.67);
    CHECKCOMPACT(-8.68);
    CHECKCOMPACT(-8.69);
    CHECKCOMPACT(-8.7);
    CHECKCOMPACT(-8.71);
    CHECKCOMPACT(-8.72);
    CHECKCOMPACT(-8.73);
    CHECKCOMPACT(-8.74);
    CHECKCOMPACT(-8.75);
    CHECKCOMPACT(-8.76);
    CHECKCOMPACT(-8.77);
    CHECKCOMPACT(-8.78);
    CHECKCOMPACT(-8.79);
    CHECKCOMPACT(-8.8);
    CHECKCOMPACT(-8.81);
    CHECKCOMPACT(-8.82);
    CHECKCOMPACT(-8.83);
    CHECKCOMPACT(-8.84);
    CHECKCOMPACT(-8.85);
    CHECKCOMPACT(-8.86);
    CHECKCOMPACT(-8.87);
    CHECKCOMPACT(-8.88);
    CHECKCOMPACT(-8.89);
    CHECKCOMPACT(-8.9);
    CHECKCOMPACT(-8.91);
    CHECKCOMPACT(-8.92);
    CHECKCOMPACT(-8.93);
    CHECKCOMPACT(-8.94);
    CHECKCOMPACT(-8.95);
    CHECKCOMPACT(-8.96);
    CHECKCOMPACT(-8.97);
    CHECKCOMPACT(-8.98);
    CHECKCOMPACT(-8.99);
    CHECKCOMPACT(-9);
    CHECKCOMPACT(-9.01);
    CHECKCOMPACT(-9.02);
    CHECKCOMPACT(-9.03);
    CHECKCOMPACT(-9.04);
    CHECKCOMPACT(-9.05);
    CHECKCOMPACT(-9.06);
    CHECKCOMPACT(-9.07);
    CHECKCOMPACT(-9.08);
    CHECKCOMPACT(-9.09);
    CHECKCOMPACT(-9.1);
    CHECKCOMPACT(-9.11);
    CHECKCOMPACT(-9.12);
    CHECKCOMPACT(-9.13);
    CHECKCOMPACT(-9.14);
    CHECKCOMPACT(-9.15);
    CHECKCOMPACT(-9.16);
    CHECKCOMPACT(-9.17);
    CHECKCOMPACT(-9.18);
    CHECKCOMPACT(-9.19);
    CHECKCOMPACT(-9.2);
    CHECKCOMPACT(-9.21);
    CHECKCOMPACT(-9.22);
    CHECKCOMPACT(-9.23);
    CHECKCOMPACT(-9.24);
    CHECKCOMPACT(-9.25);
    CHECKCOMPACT(-9.26);
    CHECKCOMPACT(-9.27);
    CHECKCOMPACT(-9.28);
    CHECKCOMPACT(-9.29);
    CHECKCOMPACT(-9.3);
    CHECKCOMPACT(-9.31);
    CHECKCOMPACT(-9.32);
    CHECKCOMPACT(-9.33);
    CHECKCOMPACT(-9.34);
    CHECKCOMPACT(-9.35);
    CHECKCOMPACT(-9.36);
    CHECKCOMPACT(-9.37);
    CHECKCOMPACT(-9.38);
    CHECKCOMPACT(-9.39);
    CHECKCOMPACT(-9.4);
    CHECKCOMPACT(-9.41);
    CHECKCOMPACT(-9.42);
    CHECKCOMPACT(-9.43);
    CHECKCOMPACT(-9.44);
    CHECKCOMPACT(-9.45);
    CHECKCOMPACT(-9.46);
    CHECKCOMPACT(-9.47);
    CHECKCOMPACT(-9.48);
    CHECKCOMPACT(-9.49);
    CHECKCOMPACT(-9.5);
    CHECKCOMPACT(-9.51);
    CHECKCOMPACT(-9.52);
    CHECKCOMPACT(-9.53);
    CHECKCOMPACT(-9.54);
    CHECKCOMPACT(-9.55);
    CHECKCOMPACT(-9.56);
    CHECKCOMPACT(-9.57);
    CHECKCOMPACT(-9.58);
    CHECKCOMPACT(-9.59);
    CHECKCOMPACT(-9.6);
    CHECKCOMPACT(-9.61);
    CHECKCOMPACT(-9.62);
    CHECKCOMPACT(-9.63);
    CHECKCOMPACT(-9.64);
    CHECKCOMPACT(-9.65);
    CHECKCOMPACT(-9.66);
    CHECKCOMPACT(-9.67);
    CHECKCOMPACT(-9.68);
    CHECKCOMPACT(-9.69);
    CHECKCOMPACT(-9.7);
    CHECKCOMPACT(-9.71);
    CHECKCOMPACT(-9.72);
    CHECKCOMPACT(-9.73);
    CHECKCOMPACT(-9.74);
    CHECKCOMPACT(-9.75);
    CHECKCOMPACT(-9.76);
    CHECKCOMPACT(-9.77);
    CHECKCOMPACT(-9.78);
    CHECKCOMPACT(-9.79);
    CHECKCOMPACT(-9.8);
    CHECKCOMPACT(-9.81);
    CHECKCOMPACT(-9.82);
    CHECKCOMPACT(-9.83);
    CHECKCOMPACT(-9.84);
    CHECKCOMPACT(-9.85);
    CHECKCOMPACT(-9.86);
    CHECKCOMPACT(-9.87);
    CHECKCOMPACT(-9.88);
    CHECKCOMPACT(-9.89);
    CHECKCOMPACT(-9.9);
    CHECKCOMPACT(-9.91);
    CHECKCOMPACT(-9.92);
    CHECKCOMPACT(-9.93);
    CHECKCOMPACT(-9.94);
    CHECKCOMPACT(-9.95);
    CHECKCOMPACT(-9.96);
    CHECKCOMPACT(-9.97);
    CHECKCOMPACT(-9.98);
    CHECKCOMPACT(-9.99);
    CHECKCOMPACT(-10);

    }

   char buffer[32];
#if defined(_MSC_VER) && defined(__STDC_SECURE_LIB__) // Use secure version with visual studio 2005 to avoid warning. 
   sprintf_s(buffer, sizeof(buffer), "%#.16g", value); 
#else	
   sprintf(buffer, "%#.16g", value); 
#endif
   char* ch = buffer + strlen(buffer) - 1;
   if (*ch != '0') return buffer; // nothing to truncate, so save time
   while(ch > buffer && *ch == '0'){
     --ch;
   }
   char* last_nonzero = ch;
   while(ch >= buffer){
     switch(*ch){
     case '0':
     case '1':
     case '2':
     case '3':
     case '4':
     case '5':
     case '6':
     case '7':
     case '8':
     case '9':
       --ch;
       continue;
     case '.':
       // Truncate zeroes to save bytes in output, but keep one.
       *(last_nonzero+2) = '\0';
       return buffer;
     default:
       return buffer;
     }
   }
   return buffer;
}


std::string valueToString( bool value )
{
   return value ? "true" : "false";
}

std::string valueToQuotedString( const char *value )
{
   // Not sure how to handle unicode...
   if (strpbrk(value, "\"\\\b\f\n\r\t") == NULL && !containsControlCharacter( value ))
      return std::string("\"") + value + "\"";
   // We have to walk value and escape any special characters.
   // Appending to std::string is not efficient, but this should be rare.
   // (Note: forward slashes are *not* rare, but I am not escaping them.)
   std::string::size_type maxsize = strlen(value)*2 + 3; // allescaped+quotes+NULL
   std::string result;
   result.reserve(maxsize); // to avoid lots of mallocs
   result += "\"";
   for (const char* c=value; *c != 0; ++c)
   {
      switch(*c)
      {
         case '\"':
            result += "\\\"";
            break;
         case '\\':
            result += "\\\\";
            break;
         case '\b':
            result += "\\b";
            break;
         case '\f':
            result += "\\f";
            break;
         case '\n':
            result += "\\n";
            break;
         case '\r':
            result += "\\r";
            break;
         case '\t':
            result += "\\t";
            break;
         //case '/':
            // Even though \/ is considered a legal escape in JSON, a bare
            // slash is also legal, so I see no reason to escape it.
            // (I hope I am not misunderstanding something.
            // blep notes: actually escaping \/ may be useful in javascript to avoid </ 
            // sequence.
            // Should add a flag to allow this compatibility mode and prevent this 
            // sequence from occurring.
         default:
            if ( isControlCharacter( *c ) )
            {
               std::ostringstream oss;
               oss << "\\u" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << static_cast<int>(*c);
               result += oss.str();
            }
            else
            {
               result += *c;
            }
            break;
      }
   }
   result += "\"";
   return result;
}

// Class Writer
// //////////////////////////////////////////////////////////////////
Writer::~Writer()
{
}


// Class FastWriter
// //////////////////////////////////////////////////////////////////

FastWriter::FastWriter()
   : yamlCompatiblityEnabled_( false )
{
}


void 
FastWriter::enableYAMLCompatibility()
{
   yamlCompatiblityEnabled_ = true;
}


std::string 
FastWriter::write( const Value &root )
{
   document_ = "";
   writeValue( root );
   document_ += "\n";
   return document_;
}


void 
FastWriter::writeValue( const Value &value )
{
   switch ( value.type() )
   {
   case nullValue:
      document_ += "null";
      break;
   case intValue:
      document_ += valueToString( value.asLargestInt() );
      break;
   case uintValue:
      document_ += valueToString( value.asLargestUInt() );
      break;
   case realValue: {
      std::string s = valueToString( value.asDouble() );
      //if ( s == "0.2000000029802322" )
      //    s = "0.2";
      document_ += s;
   }
      break;
   case stringValue:
      document_ += valueToQuotedString( value.asCString() );
      break;
   case booleanValue:
      document_ += valueToString( value.asBool() );
      break;
   case arrayValue:
      {
         document_ += "[";
         int size = value.size();
         for ( int index =0; index < size; ++index )
         {
            if ( index > 0 )
               document_ += ",";
            writeValue( value[index] );
         }
         document_ += "]";
      }
      break;
   case objectValue:
      {
         Value::Members members( value.getMemberNames() );
         document_ += "{";
         for ( Value::Members::iterator it = members.begin(); 
               it != members.end(); 
               ++it )
         {
            const std::string &name = *it;
            if ( it != members.begin() )
               document_ += ",";
            document_ += valueToQuotedString( name.c_str() );
            document_ += yamlCompatiblityEnabled_ ? ": " 
                                                  : ":";
            writeValue( value[name] );
         }
         document_ += "}";
      }
      break;
   }
}


// Class StyledWriter
// //////////////////////////////////////////////////////////////////

StyledWriter::StyledWriter()
   : rightMargin_( 74 )
   , indentSize_( 3 )
{
}


std::string 
StyledWriter::write( const Value &root )
{
   document_ = "";
   addChildValues_ = false;
   indentString_ = "";
   writeCommentBeforeValue( root );
   writeValue( root );
   writeCommentAfterValueOnSameLine( root );
   document_ += "\n";
   return document_;
}


void 
StyledWriter::writeValue( const Value &value )
{
   switch ( value.type() )
   {
   case nullValue:
      pushValue( "null" );
      break;
   case intValue:
      pushValue( valueToString( value.asLargestInt() ) );
      break;
   case uintValue:
      pushValue( valueToString( value.asLargestUInt() ) );
      break;
   case realValue: {
      std::string s = valueToString( value.asDouble() );
      //if ( s == "0.2000000029802322" )
      //    s = "0.2";
      pushValue( s );
   }
      break;
   case stringValue:
      pushValue( valueToQuotedString( value.asCString() ) );
      break;
   case booleanValue:
      pushValue( valueToString( value.asBool() ) );
      break;
   case arrayValue:
      writeArrayValue( value);
      break;
   case objectValue:
      {
         Value::Members members( value.getMemberNames() );
         if ( members.empty() )
            pushValue( "{}" );
         else
         {
            writeWithIndent( "{" );
            indent();
            Value::Members::iterator it = members.begin();
            for (;;)
            {
               const std::string &name = *it;
               const Value &childValue = value[name];
               writeCommentBeforeValue( childValue );
               writeWithIndent( valueToQuotedString( name.c_str() ) );
               document_ += " : ";
               writeValue( childValue );
               if ( ++it == members.end() )
               {
                  writeCommentAfterValueOnSameLine( childValue );
                  break;
               }
               document_ += ",";
               writeCommentAfterValueOnSameLine( childValue );
            }
            unindent();
            writeWithIndent( "}" );
         }
      }
      break;
   }
}


void 
StyledWriter::writeArrayValue( const Value &value )
{
   unsigned size = value.size();
   if ( size == 0 )
      pushValue( "[]" );
   else
   {
      bool isArrayMultiLine = isMultineArray( value );
      if ( isArrayMultiLine )
      {
         writeWithIndent( "[" );
         indent();
         bool hasChildValue = !childValues_.empty();
         unsigned index =0;
         for (;;)
         {
            const Value &childValue = value[index];
            writeCommentBeforeValue( childValue );
            if ( hasChildValue )
               writeWithIndent( childValues_[index] );
            else
            {
               writeIndent();
               writeValue( childValue );
            }
            if ( ++index == size )
            {
               writeCommentAfterValueOnSameLine( childValue );
               break;
            }
            document_ += ",";
            writeCommentAfterValueOnSameLine( childValue );
         }
         unindent();
         writeWithIndent( "]" );
      }
      else // output on a single line
      {
         assert( childValues_.size() == size );
         document_ += "[ ";
         for ( unsigned index =0; index < size; ++index )
         {
            if ( index > 0 )
               document_ += ", ";
            document_ += childValues_[index];
         }
         document_ += " ]";
      }
   }
}


bool 
StyledWriter::isMultineArray( const Value &value )
{
   int size = value.size();
   bool isMultiLine = size*3 >= rightMargin_ ;
   childValues_.clear();
   for ( int index =0; index < size  &&  !isMultiLine; ++index )
   {
      const Value &childValue = value[index];
      isMultiLine = isMultiLine  ||
                     ( (childValue.isArray()  ||  childValue.isObject())  &&  
                        childValue.size() > 0 );
   }
   if ( !isMultiLine ) // check if line length > max line length
   {
      childValues_.reserve( size );
      addChildValues_ = true;
      int lineLength = 4 + (size-1)*2; // '[ ' + ', '*n + ' ]'
      for ( int index =0; index < size  &&  !isMultiLine; ++index )
      {
         writeValue( value[index] );
         lineLength += int( childValues_[index].length() );
         isMultiLine = isMultiLine  &&  hasCommentForValue( value[index] );
      }
      addChildValues_ = false;
      isMultiLine = isMultiLine  ||  lineLength >= rightMargin_;
   }
   return isMultiLine;
}


void 
StyledWriter::pushValue( const std::string &value )
{
   if ( addChildValues_ )
      childValues_.push_back( value );
   else
      document_ += value;
}


void 
StyledWriter::writeIndent()
{
   if ( !document_.empty() )
   {
      char last = document_[document_.length()-1];
      if ( last == ' ' )     // already indented
         return;
      if ( last != '\n' )    // Comments may add new-line
         document_ += '\n';
   }
   document_ += indentString_;
}


void 
StyledWriter::writeWithIndent( const std::string &value )
{
   writeIndent();
   document_ += value;
}


void 
StyledWriter::indent()
{
   indentString_ += std::string( indentSize_, ' ' );
}


void 
StyledWriter::unindent()
{
   assert( int(indentString_.size()) >= indentSize_ );
   indentString_.resize( indentString_.size() - indentSize_ );
}


void 
StyledWriter::writeCommentBeforeValue( const Value &root )
{
   if ( !root.hasComment( commentBefore ) )
      return;
   document_ += normalizeEOL( root.getComment( commentBefore ) );
   document_ += "\n";
}


void 
StyledWriter::writeCommentAfterValueOnSameLine( const Value &root )
{
   if ( root.hasComment( commentAfterOnSameLine ) )
      document_ += " " + normalizeEOL( root.getComment( commentAfterOnSameLine ) );

   if ( root.hasComment( commentAfter ) )
   {
      document_ += "\n";
      document_ += normalizeEOL( root.getComment( commentAfter ) );
      document_ += "\n";
   }
}


bool 
StyledWriter::hasCommentForValue( const Value &value )
{
   return value.hasComment( commentBefore )
          ||  value.hasComment( commentAfterOnSameLine )
          ||  value.hasComment( commentAfter );
}


std::string 
StyledWriter::normalizeEOL( const std::string &text )
{
   std::string normalized;
   normalized.reserve( text.length() );
   const char *begin = text.c_str();
   const char *end = begin + text.length();
   const char *current = begin;
   while ( current != end )
   {
      char c = *current++;
      if ( c == '\r' ) // mac or dos EOL
      {
         if ( *current == '\n' ) // convert dos EOL
            ++current;
         normalized += '\n';
      }
      else // handle unix EOL & other char
         normalized += c;
   }
   return normalized;
}


// Class StyledStreamWriter
// //////////////////////////////////////////////////////////////////

StyledStreamWriter::StyledStreamWriter( std::string indentation )
   : document_(NULL)
   , rightMargin_( 74 )
   , indentation_( indentation )
{
}


void
StyledStreamWriter::write( std::ostream &out, const Value &root )
{
   document_ = &out;
   addChildValues_ = false;
   indentString_ = "";
   writeCommentBeforeValue( root );
   writeValue( root );
   writeCommentAfterValueOnSameLine( root );
   *document_ << "\n";
   document_ = NULL; // Forget the stream, for safety.
}


void 
StyledStreamWriter::writeValue( const Value &value )
{
   switch ( value.type() )
   {
   case nullValue:
      pushValue( "null" );
      break;
   case intValue:
      pushValue( valueToString( value.asLargestInt() ) );
      break;
   case uintValue:
      pushValue( valueToString( value.asLargestUInt() ) );
      break;
   case realValue: {
      std::string s = valueToString( value.asDouble() );
      //if ( s == "0.2000000029802322" )
      //    s = "0.2";
      pushValue( s );
   }
      break;
   case stringValue:
      pushValue( valueToQuotedString( value.asCString() ) );
      break;
   case booleanValue:
      pushValue( valueToString( value.asBool() ) );
      break;
   case arrayValue:
      writeArrayValue( value);
      break;
   case objectValue:
      {
         Value::Members members( value.getMemberNames() );
         if ( members.empty() )
            pushValue( "{}" );
         else
         {
            writeWithIndent( "{" );
            indent();
            Value::Members::iterator it = members.begin();
            for (;;)
            {
               const std::string &name = *it;
               const Value &childValue = value[name];
               writeCommentBeforeValue( childValue );
               writeWithIndent( valueToQuotedString( name.c_str() ) );
               *document_ << " : ";
               writeValue( childValue );
               if ( ++it == members.end() )
               {
                  writeCommentAfterValueOnSameLine( childValue );
                  break;
               }
               *document_ << ",";
               writeCommentAfterValueOnSameLine( childValue );
            }
            unindent();
            writeWithIndent( "}" );
         }
      }
      break;
   }
}


void 
StyledStreamWriter::writeArrayValue( const Value &value )
{
   unsigned size = value.size();
   if ( size == 0 )
      pushValue( "[]" );
   else
   {
      bool isArrayMultiLine = isMultineArray( value );
      if ( isArrayMultiLine )
      {
         writeWithIndent( "[" );
         indent();
         bool hasChildValue = !childValues_.empty();
         unsigned index =0;
         for (;;)
         {
            const Value &childValue = value[index];
            writeCommentBeforeValue( childValue );
            if ( hasChildValue )
               writeWithIndent( childValues_[index] );
            else
            {
               writeIndent();
               writeValue( childValue );
            }
            if ( ++index == size )
            {
               writeCommentAfterValueOnSameLine( childValue );
               break;
            }
            *document_ << ",";
            writeCommentAfterValueOnSameLine( childValue );
         }
         unindent();
         writeWithIndent( "]" );
      }
      else // output on a single line
      {
         assert( childValues_.size() == size );
         *document_ << "[ ";
         for ( unsigned index =0; index < size; ++index )
         {
            if ( index > 0 )
               *document_ << ", ";
            *document_ << childValues_[index];
         }
         *document_ << " ]";
      }
   }
}


bool 
StyledStreamWriter::isMultineArray( const Value &value )
{
   int size = value.size();
   bool isMultiLine = size*3 >= rightMargin_ ;
   childValues_.clear();
   for ( int index =0; index < size  &&  !isMultiLine; ++index )
   {
      const Value &childValue = value[index];
      isMultiLine = isMultiLine  ||
                     ( (childValue.isArray()  ||  childValue.isObject())  &&  
                        childValue.size() > 0 );
   }
   if ( !isMultiLine ) // check if line length > max line length
   {
      childValues_.reserve( size );
      addChildValues_ = true;
      int lineLength = 4 + (size-1)*2; // '[ ' + ', '*n + ' ]'
      for ( int index =0; index < size  &&  !isMultiLine; ++index )
      {
         writeValue( value[index] );
         lineLength += int( childValues_[index].length() );
         isMultiLine = isMultiLine  &&  hasCommentForValue( value[index] );
      }
      addChildValues_ = false;
      isMultiLine = isMultiLine  ||  lineLength >= rightMargin_;
   }
   return isMultiLine;
}


void 
StyledStreamWriter::pushValue( const std::string &value )
{
   if ( addChildValues_ )
      childValues_.push_back( value );
   else
      *document_ << value;
}


void 
StyledStreamWriter::writeIndent()
{
  /*
    Some comments in this method would have been nice. ;-)

   if ( !document_.empty() )
   {
      char last = document_[document_.length()-1];
      if ( last == ' ' )     // already indented
         return;
      if ( last != '\n' )    // Comments may add new-line
         *document_ << '\n';
   }
  */
   *document_ << '\n' << indentString_;
}


void 
StyledStreamWriter::writeWithIndent( const std::string &value )
{
   writeIndent();
   *document_ << value;
}


void 
StyledStreamWriter::indent()
{
   indentString_ += indentation_;
}


void 
StyledStreamWriter::unindent()
{
   assert( indentString_.size() >= indentation_.size() );
   indentString_.resize( indentString_.size() - indentation_.size() );
}


void 
StyledStreamWriter::writeCommentBeforeValue( const Value &root )
{
   if ( !root.hasComment( commentBefore ) )
      return;
   *document_ << normalizeEOL( root.getComment( commentBefore ) );
   *document_ << "\n";
}


void 
StyledStreamWriter::writeCommentAfterValueOnSameLine( const Value &root )
{
   if ( root.hasComment( commentAfterOnSameLine ) )
      *document_ << " " + normalizeEOL( root.getComment( commentAfterOnSameLine ) );

   if ( root.hasComment( commentAfter ) )
   {
      *document_ << "\n";
      *document_ << normalizeEOL( root.getComment( commentAfter ) );
      *document_ << "\n";
   }
}


bool 
StyledStreamWriter::hasCommentForValue( const Value &value )
{
   return value.hasComment( commentBefore )
          ||  value.hasComment( commentAfterOnSameLine )
          ||  value.hasComment( commentAfter );
}


std::string 
StyledStreamWriter::normalizeEOL( const std::string &text )
{
   std::string normalized;
   normalized.reserve( text.length() );
   const char *begin = text.c_str();
   const char *end = begin + text.length();
   const char *current = begin;
   while ( current != end )
   {
      char c = *current++;
      if ( c == '\r' ) // mac or dos EOL
      {
         if ( *current == '\n' ) // convert dos EOL
            ++current;
         normalized += '\n';
      }
      else // handle unix EOL & other char
         normalized += c;
   }
   return normalized;
}


std::ostream& operator<<( std::ostream &sout, const Value &root )
{
   Json::StyledStreamWriter writer;
   writer.write(sout, root);
   return sout;
}


} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_writer.cpp
// //////////////////////////////////////////////////////////////////////





