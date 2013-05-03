
// Module type running the node architecture
simple CSystemOmnet
{
	parameters:
		string nodeName = default("");
	gates:
		inout eth[];
}

// instances
network TestNet1_SimplePingPong
{
	submodules:
		ping: CSystemOmnet;
		router: CSystemOmnet;
		pong: CSystemOmnet;
	connections allowunconnected:
		ping.eth++   <--> {  delay = 100ms; } <--> router.eth++;
		router.eth++ <--> {  delay = 100ms; } <--> pong.eth++;
		
}

// For testing AODV/OLSR
// by Gorka Hernando Garcia
network TestNet2_AdhocRouting
{
	submodules:
		node0: CSystemOmnet;
		node1: CSystemOmnet;
		node2: CSystemOmnet;
		node3: CSystemOmnet;
		node4: CSystemOmnet;
		node5: CSystemOmnet;
		node6: CSystemOmnet;
		node7: CSystemOmnet;
		node8: CSystemOmnet;


	connections allowunconnected:
		node0.eth++ <--> {  delay = 100ms; } <--> node1.eth++;
		node0.eth++ <--> {  delay = 100ms; } <--> node2.eth++;
		node1.eth++ <--> {  delay = 100ms; } <--> node3.eth++;
		node1.eth++ <--> {  delay = 100ms; } <--> node4.eth++;
		node2.eth++ <--> {  delay = 100ms; } <--> node5.eth++;
		node2.eth++ <--> {  delay = 100ms; } <--> node6.eth++;
		node4.eth++ <--> {  delay = 100ms; } <--> node7.eth++;
		node7.eth++ <--> {  delay = 100ms; } <--> node8.eth++;
}

