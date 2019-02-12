#ifndef _STRUTIL_H_
#define _STRUTIL_H_

// 2005.07.01

#include <assert.h>
#include <tchar.h>
#include <string.h>

#include <string>
#include <vector>

using namespace std;

typedef basic_string<TCHAR> tstring;

inline int Compare( const char* s1, const char* s2 )
{
	return strcmp( s1, s2 );
}

inline int Compare( const char* s1, const char* s2, int size )
{
	return strncmp( s1, s2, size );
}

inline int CompareNoCase( const char* s1, const char* s2 )
{
	return _stricmp( s1, s2 );
}

inline int CompareNoCase( const char* s1, const char* s2, int size )
{
	return _strnicmp( s1, s2, size );
}

inline int Compare( const wchar_t* s1, const wchar_t* s2 )
{
	return wcscmp( s1, s2 );
}

inline int Compare( const wchar_t* s1, const wchar_t* s2, int size )
{
	return wcsncmp( s1, s2, size );
}

inline int CompareNoCase( const wchar_t* s1, const wchar_t* s2 )
{
	return _wcsicmp( s1, s2 );
}

inline int CompareNoCase( const wchar_t* s1, const wchar_t* s2, int size )
{
	return _wcsnicmp( s1, s2, size );
}

inline void MakeUpper( string& str )
{
	_strupr_s( &str[0], str.length() );
}

inline void MakeUpper( wstring& str )
{
	_wcsupr_s( &str[0], str.length() );
}

inline void MakeLower( string& str )
{
	_strlwr_s( &str[0], str.length() );
}

inline void MakeLower( wstring& str )
{
	_wcslwr_s( &str[0], str.length() );
}

template < class TYPE >
bool EndsWith( const basic_string<TYPE>& str, const basic_string<TYPE>& target )
{
	if ( target.size() > str.size() )
		return false;

	basic_string<TYPE>::const_iterator itr1 = str.end() - target.size();
	basic_string<TYPE>::const_iterator itr2 = target.begin();

	while ( itr2 != target.end() ) {
		if ( *itr1 != *itr2 )
			return false;
		++itr1;
		++itr2;
	}
	return true;
}

template < class TYPE >
bool EndsWith( const basic_string<TYPE>& str, const TYPE* target, int nCount )
{
	if ( nCount > (int)str.size() )
		return false;

	basic_string<TYPE>::const_iterator itr = str.end() - nCount;

	while ( itr != str.end() ) {
		if ( *itr != *target )
			return false;
		++itr;
		++target;
	}
	return true;
}

inline float ToFloat( const string& str )
{
	return (float)atof( str.c_str() );
}

inline float ToFloat( const wstring& str )
{
	return (float)_wtof( str.c_str() );
}

static wchar_t _StrUtilTmp[_CVTBUFSIZE];

inline void FromFloat( string& str, float n )
{
	sprintf_s( (char*)_StrUtilTmp, _CVTBUFSIZE * sizeof(wchar_t), "%f", n );
	str = (char*)_StrUtilTmp;
}

inline void FromFloat( wstring& str, float n )
{
	swprintf_s( _StrUtilTmp, L"%f", n );
	str = _StrUtilTmp;
}

inline double ToDouble( const string& str )
{
	return atof( str.c_str() );
}

inline double ToDouble( const wstring& str )
{
	return _wtof( str.c_str() );
}

inline int ToInt( const string& str )
{
    return atoi( str.c_str() );
}

inline int ToInt( const wstring& str )
{
    return _wtoi( str.c_str() );
}

inline long ToLong( const string& str )
{
	return atol( str.c_str() );
}

inline long ToLong( const wstring& str )
{
	return _wtol( str.c_str() );
}

template < class TYPE >
inline
bool IsNull( const basic_string<TYPE>& str )
{
	return ( str.size() == 0x00 );
}

inline
bool IsNumber( const string& str )
{
	string::const_iterator citr;
	for ( citr = str.begin(); citr != str.end(); ++citr ) {
		if ( isdigit( *citr ) == 0 )
			return false;
	}
	return true;
}

inline
bool IsNumber( const wstring& str )
{
	wstring::const_iterator citr;
	for ( citr = str.begin(); citr != str.end(); ++citr ) {
		if ( iswdigit( *citr ) == 0 )
			return false;
	}
	return true;
}

