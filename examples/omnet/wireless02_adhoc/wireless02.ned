package nodearch.examples.wireless02_adhoc;

import nodearch.examples.CSystemOmnet;

// For testing AODV/OLSR
// by Gorka Hernando Garcia
network Net1_AdhocRouting
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

