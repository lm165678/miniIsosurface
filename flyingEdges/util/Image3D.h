/*
 * Image3D.h
 *
 *  Created on: Feb 17, 2017
 *      Author: dbourge
 */

#ifndef IMAGE3DREADER_H_
#define IMAGE3DREADER_H_

#include "FlyingEdges_Config.h"

#include <vector>
#include <array>

namespace util {

class Image3D
{
public:
    // This constructor is used to construct an image of size
    // dimensions.
    Image3D(std::vector<float_t> data,
            std::array<float_t, 3> spacing,
            std::array<float_t, 3> zeroPos,
            std::array<size_t, 3> dimensions)
      : data(data), spacing(spacing), zeroPos(zeroPos),
        nx(dimensions[0]), ny(dimensions[1]), nz(dimensions[2])
    {}

    std::vector<float_t>::const_iterator
    getRowIter(size_t j, size_t k) const;

    scalarCube_t getValsCube(size_t i, size_t j, size_t k) const;

    cube_t getPosCube(size_t i, size_t j, size_t k) const;

    cube_t getGradCube(size_t i, size_t j, size_t k) const;

    size_t xdimension() const { return nx; }
    size_t ydimension() const { return ny; }
    size_t zdimension() const { return nz; }

private:
    inline float_t
    getData(size_t i, size_t j, size_t k) const;

    std::array<float_t, 3>
    computeGradient(size_t i, size_t j, size_t k) const;

private:
    std::vector<float_t>    data;       // A vector containing scalar values
                                        // along three-dimensional space.

    std::array<float_t, 3>  spacing;    // The distance between two points in
                                        // the mesh.

    std::array<float_t, 3>  zeroPos;    // The position at index (0, 0, 0).

    size_t                  nx;         //
    size_t                  ny;         // The dimensions
    size_t                  nz;         //
};

}

#endif
