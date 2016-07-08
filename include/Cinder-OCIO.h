#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "OpenColorIO/OpenColorIO.h"

namespace cinder { namespace ocio {
namespace core = OCIO_NAMESPACE;

typedef std::shared_ptr< class Node >			NodeRef;
typedef std::shared_ptr< class ONode >			ONodeRef;
typedef std::shared_ptr< class INode >			INodeRef;
typedef std::shared_ptr< class ImageONode >		ImageONodeRef;
typedef std::shared_ptr< class ImageINode >		ImageINodeRef;
typedef std::shared_ptr< class ImageIONode >	ImageIONodeRef;
typedef std::shared_ptr< class ProcessIONode >	ProcessIONodeRef;
typedef std::shared_ptr< class SurfaceINode >	SurfaceINodeRef;
typedef std::shared_ptr< class TextureONode >	TextureONodeRef;

//! A reference to an OCIO configuration file.
class Config
{
public:
	//! Construct a new configuration reference from a file on disk.
	Config( const ci::fs::path & path );

	//! Returns the underlying OCIO configuration pointer.
	core::ConstConfigRcPtr	get() const { return mConfig; }

	//! Allows indirect access to the underlying OCIO configuration.
	core::ConstConfigRcPtr	operator->() const { return get(); }

private:
	core::ConstConfigRcPtr	mConfig;
};

//! Abstract base class for all nodes.
class Node : private Noncopyable
{
public:
};

//! An output node accepts input from a single node.
class ONode : public Node, public std::enable_shared_from_this< ONode >
{
public:
	//! Set this node's \a input.
	virtual void connect( const INodeRef & input );

	virtual void update() {};

private:
	INodeRef mInput = nullptr;
};

//! An input node provides input to many outputs.
class INode : public Node, public std::enable_shared_from_this< INode >
{
public:
	//! Add an \a output to this node.
	virtual void connect( const ONodeRef & output );

	//!
	virtual void update();

private:
	std::vector< ONodeRef > mOutputs;
};


//! A node that accepts an image input.
class ImageONode : public ONode
{
public:
	//! Descendants must do something with an image in their update.
	virtual void update( Surface32fRef & image ) = 0;
};

//! A node that provides an image as input to its outputs.
class ImageINode : public INode
{
public:
	virtual void update() = 0;

	virtual void connect( const ImageONodeRef & onode );
protected:
	std::vector< ImageONodeRef > mOutputs;
};


//! A node that accepts an image input, and then provides an image to its outputs.
class ImageIONode : public ImageONode, public ImageINode
{
public:
	using ONode::connect;
	using ImageINode::connect;
};


//! A node that does basic processing.
class ProcessIONode : public ImageIONode
{
public:
	static ProcessIONodeRef create(const Config & config,
								   const std::string & src,
								   const std::string & dst)
	{
		return std::make_shared< ProcessIONode >( config, src, dst );
	}

	ProcessIONode(const Config & config,
				  const std::string & src,
				  const std::string & dst);

	virtual void update( Surface32fRef & image ) override;
private:
	virtual void update() override {};

	Config						mConfig;
	core::ConstProcessorRcPtr	mProcessor;
};

//! A node that represents a Cinder Surface32f.
class SurfaceINode : public ImageINode
{
public:
	static SurfaceINodeRef create( const Surface32fRef & surface )
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
	static TextureONodeRef create()
	{
		return std::make_shared< TextureONode >();
	}

	TextureONode();
	
	virtual void update( Surface32fRef & image ) override;

	const ci::gl::Texture2dRef getTexture() const { return mTexture; }

	operator const ci::gl::Texture2dRef () const { return getTexture(); }
private:

	ci::gl::Texture2dRef	mTexture = nullptr;
};


namespace operators {
	template< typename in_t, typename out_t >
	inline const out_t& operator>>( const in_t & input, const out_t & output )
	{
		input->connect( output );
		return output;
	}
}

} }