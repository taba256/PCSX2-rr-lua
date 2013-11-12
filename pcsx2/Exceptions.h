/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef _PCSX2_EXCEPTIONS_H_
#define _PCSX2_EXCEPTIONS_H_

#include <stdexcept>
#include "StringUtils.h"

// This class provides an easy and clean method for ensuring objects are not copyable.
class NoncopyableObject
{
protected:
	NoncopyableObject() {}
	~NoncopyableObject() {}

// Programmer's note:
//   No need to provide implementations for these methods since they should
//   never be referenced anyway.  No references?  No Linker Errors!  Noncopyable!
private:
	// Copy me?  I think not!
	explicit NoncopyableObject( const NoncopyableObject& );
	// Assign me?  I think not!
	const NoncopyableObject& operator=( const NoncopyableObject& );
};


// Base class used to implement type-safe sealed classes.
// This class should never be used directly.  Use the Sealed
// macro instead, which ensures all sealed classes derive from a unique BaseSealed
// (preventing them from accidentally cirumventing sealing by inheriting from
// multiple sealed classes.
template < int T >
class __BaseSealed
{
protected:
	__BaseSealed()
	{
	}
};

// Use this macro/class as a base to seal a class from being derrived from.
// This macro works by providing a unique base class with a protected constructor
// for every class that derives from it. 
#define Sealed private virtual __BaseSealed<__COUNTER__>

namespace Exception
{
	// std::exception sucks, so I made a replacement.
	// Note, this class is "abstract" which means you shouldn't use it directly like, ever.
	// Use Exception::RuntimeError or Exception::LogicError instead.
	class BaseException
	{
	protected:
		const std::string m_message;		// a "detailed" message of what disasterous thing has occured!

	public:
		virtual ~BaseException() throw()=0;	// the =0; syntax forces this class into "abstract" mode.
		explicit BaseException( const std::string& msg="Unhandled exception." ) :
			m_message( msg )
		{}

		const std::string& Message() const { return m_message; }
		const char* cMessage() const { return m_message.c_str(); }
	};

	class RuntimeError : public BaseException
	{
	public:
		virtual ~RuntimeError() throw() {}
		explicit RuntimeError( const std::string& msg="An unhandled runtime error has occurred, somewhere in the depths of Pcsx2's cluttered brain-matter." ) :
			BaseException( msg )
		{}
	};

	class LogicError : public BaseException
	{
	public:
		virtual ~LogicError() throw() {}
		explicit LogicError( const std::string& msg="An unhandled logic error has occured." ) :
			BaseException( msg )
		{}
	};
	
	class OutOfMemory : public RuntimeError
	{
	public:
		explicit OutOfMemory( const std::string& msg="Out of memory!" ) :
			RuntimeError( msg ) {}
	};

	// This exception thrown any time an operation is attempted when an object
	// is in an uninitialized state.
	class InvalidOperation : public LogicError
	{
	public:
		virtual ~InvalidOperation() throw() {}
		explicit InvalidOperation( const std::string& msg="Attempted method call is invalid for the current object or program state." ) :
			LogicError( msg ) {}
	};

	// This exception thrown any time an operation is attempted when an object
	// is in an uninitialized state.
	class InvalidArgument : public LogicError
	{
	public:
		virtual ~InvalidArgument() throw() {}
		explicit InvalidArgument( const std::string& msg="Invalid argument passed to a function." ) :
			LogicError( msg ) {}
	};

	// Keep those array indexers in bounds when using the SafeArray type, or you'll be
	// seeing these.
	class IndexBoundsFault : public LogicError
	{
	public:
		virtual ~IndexBoundsFault() throw() {}
		explicit IndexBoundsFault( const std::string& msg="Array index is outsides the bounds of an array." ) :
			LogicError( msg ) {}
	};

	class HardwareDeficiency : public RuntimeError
	{
	public:
		explicit HardwareDeficiency( const std::string& msg="Your machine's hardware is incapable of running Pcsx2.  Sorry dood." ) :
			RuntimeError( msg ) {}
	};

	// This exception is thrown by the PS2 emulation (R5900, etc) when bad things happen
	// that force the emulation state to terminate.  The GUI should handle them by returning
	// the user to the GUI.
	class CpuStateShutdown : public RuntimeError
	{
	public:
		virtual ~CpuStateShutdown() throw() {}
		explicit CpuStateShutdown( const std::string& msg="The PS2 emulated state was shut down unexpectedly." ) :
			RuntimeError( msg ) {}
	};

	class PluginFailure : public RuntimeError
	{
	public:
		std::string plugin_name;		// name of the plugin

		virtual ~PluginFailure() throw() {}
		explicit PluginFailure( const std::string& plugin, const std::string& msg = "A plugin encountered a critical error." ) :
			RuntimeError( msg )
		,	plugin_name( plugin ) {}
	};

	class ThreadCreationError : public RuntimeError
	{
	public:
		virtual ~ThreadCreationError() throw() {}
		explicit ThreadCreationError( const std::string& msg="Thread could not be created." ) :
			RuntimeError( msg ) {}
	};

	// This is a "special" exception that's primarily included for safe functioning in the 
	// Win32's ASCII API (ie, the non-Unicode one).  Many of the old Win32 APIs don't support
	// paths over 256 characters.
	class PathTooLong : public RuntimeError
	{
	public:
		virtual ~PathTooLong() throw() {}
		explicit PathTooLong( const std::string& msg=
			"A Pcsx2 pathname was too long for the system.  Please move or reinstall Pcsx2 to\n"
			"a location on your hard drive that has a shorter path." ) :
			RuntimeError( msg ) {}
	};

