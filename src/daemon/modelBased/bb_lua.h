/*
 * bb_lua.h
 *
 *  Created on: 2 Aug 2011
 *      Author: denis
 */

#ifndef BB_LUA_H_
#define BB_LUA_H_

#include "composableNetlet.h"

extern "C" {
#include "lua.h"
}

class Bb_Lua : public IBuildingBlock
{
protected:
	lua_State *L;

	/**
	 * @brief	Registers function as a closure where the first parameter is
	 * 			this.
	 */
	virtual void registerLuaFunction(const std::string& name, lua_CFunction func);

public:
	/**
	 * @brief 	Constructor
	 *
	 * @param nodeArch	Pointer to Node Architecture class
	 * @param sched		Associated scheduler
	 * @param netlet	Associated Netlet
	 */
	Bb_Lua(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet);

	/**
	 * @brief	Destructor
	 */
	virtual ~Bb_Lua();

	// from IMessageProcessor
	/**
	* @brief Process an event message directed to this message processing unit
	*
	* @param msg	Pointer to message
	*/
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	* @brief Process a timer message directed to this message processing unit
	*
	* @param msg	Pointer to message
	*/
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	* @brief Process an outgoing message directed towards the network.
	*
	* @param msg	Pointer to message
	*/
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	* @brief Process an incoming message directed towards the application.
	*
	* @param msg	Pointer to message
	*/
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief 	Return ID of the building block
	 */
	virtual const std::string & getId() const;

	/**
	 * @brief	Lua function: GetSysTime()
	 */
	virtual double l_GetSysTime(lua_State* S) const;

	/**
	 * @brief	Lua function: SetRawTimer()
	 */
	virtual void l_SetRawTimer(lua_State* S, double timeout);

	/**
	 * @brief	Lua function: l_SetTimer()
	 */
	virtual void l_SetTimer(lua_State* S, double timeout, int regIndex);
};

#endif /* BB_LUA_H_ */
