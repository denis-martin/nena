/*
 * bb_lua.cpp
 *
 *  Created on: 2 Aug 2011
 *      Author: denis
 */

#include "bb_lua.h"

#include "nena.h"

#include <string>

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

const std::string BB_LUA_NAME = "bb://edu.kit.tm/itm/lua";

using namespace std;


int bblua_GetSysTime(lua_State *L)
{
	int n = lua_gettop(L); // number of arguments
	if (n > 0) {
		lua_pushstring(L, "GetSysTime(): Too many arguments.");
		lua_error(L);
		lua_pushnil(L);
		return 1; // numer of results

	} else {
		Bb_Lua* self = (Bb_Lua*) lua_tointeger(L, lua_upvalueindex(1));
		lua_Number time = (lua_Number) self->l_GetSysTime(L);
		lua_pushnumber(L, time);
		return 1;

	}
}

int bblua_SetRawTimer(lua_State *L)
{
	int n = lua_gettop(L); // number of arguments
	if (n > 1) {
		lua_pushstring(L, "SetRawTimer(): Too many arguments.");
		lua_error(L);
		lua_pushnil(L);
		return 1; // numer of results

	} else if (n < 1) {
		lua_pushstring(L, "SetRawTimer(): Too few arguments.");
		lua_error(L);
		lua_pushnil(L);
		return 1; // numer of results

	} else {
		Bb_Lua* self = (Bb_Lua*) lua_tointeger(L, lua_upvalueindex(1));
		double timeout = (lua_Number) lua_tonumber(L, 1); // parameter 1
		self->l_SetRawTimer(L, timeout);
		return 0;

	}
}

int bblua_SetTimer(lua_State *L)
{
	int n = lua_gettop(L); // number of arguments
	if (n > 2) {
		lua_pushstring(L, "SetTimer(): Too many arguments.");
		lua_error(L);
		lua_pushnil(L);
		return 1; // numer of results

	} else if (n < 2) {
		lua_pushstring(L, "SetTimer(): Too few arguments.");
		lua_error(L);
		lua_pushnil(L);
		return 1; // numer of results

	} else {
		Bb_Lua* self = (Bb_Lua*) lua_tointeger(L, lua_upvalueindex(1));
		double timeout = (lua_Number) lua_tonumber(L, 1); // parameter 1
		int regIndex = luaL_ref(L, LUA_REGISTRYINDEX);
		self->l_SetTimer(L, timeout, regIndex);
		return 0;

	}
}

class BbLua_Timer : public CTimer
{
public:
	int callbackRegIndex;

	BbLua_Timer(double timeout, int callbackRegIndex, IMessageProcessor* mp)
		: CTimer(timeout, mp), callbackRegIndex(callbackRegIndex)
	{};

	virtual ~BbLua_Timer() {};
}


/**
 * @brief 	Constructor
 *
 * @param nodeArch	Pointer to Node Architecture class
 * @param sched		Associated scheduler
 * @param netlet	Associated Netlet
 */