	///////////////////////////////////////////////////////////////////////
	//                        STREAMING EXCEPTIONS

	// Generic stream error.  Contains the name of the stream and a message.
	// This exception is usually thrown via derrived classes, except in the (rare) case of a generic / unknown error.
	class Stream : public RuntimeError
	{
	public:
		std::string stream_name;		// name of the stream (if applicable)

		virtual ~Stream() throw() {}

		// copy construct!
		Stream( const Stream& src ) :
			RuntimeError( src.Message() )
		,	stream_name( src.stream_name ) {}

		explicit Stream(
			const std::string& objname=std::string(),
			const std::string& msg="Invalid stream object" ) :
		  RuntimeError( msg + "\n\tFilename: " + objname )
		, stream_name( objname ) {}
	};

	// A generic base error class for bad streams -- corrupted data, sudden closures, loss of
	// connection, or anything else that would indicate a failure to read the data after the
	// stream was successfully opened.
	class BadStream : public Stream
	{
	public:
		virtual ~BadStream() throw() {}
		explicit BadStream(
			const std::string& objname=std::string(),
			const std::string& msg="Stream data is corrupted or incomplete, or the stream connection closed unexpectedly" ) :
		Stream( objname, msg ) {}
	};

	// A generic exception for odd-ball stream creation errors.
	class CreateStream : public Stream
	{
	public:
		virtual ~CreateStream() throw() {}
		explicit CreateStream(
			const std::string& objname=std::string(),
			const std::string& msg="Stream could not be created or opened" ) :
		Stream( objname, msg ) {}	
	};

	// Exception thrown when an attempt to open a non-existent file is made.
	// (this exception can also mean file permissions are invalid)
	class FileNotFound : public CreateStream
	{
	public:
		virtual ~FileNotFound() throw() {}
		explicit FileNotFound(
			const std::string& objname=std::string(),
			const std::string& msg="File not found" ) :
		CreateStream( objname, msg ) {}
	};

	class AccessDenied : public CreateStream
	{
	public:
		virtual ~AccessDenied() throw() {}
		explicit AccessDenied(
			const std::string& objname=std::string(),
			const std::string& msg="Permission denied to file or stream" ) :
		CreateStream( objname, msg ) {}
	};

	// Generic End of Stream exception (sometimes an error, and sometimes just used as a
	// shortcut for manual feof checks).
	class EndOfStream : public Stream
	{
	public:
		virtual ~EndOfStream() throw() {}
		explicit EndOfStream( const std::string& objname=std::string(), const std::string& msg="End of stream was encountered" ) :
			Stream( objname, msg ) {}
	};

	//////////////////////////////////////////////////////////////////////////
	//                       SAVESTATE EXCEPTIONS

	// Exception thrown when a corrupted or truncated savestate is encountered.
	class BadSavedState : public BadStream
	{
	public:
		virtual ~BadSavedState() throw() {}
		explicit BadSavedState(
			const std::string& objname=std::string(),
			const std::string& msg="Savestate data is corrupted or incomplete" ) :
		BadStream( objname, msg ) {}
	};

	// Exception thrown by SaveState class when a critical plugin or gzread
	class FreezePluginFailure : public RuntimeError
	{
	public:
		std::string plugin_name;		// name of the plugin
		std::string freeze_action;

		virtual ~FreezePluginFailure() throw() {}
		explicit FreezePluginFailure( const std::string& plugin, const std::string& action ) :
			RuntimeError( plugin + " plugin returned an error while " + action + " the state." )
		,	plugin_name( plugin )
		,	freeze_action( action ){}
	};

	// The savestate code throws Recoverable errors when it fails prior to actually modifying
	// the current emulation state.  Recoverable errors are always thrown from the SaveState
	// object construction (and never from Freeze methods).
	class StateLoadError_Recoverable : public RuntimeError
	{
	public:
		virtual ~StateLoadError_Recoverable() throw() {}
		explicit StateLoadError_Recoverable( const std::string& msg="Recoverable error while loading savestate (existing emulation state is still intact)." ) :
			RuntimeError( msg ) {}
	};

	// A recoverable exception thrown when the savestate being loaded isn't supported.
	class UnsupportedStateVersion : public StateLoadError_Recoverable
	{
	public:
		u32 Version;		// version number of the unsupported state.

	public:
		virtual ~UnsupportedStateVersion() throw() {}
		explicit UnsupportedStateVersion( int version ) :
			StateLoadError_Recoverable( fmt_string( "Unknown or unsupported savestate version: 0x%x", version ) )
		{}

		explicit UnsupportedStateVersion( int version, const std::string& msg ) :
			StateLoadError_Recoverable( msg ) {}
	};

	// A recoverable exception thrown when the CRC of the savestate does not match the
	// CRC returned by the Cdvd driver.
	class StateCrcMismatch : public StateLoadError_Recoverable
	{
	public:
		u32 Crc_Savestate;
		u32 Crc_Cdvd;

	public:
		virtual ~StateCrcMismatch() throw() {}
		explicit StateCrcMismatch( u32 crc_save, u32 crc_cdvd )
		:	StateLoadError_Recoverable( fmt_string(
				"Game/CDVD does not match the savestate CRC.\n"
				"\tCdvd CRC: 0x%X\n\tGame CRC: 0x%X\n", params crc_save, crc_cdvd
			) )
		,	Crc_Savestate( crc_save )
		,	Crc_Cdvd( crc_cdvd )
		{}

		explicit StateCrcMismatch( u32 crc_save, u32 crc_cdvd, const std::string& msg )
		:	StateLoadError_Recoverable( msg )
		,	Crc_Savestate( crc_save )
		,	Crc_Cdvd( crc_cdvd )
		{}
	};
}

#endif