inline
char* TrimLeft( const char* _cpc1, const char* _cpc2 )
{
	return (char*)((*((_cpc1)+strspn(_cpc1,_cpc2))) ? ((_cpc1)+strspn(_cpc1,_cpc2)) : NULL);
}

inline
wchar_t* TrimLeft( const wchar_t* _cpc1, const wchar_t* _cpc2 )
{
	return (wchar_t*)((*((_cpc1)+wcsspn(_cpc1,_cpc2))) ? ((_cpc1)+wcsspn(_cpc1,_cpc2)) : NULL);
}

template < class TYPE >
basic_string<TYPE>& TrimLeft( basic_string<TYPE>& str, const TYPE& aChar = (TYPE)' ' )
{
	// find first non-matching character
	basic_string<TYPE>::size_type pos = str.find_first_not_of( aChar );
	if ( pos == basic_string<TYPE>::npos ) {
		str.erase();
	}
	else if ( pos != 0 ) {
		str.erase( 0, pos );
	}
	return str;
}

template < class TYPE >
basic_string<TYPE>& TrimLeft( basic_string<TYPE>& str, const basic_string<TYPE>& targets )
{
	size_type pos = str.find_first_not_of( targets );
	if ( pos == basic_string<TYPE>::npos ) {
		str.clear();
	}
	else if ( pos != 0 ) {
		str.erase( 0, pos );
	}
	return str;
}

template < class TYPE >
basic_string<TYPE>& TrimRight( basic_string<TYPE>& str, const TYPE& aChar = (TYPE)' ' )
{
	basic_string<TYPE>::size_type pos = str.find_last_not_of( aChar );
	if ( pos != basic_string<TYPE>::npos ) {
		return str.erase( pos+1 );
	}
	return str;
}

template < class TYPE >
basic_string<TYPE>& TrimRight( basic_string<TYPE>& str, const basic_string<TYPE>& targets )
{
    
	basic_string<TYPE>::size_type pos = str.find_last_not_of( targets );
	if ( pos != basic_string<TYPE>::npos ) {
		return str.erase( pos+1 );
	}
	return str;
}

template < class TYPE >
inline
int Find( const basic_string<TYPE>& str, const basic_string<TYPE>& strSub, int nStart = 0 )
{
	return str.find( strSub, nStart );
}

template < class TYPE >
inline
int ReverseFind( const basic_string<TYPE>& str, TYPE ch )
{
	// find last single character
	// return -1 if not found, distance from beginning otherwise
	return str.find_last_of(ch);
}

template < class TYPE >
inline
basic_string<TYPE> Left( const basic_string<TYPE>& str, int nCount )
{
	return str.substr( 0, nCount );
}

template < class TYPE >
inline
basic_string<TYPE> Right( const basic_string<TYPE>& str, int nCount )
{
	return str.substr( size() - nCount );
}

template < class TYPE >
inline
basic_string<TYPE> Mid( const basic_string<TYPE>& str, int nFirst, int nCount = -1 )
{
	return str.substr( nFirst, nCount );
}

template < class TYPE >
void ParseFileName( basic_string<TYPE>& rStrPath, basic_string<TYPE>& rStrTitle, basic_string<TYPE>& rStrExt, const TYPE* pStrFilePathName )
{
	rStrPath = pStrFilePathName;

	basic_string<TYPE>::size_type pos = rStrPath.find_last_of( (TYPE)'\\' );
	if ( pos == basic_string<TYPE>::npos ) {
		pos = rStrPath.find_last_of( (TYPE)'/' );
	}

    // find the path
	if ( pos == basic_string<TYPE>::npos ) {
		rStrTitle = rStrPath;
        rStrPath.erase();
	}
    else {
		rStrTitle = &rStrPath[pos+1];
		rStrPath.resize(pos);
	}

	// find the title
	pos = rStrTitle.find_last_of( (TYPE)'.' );
    if ( pos == basic_string<TYPE>::npos ) {
		rStrExt.erase();
	}
	else {
		rStrExt = &rStrTitle[pos+1];
        rStrTitle.resize(pos);
	}
}

