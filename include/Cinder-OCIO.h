#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "OpenColorIO/OpenColorIO.h"
#include "cinder/qtime/QuickTimeGl.h"

namespace cinder { namespace ocio {
namespace core = OCIO_NAMESPACE;

typedef std::shared_ptr< class Node >				NodeRef;
typedef std::shared_ptr< class ONode >				ONodeRef;
typedef std::shared_ptr< class INode >				INodeRef;
typedef std::shared_ptr< class ImageONode >			ImageONodeRef;
typedef std::shared_ptr< class ImageINode >			ImageINodeRef;
typedef std::shared_ptr< class ImageIONode >		ImageIONodeRef;
typedef std::shared_ptr< class ProcessIONode >		ProcessIONodeRef;
typedef std::shared_ptr< class SurfaceINode >		SurfaceINodeRef;
typedef std::shared_ptr< class TextureONode >		TextureONodeRef;
typedef std::shared_ptr< class QTMovieGlINode >		QTMovieGlINodeRef;
typedef std::shared_ptr< class ProcessGPUIONode >	ProcessGPUIONodeRef;

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

private:
	core::ConstConfigRcPtr		mConfig;
	std::vector< std::string >	mAllColorSpaceNames;
	std::vector< std::string >	mAllDisplayNames;
	std::map< std::string, std::vector< std::string > > mAllViewNames;
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
	virtual void update() {};
private:
};

//! An input node provides input to many outputs.
class INode : public Node, public std::enable_shared_from_this< INode >
{
public:
	//! Add an \a output to this node.
	virtual void connect( const ONodeRef & output );

	//! Remove an \a output from this node.
	virtual void disconnect( const ONodeRef & output );

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
	virtual void update( const Surface32fRef & image ) = 0;
};

//! A node that provides an image as input to its outputs.
class ImageINode : public INode
{
public:
	virtual void update() = 0;

	virtual void connect( const ImageONodeRef & output );
	virtual void disconnect( const ImageONodeRef & output );
protected:
	std::vector< ImageONodeRef > mOutputs;
};


//! A node that accepts an image input, and then provides an image to its outputs.
class ImageIONode : public ImageINode, public ImageONode
{
public:
	using ImageINode::connect;
	using ImageINode::disconnect;
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

//! A node that emits OpenGL textures.
class TextureINode : public INode
{
public:
	virtual void connect( const TextureONodeRef & output );
	virtual void disconnect( const TextureONodeRef & output );

	virtual void update() override;
protected:
	virtual void update( const ci::gl::Texture2dRef & texture );

private:
	std::vector< TextureONodeRef >	mOutputs;
	ci::gl::Texture2dRef			mOutTex;
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

	virtual void update( const Surface32fRef & image ) override;
	virtual void update( const ci::gl::Texture2dRef & texture );

	const ci::gl::Texture2dRef getTexture() const { return mTexture; }

	operator const ci::gl::Texture2dRef () const { return getTexture(); }
	operator const bool () const { return mTexture != nullptr; }

	ci::vec2 getSize() const { return mTexture->getSize(); }
private:
	
	ci::gl::Texture2dRef	mTexture = nullptr;
};


class TextureIONode : public TextureINode, public TextureONode
{
public:
	using TextureINode::connect;
	using TextureINode::disconnect;
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


//! Provides a quicktime video texture input.
class QTMovieGlINode : public TextureINode
{
public:
	static QTMovieGlINodeRef create( const ci::fs::path & path )
	{
		return std::make_shared< QTMovieGlINode >( path );
	}

	QTMovieGlINode( const ci::fs::path & path );

	virtual void update() override;


	QTMovieGlINode & loop( bool enabled = true ) { mMovie->setLoop( enabled ); return *this; }

	ci::vec2 getSize() const { return mMovie->getSize(); }

private:

	ci::qtime::MovieGlRef mMovie;
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