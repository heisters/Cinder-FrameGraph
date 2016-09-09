////////////////////////////////////////////////////////////////////////////////
// Config

Config::Config( const ci::fs::path & path )
{
	mConfig = core::Config::CreateFromFile( path.string().c_str() );

	{
		int n = mConfig->getNumColorSpaces();
		for ( int i = 0; i < n; ++i ) {
			mAllColorSpaceNames.push_back( string( mConfig->getColorSpaceNameByIndex( i ) ) );
		}
	}

	{
		int n = mConfig->getNumDisplays();
		for ( int i = 0; i < n; ++i ) {
			string display( mConfig->getDisplay( i ) );
			mAllDisplayNames.push_back( display );

			int nv = mConfig->getNumViews( display.c_str() );
			for ( int j = 0; j < nv; ++j ) {
				mAllViewNames[ display ].push_back( mConfig->getView( display.c_str(), j ) );
			}
		}
	}

}
////////////////////////////////////////////////////////////////////////////////
// ProcessIONode


ProcessIONode::ProcessIONode( const Config & config,
	const string & src,
	const string & dst ) :
	mConfig( config )
{
	mProcessor = mConfig->getProcessor( src.c_str(), dst.c_str() );
}

void ProcessIONode::update( const Surface32fRef & image )
{
	core::PackedImageDesc pid( image->getData(), image->getWidth(), image->getHeight(), image->hasAlpha() ? 4 : 3 );

	mProcessor->apply( pid );

	for ( auto & output : mOutputs ) output->update( image );
}

////////////////////////////////////////////////////////////////////////////////
// ProcessGPUIONode

static const std::string FRAG_SHADER_HEADER =
CI_GLSL( 150,
	vec4 texture3D( sampler3D s, vec3 p ) { return texture( s, p ); }
vec4 texture2D( sampler2D s, vec2 p ) { return texture( s, p ); }
uniform RAW_SAMPLER uRawTex;
uniform sampler3D uLUTTex;

in vec2 vTexCoord0;
out vec4 oColor;
);

static const std::string FRAG_SHADER_MAIN =
CI_GLSL( 150,
	void main()
{
	vec4 col = texture( uRawTex, vTexCoord0.st );
	oColor = OCIODisplay( col, uLUTTex );
}
);

static const std::string VERT_SHADER =
CI_GLSL( 150,
	uniform mat4	ciModelViewProjection;
uniform vec2	uTexCoordScale;
in vec4			ciPosition;
in vec2			ciTexCoord0;
out vec2		vTexCoord0;

void main( void ) {
	gl_Position = ciModelViewProjection * ciPosition;
	vTexCoord0 = ciTexCoord0 * uTexCoordScale;
}
);


ProcessGPUIONode::ProcessGPUIONode( const Config & config ) :
	mConfig( config )
{
	mCSInput = core::ROLE_SCENE_LINEAR;
	mCSDisplay = mConfig->getDefaultDisplay();
	mCSView = mConfig->getDefaultView( mCSDisplay.c_str() );

	mProcessorNeedsUpdate = true;
}

void ProcessGPUIONode::setInputColorSpace( const string &inputName )
{
	mCSInput = inputName;
	mProcessorNeedsUpdate = true;
}
void ProcessGPUIONode::setDisplayColorSpace( const string &displayName )
{
	mCSDisplay = displayName;

	setViewColorSpace( mConfig->getDefaultView( mCSDisplay.c_str() ) );

	mProcessorNeedsUpdate = true;
}
void ProcessGPUIONode::setViewColorSpace( const std::string &viewName )
{
	mCSView = viewName;
	mProcessorNeedsUpdate = true;
}
void ProcessGPUIONode::setExposureFStop( float exposure )
{
	mExposureFStop = exposure;
	mProcessorNeedsUpdate = true;
}