inline
string& FormatV( string& str, const char* lpszFormat, va_list argList )
{
	const int maxSize = 1024 * 1024; // about 1MB bytes
	const int bufSize = MAX_PATH;
	int attemptedSize = bufSize;

	vector<char> temp;
	temp.resize( attemptedSize );

	int numChars = _vsnprintf_s( &temp[0], attemptedSize, attemptedSize - 1, lpszFormat, argList );

	if (numChars >= 0) {
		str = &temp[0];
		return str;
	}

    while ((numChars == -1) && (attemptedSize < maxSize)) {
		// Try a bigger size
		attemptedSize *= 2;
		temp.resize( attemptedSize );
		numChars = _vsnprintf_s( &temp[0], attemptedSize, attemptedSize- 1, lpszFormat, argList );
	}

    if (numChars >= 0) {
		str = &temp[0];
		return str;
	}
	else {
		str.erase();
		return str;
	}
}

inline
wstring& FormatV( wstring& str, const wchar_t* lpszFormat, va_list argList )
{
	const int maxSize = 1024 * 1024; // about 1MB bytes
	const int bufSize = MAX_PATH;
	int attemptedSize = bufSize;

	vector<wchar_t> temp;
	temp.resize( attemptedSize );

	int numChars = _vsnwprintf_s( &temp[0], attemptedSize, attemptedSize - 1, lpszFormat, argList );

	if (numChars >= 0) {
		str = &temp[0];
		return str;
	}

    while ((numChars == -1) && (attemptedSize < maxSize)) {
		// Try a bigger size
		attemptedSize *= 2;
		temp.resize( attemptedSize );
		numChars = _vsnwprintf_s( &temp[0], attemptedSize, attemptedSize - 1, lpszFormat, argList );
	}

    if (numChars >= 0) {
		str = &temp[0];
		return str;
	}
	else {
		str.erase();
		return str;
	}
}

template < class TYPE >
inline
basic_string<TYPE>& Format( basic_string<TYPE>& str, const TYPE* lpszFormat, ... )
{
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV( str, lpszFormat, argList);
	va_end(argList);
	return str;
}

template < class TYPE >
void Tokenize( vector< basic_string<TYPE> >& aStringVector, const basic_string<TYPE>& CmdLine, const basic_string<TYPE>& Delimiter )
{
	aStringVector.clear();
	basic_string<TYPE>::size_type posS = 0;
	basic_string<TYPE>::size_type posE = 0;

	while ( posE < CmdLine.size() ) {
		posS = CmdLine.find_first_not_of( Delimiter, posE );
		if ( posS == basic_string<TYPE>::npos )
			break;

        posE = CmdLine.find_first_of( Delimiter, posS );
        if ( posE == basic_string<TYPE>::npos ) {
			aStringVector.push_back( CmdLine.substr(posS) );
			break;
		}
		else {
			aStringVector.push_back( CmdLine.substr( posS, posE - posS ) );
			posE++;
		}
	}
}

template < class TYPE >
inline
void Tokenize( vector< basic_string<TYPE> >& aStringVector, const basic_string<TYPE>& CmdLine, const TYPE* pszDelimiter )
{
	Tokenize( aStringVector, CmdLine, basic_string<TYPE>(pszDelimiter) );
}

template < class TYPE >
inline
void Tokenize( vector< basic_string<TYPE> >& aStringVector, const TYPE* pszCmdLine, const TYPE* pszDelimiter )
{
	Tokenize( aStringVector, basic_string<TYPE>(pszCmdLine), basic_string<TYPE>(pszDelimiter) );
}

template < class TYPE >
bool ExtractSubString( basic_string<TYPE>& rString, const basic_string<TYPE>& rFullString,
					  int iSubString, TYPE chSep )
{
	if ( rFullString.size() == 0 )
		return false;

	basic_string<TYPE>::size_type posS = 0;
	while (iSubString--) {
		posS = rFullString.find_first_of( chSep, posS );
		if ( posS == basic_string<TYPE>::npos )	{
            rString.clear(); // return empty string as well
			return false;
		}
		posS++; // point past the separator
	}

	basic_string<TYPE>::size_type posE;
	posE = rFullString.find_first_of( chSep, posS );

	if ( posE == basic_string<TYPE>::npos )
		posE = rFullString.size();

	rString = rFullString.substr( posS, posE - posS );

	return true;
}

