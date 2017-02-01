#pragma once

#include "Cinder-FrameGraph.h"
#include "Cinder-FrameGraph/concurrent_queue.h"
#include "cinder/qtime/QuickTimeGl.h"
#include "cinder/qtime/AvfWriter.h"
#include <thread>
#include <atomic>

namespace cinder { namespace frame_graph {

typedef ref< class QTMovieGlINode >				QTMovieGlINodeRef;
typedef ref< class QTMovieWriterONode >			QTMovieWriterONodeRef;
typedef ref< class QTThreadedMovieWriterONode >	QTThreadedMovieWriterONodeRef;

//! Provides a quicktime video texture input.
class QTMovieGlINode : public TextureINode
{
public:
	static QTMovieGlINodeRef create( const ci::fs::path & path, bool playImmediately = true )
	{
		return std::make_shared< QTMovieGlINode >( path, playImmediately );
	}

	QTMovieGlINode( const ci::fs::path & path, bool playImmediately = true );

	virtual void update() override;


	QTMovieGlINode & play() { mMovie->play(); return *this; }
	QTMovieGlINode & stop() { mMovie->stop(); return *this; }
	QTMovieGlINode & loop( bool enabled = true ) { mMovie->setLoop( enabled ); return *this; }
	QTMovieGlINode & seekToTime( float seconds ) { mMovie->seekToTime( seconds ); return *this; }
	QTMovieGlINode & seekToFrame( int frame ) { mMovie->seekToFrame( frame ); return *this; }
	QTMovieGlINode & seekToStart() { mMovie->seekToStart(); return *this; }
	QTMovieGlINode & setRate( float rate ) { mMovie->setRate( rate ); return *this; }

	float getDuration() const { return mMovie->getDuration(); }
	float getCurrentTime() const { return mMovie->getCurrentTime(); }
	float getRemainingTime() const { return mMovie->getDuration() - mMovie->getCurrentTime(); }
	bool isPlaying() const { return mMovie->isPlaying(); }

	ci::vec2 getSize() const { return mMovie->getSize(); }

private:

	ci::qtime::MovieGlRef mMovie;
};


class QTMovieWriterONode : public TextureONode {
public:

	static QTMovieWriterONodeRef create(const ci::fs::path & path,
										int32_t width,
										int32_t height,
										const ci::qtime::MovieWriter::Format & format = ci::qtime::MovieWriter::Format())
	{
		return std::make_shared< QTMovieWriterONode >( path, width, height, format );
	}
	QTMovieWriterONode(const ci::fs::path & path,
					   int32_t width,
					   int32_t height,
					   const ci::qtime::MovieWriter::Format & format = ci::qtime::MovieWriter::Format());

	~QTMovieWriterONode();

	virtual void update( const ci::gl::Texture2dRef & texture ) override;
	virtual void finish() { mWriter->finish(); }

protected:
	ci::qtime::MovieWriterRef mWriter;
};


class QTThreadedMovieWriterONode : public QTMovieWriterONode {
public:

	static QTThreadedMovieWriterONodeRef create(const ci::fs::path & path,
										int32_t width,
										int32_t height,
										const ci::qtime::MovieWriter::Format & format = ci::qtime::MovieWriter::Format())
	{
		return std::make_shared< QTThreadedMovieWriterONode >( path, width, height, format );
	}
	QTThreadedMovieWriterONode(const ci::fs::path & path,
							   int32_t width,
							   int32_t height,
							   const ci::qtime::MovieWriter::Format & format = ci::qtime::MovieWriter::Format());

	~QTThreadedMovieWriterONode();


	virtual void update( const ci::gl::Texture2dRef & texture ) override;
	virtual void finish() override;

private:
	void updateThread();

	struct Frame {
		ci::ImageSourceRef imageSource;
	};
	concurrent_queue< Frame >	mQueue;
	std::thread					mThread;
	std::atomic_bool			mRun{ true };
};

} }
