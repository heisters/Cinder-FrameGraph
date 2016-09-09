#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"

#include "Cinder-OCIO/NodeContainer.h"
#include "Cinder-OCIO/Types.h"
#include "mapbox/variant.hpp"
#include "mapbox/variant_io.hpp"

namespace cinder { namespace ocio {

//! Abstract base class for all nodes.
class Node : private Noncopyable
{
public:
};

//! An output node accepts input from a single node.
class ONode : public Node
{
public:
	virtual void update() {};
private:
};

//! An input node provides input to many outputs.
template< class o_node_t >
class INode : public Node
{
public:
	typedef NodeContainer< o_node_t > output_container_t;

	//! Add an \a output to this node.
	virtual void connect( const o_node_t & output )
	{
		assert( output != nullptr );
		mOutputs.push_back( output );
	}

	//! Remove an \a output from this node.
	virtual void disconnect( const o_node_t & output )
	{
		mOutputs.erase( remove( mOutputs.begin(), mOutputs.end(), output ), mOutputs.end() );
	}

	//! Remove all outputs from this node.
	virtual void disconnect()
	{
		mOutputs.clear();
	}

	//! Updates all connected outputs. Override in order to pass data to output
	//! nodes.
	virtual void update()
	{
		for ( auto & output : mOutputs ) output->update();
	}

	//! Returns true if this node is connected to any outputs.
	bool isConnected() const { return ! mOutputs.empty(); }

protected:
	output_container_t mOutputs;
};

//! A node that takes multiple inputs.
template< class o_node_t, class o_node_ref_t = ref< o_node_t > >
class MuxONode
{
public:
	static MuxONodeRef< o_node_t > create( std::size_t size )
	{
		return std::make_shared< MuxONode< o_node_t > >( size );
	}

	MuxONode( std::size_t size )
	{
		mONodes.reserve( size );
		for ( size_t i = 0; i < size; ++i ) mONodes.push_back( o_node_t::create() );
	}

	const NodeContainer< o_node_ref_t > & onodes() { return mONodes; }

private:
	NodeContainer< o_node_ref_t >	mONodes;
};


//! A node that accepts an image input.
class ImageONode : public ONode
{
public:
	using ONode::update;
	//! Descendants must do something with an image in their update.
	virtual void update( const Surface32fRef & image ) = 0;
};
typedef ref< ImageONode > ImageONodeRef;

//! A node that provides an image as input to its outputs.
class ImageINode : public INode< ImageONodeRef >
{
public:
};


//! A node that accepts an image input, and then provides an image to its outputs.
class ImageIONode : public ImageINode, public ImageONode
{
public:
};

//! A node that represents a Cinder Surface32f.
class SurfaceINode : public ImageINode
{
public:
	static ref< SurfaceINode > create( const Surface32fRef & surface )
	{
		return std::make_shared< SurfaceINode >( surface );
	}

	SurfaceINode( const Surface32fRef & surface );

	virtual void update() override;

	const ci::Surface32fRef & getSurface() const { return mSurface; }

	operator const ci::Surface32fRef & () const { return getSurface(); }

private:
	ci::Surface32fRef		mSurface;
};


//! A node that represents a Cinder gl::Texture2d, useful for displaying
//! results.
class TextureONode : public ImageONode
{
public:
	static ref< TextureONode > create()
	{
		return std::make_shared< TextureONode >();
	}

	TextureONode();

	using ImageONode::update;
	virtual void update( const Surface32fRef & image ) override;
	virtual void update( const ci::gl::Texture2dRef & texture );

	const ci::gl::Texture2dRef getTexture() const { return mTexture; }

	operator const ci::gl::Texture2dRef () const { return getTexture(); }
	operator const bool () const { return mTexture != nullptr; }

	ci::ivec2 getSize() const { return mTexture->getSize(); }
private:

	ci::gl::Texture2dRef	mTexture = nullptr;
};
typedef ref< TextureONode > TextureONodeRef;


//! A node that emits OpenGL textures.
class TextureINode : public INode< TextureONodeRef >
{
public:
	virtual void update() override;
protected:
	virtual void update( const ci::gl::Texture2dRef & texture );

private:
	ci::gl::Texture2dRef			mOutTex;
};


class TextureIONode : public TextureINode, public TextureONode
{
public:
};


//! A node that applies a shader to a texture input.
class TextureShaderIONode : public TextureINode, public TextureONode
{
public:
	static ref< TextureShaderIONode > create( const ci::gl::GlslProgRef & shader )
	{
		return std::make_shared< TextureShaderIONode >( shader );
	}
	TextureShaderIONode( const ci::gl::GlslProgRef & shader );
	virtual void update( const ci::gl::Texture2dRef & texture ) override;

private:
	ci::gl::BatchRef mBatch;
	ci::gl::FboRef mFbo;
	ci::mat4 mModelMatrix;
};


// FIXME: oops! Compound is a better name than Composite to avoid confusion with
// image compositing
template < typename ... Types >
class CompositeNode
{
public:
	typedef mapbox::util::variant< Types... > variant;

	static CompositeNodeRef< Types... > create( const Types& ... nodes )
	{
		return std::make_shared< CompositeNode< Types... > >( nodes... );
	}

	CompositeNode( const Types& ... nodes ) :
		mNodes{ nodes... }
	{}

protected:
	std::vector< variant > mNodes;
};

namespace operators {
	template< typename in_t, typename out_t >
	inline const ref< out_t > & operator>>( const ref< in_t > & input, const ref< out_t > & output )
	{
		input->connect( output );
		return output;
	}
}

} }