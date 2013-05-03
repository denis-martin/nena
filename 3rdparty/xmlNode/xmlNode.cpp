/** @file
 *
 * Simple class to read (parts of) and write an XML file. The input is supposed
 * to be compatible with ASCII-7 (e.g. UTF-8, ANSI or ISO-8859-1).
 *
 * The code may be used freely in any way. It is distributed WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * @author Denis Martin
 * @copyright (c) 2006-2009
 */

#include "xmlNode.h"
//#include "debug.h"

#include <assert.h>

#include <istream>
#include <stack>

namespace sxml
{
	
using namespace std;

/**
 * Possible states during parsing the XML file
 */
typedef enum {
	psNone,
	psElement,
	psAttribute,
	psAttrValue,
	psComment,
	psDocType,
	psText
} XmlParseState;

/**
 * Create an incomplete node
 */
XmlNode::XmlNode()
{
	type = ntUndefined;
	complete = false;
}

/**
 * Constructor. Create a completed node.
 * 
 * @param type	NodeType of node
 * @param name	Node name
 */
XmlNode::XmlNode(const NodeType type, const string& name)
{
	this->type = type;
	this->name = name;
	this->complete = true;
}

/**
 * Destructor. All child nodes are freed, too (in a recursive manner).
 */
XmlNode::~XmlNode()
{	
	freeSubTree();
}

void XmlNode::freeSubTree() {
	vector<XmlNode*>::iterator it;
	for (it = children.begin(); it != children.end(); it++) {
		delete *it;
	}
	children.clear();
}

/**
 * Read the configuration of the node from a stream. The stream should be positioned 
 * to a valid start tag of an XML element (e.g. the beginning of a file or any
 * intermediate XML element) - if not, it will return a text element.
 * 
 * @param readFrom		Stream where to read from
 * @param readChildren	False, if all child elements should also be read (defaults to true)
 */
void XmlNode::readFromStream(istream& readFrom, const bool readChildren) {
	if (((void*) readFrom) == NULL) throw eBadStream;
	if (!readFrom.good()) throw eBadStream;
	
	const char* whitespaces = "\t\r\n ";
	
	char prompt;
	
	XmlParseState state = psNone; // parsing is modelled as a finite state machine
	unsigned char c;
	bool tagFinished = false;
	bool bom = false; // true if just parsed UTF-8 byte order mark
	bool docTypeString = false;
	
	stack<XmlNode*> parents; // if we have to descend, remember parent nodes on a stack
	XmlNode* cnode = this; // current node
	
	string attrName, attrValue;
	
	readFrom.setf(ios::skipws);
	
	try {
		while (!readFrom.eof()) {
			readFrom >> c;
			readFrom.unsetf(ios::skipws);
			
			switch (state) {
				
				// *** psElement ************************************************
				case psElement: {
					
					switch (c) {
						
						case '/': {
							readFrom >> c;
							if ('>' == c) {
								cnode->complete = true;
								state = psNone;
								tagFinished = true;
							} else {
								throw eXmlParseError;
							}
							break;
						}
						
						case '?': {
							readFrom >> c;
							if ('>' == c) {
								state = psNone;
								tagFinished = true;
							} else {
								throw eXmlParseError;
							}
							break;
						}
						
						case '>': {
							state = psNone;
							tagFinished = true;
							break;
						}
						
						case '\t':
						case '\n':
						case '\r':
						case ' ': {
							readFrom.setf(ios::skipws);
							readFrom >> c;
							readFrom.unsetf(ios::skipws);

							switch (c) {
								case '/': {
									readFrom >> c;
									if ('>' == c) {
										cnode->complete = true;
										state = psNone;
										tagFinished = true;
									} else {
										throw eXmlParseError;
									}
									break;
								}
								case '>': {
									state = psNone;
									tagFinished = true;
									cout << "Tag: " << cnode->name;
									cin >> prompt;
									break;
								}
								default: {
									attrName = c;
									state = psAttribute;
								}
							}

							break;
						}
						
						default: {
							if ((c >= 'a' && c <= 'z') ||
								(c >= 'A' && c <= 'Z') ||
								(c >= '0' && c <= '9') ||
								(c == ':') || (c == '-') || (c == '_') || (c == '.'))
							{
								cnode->name += c;
							} else {
								throw eXmlParseError;
							}
						}
					}
					break;
				}
				
				// *** psAttribute **********************************************
				case psAttribute: {
					
					switch (c) {
						
						case '=': {
							readFrom.setf(ios::skipws);
							readFrom >> c;
							readFrom.unsetf(ios::skipws);
							
							if ('"' == c) {
								state = psAttrValue;
							} else {
								throw eXmlParseError;
							}
							break;
						}
						
						case '\t':
						case '\n':
						case '\r':
						case ' ': {
							readFrom.setf(ios::skipws);
							readFrom >> c;
							
							if ('=' == c) {
								readFrom >> c;
								
								if ('"' == c) {
									state = psAttrValue;
								} else {
									throw eXmlParseError;
								}
							} else {
								throw eXmlParseError;
							}
							readFrom.unsetf(ios::skipws);
							break;
						}
						
						default: {
							if ((c >= 'a' && c <= 'z') ||
								(c >= 'A' && c <= 'Z') ||
								(c >= '0' && c <= '9') ||
								(c == ':') || (c == '-') || (c == '_') || (c == '.'))
							{
								attrName += c;
							} else {
								throw eXmlParseError;
							}
						}
					}
					break;
				}
				
				// *** psAttrValue **********************************************
				case psAttrValue: {
					
					switch (c) {
						
						case '"': {
							// end of value
							state = psElement;
							cnode->attributes[attrName] = attrValue;
							attrName = "";
							attrValue = "";
							break;
						}
						
						default: {
							attrValue += c;
						}
					}
					break;
				}
				
				// *** psComment ************************************************
				case psComment: {
					
					switch (c) {
						
						case '-': {
							readFrom >> c;
							if ('-' == c) {
								readFrom >> c;
								if ('>' == c) {
									// end of comment
									
									state = psNone;
									cnode->complete = true;
									tagFinished = true;
								} else {
									cnode->name += "--" + c;
								}
							} else {
								cnode->name += '-' + c;
							}
							break;
						}
						
						default: {
							cnode->name += c;
						}
					}
					break;
				}
					
				// *** psText ***************************************************
				case psText: {
					
					switch (c) {
						
						case '<': {
							// end of text node
							// erase trailing white spaces
							string::size_type ws_start = cnode->name.find_last_not_of(whitespaces);
							cnode->name.erase(ws_start + 1);
							
							readFrom.unget();
							state = psNone;
							cnode->complete = true;
							tagFinished = true;
							break;
						}
						
						default: {
							cnode->name += c;
						}
					}
					break;
				}
				
				// *** psDocType ************************************************
				case psDocType: {
						
					switch (c) {
						case '>': {
							if (!docTypeString) {
								state = psNone;
								cnode->complete = true;
								tagFinished = true;
							}
							break;
						}
						case '"': {
							docTypeString = !docTypeString;
							cnode->name += c;
							break;
						}
						default: {
							cnode->name += c;
						}
					}
					
					break;
				}
					
				// *** psNone ***************************************************
				case psNone: {
					
					switch (c) {
				
						// element
						case '<': {
							assert(cnode->type == ntUndefined);
							
							readFrom >> c;
							switch (c) {
								case '!': {
									readFrom >> c; 
									if ('-' == c) {
										readFrom >> c; if ('-' != c) throw eXmlParseError;
										cnode->type = ntCommentNode;
										state = psComment;
										
									} else {
										if (c != 'D') throw eXmlParseError;
										readFrom >> c; if (c != 'O') throw eXmlParseError;
										readFrom >> c; if (c != 'C') throw eXmlParseError;
										readFrom >> c; if (c != 'T') throw eXmlParseError;
										readFrom >> c; if (c != 'Y') throw eXmlParseError;
										readFrom >> c; if (c != 'P') throw eXmlParseError;
										readFrom >> c; if (c != 'E') throw eXmlParseError;
										
										cnode->type = ntDocTypeNode;
										state = psDocType;
										readFrom.setf(ios::skipws);
									}
									break;
								}
								case '?': {
									cnode->type = ntDocumentNode;
									state = psElement;
									break;
								}
								case '/': {
									// close tag								
									// read name
									string closeTagName;
									readFrom >> c;
									while (!readFrom.eof() && '>' != c) {
										if ((c >= 'a' && c <= 'z') ||
											(c >= 'A' && c <= 'Z') ||
											(c >= '0' && c <= '9') ||
											(c == ':') || (c == '-') || (c == '_') || (c == '.'))
										{
											closeTagName += c;
										} else {
											throw eXmlParseError;
										}
										readFrom >> c;
									}
									
									if (parents.size() == 0) throw eUnexpectedCloseTag;
									XmlNode* pnode = parents.top();
									parents.pop();
									if (pnode->name != closeTagName) {
										throw eUnexpectedCloseTag;
										
									} else {
										delete cnode; // this was an empty node struct
										cnode = pnode;
										cnode->complete = true; // this node will be linked to its parent below
										state = psNone;	
										tagFinished = true;
									}
									break;
								}
								default: {
									readFrom.unget();
									cnode->type = ntElementNode;
									state = psElement;
								}
							}
							break;
						}
						
						// UTF-8 byte order mark
						case 0xEF: {
							readFrom >> c; if (c != 0xBB) throw eUtf8BomError;
							readFrom >> c; if (c != 0xBF) throw eUtf8BomError;
							bom = true;
							break;
						}
						
						default: {
							cnode->type = ntTextNode;
							cnode->name += c;
							state = psText;
						}
						
					}
					break;
					
				} // psNone;
				
				default: {
					assert(false);
				}
				
			} // switch (state)
			
			if (psNone == state) {
				if (!(tagFinished || bom)) throw eXmlParseError;
					
				if (bom) {
					bom = false;
					
				} else if (cnode->complete) {
					// check, if we can ascend
					if (parents.size() > 0) {
						// yep
						XmlNode* pnode = parents.top();
//						DEBUG(DBG_XML, DBGL_INFO, "Ascending to %s\n", pnode->name.c_str());
						assert(!pnode->complete);
						// link node to parent
						pnode->children.push_back(cnode);
						cnode = new XmlNode(); // perhaps we get further children
						
					} else {
						// nope, so we are finished
						break;
					}
					
				} else {
					// we probably get children ;-)
					if (readChildren) {
//						DEBUG(DBG_XML, DBGL_INFO, "Descending from %s\n", cnode->name.c_str());
						parents.push(cnode);
						cnode = new XmlNode();
						
					} else {
						break;
					}
				}
				
				readFrom.setf(ios::skipws);
			}
			
		} // while (!readFrom.eof());
		
		if (parents.size() == 1) {
			cnode = parents.top();
			parents.pop();
			if (cnode->type == ntDocumentNode) {
				// this is ok
				cnode->complete = true;
				
			} else {
				throw eUnexpectedEof;
			}
			
		} else if (parents.size() > 1) {
			throw eUnexpectedEof;
		}
		
	} catch (sxml::Exception e) {
		// clean up
		
		while (parents.size() > 0) {
			XmlNode* n = parents.top();
			if (n != this) {
				if (n == cnode) cnode = NULL;
				delete n;
			}
			parents.pop();
		}
		
		if (cnode != this && cnode != NULL) {
			delete cnode; // we are not yet attached to a parent!
		}
		
		throw;
	}
	
	assert(cnode == this);
}

/**
 * Write the node and all child nodes to a stream. The format resembles to XML.
 * 
 * @param writeTo	Stream where to write to (e.g. ofstream)
 * @param pretty	If false, the elements will be written without indention or
 * 					line breaks (defaults to true)
 */
void XmlNode::writeToStream(ostream& writeTo, const bool pretty) {
	if (!writeTo.good()) throw eBadStream;
	
	int indent = 0;
	
	typedef struct {
		XmlNode* node;
		int childindex;
	} ParentIndex;
	
	stack<XmlNode*> parents; // if we have to decend, remember parent nodes on a stack
	stack<int> childIndices; // ... and the current child index
	
	XmlNode* cnode = this; // current node
	int childIndex = 0;
	
	while (writeTo.good()) {
		if (!cnode->complete) throw eNodeIncomplete;
		
		if (pretty) {
			for (int i = 0; i < indent; i++) {
				writeTo << '\t';
			}
		}
		
		switch (cnode->type) {
			
			case ntElementNode: {
				// tag
				writeTo << '<' << cnode->name;
				
				// attributes
				map<string, string>::iterator it;
				for (it = cnode->attributes.begin(); it != cnode->attributes.end(); it++) {
					if (it->second != "") {
						writeTo << ' ' << it->first << "=\"" << it->second << '"';
					}
				}
				
				// closing brace
				if (childIndex < (int) cnode->children.size()) {
					writeTo << '>';
				} else {
					writeTo << " />";
				}
				
				break;
			}
			
			case ntTextNode: {
				writeTo << cnode->name;
				break;
			}
			
			case ntCommentNode: {
				writeTo << "<!-- " << cnode->name << " -->";
				break;
			}
			
			case ntDocumentNode: {
				// tag
				writeTo << "<?" << cnode->name;
				
				// attributes (version and encoding at first, rest afterwards)
				map<string, string>::iterator it;
				it = cnode->attributes.find("version");
				if (it != cnode->attributes.end() && !it->second.empty()) {
					writeTo << ' ' << it->first << "=\"" << it->second << '"';
				}
				it = cnode->attributes.find("encoding");
				if (it != cnode->attributes.end() && !it->second.empty()) {
					writeTo << ' ' << it->first << "=\"" << it->second << '"';
				}
				for (it = cnode->attributes.begin(); it != cnode->attributes.end(); it++) {
					if (!it->second.empty() && it->first != "version" && it->first != "encoding") {
						writeTo << ' ' << it->first << "=\"" << it->second << '"';
					}
				}
				
				// closing brace
				writeTo << "?>";
				break;
			}
			
			case ntDocTypeNode: {
				// tag
				writeTo << "<!DOCTYPE " << cnode->name << '>';
				break;
			}
			
			default: {
				throw eUnknownNodeType;
			}
			
		} // switch
		
		if (pretty) writeTo << endl;
		
		if (childIndex < (int) cnode->children.size()) {
			if (cnode->type != ntDocumentNode) indent++;
			
			// descend
			parents.push(cnode);
			childIndices.push(childIndex);
			cnode = cnode->children[childIndex];
			childIndex = 0;
			
		} else {
			// ascend
			while (parents.size() > 0) {
				cnode = parents.top();
				childIndex = childIndices.top();
				childIndices.pop();
				childIndex++;
				if (childIndex < (int) cnode->children.size()) {
					// there are still children left
					childIndices.push(childIndex);
					cnode = cnode->children[childIndex];
					childIndex = 0;
					break;
					
				} else {
					// no more children, write end-tag
					parents.pop();
					indent--;
					
					if (cnode->type == ntElementNode) {
						if (pretty) {
							for (int i = 0; i < indent; i++) {
								writeTo << '\t';
							}
						}
						writeTo << "</" << cnode->name << '>';
						if (pretty) writeTo << endl;
					}
				}
			} // while
		}
		
		if (parents.size() == 0) break;
		
	} // while
}

/**
 * Create a new search handle. This handle can be used to iterate nodes with
 * the same name.
 * 
 * @param name				The name of the node to find
 * @param ignoreNamespaces	If true, anything before a ':' in the node's name
 * 							will be ignored.
 * 
 * @return 	A pointer to a NodeSearch handle. This must be freed by 
 * 			XmlNode::findFree().
 * 
 * @sa XmlNode::findNext(), XmlNode::findFree(), XmlNode::findFirst()
 */
NodeSearch* XmlNode::findInit(const string& name, const bool ignoreNamespaces) {
	NodeSearch* ns = new NodeSearch;
	ns->ignoreNamespaces = ignoreNamespaces;
	ns->owner = this;
	ns->name = name;
	ns->parents.push(this);
	ns->childIndices.push(-1);
	
	return ns;
}

/**
 * Return the next occurrence of the node name to find, NULL if nothing is
 * found.
 * 
 * @param ns	NodeSearch handle returned by XmlNode::findInit()
 * 
 * @return	The next occurrence of the node name to find or NULL if nothing is
 * 			found.
 * 
 * @sa XmlNode::findInit(), XmlNode::findFree(), XmlNode::findFirst()
 */
XmlNode* XmlNode::findNext(NodeSearch* ns) {
	assert(ns != NULL && ns->owner == this);
	assert(ns->parents.size() > 0);
	assert(ns->childIndices.size() > 0);
	
	XmlNode* cnode;
	int childIndex;
	
	while (ns->parents.size() > 0) {
		cnode = ns->parents.top();
		childIndex = ns->childIndices.top();
		if (childIndex == -1) {
			childIndex++;
			ns->childIndices.pop();
			ns->childIndices.push(childIndex);
			
			if (ns->ignoreNamespaces) {
				string::size_type pos = cnode->name.find(":", 0);
				if (pos != string::npos) {
					// namespace given
					string s = cnode->name.substr(pos + 1, cnode->name.length() - pos - 1);
					if (s == ns->name) {
						return cnode;
					}
					
				} else {
					if (cnode->name == ns->name) {
						return cnode;
					}
				}
				
			} else {
				if (cnode->name == ns->name) {
					return cnode;
				}
			}
			
		} else {
			if (childIndex < (int) cnode->children.size()) {
				// descend
				ns->parents.push(cnode->children[childIndex]);
				childIndex++;
				ns->childIndices.pop();
				ns->childIndices.push(childIndex);
				ns->childIndices.push(-1);
				
			} else {
				// ascend
				ns->parents.pop();
				ns->childIndices.pop();
			}
		}
	}
	
	return NULL;
}

/**
 * Free a NodeSearch handle.
 * 
 * @param ns	NodeSearch handle to free
 * 
 * @sa XmlNode::findInit(), XmlNode::findNext(), XmlNode::findFirst()
 */
void XmlNode::findFree(NodeSearch* ns) {
	assert(ns != NULL && ns->owner == this);
	delete ns;
}

/**
 * Find the first occurrence of a node given by name.
 * 
 * @param name				The name of the node to find
 * @param ignoreNamespaces	If true, anything before a ':' in the node's name
 * 							will be ignored.
 * 
 * @return The pointer to the node found, or NULL if the node is not found.
 * 
 * @sa XmlNode::findInit(), XmlNode::findNext(), XmlNode::findFree()
 */
XmlNode* XmlNode::findFirst(const string& name, const bool ignoreNamespaces) {
	NodeSearch* ns = findInit(name, ignoreNamespaces);
	XmlNode* res = findNext(ns);
	findFree(ns);
	
	return res;
}

} // namespace xsml
