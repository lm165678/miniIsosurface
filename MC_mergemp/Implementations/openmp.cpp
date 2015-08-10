/*
 * openmp.cpp
 *
 *  Created on: Jul 10, 2015
 *      Author: sjmunn
 */

#include"openmp.h"

static const unsigned grainDim = 1024;

namespace openmp {

void extractIsosurface(const Image3D_t &vol, float_t isoval,
		TriangleMesh_t *&mesh, YAML_Doc& doc) {

	mesh = new TriangleMesh_t();

	const unsigned *dims = vol.getDimension();
	const float_t *origin = vol.getOrigin();
	const float_t *spacing = vol.getSpacing();

	CLOG(logYAML) << "Marching cubes algorithm: OpenMP";
	doc.add("Marching cubes algorithm", "OpenMP");

	float_t range[6];
	for (int i = 0; i < 3; ++i) {
		range[i * 2] = origin[i];
		range[(i * 2) + 1] = origin[i]
				+ (static_cast<float_t>(dims[i] - 1) * spacing[i]);
	}

	/*
	 * fullRange is the entire image volume
	 */
	Range3D fullRange(0, dims[2] - 1, grainDim, 0, dims[1] - 1, grainDim, 0,
			dims[0] - 1, grainDim);

	unsigned fullExtent[6];
	fullRange.extent(fullExtent);
	EdgeIndexer_t edgeIndices(fullExtent);

	unsigned numBlockPages = numBlocks(fullRange.pages());
	unsigned numBlockRows = numBlocks(fullRange.rows());
	unsigned numBlockCols = numBlocks(fullRange.cols());

	unsigned nblocks = numBlockPages * numBlockRows * numBlockCols;
	unsigned nblocksPerPage = numBlockRows * numBlockCols;
	CLOG(logDEBUG1) << "Number of OpenMP Blocks " << nblocks;
	CLOG(logDEBUG) << "nAllEdges: " << edgeIndices.nAllEdges;

	PointMap_t ptMapDEBUG;

	countPointsInBlock(vol, fullExtent, isoval,edgeIndices,ptMapDEBUG);

	#pragma omp ordered
	{
		TriangleMesh_t threadMesh;
		PointMap_t threadPointMap;

		#pragma omp for nowait
		for (unsigned i = 0; i < nblocks; ++i) {
			//CLOG(logDEBUG) << "Iteration " << i;
			unsigned blockPageIdx = i / nblocksPerPage;
			unsigned blockRowIdx = (i % nblocksPerPage) / numBlockCols;
			unsigned blockColIdx = (i % nblocksPerPage) % numBlockCols;

			unsigned pfrom = blockPageIdx * fullRange.pages().grain();
			unsigned pto = std::min(pfrom + fullRange.pages().grain(),
					fullRange.pages().end());
			unsigned rfrom = blockRowIdx * fullRange.rows().grain();
			unsigned rto = std::min(rfrom + fullRange.rows().grain(),
					fullRange.rows().end());
			unsigned cfrom = blockColIdx * fullRange.cols().grain();
			unsigned cto = std::min(cfrom + fullRange.cols().grain(),
					fullRange.cols().end());

			Range3D blockRange(pfrom, pto, rfrom, rto, cfrom, cto);
			unsigned blockExtent[6];
			blockRange.extent(blockExtent);

			unsigned approxNumberOfEdges = 3*(pto-pfrom+1)*(rto-rfrom+1)*(cto-cfrom+1);

			unsigned mapSize = approxNumberOfEdges; // Very approximate hack..
			threadPointMap.rehash(mapSize);
			//threadPointMap.resize(localEdgeIndices.nAllEdges,-1);

			extractIsosurfaceFromBlock(vol, blockExtent, isoval, threadPointMap,edgeIndices,
					&threadMesh);
		}

		/*
		 * The next sections merges all the meshes in each block.
		 * It must be done in serial to construct an accurate mesh.
		 */
		#pragma omp critical
		{
			// The += operator for TriangleMesh3D object is overloaded to merge mesh objects
			*mesh += threadMesh;
		}

		// ============= Debug edges =======
		unsigned diff=0;
		for (PointMap_t::iterator it = ptMapDEBUG.begin(); it != ptMapDEBUG.end(); ++it) {
			unsigned EdgeIdx=it->first;
			if (threadPointMap.find(EdgeIdx) == threadPointMap.end()) {
				CLOG(logDEBUG) << "Overcounted edge indices: " << EdgeIdx;
				diff++;
			}
			if (diff>3) {
				break;
			}
		}
	}
	CLOG(logDEBUG1) << "Mesh verts: " << mesh->numberOfVertices();
	CLOG(logDEBUG1) << "Mesh tris: " << mesh->numberOfTriangles();
}

unsigned numBlocks(const Range oneDRange) {
	unsigned numberOfBlocks = (oneDRange.end() - oneDRange.begin() + oneDRange.grain() - 1)
			/ oneDRange.grain();
	return numberOfBlocks;
}

}