template < class TYPE >
void ExtractSubStrings( vector< basic_string<TYPE> >& aStringVector, const basic_string<TYPE>& rFullString, TYPE chSep )
{
	aStringVector.clear();
	basic_string<TYPE>::size_type posS = 0;
	basic_string<TYPE>::size_type posE = 0;

	while ( posS < rFullString.size() ) {
		posE = rFullString.find_first_of( chSep, posS );
		if ( posE == basic_string<TYPE>::npos ) {
			aStringVector.push_back( rFullString.substr(posS) );
			break;
		}
		else {
			aStringVector.push_back( rFullString.substr(posS,posE-posS) );
		}
		posS = posE+1;
	}
}

template < class TYPE >
void ExtractSubStrings( vector< basic_string<TYPE> >& aStringVector, const TYPE* pszFullString, TYPE chSep )
{
	ExtractSubStrings( aStringVector, basic_string<TYPE>(pszFullString), chSep );	
}

template < class TYPE >
void ReplaceWith( basic_string<TYPE>& rString, const TYPE ch, const TYPE replaced )
{
	basic_string<TYPE>::iterator i = rString.begin();
	basic_string<TYPE>::iterator itrEnd = rString.end();
	while ( i != itrEnd ) {
		if ( *i == ch ) {
			*i = replaced;
		}
		++i;
	}
}

inline
void ConvertFormat( char* pszOutStr, int nOutChSize, const char* pszInStr )
{
	strcpy_s( pszOutStr, nOutChSize, pszInStr );
}

template <size_t size>
void ConvertFormat( char (&pszOutStr)[size], const char* pszInStr )
{
	strcpy_s( pszOutStr, pszInStr );
}

inline
void ConvertFormat( wchar_t* pszOutStr, int nOutChSize, const wchar_t* pszInStr )
{
	wcscpy_s( pszOutStr, nOutChSize, pszInStr );
}

template <size_t size>
void ConvertFormat( wchar_t (&pszOutStr)[size], const wchar_t* pszInStr )
{
	wcscpy_s( pszOutStr, pszInStr );
}

inline
int ConvertFormat( char* outStr, int outSize, const wchar_t* pszInStr )
{
	int n = WideCharToMultiByte( CP_ACP, NULL, pszInStr, (int)wcslen(pszInStr), outStr, outSize, NULL, NULL );
	outStr[n] = 0;
	return n;
}

template <size_t outSize>
int ConvertFormat( char (&outStr)[outSize], const wchar_t* pszInStr )
{
	int n = WideCharToMultiByte( CP_ACP, NULL, pszInStr, (int)wcslen(pszInStr), outStr, outSize, NULL, NULL );
	outStr[n] = 0;
	return n;
}

inline
int ConvertFormat( wchar_t* outStr, int outSize, const char* pszInStr )
{
	int n = MultiByteToWideChar( CP_ACP, NULL, pszInStr, (int)strlen(pszInStr), outStr, outSize );
	outStr[n] = 0;
	return n;
}

template <size_t outSize>
int ConvertFormat( wchar_t (&outStr)[outSize], const char* pszInStr )
{
	int n = MultiByteToWideChar( CP_ACP, NULL, pszInStr, (int)strlen(pszInStr), outStr, outSize );
	outStr[n] = 0;
	return n;
}

inline
void ConvertFormat( string& outStr, const char* pszInStr )
{
	outStr = pszInStr;
}

inline
void ConvertFormat( wstring& outStr, const wchar_t* pszInStr )
{
	outStr = pszInStr;
}

inline
int ConvertFormat( string& outStr, const wchar_t* pszInStr )
{
	int n = WideCharToMultiByte( CP_ACP, NULL, pszInStr, (int)wcslen(pszInStr), NULL, 0, NULL, NULL );
    outStr.resize(n);
	return WideCharToMultiByte( CP_ACP, NULL, pszInStr, (int)wcslen(pszInStr), &outStr[0], n, NULL, NULL );
}

inline
int ConvertFormat( wstring& outStr, const char* pszInStr )
{
	int n = MultiByteToWideChar( CP_ACP, NULL, pszInStr, (int)strlen(pszInStr), NULL, 0 );
	outStr.resize(n);
	return MultiByteToWideChar( CP_ACP, NULL, pszInStr, (int)strlen(pszInStr), &outStr[0], n );
}

