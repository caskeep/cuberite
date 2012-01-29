
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#define LUA_USE_POSIX
#include "cPlugin_NewLua.h"
#include "cMCLogger.h"
#include "cWebPlugin_Lua.h"

extern "C"
{
#include "lualib.h"
}

#include "tolua++.h"
#include "Bindings.h"
#include "ManualBindings.h"

#ifdef _WIN32
#include "wdirent.h"
#else
#include <dirent.h>
#endif

extern bool report_errors(lua_State* lua, int status);

cPlugin_NewLua::cPlugin_NewLua( const char* a_PluginName )
	: m_LuaState( 0 )
{
	m_Directory = a_PluginName;
}

cPlugin_NewLua::~cPlugin_NewLua()
{
	for( WebPluginList::iterator itr = m_WebPlugins.begin(); itr != m_WebPlugins.end(); ++itr )
	{
		delete *itr;
	}
	m_WebPlugins.clear();

	if( m_LuaState )
	{
		lua_close( m_LuaState );
		m_LuaState = 0;
	}
}

bool cPlugin_NewLua::Initialize()
{
	if( !m_LuaState ) 
	{	
		m_LuaState = lua_open();
		luaL_openlibs( m_LuaState );
		tolua_AllToLua_open(m_LuaState);
		ManualBindings::Bind( m_LuaState );
	}

	std::string PluginPath = std::string("Plugins/") + m_Directory + "/";

	// Load all files for this plugin, and execute them
	DIR* dp;
	struct dirent *entry;
	if(dp = opendir( PluginPath.c_str() ))
	{
		while(entry = readdir(dp))
		{
			std::string FileName = entry->d_name;
			if( FileName.find(".lua") != std::string::npos )
			{
				std::string Path = PluginPath + FileName;
				int s = luaL_loadfile(m_LuaState, Path.c_str() );
				if( report_errors( m_LuaState, s ) )
				{
					LOGERROR("Can't load plugin %s because of an error in file %s", m_Directory.c_str(), Path.c_str() );
					lua_close( m_LuaState );
					m_LuaState = 0;
					return false;
				}

				s = lua_pcall(m_LuaState, 0, LUA_MULTRET, 0);
				if( report_errors( m_LuaState, s ) )
				{
					LOGERROR("Error in plugin %s in file %s", m_Directory.c_str(), Path.c_str() );
					lua_close( m_LuaState );
					m_LuaState = 0;
					return false;
				}
			}
		}
		closedir( dp );
	}


	// Call intialize function
	if( !PushFunction("Initialize") )
	{
		lua_close( m_LuaState );
		m_LuaState = 0;
		return false;
	}

	tolua_pushusertype(m_LuaState, this, "cPlugin_NewLua");

	
	if( !CallFunction(1, 1, "Initialize") ) 
	{
		lua_close( m_LuaState );
		m_LuaState = 0;
		return false;
	}

	if( !lua_isboolean( m_LuaState, -1 ) )
	{
		LOGWARN("Error in plugin %s Initialize() must return a boolean value!", m_Directory.c_str() );
		lua_close( m_LuaState );
		m_LuaState = 0;
		return false;
	}

	bool bSuccess = (tolua_toboolean( m_LuaState, -1, 0) > 0);
	return bSuccess;
}

void cPlugin_NewLua::Tick(float a_Dt)
{
	if( !PushFunction("Tick") )
		return;

	tolua_pushnumber( m_LuaState, a_Dt );

	CallFunction(1, 0, "Tick");
}

bool cPlugin_NewLua::OnPlayerJoin( cPlayer* a_Player )
{
	if( !PushFunction("OnPlayerJoin") )
		return false;

	tolua_pushusertype(m_LuaState, a_Player, "cPlayer");

	if( !CallFunction(1, 1, "OnPlayerJoin") )
		return false;

	bool bRetVal = (tolua_toboolean( m_LuaState, -1, 0) > 0);
	return bRetVal;
}

bool cPlugin_NewLua::OnLogin( cPacket_Login* a_PacketData )
{
	if( !PushFunction("OnLogin") )
		return false;

	tolua_pushusertype(m_LuaState, a_PacketData, "cPacket_Login");

	if( !CallFunction(1, 1, "OnLogin") )
		return false;

	bool bRetVal = (tolua_toboolean( m_LuaState, -1, 0) > 0);
	return bRetVal;
}

bool cPlugin_NewLua::OnBlockPlace( cPacket_BlockPlace* a_PacketData, cPlayer* a_Player )
{
	if( !PushFunction("OnBlockPlace") )
		return false;

	tolua_pushusertype(m_LuaState, a_PacketData, "cPacket_BlockPlace");
	tolua_pushusertype(m_LuaState, a_Player, "cPlayer");

	if( !CallFunction(2, 1, "OnBlockPlace") )
		return false;

	bool bRetVal = (tolua_toboolean( m_LuaState, -1, 0) > 0);
	return bRetVal;
}

bool cPlugin_NewLua::OnKilled( cPawn* a_Killed, cEntity* a_Killer )
{
	if( !PushFunction("OnKilled") )
		return false;

	tolua_pushusertype(m_LuaState, a_Killed, "cPawn");
	tolua_pushusertype(m_LuaState, a_Killer, "cEntity");

	if( !CallFunction(2, 1, "OnKilled") )
		return false;

	bool bRetVal = (tolua_toboolean( m_LuaState, -1, 0) > 0);
	return bRetVal;
}

cWebPlugin_Lua* cPlugin_NewLua::CreateWebPlugin(lua_State* a_LuaState)
{
	if( a_LuaState != m_LuaState )
	{
		LOGERROR("Not allowed to create a WebPlugin from another plugin but your own!");
		return 0;
	}
	cWebPlugin_Lua* WebPlugin = new cWebPlugin_Lua( this );

	m_WebPlugins.push_back( WebPlugin );

	return WebPlugin;
}


// Helper functions
bool cPlugin_NewLua::PushFunction( const char* a_FunctionName )
{
	lua_getglobal(m_LuaState, a_FunctionName);
	if(!lua_isfunction(m_LuaState,-1))
	{
		LOGWARN("Error in plugin %s: Could not find function %s()", m_Directory.c_str(), a_FunctionName );
		lua_pop(m_LuaState,1);
		return false;
	}
	return true;
}

bool cPlugin_NewLua::CallFunction( int a_NumArgs, int a_NumResults, const char* a_FunctionName )
{
	int s = lua_pcall(m_LuaState, a_NumArgs, a_NumResults, 0);
	if( report_errors( m_LuaState, s ) )
	{
		LOGWARN("Error in plugin %s calling function %s()", m_Directory.c_str(), a_FunctionName );
		return false;
	}
	return true;
}