Bb_Lua::Bb_Lua(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet) :
	IBuildingBlock(nodeArch, sched, netlet)
{
	className += "::BBLua";

	int status, result, i;
	double sum;

	L = luaL_newstate();
	luaL_openlibs(L); /* Load Lua libraries */

	/* Load the file containing the script we are going to run */
	status = luaL_loadfile(L, "script.lua");
	if (status) {
		/* If something went wrong, error message is at the top of */
		/* the stack */
		DBG_DEBUG(FMT("Couldn't load Lua file: %1%") % lua_tostring(L, -1));
		lua_close(L);
		L = NULL;

	} else {
		/*
		 * Ok, now here we go: We pass data to the lua script on the stack.
		 * That is, we first have to prepare Lua's virtual stack the way we
		 * want the script to receive it, then ask Lua to run it.
		 */
		lua_newtable(L);    /* We will pass a table */

		/*
		 * To put values into the table, we first push the index, then the
		 * value, and then call lua_rawset() with the index of the table in the
		 * stack. Let's see why it's -3: In Lua, the value -1 always refers to
		 * the top of the stack. When you create the table with lua_newtable(),
		 * the table gets pushed into the top of the stack. When you push the
		 * index and then the cell value, the stack looks like:
		 *
		 * <- [stack bottom] -- table, index, value [top]
		 *
		 * So the -1 will refer to the cell value, thus -3 is used to refer to
		 * the table itself. Note that lua_rawset() pops the two last elements
		 * of the stack, so that after it has been called, the table is at the
		 * top of the stack.
		 */
		for (i = 1; i <= 5; i++) {
			lua_pushnumber(L, i);   /* Push the table index */
			lua_pushnumber(L, i*2); /* Push the cell value */
			lua_rawset(L, -3);      /* Stores the pair in the table */
		}

		/* By what name is the script going to reference our table? */
		lua_setglobal(L, "foo");

		registerLuaFunction("GetSysTime", &bblua_GetSysTime);
		registerLuaFunction("SetRawTimer", &bblua_SetRawTimer);

		/* Ask Lua to run our little script */
		result = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (result) {
			DBG_DEBUG(FMT("Failed to run script: %1%") % lua_tostring(L, -1));

		} else {
			/* Get the returned value at the top of the stack (index -1) */
			sum = lua_tonumber(L, -1);

			printf("Script returned: %.0f\n", sum);

			lua_pop(L, 1);  /* Take the returned value out of the stack */

		}

	}

};

/**
 * @brief	Destructor
 */
Bb_Lua::~Bb_Lua()
{
	if (L) {
		lua_close(L);
		L = NULL;

	}
};

// from IMessageProcessor
/**
* @brief Process an event message directed to this message processing unit
*
* @param msg	Pointer to message
*/
void Bb_Lua::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_DEBUG(FMT("Bb_Lua::processEvent(): NYI"));
}

/**
* @brief Process a timer message directed to this message processing unit
*
* @param msg	Pointer to message
*/
void Bb_Lua::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_DEBUG(FMT("Bb_Lua::processTimer(): NYI"));

	lua_getglobal(L, "OnProcessTimer"); // function to be called
	if (lua_isfunction(L, -1)) {
		int err = lua_pcall(L, 0, 0, 0); // call function with 0 arguments and 0 result
		if (err) {
			DBG_DEBUG(FMT("Failed to call OnProcessTimer(): %1%") % lua_tostring(L, -1));
		}

	} else {
		DBG_DEBUG(FMT("Global OnProcessTimer not a function"));
		lua_pop(L, 1);

	}
}

/**
* @brief Process an outgoing message directed towards the network.
*
* @param msg	Pointer to message
*/
void Bb_Lua::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_DEBUG(FMT("Bb_Lua::processOutgoing(): NYI"));
}

/**
* @brief Process an incoming message directed towards the application.
*
* @param msg	Pointer to message
*/
void Bb_Lua::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_DEBUG(FMT("Bb_Lua::processIncoming(): NYI"));
}

/**
 * @brief 	Return ID of the building block
 */
const std::string & Bb_Lua::getId() const
{
	return BB_LUA_NAME;
}

/**
 * @brief	Registers function as a closure where the first parameter is
 * 			this.
 */
void Bb_Lua::registerLuaFunction(const std::string& name, lua_CFunction func)
{
	lua_pushnumber(L, (lua_Integer) this);
	lua_pushcclosure(L, func, 1);
	lua_setglobal(L, name.c_str());
}

/**
 * @brief	Lua function: GetSysTime()
 */
double Bb_Lua::l_GetSysTime(lua_State* S) const
{
	return nodeArch->getSysTime();
}

/**
 * @brief	Lua function: SetRawTimer()
 */
void Bb_Lua::l_SetRawTimer(lua_State* S, double timeout)
{
	scheduler->setTimer(new CTimer(timeout, this));
}

/**
 * @brief	Lua function: l_SetTimer()
 */
void Bb_Lua::l_SetTimer(lua_State* S, double timeout, int regIndex)
{
	scheduler->setTimer(new BbLua_Timer(timeout, regIndex, this));

	//retrive function and call it
	lua_rawgeti(L, LUA_REGISTRYINDEX, regIndex);
	//push the parameters and call it
	lua_pushnumber(L, 5); // push first argument to the function
	lua_pcall(L, 1, 0, 0); // call a function with one argument and no return values
}


