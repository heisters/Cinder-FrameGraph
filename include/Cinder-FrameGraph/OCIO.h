#pragma once
#include "OpenColorIO/OpenColorIO.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"

#include "Cinder-FrameGraph.h"
#include "Cinder-FrameGraph/NodeContainer.h"
#include "Cinder-FrameGraph/Types.h"

namespace cinder {
namespace frame_graph {
namespace ocio {
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



//! A node that does basic processing.
class ProcessIONode : public Node< Inlets< Surface32fRef >, Outlets< Surface32fRef > >
{
public:
	static ProcessIONodeRef create( const Config & config,
		const std::string & src,
		const std::string & dst )
	{
		return std::make_shared< ProcessIONode >( config, src, dst );
	}

	ProcessIONode( const Config & config,
		const std::string & src,
		const std::string & dst );

	virtual void update( const Surface32fRef & image );

	const Config & getConfig() const { return mConfig; }
private:
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

	ProcessGPUIONode( const Config & config );

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

}
}
}
