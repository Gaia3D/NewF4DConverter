
#ifndef _PARSERUTIL_H_
#define _PARSERUTIL_H_
#pragma once


#include <iostream>
#include <string>
#include <vector>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>


using namespace std;
using namespace xercesc;

namespace util {
	class ParserUtil {
	public:
		ParserUtil();
		~ParserUtil();

		bool isTextNode(DOMNode* node);
		bool isMatchedNodeName(DOMNode* node, string nodeType);
		bool hasNamedChild(DOMNode* node, string s);
		bool hasNamedAttribute(DOMNode* node, string s);
		string changeXMLCh2str(const XMLCh* x);
		string getNamedAttribute(DOMNamedNodeMap* list, string s);

		DOMNode* getNamedNode(DOMNodeList* list, string s);
		DOMNode* getNamedNode(vector<DOMNode*> list, string s);

		vector<DOMNode*> getNamedNodes(DOMNodeList* list, string s);
		vector<DOMNode*> getNamedNodes(vector<DOMNode*> list, string s);
	};
}
#endif