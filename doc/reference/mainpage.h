/** @file
 *
 * @brief Doxygen main page content
 *
 * \mainpage
 *
 * Reference documentation of the NENA prototype. Basis for this
 * implementation are the concepts described here:
 * http://nena.intend-net.org/wiki/whatis
 *
 * \section Intro Introduction
 *
 * This is an automatically generated reference documentation of the NENA
 * prototype. On this page, an overview of the implementation design is given,
 * while the other pages provide descriptions of the current interfaces. Beware,
 * that these interfaces may be incomplete as they are about to evolve as
 * needed.
 *
 * The code is pure C++/STL with some use of the Boost libraries. The daemon as
 * well as the example implementations of Netlets etc. are designed to run in
 * multiple target environment, such as Linux and OMNeT++.
 *
 * \subsection SystemWrapper System Wrapper
 *
 * A system wrapper interface (ISystemWrapper) provides the necessary
 * abstractions to access services of the underlying system (such as timers,
 * network send/receive, ...). The current implementation focuses on the wrapper
 * for OMNeT++ (CSystemOmnet) and real systems like Linux, BSD, or others where
 * the Boost ASIO library is available (CSystemBoost). In total, a wrapper
 * consists of implementations of three classes:
 *
 * ISystemWrapper - The ISystemWrapper provides general information and access
 * methods to the target system. It should generally be instantiated by the main
 * initialization routing of the target system.
 *
 * IMessageScheduler - At least one scheduler must be created for the target
 * system. This is an abstraction for a thread and at least one must be
 * returned by ISystemWrapper::getMainScheduler().
 *
 * INetAdapt - At least one Network Access / Network Adaptor must be
 * implemented. This provides access to a medium where the outgoing messages are
 * sent to (and incoming are expected from).
 *
 * \subsection Daemon The NENA Daemon
 *
 * The NENA daemon is the core system, which interfaces with the
 * system wrapper and which is able to load and instantiate architecture
 * specific multiplexers (INetletMultiplexer) and Netlets (INetlet). Both are
 * compiled as shared libraries, which in turn are opened by the daemon (or the
 * repository, respectively).
 *
 * When opening the libraries, factory classes (IMultiplexerMetaData,
 * INetletMetaData) are automatically instantiated which register themselves in
 * a factory list (multiplexerFactory and netletFactory). From that point on,
 * the daemon is able to access the meta data and to create instances of the
 * registered Netlets / Multiplexer.
 *
 * \subsection ProcSched Message processors and schedulers
 *
 * Each separate entity doing some message processing is a message processor
 * (IMessageProcessor).
 * To each processor, a message scheduler (IMessageScheduler) is associated.
 * It schedules / enqueues the messages between processors. Schedulers may run
 * in separate threads, but all processors belonging to the same scheduler will
 * run in the same thread. The scheduler will take care about inter-thread
 * communication as necessary. In OMNeT++, schedulers are represented by
 * different modules.
 *
 * \subsection SourceOverview Source and message flow overview
 *
 * Here is an overview of the involved source files and components for a
 * communication request started by an application.
 *
 * \htmlinclude sourceOverview.html
 */

/* outdated...
 *
 * \section Tutorials Tutorials
 *
 * This section provides brief tutorials or code-walkthroughs.
 *
 * \subsection Tut1 Getting started
 *
 * In this section, we will give a little overview on how things are working
 * together in NENA. Here, we will only consider the OMNeT implementation, so
 * everything above the Netlet Selector and below the Multiplexer / Network
 * Access Broker will differ a lot for real implementations.
 *
 * When running the omnet_na binary, a small demo network with three nodes
 * should be shown (rev78, 2009-07-28). Node n[0] sends periodic ping-requests
 * to node n[2], which is supposed to answer it. Some additional messages are
 * sent between all nodes to exchange routing information. After the routing
 * information is up-to-date at all nodes, the ping will finally succeed.
 *
 * When looking at the code, ping is realized as an application in the class
 * CPingPong (src/targets/omnetpp/apps). Although this class has no OMNeT
 * specific includes, it resides in the OMNeT target directory. The reason
 * for this is, that applications running in a simulator don't have any user
 * interaction and are specifically written for traffic generation.
 *
 * CPingPong is a descendant of IAppConnector. This interface class has
 * a proxy role when communicating with the application. Since the application
 * and NENA are in the same executable for simulators, CPingPong may just
 * inherit it.
 *
 * IAppConnector itself inherits from IMessageProcessor which is a base class
 * for all message processing entities within NENA. INetlet,
 * INetletMultiplexer etc will all inherit from this.
 *
 * When CPingPong is created, it sets a timeout upon which it will send its
 * first ping-request. This message will be sent to the CNetletSelector which,
 * for the time being, is hard-wired to CSimpleNetlet. CSimpleNetlet "provides"
 * some transport functionalities, which essentially means that it adds a
 * header to identify the application that sent the message. After that, it just
 * hands it over to the CSimpleMultiplexer.
 *
 * CSimpleMultiplexer adds addressing information to the packet and realises
 * basic forwarding mechanisms. Note, that this is a decision of the "Simple
 * Architecture". Forwarding may also be realized within Netlets, if this is
 * desired. For forwarding, the CSimpleMultiplexer has a very simple forwarding
 * information base (FIB), which is maintained by the CSimpleRoutingNetlet.
 *
 * The CSimpleRoutingNetlet is a so-called control Netlet, which means that
 * there is no application associated with it and that it runs for its own
 * in the Node Architecture. It sends some route information exchange (RIX)
 * messages via the known network adaptors / accesses and collects any
 * incoming RIX messages. Based on these messages, the FIB of the
 * CSimpleMultiplexer is updated. Note, that this is a very dumb mechanism.
 * It will suffer from every problem encountered in routing and it does no
 * selection of which path would be best.
 *
 * The CSimpleMultiplexer selects the INetAdapt via which the outgoing
 * messages are sent. Either it uses its FIB, or the emitting entity has
 * already chosen the INetAdapt to use (as, e.g., the CSimpleRoutingNetlet
 * does).
 *
 * The INetAdapt interface is implemented in COmnetNetAdapt, which basically
 * maps the network adaptor to an OMNeT port.
 *
 * \subsection Tut2 Creating an own Control Netlet
 *
 * To create an own Control Netlet, the best thing is to start with
 * CSimpleRoutingNetlet and to construct it in top of the CSimpleMultiplexer.
 * So, just copy the two files of CSimpleRoutingNetlet and name them
 * appropriately (e.g. simpleControlNetlet.(h|cpp)). Edit the SConscript file
 * and add an entry for the new Netlet. Go into the two class files and rename
 * everything that is related to the "Simple Routing" to your new name. Be sure,
 * to also rename its ID (i.e., the string in SIMPLE_ROUTING_NETLET_NAME).
 *
 * After you did all those changes and renaming, it should compile and create
 * a new shared library in build/targets/netlets. When running the OMNeT
 * simulation, it should also be instantiated automatically.
 *
 */


