void ProcessGPUIONode::updateProcessor()
{
	if ( ! mProcessorNeedsUpdate ) return;

	core::ConstColorSpaceRcPtr cs_input = mConfig->getColorSpace( mCSInput.c_str() );
	if ( ! cs_input ) {
		mProcessor = nullptr;
		return;
	}

	core::ConstColorSpaceRcPtr cs_display = mConfig->getColorSpace( mCSDisplay.c_str() );
	if ( ! cs_display ) {
		mProcessor = nullptr;
		return;
	}




	// Setup the transform
	core::DisplayTransformRcPtr transform = core::DisplayTransform::Create();
	transform->setInputColorSpaceName( mCSInput.c_str() );
	transform->setDisplay( mCSDisplay.c_str() );
	transform->setView( mCSView.c_str() );

	// Apply exposure
	if ( mExposureFStop != 0.f ) {
		float gain = powf( 2.f, mExposureFStop );
		const float slope4f[] = { gain, gain, gain, gain };
		float m44[ 16 ];
		float offset4[ 4 ];
		core::MatrixTransform::Scale( m44, offset4, slope4f );
		core::MatrixTransformRcPtr mtx = core::MatrixTransform::Create();
		mtx->setValue( m44, offset4 );
		transform->setLinearCC( mtx );
	}

	core::ConstProcessorRcPtr processor;
	try {
		processor = mConfig->getProcessor( transform );
	}
	catch ( core::Exception & e ) {
		CI_LOG_E( "Error creating OCIO processor: " << e.what() );
		return;
	}

	mProcessor = processor;



	// Compute the LUT and store it in a texture.

	static const int sSize = 32;

	mShaderDesc.setLanguage( core::GPU_LANGUAGE_GLSL_1_3 );
	mShaderDesc.setFunctionName( "OCIODisplay" );
	mShaderDesc.setLut3DEdgeLen( sSize );

	int num3Dentries = 3 * sSize * sSize * sSize;
	vector< float > g_lut3d;
	g_lut3d.resize( num3Dentries );
	mProcessor->getGpuLut3D( &g_lut3d[ 0 ], mShaderDesc );

	{
		if ( mLUTTex ) {
			mLUTTex->update( g_lut3d.data(), GL_RGB, GL_FLOAT, 0, sSize, sSize, sSize );
		}
		else {
			gl::Texture3d::Format fmt;
			fmt.minFilter( GL_LINEAR ).magFilter( GL_LINEAR ).wrap( GL_CLAMP_TO_EDGE );
			fmt.setDataType( GL_FLOAT );
			fmt.setInternalFormat( GL_RGB16F_ARB );
			mLUTTex = gl::Texture3d::create( g_lut3d.data(), GL_RGB, sSize, sSize, sSize, fmt );
		}
	}


	mProcessorNeedsUpdate = false;
}


void ProcessGPUIONode::updateBatch( const BatchFormat & fmt )
{
	if ( mBatchFormat == fmt ) return;
	mBatchFormat = fmt;


	std::ostringstream os;
	os << FRAG_SHADER_HEADER << mProcessor->getGpuShaderText( mShaderDesc ) << FRAG_SHADER_MAIN;

	gl::GlslProg::Format shaderFmt;
	shaderFmt.vertex( VERT_SHADER ).fragment( os.str() );

	string sampler( "sampler2D" );
	if ( mBatchFormat.getTextureTarget() == GL_TEXTURE_RECTANGLE_ARB )
		sampler = "sampler2DRect";

	shaderFmt.define( "RAW_SAMPLER", sampler );


	auto shader = gl::GlslProg::create( shaderFmt );
	shader->uniform( "uRawTex", 0 );
	shader->uniform( "uLUTTex", 1 );

	vec2 texCoordScale( 1.f, 1.f );
	if ( mBatchFormat.getTextureTarget() == GL_TEXTURE_RECTANGLE_ARB )
		texCoordScale = mBatchFormat.getTextureSize();

	shader->uniform( "uTexCoordScale", texCoordScale );

	mBatch = gl::Batch::create( geom::Rect(), shader );
}

void ProcessGPUIONode::update( const gl::Texture2dRef & texture )
{
	updateProcessor();
	if ( ! mProcessor ) return;


	if ( ! mFbo || mFbo->getWidth() != texture->getWidth() || mFbo->getHeight() != texture->getHeight() ) {
		mFbo = gl::Fbo::create( texture->getWidth(), texture->getHeight() );
		mModelMatrix = scale( vec3( mFbo->getSize(), 1.f ) ) * translate( vec3( 0.5f, 0.5f, 0.f ) );
	}

	updateBatch( BatchFormat().textureTarget( texture->getTarget() ).textureSize( texture->getSize() ) );

	{
		gl::ScopedFramebuffer scp_fbo( mFbo );
		gl::ScopedViewport scp_viewport( mFbo->getSize() );
		gl::ScopedMatrices scp_matrices;
		gl::ScopedTextureBind scp_rawTex( texture, 0 );
		gl::ScopedTextureBind scp_lutTex( mLUTTex, 1 );

		gl::setMatricesWindow( mFbo->getWidth(), mFbo->getHeight(), false );
		gl::multModelMatrix( mModelMatrix );
		gl::clear();

		mBatch->draw();
	}

	TextureINode::update( mFbo->getColorTexture() );
}

