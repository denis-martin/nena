/** @file 
 *
 * Example usage of XmlNode.
 *
 * The code may be used freely in any way. It is distributed WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * @author Denis Martin
 */

#include "xmlNode.h"

#include <stdio.h>

#include <fstream>

using namespace std;
using namespace sxml;

/**
 * Example usage of XmlNode
 */
int main(int argc, char* argv)
{
	ifstream fin("test.xml");
	
	printf("\n");
	
	if (!fin.good()) {
		printf("Cannot open test.xml!\n");
		exit(1);
		
	}
	
	XmlNode* xmlNode = new XmlNode();
	
	try {
		xmlNode->readFromStream(fin);
		fin.close();
		
	} catch (sxml::Exception e) {
		printf("Error parsing XML file: exception %d\n", (int) e);
		fin.close();
		exit(1);
		
	}
	
	XmlNode* treasures = xmlNode->findFirst("treasures");
	if (treasures != NULL) {
		NodeSearch* ns = treasures->findInit("treasure");
		XmlNode* tnode = treasures->findNext(ns);
		while (tnode != NULL) {
			XmlNode* msgNode = tnode->findFirst("message");
			if (msgNode->children.size() > 0 && 
				msgNode->children[0]->type == ntTextNode) 
			{
				printf("%s: %s...\n", 
					tnode->attributes["name"].c_str(), 
					msgNode->children[0]->name.substr(0, 60).c_str());
				
			}
			
			tnode = treasures->findNext(ns); 
			
		}
		
	
	} else {
		printf("Node 'treasures' not found.\n");
		exit(1);
		
	}
	
	printf("\n");
	
}
