package nodearch.examples;

@license(LGPL);

//
// This simple module represents the Node Architecture Daemon
// 
simple CSystemOmnet
{
    parameters:
        string nodeName = default(""); // host name - if empty use module name instead 
	gates:
		inout eth[];
		input lowerControlIn; // unused
}
