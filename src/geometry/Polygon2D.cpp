#pragma once


#define _USE_MATH_DEFINES

#include <vector>
#include <math.h>

#include <algorithm>

#include "Polygon2D.h"


class Tesselator;

namespace gaia3d {



	// Get the vertices
	const std::vector<Point2D>& Polygon2D::getVertices() const {
		return m_vertices;
	}
	std::vector<Point2D>& Polygon2D::getVertices() {
		return m_vertices;
	}

	// Get the indices
	const std::vector<unsigned int>& Polygon2D::getIndices() const {
		return m_indices;
	}

	//bool Polygon2D::neg_normal() const {}
	//void Polygon2D::setNeg_normal(bool neg_normal) {}

/*
	void Polygon2D::addRing(LinearRing2D*) {}

	//void finish(bool optimize, std::shared_ptr<Logger> logger);

	std::shared_ptr<LinearRing2D> Polygon2D::exteriorRing() {
		return m_exteriorRing;
	}

	const std::shared_ptr<LinearRing2D> Polygon2D::exteriorRing() const {
		return m_exteriorRing;
	}

	std::vector<std::shared_ptr<LinearRing2D> >& Polygon2D::interiorRings() {
		return m_interiorRings;
	}

	const std::vector<std::shared_ptr<LinearRing2D> >& Polygon2D::interiorRings() const {
		return m_interiorRings;
	}
	void Polygon2D::setExterior(shared_ptr<LinearRing2D> l) {
		m_exteriorRing = l;
	}
	shared_ptr<LinearRing2D> Polygon2D::getExterior() {
		return m_exteriorRing;
	}

*/

	Point2D Polygon2D::getEdgeDirection(int index) {
		Point2D result = getEdgeVector(index);
		result.unitary();
		return result;
	}

	Point2D Polygon2D::getEdgeVector(int index) {
		//segment를 구하고
		Point2D curr = m_vertices.at(index);
		int vertexSize = m_vertices.size();
		int nextIndex = (index + 1) % vertexSize;
		Point2D next = m_vertices.at(nextIndex);
		if (nextIndex == vertexSize)
			nextIndex = 0;

		//그 segment에서 getVector를 한다.
		Point2D result(next.x - curr.x, next.y - curr.y);

		return result;

	}

	vector<int> Polygon2D::calculateNormal() {
		vector<int> result;
		for (int i = 0; i < m_vertices.size(); i++) {
			Point2D temp = m_vertices.at(i);
			int prevIndex, nextIndex;
			if (i == 0) {
				prevIndex = m_vertices.size() - 1;
			}
			else {
				prevIndex = i - 1;
			}
			if (i == m_vertices.size() - 1) {
				nextIndex = 0;
			}
			else {
				nextIndex = i + 1;
			}
			Point2D start = getEdgeDirection(prevIndex);
			Point2D end = getEdgeDirection(i);

			double crossProd = start.cross(end);
			double scalaProd = start.scalar(end);

			if (crossProd < 0.0) {
				crossProd = -1;
				result.push_back(i);
			}
			else if (crossProd > 0.0) {
				crossProd = 1;
			}
			else {
				continue;
			}
			double cosAlfa = scalaProd;
			double alfa = acos(cosAlfa);
			_normal += (crossProd * alfa);
		}

		if (_normal > 0) {
			_normal = 1;
		}
		else {
			_normal = -1;
		}
		return result;
	}

	vector<int> Polygon2D::getPointsIndexSortedByDistFromPoint(Point2D point) {
		vector<int> result;
		vector<pair<double, int>>distlist;

		//거리 계산
		for (int i = 0; i < m_vertices.size(); i++) {
			if (point.isSame(m_vertices.at(i))) {
				continue;
			}
			Point2D tempPoint = m_vertices.at(i);
			double dist = sqrt(pow(point.x - tempPoint.x, 2.0) + pow(point.y - tempPoint.y, 2.0));
			distlist.push_back(make_pair(dist, i));
		}

		sort(distlist.begin(), distlist.end());

		for (int i = 0; i < distlist.size(); i++) {

			result.push_back(distlist.at(i).second);
		}

		return result;
	}

