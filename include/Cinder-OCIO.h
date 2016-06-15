#pragma once

#include "cinder/Surface.h"
#include "OpenColorIO/OpenColorIO.h"

namespace cinder {
	namespace ocio {
		namespace core = OCIO_NAMESPACE;

		class Config
		{
		public:
			Config( const ci::fs::path & path )
			{
				mConfig = core::Config::CreateFromFile( path.string().c_str() );
			}


			core::ConstConfigRcPtr	get() const { return mConfig; }
			core::ConstConfigRcPtr	operator->() const { return get(); }

		private:
			core::ConstConfigRcPtr	mConfig;
		};


		void process( const Config & config, Surface32f *surface )
		{
			core::ConstProcessorRcPtr processor = config->getProcessor( core::ROLE_COMPOSITING_LOG,
																	   core::ROLE_SCENE_LINEAR );

			core::PackedImageDesc img( surface->getData(), surface->getWidth(), surface->getHeight(), 4 );
			processor->apply( img );
		}
	}
}