inline
int ConvertFormat( string& outStr, const string& inStr )
{
	outStr = inStr;
	return (int)outStr.size();
}

inline
int ConvertFormat( wstring& outStr, const wstring& inStr )
{
	outStr = inStr;
	return (int)outStr.size();
}

inline
int ConvertFormat( string& outStr, const wstring& inStr )
{
	int n = WideCharToMultiByte( CP_ACP, NULL, inStr.c_str(), (int)inStr.size(), NULL, 0, NULL, NULL );
    outStr.resize(n);
	return WideCharToMultiByte( CP_ACP, NULL, inStr.c_str(), (int)inStr.size(), &outStr[0], n, NULL, NULL );
}

inline
int ConvertFormat( wstring& outStr, const string& inStr )
{
	int n = MultiByteToWideChar( CP_ACP, NULL, inStr.c_str(), (int)inStr.size(), NULL, 0 );
	outStr.resize(n);
	return MultiByteToWideChar( CP_ACP, NULL, inStr.c_str(), (int)inStr.size(), &outStr[0], n );
}

class CStringHelper
{
	string			m_strA;
	wstring			m_strW;
public:
	CStringHelper( const char* psz )
	{
		m_strA = psz;
		ConvertFormat( m_strW, m_strA );
	}

	CStringHelper( const wchar_t* psz )
	{
		m_strW = psz;
		ConvertFormat( m_strA, m_strW );
	}

	const char* ToA()
	{
		return m_strA.c_str();
	}

	const wchar_t* ToW()
	{
		return m_strW.c_str();
	}

};

inline
int GetBitCount( wchar_t n )
{
	int i;
	for ( i = 16; i > 0; i-- ) {
		if ( n & 0x8000 ) {
            return i;
		}
		n <<= 1;
	}
	return i;
}

inline
int UnicodeToUtf8( wchar_t in, BYTE* out )
{
	// When 7     bits, 0*******
	// When 8-11  bits, 110***** 10******
	// When 12-16 bits, 1110**** 10****** 10******
	int nBitCount = GetBitCount(in);
	if ( nBitCount <= 7 ) {
		*out = (BYTE)in;
		return 1;
	}
	else if ( nBitCount <= 11 ) {
		*out     = (0xC0|(in>>6));
		*(out+1) = (0x80|(in&0x3F));
		return 2;
	}
	else {
		*out     = (0xE0|(in>>12));
		*(out+1) = (0x80|((in>>6)&0x3F));
		*(out+2) = (0x80|(in&0x3F));
		return 3;
	}
	return 0;
}

inline
int UnicodeToUtf8( const wchar_t* in, BYTE* out )
{
	BYTE* p = out;
	while ( *in != NULL ) {
		p += UnicodeToUtf8( *in, p );
		in++;
	}
	return (int)(p-out);
}

inline
int Utf8ToUnicode( const BYTE* in, wchar_t& out )
{
	if ( (in[0] & 0x80) == 0x00 ) {
		// When 7 bits, 0*******
		out = in[0];
		return 1;
	}
	else if ( (in[0] & 0xE0) == 0xC0 ) {
		// When 8-11 bits, 110***** 10******
		out = ((in[0]&0x1F)<<6)|(in[1]&0x3F);
		return 2;
	}
	else if ( (in[0] & 0xF0) == 0xE0 ) {
		// When 12-16 bits, 1110**** 10****** 10******
		out = ((in[0]&0x0F)<<12)|((in[1]&0x3F)<<6)|(in[2]&0x3F);
		return 3;
	}

	out = 0;
	return 1;
}

inline
void Utf8ToUnicode( const BYTE* in, wstring& out )
{
	while (1)
	{
        wchar_t wch;
		int n = Utf8ToUnicode(in,wch);
		if ( wch == 0 )
			break;
		out.push_back(wch);
		in += n;
	}
}

inline
int Utf8Length( const BYTE* str )
{
	int str_len = 0;
	int l = (int)strlen((const char*)str);

	for ( int i = 0; i < l; ++i )
	{
        BYTE byte0 = str[i];
		if (byte0 & 0x80)
		{
			if ((byte0 & 0xe0) == 0xc0)
			{
				if (++i >= l)
					break;
				BYTE byte1 = str[i];
				++str_len;
			}
			else if ((byte0 & 0xf0) == 0xe0)
			{
				if (++i >= l)
					break;
				BYTE byte1 = str[i];
				if (++i >= l)
					break;
				BYTE byte2 = str[i];
				++str_len;
			}
			else
				break;
			/* MMM - currently up to 16-bit code are supported */
		}
		else
			++str_len;
	}
	return str_len;
}

