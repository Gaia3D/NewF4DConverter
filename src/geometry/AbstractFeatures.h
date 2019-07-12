#ifndef _ABSTRACTFEATURE_H_
#define _ABSTRACTFEATURE_H_

#pragma once

#include <string>

using namespace std;

namespace indoorgml {
	class AbstractFeatures {
	public:
		AbstractFeatures(string id);
		AbstractFeatures();
		string getId();

		virtual ~AbstractFeatures();

	protected:

		string m_id;
	};
}

#endif