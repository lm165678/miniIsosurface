/*
 * BlockMarchFunctor.cpp
 *
 *  Created on: Aug 19, 2015
 *      Author: sjmunn
 */

#include "BlockMarchFunctor.h"

template<typename T>
BlockMarchFunctor<T>::BlockMarchFunctor(Image3DReader_type &volReader, const unsigned blockExt[6],
		T isoval, PointMap_type &pointMap, EdgeIndexer_type &edgeIndices,
		TriangleMesh_type &mesh) {

	dims = volReader.imageData->getDimension();
	origin = volReader.imageData->getOrigin();
	spacing = volReader.imageData->getSpacing();
	T val[8]; // Vertex values for each cube

	sliceSize = dims[0] * dims[1];

	unsigned ptIdx = mesh.numberOfVertices();

	// march through each cell
	T zpos = origin[2] + (T(blockExt[4]) * spacing[2]);
	for (zidx = blockExt[4]; zidx <= blockExt[5]; ++zidx, zpos += spacing[2]) {

		T ypos = origin[1] + (T(blockExt[2]) * spacing[1]);
		for (yidx = blockExt[2]; yidx <= blockExt[3];
				++yidx, ypos += spacing[1]) {
			volReader.setImage3DOutputBuffers(blockExt[0],yidx,zidx);

			T xpos = origin[0] + (T(blockExt[0]) * spacing[0]);
			for (xidx = blockExt[0]; xidx <= blockExt[1]; ++xidx, xpos +=
					spacing[0]) {

				T pos[8][3], grad[8][3];
				volReader.getVertexValues(val,xidx,blockExt[0]);
				
				if (xidx+yidx+zidx==0) {
				  CLOG(logDEBUG1) << "Vertex vals at origin:";
				  for (int iV=0;iV<8;iV++) {
				    CLOG(logDEBUG1) << val[iV];
				  }
				}
				cellCaseId = findCaseId(val,isoval);
				// no intersections
				if (cellCaseId == 0 || cellCaseId == 255) {
					continue;
				}

				// get physical position and gradient of the points
				pos[0][0] = xpos;
				pos[0][1] = ypos;
				pos[0][2] = zpos;

				pos[1][0] = xpos + spacing[0];
				pos[1][1] = ypos;
				pos[1][2] = zpos;

				pos[2][0] = xpos + spacing[0];
				pos[2][1] = ypos + spacing[1];
				pos[2][2] = zpos;

				pos[3][0] = xpos;
				pos[3][1] = ypos + spacing[1];
				pos[3][2] = zpos;

				pos[4][0] = xpos;
				pos[4][1] = ypos;
				pos[4][2] = zpos + spacing[2];

				pos[5][0] = xpos + spacing[0];
				pos[5][1] = ypos;
				pos[5][2] = zpos + spacing[2];

				pos[6][0] = xpos + spacing[0];
				pos[6][1] = ypos + spacing[1];
				pos[6][2] = zpos + spacing[2];

				pos[7][0] = xpos;
				pos[7][1] = ypos + spacing[1];
				pos[7][2] = zpos + spacing[2];

				// get the triangles to generate
				const int *edges = MarchingCubesTables::getCaseTrianglesEdges(
						cellCaseId);
				for (; *edges != -1; edges += 3) {

					unsigned tri[3];
					for (int i = 0; i < 3; ++i) {
						int v1 =
								MarchingCubesTables::getEdgeVertices(edges[i])[0];
						int v2 =
								MarchingCubesTables::getEdgeVertices(edges[i])[1];
						T w = (isoval - val[v1]) / (val[v2] - val[v1]);

						// interpolate vertex position
						T  newPtCoordinates[3];
						PositionVector_type newPt;

						bool exists = false;
						unsigned pointId;

						unsigned edgeIndex = edgeIndices.getEdgeIndex(xidx, yidx,
								zidx, edges[i]);
						if (pointMap.find(edgeIndex) == pointMap.end()) {
							// not found -- this is a new point
							pointMap[edgeIndex] = ptIdx;
							for (int iAxis = 0; iAxis < 3; iAxis++) {
								newPtCoordinates[iAxis] = lerp(pos[v1][iAxis],
										pos[v2][iAxis], w);
							}
							newPt.setCoordinates(newPtCoordinates);
						} else {
							// found -- we already have this point
							exists=true;
							pointId = pointMap[edgeIndex];
						}

						if (!exists) {
							mesh.addPoint(newPt);

							computeAllGradients(xidx, yidx, zidx, &volReader, grad);

							T norm[3];
							for (int iAxis = 0; iAxis < 3; iAxis++) {
								norm[iAxis] = lerp(grad[v1][iAxis],
										grad[v2][iAxis], w);
							}
							mesh.addNormal(norm);

							tri[i] = ptIdx++;
						} else {
							tri[i] = pointId;
						}
					}

					if (tri[0] == tri[1] || tri[1] == tri[2]
							|| tri[2] == tri[0]) {
					} else {
						mesh.addTriangle(tri);
						CLOG(logDEBUG_Step) << "Num tri " << mesh.numberOfTriangles();
					}
				}
			}
		}
	}
}

template<typename T>
BlockMarchFunctor<T>::~BlockMarchFunctor() {
}

template<typename T>
T BlockMarchFunctor<T>::lerp(T a, T b, T w) {
	//return ((1.0 - w) * a) + (w * b);
	return a + (w * (b - a));
}

template<typename T>
int BlockMarchFunctor<T>::findCaseId(T* cubeVertexVals, T isoval) {
	int caseId=0;
	for (int i = 0; i < 8; ++i) {
		caseId |= (cubeVertexVals[i] >= isoval) ? caseMask[i] : 0;
	}
	return caseId;
}

//template<typename T>
//void BlockMarchFunctor<T>::updateBuffers() {
//	X1buffer = &buffer[bufferIdx];
//	X2buffer = &buffer[bufferIdx + dims[0]];
//	X3buffer = &buffer[bufferIdx + sliceSize];
//	X4buffer = &buffer[bufferIdx + dims[0] + sliceSize];
//}

// Must instantiate class for separate compilation
template class BlockMarchFunctor<float_t> ;