	bool Polygon2D::checkSharedPointsWithSegement(int seg1Index1, int seg1Index2, int seg2Index1, int seg2Index2) {
		bool result = false;
		if (m_vertices.at(seg1Index1).isSame(m_vertices.at(seg2Index1)) || m_vertices.at(seg1Index1).isSame(m_vertices.at(seg2Index2))) {
			return true;
		}
		if (m_vertices.at(seg1Index2).isSame(m_vertices.at(seg2Index1)) || m_vertices.at(seg1Index2).isSame(m_vertices.at(seg2Index2))) {
			return true;
		}

		return result;
	}

	bool Polygon2D::checkParallel(int seg1Index1, int seg1Index2, int seg2Index1, int seg2Index2) {
		Point2D seg1P1 = m_vertices.at(seg1Index1);
		Point2D seg1P2 = m_vertices.at(seg1Index2);

		Point2D seg2P1 = m_vertices.at(seg2Index1);
		Point2D seg2P2 = m_vertices.at(seg2Index2);

		Point2D direct1 = Point2D(seg1P1.x - seg1P2.x, seg1P1.y - seg1P2.y);
		direct1.unitary();
		Point2D direct2 = Point2D(seg2P1.x - seg2P2.x, seg2P1.y - seg2P2.y);
		direct2.unitary();

		double zero = 10E-10;
		double angRad = direct1.angleToVector(direct2);

		if (angRad < zero || abs(angRad - M_PI) < zero) {
			return true;
		}
		return false;
	}

	bool Polygon2D::isthePointIntersectWithSegment(Point2D segP1, Point2D segP2, Point2D thePoint) {

		double error = 10E-8;

		double distA = segP1.distTo(thePoint);
		double distB = segP2.distTo(thePoint);
		double distTotal = segP1.distTo(segP2);

		//if(distA < error)
		//if(distB < error)

		if (abs(distA + distB - distTotal) < error) {
			return true;
		}

		return false;

	}

	bool Polygon2D::isIntersectedWithSegment(int index1, int index2) { // index1, index2가 분할할 선분을 의미
		bool result = false;

		double error = 10E-8;

		for (int i = 0; i < m_vertices.size() - 1; i++) {
			//check whether the segment overlapped to the other vertex or parrallel to the other.
			if (checkSharedPointsWithSegement(index1, index2, i, i + 1) || checkParallel(index1, index2, i, i + 1)) {
				continue;
			}


			//calculate line equation
			Point2D seg1P1 = m_vertices.at(i); //start point of segment1
			Point2D seg1P2 = m_vertices.at(i + 1);

			Point2D seg2P1 = m_vertices.at(index1); //start point of segment2
			Point2D seg2P2 = m_vertices.at(index2);

			Point2D direct1 = Point2D(seg1P1.x - seg1P2.x, seg1P1.y - seg1P2.y);
			direct1.unitary();
			Point2D direct2 = Point2D(seg2P1.x - seg2P2.x, seg2P1.y - seg2P2.y);
			direct2.unitary();

			double zero = 10E-10;

			//get the candidate of intersection point
			double intersectX;
			double intersectY;

			if (abs(direct1.x) < zero) {
				double slope = direct2.y / direct2.x;
				double b = seg2P1.y - slope * seg2P1.x;
				intersectX = seg1P1.x;
				intersectY = slope * intersectX + b;
			}
			else if (abs(direct1.y) < zero) {
				if (abs(direct2.x) < zero) {
					intersectX = seg2P1.x;
					intersectY = seg1P1.y;
				}
				else {
					double slope = direct2.y / direct2.x;
					double b = seg2P1.y - slope * seg2P1.x;
					intersectX = (seg1P1.y - b) / slope;
					intersectY = seg1P1.y;

				}
			}
			else {
				if (abs(direct2.x) < zero) {
					double mySlope = direct1.y / direct1.x;
					double myB = seg1P1.y - mySlope * seg1P1.x;

					intersectX = seg2P1.x;
					intersectY = intersectX * mySlope + myB;
				}
				else {
					double mySlope = direct1.y / direct1.x;
					double myB = seg1P1.y - mySlope * seg1P1.x;

					double slope = direct2.y / direct2.x;
					double b = seg2P1.y - slope * seg2P1.x;

					intersectX = (myB - b) / (slope - mySlope);
					intersectY = slope * intersectX + b;
				}
			}

			Point2D intersect(intersectX, intersectY);
			//check the candidate is real intersection point
			if (isthePointIntersectWithSegment(seg1P1, seg1P2, intersect) || isthePointIntersectWithSegment(seg2P1, seg2P2, intersect)) {
				return true;
			}

		}

		return result;
	}
	vector<Polygon2D> Polygon2D::splitPolygonbyIndex(int index1, int index2) {
		vector<Polygon2D> result;


		Polygon2D first, second;

		first.m_vertices.push_back(m_vertices.at(index1));
		first.m_vertices.push_back(m_vertices.at(index2));

		int count = 0;
		int startIndex = index1;
		int currIndex = index2;
		while (count < m_vertices.size()) {
			int nextIndex = currIndex + 1;
			if (nextIndex == m_vertices.size()) {
				nextIndex = 0;
			}
			if (nextIndex == startIndex) {
				break;
			}
			else {
				first.m_vertices.push_back(m_vertices.at(nextIndex));
				currIndex = nextIndex;
			}
			count++;
		}
		result.push_back(first);

		second.m_vertices.push_back(m_vertices.at(index2));
		second.m_vertices.push_back(m_vertices.at(index1));

		startIndex = index2;
		currIndex = index1;
		count = 0;

		while (count < m_vertices.size()) {
			int nextIndex = currIndex + 1;
			if (nextIndex == m_vertices.size()) {
				nextIndex = 0;
			}
			if (nextIndex == startIndex) {
				break;
			}
			else {
				second.m_vertices.push_back(m_vertices.at(nextIndex));
				currIndex = nextIndex;
			}
			count;

		}
		result.push_back(second);
		return result;

	}
	double Polygon2D::getNormal() {
		return _normal;
	}

