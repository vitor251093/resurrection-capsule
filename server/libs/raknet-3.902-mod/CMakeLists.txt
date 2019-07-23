cmake_minimum_required(VERSION 2.6)

IF(WIN32 AND NOT UNIX)
	set(CMAKE_SUPPRESS_REGENERATION TRUE) #Gets rid of ZERO_CHECK 
	set(CMAKE_USE_RELATIVE_PATHS TRUE)#Sets paths to relative for the windows projects
	set(DISABLEDEPENDENCIES "FALSE" CACHE BOOL "Disable dependencies?")
	set(USEFMOD "TRUE" CACHE BOOL "Enable projects that use FMOD?")
	set(USEBOOST "TRUE" CACHE BOOL "Enable projects that use Boost?")
	set(USESCALEFORM "TRUE" CACHE BOOL "Enable projects that use ScaleForm?")
	set(USEMYSQL "TRUE" CACHE BOOL "Enable projects that use MySql?")
	set(USEPOSTGRESQL "TRUE" CACHE BOOL "Enable projects that use PostgreSQL?")
	set(USEDX3D9 "TRUE" CACHE BOOL "Enable projects that use Direct X 9?")
	set(USEDSOUND "TRUE" CACHE BOOL "Enable projects that use Direct Sound?")
	set(USEIRRLICHT "TRUE" CACHE BOOL "Enable projects that use Irrlicht?")
	set(BUILDLOBBY2CLIENT "FALSE" CACHE BOOL "Build the Lobby2Client project?")
	set(USEOGRE3D "FALSE" CACHE INTERNAL "HIDDEN" FORCE)#This is not working for windows
	set(USESPEEX "TRUE" CACHE INTERNAL "HIDDEN" FORCE)#This is included for windows build, don't need to disable
	set(USEPORTAUDIO "TRUE" CACHE INTERNAL "HIDDEN" FORCE)#This is included for windows build, don't need to disable
	set(USEIRRKLANG "TRUE"	CACHE INTERNAL "HIDDEN" FORCE)#This is included for windows build, don't need to disable
	set(WIN32NOTUNIX "TRUE" CACHE INTERNAL "HIDDEN" FORCE)
ELSE(WIN32 AND NOT UNIX)
	set(DISABLEDEPENDENCIES "FALSE" CACHE BOOL "Disable dependencies?")
	set(USEFMOD "FALSE" CACHE BOOL "Enable projects that use FMOD?")
	set(USEBOOST "TRUE" CACHE BOOL "Enable projects that use Boost?")
	set(USESCALEFORM "FALSE" CACHE BOOL "Enable projects that use ScaleForm?")
	set(USEMYSQL "TRUE" CACHE BOOL "Enable projects that use MySql?")
	set(USEPOSTGRESQL "TRUE" CACHE BOOL "Enable projects that use PostgreSQL?")
	set(USESPEEX "TRUE" CACHE BOOL "Enable projects that use Speex?")
	set(USEPORTAUDIO "TRUE" CACHE BOOL "Enable projects that use PortAudio?")
	set(USEIRRLICHT "TRUE" CACHE BOOL "Enable projects that use Irrlicht?")
	set(USEIRRKLANG "TRUE" CACHE BOOL "Enable projects that use irrKlang?")
	set(USEOGRE3D "TRUE" CACHE BOOL "Enable projects that use Ogre 3d?")
	set(USEOIS "TRUE" CACHE BOOL "Enable projects that use OIS?")
        set(BUILDLOBBY2CLIENT "FALSE" CACHE BOOL "Build the Lobby2Client project?")
	set(WIN32NOTUNIX "FALSE" CACHE INTERNAL "HIDDEN" FORCE)
ENDIF(WIN32 AND NOT UNIX)

project(RakNet)
set (PS3ENABLED FALSE)
if( NOT APPLE )
# check 64 bit
if( CMAKE_SIZEOF_VOID_P MATCHES "4" )
  set( HAVE_64_BIT 0 )
else(  CMAKE_SIZEOF_VOID_P MATCHES "4")
  set( HAVE_64_BIT 1 )
endif(  CMAKE_SIZEOF_VOID_P MATCHES "4")

# check endianness
include( TestBigEndian )
test_big_endian( HAVE_BIG_ENDIAN )
if( HAVE_BIG_ENDIAN )
  set( HAVE_BIG_ENDIAN 1 )
else( HAVE_BIG_ENDIAN )
  set( HAVE_BIG_ENDIAN 0 )
endif( HAVE_BIG_ENDIAN )
endif( NOT APPLE )

IF (WIN32 AND NOT UNIX)
	IF (HAVE_64_BIT)
		set (PROGRAMFILESX86 $ENV{PROGRAMFILESX86})
	ELSE(HAVE_64_BIT)
		set (PROGRAMFILESX86 $ENV{PROGRAMFILES})
	ENDIF(HAVE_64_BIT)
	string(REPLACE "\\" "/" PROGRAMFILESX86 ${PROGRAMFILESX86})
ENDIF(WIN32 AND NOT UNIX)

IF (WIN32 AND NOT UNIX)
	set(RAKNET_LIBRARY_LIBS ws2_32.lib)
ELSE(WIN32 AND NOT UNIX)
	set(RAKNET_LIBRARY_LIBS pthread)
ENDIF(WIN32 AND NOT UNIX)

include(./CmakeIncludes/CmakeMacros.txt)
FIXLINKOPTIONS()
FIXCOMPILEOPTIONS()

add_subdirectory(Source)
add_subdirectory(Lib)

IF (WIN32 AND NOT UNIX)
	set(RAKNET_COMMON_LIBS LibStatic)
ELSE(WIN32 AND NOT UNIX)
	set(RAKNET_COMMON_LIBS RakNetStatic)
ENDIF(WIN32 AND NOT UNIX)

set(RAKNETHEADERFILES ${RakNet_SOURCE_DIR}/Source)

add_subdirectory(DependentExtensions)
add_subdirectory(Samples)
