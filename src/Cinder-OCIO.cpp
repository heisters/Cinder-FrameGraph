#include "Cinder-OCIO.h"
#include "cinder/Log.h"

using namespace cinder;
using namespace ocio;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Config

Config::Config( const ci::fs::path & path )
{
	mConfig = core::Config::CreateFromFile( path.string().c_str() );
}

////////////////////////////////////////////////////////////////////////////////
// ONode

void ONode::connect( const INodeRef & input )
{
	if ( mInput ) {
		throw runtime_error( "Cannot connect an input to an output that already has an input." );
	}
	mInput = input;
}


////////////////////////////////////////////////////////////////////////////////
// INode

void INode::update()
{
	for ( auto & output : mOutputs ) output->update();
}

void INode::connect( const ONodeRef & output )
{
	output->connect( shared_from_this() );
	mOutputs.push_back( output );
}

////////////////////////////////////////////////////////////////////////////////
// ImageINode
void ImageINode::connect( const ImageONodeRef & output )
{
	output->connect( shared_from_this() );
	mOutputs.push_back( output );
}

////////////////////////////////////////////////////////////////////////////////
// ProcessIONode


ProcessIONode::ProcessIONode(const Config &config,
							 const std::string & src,
							 const std::string &dst) :
mConfig( config )
{
	mProcessor = mConfig->getProcessor( src.c_str(), dst.c_str() );
}

void ProcessIONode::update( Surface32fRef & image )
{
	core::PackedImageDesc pid( image->getData(), image->getWidth(), image->getHeight(), image->hasAlpha() ? 4 : 3 );

	mProcessor->apply( pid );

	for ( auto & output : mOutputs ) output->update( image );
}


////////////////////////////////////////////////////////////////////////////////
// SurfaceINode

SurfaceINode::SurfaceINode( const Surface32fRef & surface ) :
mSurface( surface )
{
}

void SurfaceINode::update()
{
	for ( auto & output : mOutputs ) output->update( mSurface );
}


////////////////////////////////////////////////////////////////////////////////
// TextureONode

TextureONode::TextureONode()
{}

void TextureONode::update( Surface32fRef & image )
{
	mTexture = gl::Texture2d::create( *image );
}
