/*
 * LoadImage3DMPI.cpp
 *
 *  Created on: Aug 20, 2015
 *      Author: sjmunn
 */

#include "LoadImage3DMPI.h"

template<typename T>
LoadImage3DMPI<T>::LoadImage3DMPI() {
	xdimFile=0;
	ydimFile=0;
	zdimFile=0;
	fileNpoints=0;
	reader=0;
	vtkFile=0;
	vtkDataFile=0;
	nPointsInBlock=0;
	typeInfo=0;
	blockExtentSet=false;
	imageDataIdx=0;
	seperateData=false;
}

template<typename T>
LoadImage3DMPI<T>::LoadImage3DMPI(LoadImage3DMPI& loaderObject) {
	const unsigned * loaderObjectDim=loaderObject.getVolumeDimensions();
	const T * loaderObjectSpacing=loaderObject.getSpacing();
	const T * loaderObjectOrigin=loaderObject.getOrigin();

	if (loaderObjectDim[0]==0) throw zero_dimensions("Zero dimension error");

	xdimFile=loaderObjectDim[0];
	ydimFile=loaderObjectDim[1];
	zdimFile=loaderObjectDim[2];
	for (int iAxis=0;iAxis<3;++iAxis) {
		spacing[iAxis]=loaderObjectSpacing[iAxis];
		origin[iAxis]=loaderObjectOrigin[iAxis];
	}

	fileNpoints=loaderObject.getnVolumePoints();
	nPointsInBlock=0;
	typeInfo=new TypeInfo(loaderObject.typeInfo->getId());

	// Copy the line reader locally for this object
	seperateData=loaderObject.seperateData;
	if (seperateData) {
		vtkFile=loaderObject.vtkFile;
		vtkDataFile=loaderObject.vtkDataFile;
		stream.open(vtkDataFile);
	}
	else {
		vtkFile=loaderObject.vtkFile;
		vtkDataFile=0;
		stream.open(vtkFile);
		// Skip all the header lines
		for(int iLine=0; iLine<N_HEADER_LINES;++iLine) {
			stream.ignore ( 256, '\n' );
		}
	}

	if (!stream) {
		throw file_not_found(vtkFile);
	}
	reader = new LineStream(stream);
	blockExtentSet=false;
	imageDataIdx=0;
}

template<typename T>
LoadImage3DMPI<T>::~LoadImage3DMPI() {
	stream.close();
	delete reader;
	delete typeInfo;
}

template<typename T>
void LoadImage3DMPI<T>::setSperateData(const char * inDataFile) {
	if (inDataFile != 0) {
		seperateData=true;
		vtkDataFile=inDataFile;
	}
	else {
		seperateData=false;
		vtkDataFile=0;
	}
}

template<typename T>
void LoadImage3DMPI<T>::loadHeader(const char *vtkFileName) {
	vtkFile=vtkFileName;
	stream.open(vtkFileName);
	if (!stream) {
		throw file_not_found(vtkFileName);
	}

	reader = new LineStream(stream);

	// read and discard header
	reader->readline();

	reader->readline();
	std::string name;
	reader->stream() >> name;

	reader->readline();
	std::string format;
	reader->stream() >> format;
	if (format != "BINARY") {
		throw bad_format("Only 'BINARY' format supported");
	}

	std::string tag;

	reader->readline();
	std::string dataset;
	reader->stream() >> tag >> dataset;
	if (tag != "DATASET" || dataset != "STRUCTURED_POINTS") {
		throw bad_format("Expecting STRUCTURED_POINTS dataset");
	}

	int count = 3;
	while (count) {
		reader->readline();
		reader->stream() >> tag;

		if (tag == "DIMENSIONS") {
			reader->stream() >> xdimFile >> ydimFile >> zdimFile;
			if (reader->stream().bad()) {
				throw bad_format("Expecting DIMENSIONS [3]");
			}
			--count;
		} else if (tag == "SPACING") {
			reader->stream() >> spacing[0] >> spacing[1] >> spacing[2];
			if (reader->stream().bad()) {
				throw bad_format("Expecting SPACING [3]");
			}
			--count;
		} else if (tag == "ORIGIN") {
			reader->stream() >> origin[0] >> origin[1] >> origin[2];
			if (reader->stream().bad()) {
				throw bad_format("Expecting ORIGIN [3]");
			}
			--count;
		} else {
			throw bad_format("Expecting DIMENSIONS, SPACING and ORIGIN");
		}
	}

	reader->readline();
	unsigned npoints = 0;
	reader->stream() >> tag >> npoints;
	if (tag != "POINT_DATA" || reader->stream().bad()) {
		throw bad_format("Expecting POINT_DATA <npoints>");
	}

	if(LOG::ReportingLevel() == logDEBUG_Step) {
		if(npoints>1000) {
			throw file_too_large(npoints);
		}
	}
	fileNpoints=npoints;

	reader->readline();
	std::string scalName, typeName;
	reader->stream() >> tag >> scalName >> typeName;
	if (tag != "SCALARS" || reader->stream().bad()) {
		throw bad_format("Expecting SCALARS <name> <type>");
	}

	TypeInfo newTypeInfo(typeName.c_str());
	typeInfo = new TypeInfo(newTypeInfo.getId()); // newTypeInfo has limited scope
	if (typeInfo->getId() == TypeInfo::ID_UNKNOWN) {
		throw bad_format("Unsupported data type");
	}

	reader->readline();
	reader->stream() >> tag >> name;
	if (tag != "LOOKUP_TABLE" || reader->stream().bad()) {
		throw bad_format("Expecting LOOKUP_TABLE name");
	}
	if (name != "default") {
		unsigned size;
		reader->stream() >> size;
	}
}

