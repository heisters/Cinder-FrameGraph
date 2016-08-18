#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "OpenColorIO/OpenColorIO.h"

#include "Cinder-OCIO/NodeContainer.h"
#include "Cinder-OCIO/Types.h"

namespace cinder { namespace ocio {
namespace core = OCIO_NAMESPACE;

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

	//!	Returns the names of all available color spaces.
	const std::vector< std::string > & getAllColorSpaceNames() const { return mAllColorSpaceNames; }

	//! Returns the names of all available displays.
	const std::vector< std::string > & getAllDisplayNames() const { return mAllDisplayNames; }

	//! Returns true if there are views for the \a display.
	bool hasViewsForDisplay( const std::string & display ) { return mAllViewNames.find( display ) != mAllViewNames.end(); }

	//! Returns the names of all views for the given \a display.
	const std::vector< std::string > & getAllViewNames( const std::string & display ) const { return mAllViewNames.at( display );  }

	//! Returns the names of all looks for a display and view.
	const std::string getLooks( const std::string & display, const std::string & view ) const { return mConfig->getDisplayLooks( display.c_str(), view.c_str() ); }

	const std::vector< std::string > & getAllLookNames() const { return mAllLookNames; }

private:
	core::ConstConfigRcPtr		mConfig;
	std::vector< std::string >	mAllColorSpaceNames;
	std::vector< std::string >	mAllDisplayNames;
	std::map< std::string, std::vector< std::string > > mAllViewNames;
	std::vector< std::string >	mAllLookNames;
};



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
		for ( const auto & out : mOutputs ) disconnect( out );
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

	virtual void update( const Surface32fRef & image ) override;

	const Config & getConfig() const { return mConfig; }
private:
	virtual void update() override {};

	Config						mConfig;
	core::ConstProcessorRcPtr	mProcessor;
};

//! A node that does processing on the GPU.
class ProcessGPUIONode : public TextureIONode
{
public:
	static ProcessGPUIONodeRef create( const Config & config )
	{
		return std::make_shared< ProcessGPUIONode >( config );
	}

	ProcessGPUIONode(const Config & config );

	virtual void update( const ci::gl::Texture2dRef & texture ) override;

	const Config & getConfig() const { return mConfig; }

	void setInputColorSpace( const std::string &inputName );
	std::string getInputColorSpace() const { return mCSInput; }

	void setDisplayColorSpace( const std::string &displayName );
	std::string getDisplayColorSpace() const { return mCSDisplay; }

	void setViewColorSpace( const std::string &viewName );
	std::string getViewColorSpace() const { return mCSView; }

	void setLook( const std::string &look );
	std::string getLook() const { return mLook; }

	void setExposureFStop( float exposure );
private:
	class BatchFormat
	{
	public:
		BatchFormat & textureTarget( GLenum target ) { mTextureTarget = target; return *this; }
		GLenum getTextureTarget() const { return mTextureTarget; }

		BatchFormat & textureSize( const ci::vec2 & size ) { mTextureSize = size; return *this; }
		ci::vec2 getTextureSize() const { return mTextureSize; }

		bool operator == ( const BatchFormat & rhs ) const { return mTextureTarget == rhs.mTextureTarget && mTextureSize == rhs.mTextureSize; }

	private:
		GLenum		mTextureTarget;
		ci::vec2	mTextureSize;
	};

	void						updateBatch( const BatchFormat & fmt );
	void						updateProcessor();


	Config						mConfig;
	std::string					mCSInput;
	std::string					mCSDisplay;
	std::string					mCSView;
	std::string					mLook = "";
	float						mExposureFStop = 0.f;
	bool						mProcessorNeedsUpdate = false;

	core::ConstProcessorRcPtr	mProcessor = nullptr;
	core::GpuShaderDesc			mShaderDesc;
	ci::gl::Texture3dRef		mLUTTex = nullptr;
	ci::gl::FboRef				mFbo;
	ci::gl::BatchRef			mBatch;
	ci::mat4					mModelMatrix;
	BatchFormat					mBatchFormat;
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