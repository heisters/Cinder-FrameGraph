#pragma once

#include "Cinder-FrameGraph.h"

typedef ci::frame_graph::ref< class CubeONode > CubeONodeRef;


/*
 
 An example of a class that extends one of the provided node classes. This acts
 like a texture input, but generates the content of the texture by rendering a
 GL batch.
 
 */
class CubeONode : public ci::frame_graph::TextureINode {
public:
	static CubeONodeRef create()
	{
		return std::make_shared< CubeONode >();
	}

	CubeONode();
	virtual void update() override;

	void resize( const ci::ivec2 & size );

private:
	ci::gl::BatchRef	mBatch;
	ci::CameraPersp		mCam;
	ci::gl::FboRef		mFbo;
};
