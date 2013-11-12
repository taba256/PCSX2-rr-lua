

#include "PrecompiledHeader.h"
#include "Win32.h"

// Translates an Errno code into an exception.
void StreamException_ThrowFromErrno( const string& streamname, errno_t errcode )
{
	if( errcode == 0 ) return;

	switch( errcode )
	{
		case EINVAL:
			throw Exception::InvalidArgument( "Invalid argument." );

		case EACCES:	// Access denied!
			throw Exception::AccessDenied( streamname );

		case EMFILE:	// Too many open files!
			throw Exception::CreateStream( streamname, "File handle allocation failure - Too many open files" );

		case EEXIST:
			throw Exception::CreateStream( streamname, "Cannot create - File already exists" );

		case ENOENT:	// File not found!
			throw Exception::FileNotFound( streamname );

		case EPIPE:
			throw Exception::BadStream( streamname, "Broken pipe" );

		case EBADF:
			throw Exception::BadStream( streamname, "Bad file number" );

		default:
			throw Exception::Stream( streamname,
				fmt_string( "General file/stream error occured [errno: %d]", errcode )
			);
	}
}

void StreamException_ThrowLastError( const string& streamname, HANDLE result )
{
	if( result != INVALID_HANDLE_VALUE ) return;

	int error = GetLastError();
	
	switch( error )
	{
		case ERROR_FILE_NOT_FOUND:
			throw Exception::FileNotFound( streamname );
		
		case ERROR_PATH_NOT_FOUND:
			throw Exception::FileNotFound( streamname );
		
		case ERROR_TOO_MANY_OPEN_FILES:
			throw Exception::CreateStream( streamname, "File handle allocation failure - Too many open files " );
		
		case ERROR_ACCESS_DENIED:
			throw Exception::AccessDenied( streamname );

		default:
		{
			throw Exception::Stream( streamname,
				fmt_string( "General Win32 File/stream error [GetLastError: %d]", error )
			);
		}
	}
}

// returns TRUE if an error occurred.
bool StreamException_LogFromErrno( const string& streamname, const char* action, errno_t result )
{
	try
	{
		StreamException_ThrowFromErrno( streamname, result );
	}
	catch( Exception::Stream& ex )
	{
		Console::Notice( "%s > %s", params action, ex.cMessage() );
		return true;
	}
	return false;
}

// returns TRUE if an error occurred.
bool StreamException_LogLastError( const string& streamname, const char* action, HANDLE result )
{
	try
	{
		StreamException_ThrowLastError( streamname, result );
	}
	catch( Exception::Stream& ex )
	{
		Console::Notice( "%s > %s", params action, ex.cMessage() );
	}
	return false;
}

// Exceptions thrown: None.  Errors are logged to console.  Failures are considered non-critical
void NTFS_CompressFile( const char* file, bool compressStatus )
{
	bool isFile = Path::isFile( file );
	
	const DWORD flags = isFile ? FILE_ATTRIBUTE_NORMAL : (FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_DIRECTORY);

	HANDLE bloated_crap = CreateFile( file,
		FILE_GENERIC_WRITE | FILE_GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		flags,
		NULL
	);

	// Fail silently -- non-compression of files and folders is not an errorable offense.

	if( !StreamException_LogLastError( file, "NTFS Compression Enable", bloated_crap ) )
	{
		DWORD bytesReturned = 0;
		DWORD compressMode = compressStatus ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
		
		BOOL result = DeviceIoControl(
			bloated_crap, FSCTL_SET_COMPRESSION,
			&compressMode, 2, NULL, 0,
			&bytesReturned, NULL
		);
		
		if( !result )
			StreamException_LogLastError( file, "NTFS Compression Enable" );

		CloseHandle( bloated_crap );
	}
}