	void Polygon2D::setVertices(vector<Point2D>l) {
		m_vertices = l;
		//if you need, set index of the verteces.
	}
	vector<Polygon2D> Polygon2D::tessellate(vector<int>concaveVerticesIndices) {
		vector<Polygon2D>result;
		if (concaveVerticesIndices.size() == 0) {
			Polygon2D convexPolygon;
			convexPolygon.setVertices(getVertices());
			result.push_back(convexPolygon);


			return result;
		}
		bool find = false;
		int index2;
		int count = 0;
		while (!find && count < concaveVerticesIndices.size()) {
			int index = concaveVerticesIndices.at(count);
			Point2D tempPoint = m_vertices.at(index);
			vector<int>resultSortedPointsIdxArray;

			resultSortedPointsIdxArray = getPointsIndexSortedByDistFromPoint(tempPoint);

			int count2 = 0;

			while (!find && count2 < resultSortedPointsIdxArray.size()) {

				index2 = resultSortedPointsIdxArray.at(count2);

				int prevIndex = index - 1;
				if (prevIndex < 0)
					prevIndex = resultSortedPointsIdxArray.size() - 1;
				int nextIndex = (index + 1) % resultSortedPointsIdxArray.size();

				if (index2 == prevIndex || index2 == nextIndex) {
					count2++;
					continue;
				}

				//check intersection with the polygon and this pair(index-index2)
				if (isIntersectedWithSegment(index, index2)) {
					count2++;
					continue;
				}

				vector<Polygon2D> splittedPolygons = splitPolygonbyIndex(index, index2);

				if (splittedPolygons.size() < 2) {
					count2++;
					continue;
				}

				Polygon2D first = splittedPolygons.at(0);
				Polygon2D second = splittedPolygons.at(1);

				vector<int>firstConcavePointsIndex = first.calculateNormal();
				vector<int>secondConcavePointsIndex = second.calculateNormal();

				if (first.getNormal() == getNormal() && second.getNormal() == getNormal()) {
					find = true;

					if (firstConcavePointsIndex.size() > 0) {
						result = first.tessellate(firstConcavePointsIndex);
					}
					else {
						result.push_back(first);
					}

					if (secondConcavePointsIndex.size() > 0) {
						result = second.tessellate(secondConcavePointsIndex);
					}
					else {
						result.push_back(second);
					}
				}
				count2++;
			}
			count++;

		}

		return result;

	}

	Polygon2D::Polygon2D() {}

	Polygon2D::~Polygon2D() {}


}