inline
bool IsInteger( const char* p )
{
	char* endptr = NULL;
	long l = strtoul( p, &endptr, 10 );
	return (endptr - p) == strlen(p);
}

inline
bool IsInteger( const wchar_t* p )
{
	wchar_t* endptr = NULL;
	long l = wcstoul( p, &endptr, 10 );
	return (endptr - p) == wcslen(p);
}

inline
bool IsDouble( const char* p )
{
	char* endptr = NULL;
	double d = strtod( p, &endptr );
	return (endptr - p) == strlen(p);
}

inline
bool IsDouble( const wchar_t* p )
{
	wchar_t* endptr = NULL;
	double d = wcstod( p, &endptr );
	return (endptr - p) == wcslen(p);
}

// Grammer 
// identifier :
//		nondigit
//		identifier nodigit
//		identifier digit
// nondigit : one of
//		_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ
// digit : one of 
//		0123456789
inline
bool IsVariable( const char* p )
{
	const char* p1 = strpbrk( p, "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" );
	if ( p1 == NULL || p1 != p )
		return false;
	p++;
	size_t n = strspn( p, "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" );
	if ( n != strlen(p) )
		return false;
	return true;
}

inline
bool IsVariable( const wchar_t* p )
{
	const wchar_t* p1 = wcspbrk( p, L"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" );
	if ( p1 == NULL || p1 != p )
		return false;
	p++;
	size_t n = wcsspn( p, L"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" );
	if ( n != wcslen(p) )
		return false;
	return true;
}

inline
bool IsString( const char* p )
{
	size_t len = strlen(p);
	if ( len < 2 )
		return false;
	if ( *p != '"' || *(p+len-1) != '"' )
		return false;
	return true;
}

inline
bool IsString( const wchar_t* p )
{
	size_t len = wcslen(p);
	if ( len < 2 )
		return false;
	if ( *p != L'"' || *(p+len-1) != L'"' )
		return false;
	return true;
}

inline
void ConvertEscapeSequences( const char* p, string& out )
{
	while ( *p != NULL ) {
		if ( *p != '\\' )
			out.push_back(*p);
		else if ( *(p+1) != '\0' ){
			switch ( *(p+1) ) {
			case 'a' : out.push_back('\a'); break;
			case 'b' : out.push_back('\b'); break;
			case 'f' : out.push_back('\f'); break;
			case 'n' : out.push_back('\n'); break;
			case 'r' : out.push_back('\r'); break;
			case 't' : out.push_back('\t'); break;
			case 'v' : out.push_back('\v'); break;
			case '\'': out.push_back('\''); break;
			case '"' : out.push_back('\"'); break;
			case '\\': out.push_back('\\'); break;
			case '?' : out.push_back('\?'); break;
			}
			// The illegal escape character is just skipped.
			p++;
		}
		else {
			// the last character is '\\', skip it
		}
		p++;
	}
}

inline
void ConvertEscapeSequences( const wchar_t* p, wstring& out )
{
	while ( *p != NULL ) {
		if ( *p != L'\\' )
			out.push_back(*p);
		else if ( *(p+1) != L'\0' ){
			switch ( *(p+1) ) {
			case L'a' : out.push_back(L'\a'); break;
			case L'b' : out.push_back(L'\b'); break;
			case L'f' : out.push_back(L'\f'); break;
			case L'n' : out.push_back(L'\n'); break;
			case L'r' : out.push_back(L'\r'); break;
			case L't' : out.push_back(L'\t'); break;
			case L'v' : out.push_back(L'\v'); break;
			case L'\'': out.push_back(L'\''); break;
			case L'"' : out.push_back(L'\"'); break;
			case L'\\': out.push_back(L'\\'); break;
			case L'?' : out.push_back(L'\?'); break;
			}
			// The illegal escape character is just skipped.
			p++;
		}
		else {
			// the last character is '\\', skip it
		}
		p++;
	}
}

#endif _STRUTIL_H_
