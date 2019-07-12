#include <iostream>
#include <vector>
#include "Solid.h"

using namespace std;

namespace gaia3d {

	bool Solid::hasExterior() {
		if (exterior.size() == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	bool Solid::hasInterior() {
		if (interior.size() == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	//const Solid& Solid::getExterior() const {	}
	vector<shared_ptr<Polygon>> Solid::getExterior() {
		return exterior;
	}
	std::vector<std::shared_ptr<Solid>> Solid::getInterior() {
		return interior;
	}
	void Solid::addInterior(std::shared_ptr<Solid> s) {
		interior.push_back(s);
	}
	//void deleteInterior(){}
	void Solid::setExterior(vector<std::shared_ptr<Polygon>> s) {
		exterior = s;
	}

	Solid::Solid(const std::string& id) : AbstractFeatures(id) {}

	Solid::~Solid() {}

}