template<typename T>
void LoadImage3DMPI<T>::report(YAML_Doc &doc) const {
	// Console output,
	CLOG(logYAML) << "File x-dimension: " << static_cast<long long int>(xdimFile);
	CLOG(logYAML) << "File y-dimension: " << static_cast<long long int>(ydimFile);
	CLOG(logYAML) << "File z-dimension: " << static_cast<long long int>(zdimFile);

	CLOG(logYAML) << "Number of points in image volume: " << static_cast<long long int>(fileNpoints);

	CLOG(logINFO) << "File x-spacing: " << static_cast<long long int>(spacing[0]);
	CLOG(logINFO) << "File y-spacing: " << static_cast<long long int>(spacing[1]);
	CLOG(logINFO) << "File z-spacing: " << static_cast<long long int>(spacing[2]);

	CLOG(logINFO) << "File x-origin: " << static_cast<long long int>(origin[0]);
	CLOG(logINFO) << "File y-origin: " << static_cast<long long int>(origin[1]);
	CLOG(logINFO) << "File z-origin: " << static_cast<long long int>(origin[2]);

	// YAML output,
	doc.add("File x-dimension",static_cast<long long int>(xdimFile));
	doc.add("File y-dimension",static_cast<long long int>(ydimFile));
	doc.add("File z-dimension",static_cast<long long int>(zdimFile));

	doc.add("Number of points in image volume", static_cast<long long int>(fileNpoints));
}

template<typename T>
void LoadImage3DMPI<T>::setBlockExtent(const unsigned * blkExt) {
	if (blkExt[0] > xdimFile || blkExt[1] > xdimFile) throw impossible_extent("x-axis extent doesn't make sense");
	if (blkExt[2] > ydimFile || blkExt[3] > ydimFile) throw impossible_extent("y-axis extent doesn't make sense");
	if (blkExt[4] > zdimFile || blkExt[5] > zdimFile) throw impossible_extent("z-axis extent doesn't make sense");

	for (int iExt=0;iExt<6;++iExt) {
		blockExtent[iExt]=blkExt[iExt];
	}
	// add one because the march extent stops 1 unit before the end
	nPointsInBlock=blockExtent[1]-blockExtent[0]+2;
	nPointsInBlock*=blockExtent[3]-blockExtent[2]+2;
	nPointsInBlock*=blockExtent[5]-blockExtent[4]+2;
	blockExtentSet=true;
}

template<typename T>
void LoadImage3DMPI<T>::readEntireVolumeData(Image3D<T>& image) {
	unsigned blkExt[6];
	blkExt[0]=blkExt[2]=blkExt[4]=0;
	blkExt[1]=xdimFile-2; // Because of the block extent definition
	blkExt[3]=ydimFile-2; // we remove first and last points, so -2
	blkExt[5]=zdimFile-2;
	this->setBlockExtent(blkExt);
	this->readBlockData(image);
}

