#include "parserutil.h"

namespace util {

	ParserUtil::ParserUtil() {};
	ParserUtil::~ParserUtil() {};
	bool ParserUtil::isTextNode(DOMNode* node) {
		return (node->getNodeType() == DOMNode::TEXT_NODE);
	}
	bool ParserUtil::isMatchedNodeName(DOMNode* node, string nodeType) {
		return(!strcmp(XMLString::transcode(node->getNodeName()), nodeType.c_str()));
	}
	bool ParserUtil::hasNamedChild(DOMNode* node, string s) {
		if (node->hasChildNodes()) {
			for (int i = 0; i < node->getChildNodes()->getLength(); i++) {
				if (isMatchedNodeName(node->getChildNodes()->item(i), s)) {
					return true;
				}
			}
		}
		return false;
	}
	bool ParserUtil::hasNamedAttribute(DOMNode* node, string s) {
		if (node->hasAttributes()) {
			for (int i = 0; i < node->getChildNodes()->getLength(); i++) {
				if (isMatchedNodeName(node->getChildNodes()->item(i), s)) {
					return true;
				}
			}
		}
		return false;
	}
	string ParserUtil::changeXMLCh2str(const XMLCh* x) {
		return XMLString::transcode(x);
	}
	string ParserUtil::getNamedAttribute(DOMNamedNodeMap* list, string s) {
		string result;
		for (int j = 0; j < list->getLength(); j++) {
			if (isTextNode(list->item(j))) {
				continue;
			}
			if (isMatchedNodeName(list->item(j), s.c_str())) {
				result = changeXMLCh2str(list->item(j)->getNodeValue());
				cout << result << endl;
				break;
			}
		}
		return result;
	}
	DOMNode* ParserUtil::getNamedNode(DOMNodeList* list, string s) {
		DOMNode* result = 0;
		for (int i = 0; i < list->getLength(); i++) {
			if (!isTextNode(list->item(i))) {
				if (isMatchedNodeName(list->item(i), s)) {
					result = list->item(i);
					break;
				}
			}
		}
		return result;
	}

	DOMNode* ParserUtil::getNamedNode(vector<DOMNode*> list, string s) {
		DOMNode* result = 0;
		for (int i = 0; i < list.size(); i++) {
			if (!isTextNode(list.at(i))) {
				if (isMatchedNodeName(list.at(i), s)) {
					result = list.at(i);
					break;
				}
			}
		}
		return result;
	}

	vector<DOMNode*> ParserUtil::getNamedNodes(DOMNodeList* list, string s) {
		vector<DOMNode*>result;
		for (int i = 0; i < list->getLength(); i++) {
			if (!isTextNode(list->item(i))) {
				if (isMatchedNodeName(list->item(i), s)) {
					result.push_back(list->item(i));

				}
			}
		}
		return result;
	}

	vector<DOMNode*> ParserUtil::getNamedNodes(vector<DOMNode*> list, string s) {
		vector<DOMNode*>result;
		for (int i = 0; i < list.size(); i++) {
			if (!isTextNode(list.at(i))) {
				if (isMatchedNodeName(list.at(i), s)) {
					result.push_back(list.at(i));

				}
			}
		}
		return result;
	}
}