#pragma comment(lib, "rpcrt4.lib")
#include <sstream>
#include <iostream>
#include "AbstractFeatures.h"
#include <RPC.h>

namespace indoorgml {
	AbstractFeatures::AbstractFeatures(string id) : m_id(id) {
		if (id == "" || id.empty()) {
			UUID uuid = { 0 };
			string guid;

			// Create uuid or load from a string by UuidFromString() function
			::UuidCreate(&uuid);

			// If you want to convert uuid to string, use UuidToString() function
			RPC_CSTR szUuid = NULL;
			if (::UuidToStringA(&uuid, &szUuid) == RPC_S_OK)
			{
				guid = (char*)szUuid;
				::RpcStringFreeA(&szUuid);
			}
			m_id = guid;
		}

	}


	string AbstractFeatures::getId() {
		return m_id;
	}


	AbstractFeatures::~AbstractFeatures() {}
}