template<typename T>
void LoadImage3DMPI<T>::readBlockData(Image3D<T>& image) {
	if (!blockExtentSet) throw block_extent_not_set("Set block extent first");

	// Get to initial position
	unsigned npointsIgnore =
        blockExtent[0]+ (blockExtent[2] * xdimFile) + (blockExtent[4] * xdimFile*ydimFile);
	this->streamIgnore(npointsIgnore);

	unsigned nXpoints = blockExtent[1]-blockExtent[0]+2; // + 2 for first and last pts
	unsigned nYpoints = blockExtent[3]-blockExtent[2]+2;
	unsigned nZpoints = blockExtent[5]-blockExtent[4]+2;

	unsigned nXpointsIgnore = xdimFile - nXpoints;
	unsigned nYpointsIgnore = xdimFile*(ydimFile-nYpoints);

	size_t readXlineSize = nXpoints * typeInfo->size();
	size_t totalReadSize = nPointsInBlock * typeInfo->size();
	std::vector<char> rbufRead(totalReadSize);

	unsigned iYline;
	unsigned iZline;

	for (iZline=0;iZline < nZpoints;++iZline) {
		for (iYline=0;iYline < nYpoints;++iYline) {
			// Read a line along the x-dimension
			stream.read(&rbufRead[imageDataIdx], readXlineSize);

			// Ignore the rest of the line
			this->streamIgnore(nXpointsIgnore);
			// Update rbufRead location
			imageDataIdx+=readXlineSize;
		}
		// Ignore the x-axis lines outside the extent
		this->streamIgnore(nYpointsIgnore);
	}

	image.setDimension(nXpoints, nYpoints, nZpoints);
	image.setMPIorigin(blockExtent[0],blockExtent[1],blockExtent[2]);
	image.setSpacing(spacing[0], spacing[1], spacing[2]);
	image.setOrigin(origin[0], origin[1], origin[2]);
	image.allocate();

	convertBufferWithTypeInfo(&rbufRead[0], *typeInfo, nPointsInBlock, image.getData());
}

template<typename T>
void LoadImage3DMPI<T>::streamIgnore(unsigned nPointsIgnore) {
	// This operation is prone to data type overflow
	unsigned iPointIgnore = 0;
	unsigned iPtIgnoreIncrement=134217728; // Read at most 1 GB at a time

	// Will ignore data in chunks of 1 GB
	for (;iPointIgnore<nPointsIgnore;iPointIgnore+=iPtIgnoreIncrement) {
		unsigned diffIgnore = nPointsIgnore-iPointIgnore;
		if (diffIgnore>iPtIgnoreIncrement) {
			size_t ignoreSize = iPtIgnoreIncrement * typeInfo->size();
			std::vector<char> rbufIgnore(ignoreSize);
			stream.read(&rbufIgnore[0], ignoreSize);
		}
		else {
			size_t ignoreSize = diffIgnore * typeInfo->size();
			std::vector<char> rbufIgnore(ignoreSize);
			stream.read(&rbufIgnore[0], ignoreSize);
		}
	}
}

template<typename T>
unsigned const* LoadImage3DMPI<T>::getVolumeDimensions(void) {
	if (xdimFile+ydimFile+zdimFile==0) throw block_extent_not_set("@ LoadImage3DMPI<T>::getVolumeDimensions");
	dimsArray[0]=xdimFile;
	dimsArray[1]=ydimFile;
	dimsArray[2]=zdimFile;
	return dimsArray;
}

template<typename T>
unsigned LoadImage3DMPI<T>::getMaxVoumeDimension(void) const {
	unsigned maxDimension;
	maxDimension=std::max(xdimFile,std::max(ydimFile,zdimFile));
	return maxDimension;
}

template<typename T>
unsigned LoadImage3DMPI<T>::getnVolumePoints(void) const {
	return fileNpoints;
}

template<typename T>
const T* LoadImage3DMPI<T>::getSpacing(void) const {
	return spacing;
}

template<typename T>
const T* LoadImage3DMPI<T>::getOrigin(void) const {
	return origin;
}

// Must instantiate class for separate compilation
template class LoadImage3DMPI<float_t> ;
