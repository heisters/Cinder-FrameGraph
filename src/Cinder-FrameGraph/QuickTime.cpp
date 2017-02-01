#include "Cinder-FrameGraph/QuickTime.h"

using namespace cinder;
using namespace frame_graph;
using namespace std;
using namespace qtime;

QTMovieGlINode::QTMovieGlINode( const fs::path & path, bool playImmediately ) :
mMovie( qtime::MovieGl::create( path ) )
{
	if ( playImmediately ) mMovie->play();
}

void QTMovieGlINode::update()
{
	auto tex = mMovie->getTexture();
	if ( tex ) TextureINode::update( tex );
}




QTMovieWriterONode::QTMovieWriterONode(const fs::path & path,
									   int32_t width,
									   int32_t height,
									   const MovieWriter::Format & format) :
mWriter( qtime::MovieWriter::create( path, width, height, format ) )
{
}

QTMovieWriterONode::~QTMovieWriterONode()
{
	finish();
}

void QTMovieWriterONode::update( const gl::Texture2dRef &texture )
{
	TextureONode::update( texture );

	if ( getTexture() ) {
		mWriter->addFrame( Surface8u( getTexture()->createSource() ) );
	}
}



QTThreadedMovieWriterONode::QTThreadedMovieWriterONode(const fs::path & path,
													   int32_t width,
													   int32_t height,
													   const MovieWriter::Format & format) :
QTMovieWriterONode( path, width, height, format ),
mThread( thread( bind( &QTThreadedMovieWriterONode::updateThread, this ) ) )
{

}

QTThreadedMovieWriterONode::~QTThreadedMovieWriterONode()
{
	finish();
	if ( mThread.joinable() ) mThread.join();
}

void QTThreadedMovieWriterONode::update( const gl::Texture2dRef & texture )
{
	TextureONode::update( texture );

	if ( getTexture() ) {
		mQueue.emplace( Frame{ getTexture()->createSource() } );
	}
}

void QTThreadedMovieWriterONode::updateThread()
{
	Frame f;
	while ( mRun || ! mQueue.empty() ) {
		if ( mQueue.try_pop( &f ) ) {
			mWriter->addFrame( Surface8u{ f.imageSource } );
		}
	}
}

void QTThreadedMovieWriterONode::finish()
{
	mRun = false;
}
