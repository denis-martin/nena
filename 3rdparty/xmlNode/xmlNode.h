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

#ifndef XMLNODE_H_
#define XMLNODE_H_

#include <iostream>
#include <vector>
#include <map>
#include <stack>

namespace sxml
{
	
using namespace std;

class XmlNode;

/**
 * Exceptions that may occur while reading or writing the node
 * (and its child nodes) from/to a stream
 */
typedef enum {
	eBadStream,				///< the stream passed as parameter was bad
	// input exceptions
	eUtf8BomError,			///< incorrect UTF-8 byte order mark
	eXmlParseError,			///< general parsing error
	eMissingCloseTag,		///< parsing reached end of stream but remains incomplete (e.g. missing close tag)
	eUnexpectedCloseTag,	///< encountered close tag does not match current node name
	eUnexpectedEof,			///< unexpected end of stream
	// output exceptions
	eUnknownNodeType,		///< attempt to write a node of type ntUndefined
	eNodeIncomplete			///< node is marked as incomplete
} Exception;

/**
 * Node types
 */
typedef enum {
	ntUndefined,
	ntDocumentNode, ///< <?xml ... ?>
	ntDocTypeNode,  ///< <!DOCTYPE ... > 
	ntCommentNode,  ///< <!-- ... -->
	ntElementNode,  ///< <.../> or <...>...</...>
	ntTextNode
} NodeType;

/**
 * Node search handle
 */
class NodeSearch
{
private:
	XmlNode* owner;
	bool ignoreNamespaces;
	stack<XmlNode*> parents;
	stack<int> childIndices;
	
	string name;
	
	friend class XmlNode;
};

typedef map<string, string> NodeAttributes;
typedef map<string, string>::iterator NodeAttributesIterator;
typedef vector<XmlNode*> NodeChildren;
typedef vector<XmlNode*>::iterator NodeChildrenIterator;

/**
 * Class that represents one XML node
 */
class XmlNode
{
public:
	NodeType type;	///< node type
	string name;	///< name of node (or content if it is a ntTextNode)
	bool complete;	///< whether the subtree is read/written completely
	
	NodeAttributes attributes; ///< key, value
	NodeChildren children;

	XmlNode();
	XmlNode(const NodeType type, const string& name);
	virtual ~XmlNode();
	
	void freeSubTree();
	
	void readFromStream(istream& readFrom, const bool readChildren = true);
	void writeToStream(ostream& writeTo, const bool pretty = true);
	
	NodeSearch* findInit(const string& name, const bool ignoreNamespaces = false);
	XmlNode* findNext(NodeSearch* ns);
	void findFree(NodeSearch* ns);
	
	XmlNode* findFirst(const string& name, const bool ignoreNamespaces = false);
};

} // namespace sxml

#endif /*XMLNODE_H_*/
