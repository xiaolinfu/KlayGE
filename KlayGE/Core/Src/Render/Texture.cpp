// Texture.cpp
// KlayGE 纹理类 实现文件
// Ver 2.4.0
// 版权所有(C) 龚敏敏, 2005
// Homepage: http://klayge.sourceforge.net
//
// 2.4.0
// 初次建立 (2005.3.6)
//
// 修改记录
//////////////////////////////////////////////////////////////////////////////////

#include <KlayGE/KlayGE.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/ResLoader.hpp>
#include <KlayGE/Util.hpp>

#include <fstream>

#include <KlayGE/Texture.hpp>

namespace
{
	using namespace KlayGE;

#pragma pack(push, 1)

	enum
	{
		// The surface has alpha channel information in the pixel format.
		DDSPF_ALPHAPIXELS = 0x00000001,

		// The FourCC code is valid.
		DDSPF_FOURCC = 0x00000004,

		// The RGB data in the pixel format structure is valid.
		DDSPF_RGB = 0x00000040,

		// Luminance data in the pixel format is valid.
		// Use this flag for luminance-only or luminance+alpha surfaces,
		// the bit depth is then ddpf.dwLuminanceBitCount.
		DDSPF_LUMINANCE = 0x00020000,
	};

	struct DDSPIXELFORMAT
	{
		uint32_t	size;				// size of structure
		uint32_t	flags;				// pixel format flags
		uint32_t	four_cc;			// (FOURCC code)
		uint32_t	rgb_bit_count;		// how many bits per pixel
		uint32_t	r_bit_mask;			// mask for red bit
		uint32_t	g_bit_mask;			// mask for green bits
		uint32_t	b_bit_mask;			// mask for blue bits
		uint32_t	rgb_alpha_bit_mask;	// mask for alpha channels
	};

	enum
	{
		// Indicates a complex surface structure is being described.  A
		// complex surface structure results in the creation of more than
		// one surface.  The additional surfaces are attached to the root
		// surface.  The complex structure can only be destroyed by
		// destroying the root.
		DDSCAPS_COMPLEX		= 0x00000008,

		// Indicates that this surface can be used as a 3D texture.  It does not
		// indicate whether or not the surface is being used for that purpose.
		DDSCAPS_TEXTURE		= 0x00001000,

		// Indicates surface is one level of a mip-map. This surface will
		// be attached to other DDSCAPS_MIPMAP surfaces to form the mip-map.
		// This can be done explicitly, by creating a number of surfaces and
		// attaching them with AddAttachedSurface or by implicitly by CreateSurface.
		// If this bit is set then DDSCAPS_TEXTURE must also be set.
		DDSCAPS_MIPMAP		= 0x00400000,
	};

	enum
	{
		// This flag is used at CreateSurface time to indicate that this set of
		// surfaces is a cubic environment map
		DDSCAPS2_CUBEMAP	= 0x00000200,

		// These flags preform two functions:
		// - At CreateSurface time, they define which of the six cube faces are
		//   required by the application.
		// - After creation, each face in the cubemap will have exactly one of these
		//   bits set.
		DDSCAPS2_CUBEMAP_POSITIVEX	= 0x00000400,
		DDSCAPS2_CUBEMAP_NEGATIVEX	= 0x00000800,
		DDSCAPS2_CUBEMAP_POSITIVEY	= 0x00001000,
		DDSCAPS2_CUBEMAP_NEGATIVEY	= 0x00002000,
		DDSCAPS2_CUBEMAP_POSITIVEZ	= 0x00004000,
		DDSCAPS2_CUBEMAP_NEGATIVEZ	= 0x00008000,

		// Indicates that the surface is a volume.
		// Can be combined with DDSCAPS_MIPMAP to indicate a multi-level volume
		DDSCAPS2_VOLUME		= 0x00200000,
	};

	struct DDSCAPS2
	{
		uint32_t	caps;			// capabilities of surface wanted
		uint32_t	caps2;
		uint32_t	reserved[2];
	};

	enum
	{
		DDSD_CAPS			= 0x00000001,	// default, dds_caps field is valid.
		DDSD_HEIGHT			= 0x00000002,	// height field is valid.
		DDSD_WIDTH			= 0x00000004,	// width field is valid.
		DDSD_PITCH			= 0x00000008,	// pitch is valid.
		DDSD_PIXELFORMAT	= 0x00001000,	// pixel_format is valid.
		DDSD_MIPMAPCOUNT	= 0x00020000,	// mip_map_count is valid.
		DDSD_LINEARSIZE		= 0x00080000,	// linear_size is valid
		DDSD_DEPTH			= 0x00800000,	// depth is valid
	};

	struct DDSSURFACEDESC2
	{
		uint32_t	size;					// size of the DDSURFACEDESC structure
		uint32_t	flags;					// determines what fields are valid
		uint32_t	height;					// height of surface to be created
		uint32_t	width;					// width of input surface
		union
		{
			int32_t		pitch;				// distance to start of next line (return value only)
			uint32_t	linear_size;		// Formless late-allocated optimized surface size
		};
		uint32_t		depth;				// the depth if this is a volume texture 
		uint32_t		mip_map_count;		// number of mip-map levels requestde
		uint32_t		reserved1[11];		// reserved
		DDSPIXELFORMAT	pixel_format;		// pixel format description of the surface
		DDSCAPS2		dds_caps;			// direct draw surface capabilities
		uint32_t		reserved2;
	};

#pragma pack(pop)
}

namespace KlayGE
{
	// 载入DDS格式文件
	TexturePtr LoadTexture(std::string const & tex_name)
	{
		boost::shared_ptr<std::istream> file(ResLoader::Instance().Load(tex_name));

		uint32_t magic;
		file->read(reinterpret_cast<char*>(&magic), sizeof(magic));
		assert((MakeFourCC<'D', 'D', 'S', ' '>::value) == magic);

		DDSSURFACEDESC2 desc;
		file->read(reinterpret_cast<char*>(&desc), sizeof(desc));

		assert((desc.flags & DDSD_CAPS) != 0);
		assert((desc.flags & DDSD_PIXELFORMAT) != 0);
		assert((desc.flags & DDSD_WIDTH) != 0);
		assert((desc.flags & DDSD_HEIGHT) != 0);

		if (0 == (desc.flags & DDSD_MIPMAPCOUNT))
		{
			desc.mip_map_count = 1;
		}

		PixelFormat format = PF_A8R8G8B8;
		if ((desc.pixel_format.flags & DDSPF_FOURCC) != 0)
		{
			switch (desc.pixel_format.four_cc)
			{
			case 111:
				format = PF_R16F;
				break;

			case 112:
				format = PF_G16R16F;
				break;

			case 113:
				format = PF_A16B16G16R16F;
				break;

			case 114:
				format = PF_R32F;
				break;

			case 115:
				format = PF_G32R32F;
				break;

			case 116:
				format = PF_A32B32G32R32F;
				break;

			case MakeFourCC<'D', 'X', 'T', '1'>::value:
				format = PF_DXT1;
				break;

			case MakeFourCC<'D', 'X', 'T', '3'>::value:
				format = PF_DXT3;
				break;

			case MakeFourCC<'D', 'X', 'T', '5'>::value:
				format = PF_DXT5;
				break;
			}
		}
		else
		{
			if ((desc.pixel_format.flags & DDSPF_RGB) != 0)
			{
				switch (desc.pixel_format.rgb_bit_count)
				{
				case 16:
					if ((0xF800 == desc.pixel_format.r_bit_mask)
						&& (0x7E0 == desc.pixel_format.g_bit_mask)
						&& (0x1F == desc.pixel_format.b_bit_mask))
					{
						format = PF_R5G6B5;
					}
					else
					{
						if ((0xF000 == desc.pixel_format.rgb_alpha_bit_mask)
							&& (0x0F00 == desc.pixel_format.r_bit_mask)
							&& (0x00F0 == desc.pixel_format.g_bit_mask)
							&& (0x000F == desc.pixel_format.b_bit_mask))
						{
							format = PF_A4R4G4B4;
						}
						else
						{
							assert(false);
						}
					}
					break;

				case 32:
					if ((0x00FF0000 == desc.pixel_format.r_bit_mask)
						&& (0x0000FF00 == desc.pixel_format.g_bit_mask)
						&& (0x000000FF == desc.pixel_format.b_bit_mask))
					{
						if ((desc.pixel_format.flags & DDSPF_ALPHAPIXELS) != 0)
						{
							format = PF_A8R8G8B8;
						}
						else
						{
							format = PF_X8R8G8B8;
						}
					}
					else
					{
						if ((0xC0000000 == desc.pixel_format.rgb_alpha_bit_mask)
							&& (0x3FF00000 == desc.pixel_format.r_bit_mask)
							&& (0x000FFC00 == desc.pixel_format.g_bit_mask)
							&& (0x000003FF == desc.pixel_format.b_bit_mask))
						{
							format = PF_A2R10G10B10;
						}
						else
						{
							assert(false);
						}
					}
					break;
				}
			}
			else
			{
				if ((desc.pixel_format.flags & DDSPF_LUMINANCE) != 0)
				{
					switch (desc.pixel_format.rgb_bit_count)
					{
					case 8:
						if ((desc.pixel_format.flags & DDSPF_ALPHAPIXELS) != 0)
						{
							format = PF_A4L4;
						}
						else
						{
							format = PF_L8;
						}
						break;

					case 16:
						if ((desc.pixel_format.flags & DDSPF_ALPHAPIXELS) != 0)
						{
							format = PF_A8L8;
						}
						break;

					default:
						assert(false);
						break;
					}
				}
				else
				{
					if ((desc.pixel_format.flags & DDSPF_ALPHAPIXELS) != 0)
					{
						format = PF_A8;
					}
					else
					{
						assert(false);
					}
				}
			}
		}
		
		uint32_t main_image_size;
		if ((desc.flags & DDSD_LINEARSIZE) != 0)
		{
			main_image_size = desc.linear_size;
		}
		else
		{
			if ((desc.flags & DDSD_PITCH) != 0)
			{
				main_image_size = desc.pitch * desc.height;
			}
			else
			{
				if ((desc.flags & desc.pixel_format.flags & 0x00000040) != 0)
				{
					main_image_size = desc.width * desc.height * desc.pixel_format.rgb_bit_count / 8;
				}
				else
				{
					main_image_size = desc.width * desc.height * PixelFormatBits(format) / 8;
				}
			}
		}

		KlayGE::TexturePtr texture;

		if ((desc.dds_caps.caps2 & DDSCAPS2_CUBEMAP) != 0)
		{
			texture = Context::Instance().RenderFactoryInstance().MakeTextureCube(desc.width,
					static_cast<uint16_t>(desc.mip_map_count), format);
		}
		else
		{
			if ((desc.dds_caps.caps2 & DDSCAPS2_VOLUME) != 0)
			{
				texture = Context::Instance().RenderFactoryInstance().MakeTexture3D(desc.width,
					desc.height, desc.depth, static_cast<uint16_t>(desc.mip_map_count), format);
			}
			else
			{
				texture = Context::Instance().RenderFactoryInstance().MakeTexture2D(desc.width,
					desc.height, static_cast<uint16_t>(desc.mip_map_count), format);
			}
		}

		switch (texture->Type())
		{
		case Texture::TT_1D:
		case Texture::TT_2D:
			{
				for (uint32_t level = 0; level < desc.mip_map_count; ++ level)
				{
					uint32_t image_size;
					if (IsCompressedFormat(format))
					{
						int block_size;
						if (PF_DXT1 == format)
						{
							block_size = 8;
						}
						else
						{
							block_size = 16;
						}

						image_size = ((desc.width / (1UL << level) + 3) / 4) * ((desc.height / (1UL << level) + 3) / 4) * block_size;
					}
					else
					{
						image_size = main_image_size / (1UL << (level * 2));
					}

					std::vector<uint8_t> data(image_size);

					file->read(reinterpret_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
					assert(file->gcount() == data.size());

					texture->CopyMemoryToTexture2D(level, &data[0], format,
						texture->Width() / (1UL << level), texture->Height() / (1UL << level), 0, 0);
				}
			}
			break;

		case Texture::TT_3D:
			{
				for (uint32_t level = 0; level < desc.mip_map_count; ++ level)
				{
					uint32_t image_size;
					if (IsCompressedFormat(format))
					{
						int block_size;
						if (PF_DXT1 == format)
						{
							block_size = 8;
						}
						else
						{
							block_size = 16;
						}

						image_size = ((desc.width / (1UL << level) + 3) / 4) * ((desc.height / (1UL << level) + 3) / 4) * block_size;
					}
					else
					{
						image_size = main_image_size / (1UL << (level * 2));
					}
				
					std::vector<uint8_t> data(image_size * texture->Depth());

					file->read(reinterpret_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
					assert(file->gcount() == data.size());

					texture->CopyMemoryToTexture3D(level, &data[0], format,
						texture->Width() / (1UL << level), texture->Height() / (1UL << level),
						texture->Depth() / (1UL << level), 0, 0, 0);
				}
			}
			break;

		case Texture::TT_Cube:
			{
				for (uint32_t face = Texture::CF_Positive_X; face < Texture::CF_Negative_Z; ++ face)
				{
					for (uint32_t level = 0; level < desc.mip_map_count; ++ level)
					{
						uint32_t image_size;
						if (IsCompressedFormat(format))
						{
							int block_size;
							if (PF_DXT1 == format)
							{
								block_size = 8;
							}
							else
							{
								block_size = 16;
							}

							image_size = ((desc.width / (1UL << level) + 3) / 4) * ((desc.height / (1UL << level) + 3) / 4) * block_size;
						}
						else
						{
							image_size = main_image_size / (1UL << (level * 2));
						}
					
						std::vector<uint8_t> data(image_size);

						file->read(reinterpret_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
						assert(file->gcount() == data.size());

						texture->CopyMemoryToTextureCube(static_cast<Texture::CubeFaces>(face),
							level, &data[0], format, texture->Width() / (1UL << level), 0);
					}
				}
			}
			break;
		}

		return texture;		
	}

	// 把纹理保存入DDS文件
	void SaveToFile(TexturePtr texture, std::string const & tex_name)
	{
		std::ofstream file(tex_name.c_str(), std::ios_base::binary);

		uint32_t magic = MakeFourCC<'D', 'D', 'S', ' '>::value;
		file.write(reinterpret_cast<char*>(&magic), sizeof(magic));

		DDSSURFACEDESC2 desc;
		std::memset(&desc, 0, sizeof(desc));

		desc.flags |= DDSD_CAPS;
		desc.flags |= DDSD_PIXELFORMAT;
		desc.flags |= DDSD_WIDTH;
		desc.flags |= DDSD_HEIGHT;

		if (texture->NumMipMaps() != 0)
		{
			desc.flags |= DDSD_MIPMAPCOUNT;
			desc.mip_map_count = texture->NumMipMaps();
		}

		if (IsFloatFormat(texture->Format()) || IsCompressedFormat(texture->Format()))
		{
			desc.pixel_format.flags |= DDSPF_FOURCC;

			switch (texture->Format())
			{
			case PF_R16F:
				desc.pixel_format.four_cc = 111;
				break;

			case PF_G16R16F:
				desc.pixel_format.four_cc = 112;
				break;

			case PF_A16B16G16R16F:
				desc.pixel_format.four_cc = 113;
				break;

			case PF_R32F:
				desc.pixel_format.four_cc = 114;
				break;

			case PF_G32R32F:
				desc.pixel_format.four_cc = 115;
				break;

			case PF_A32B32G32R32F:
				desc.pixel_format.four_cc = 116;
				break;

			case PF_DXT1:
				desc.pixel_format.four_cc = MakeFourCC<'D', 'X', 'T', '1'>::value;
				break;

			case PF_DXT3:
				desc.pixel_format.four_cc = MakeFourCC<'D', 'X', 'T', '3'>::value;
				break;

			case PF_DXT5:
				desc.pixel_format.four_cc = MakeFourCC<'D', 'X', 'T', '5'>::value;
				break;
			}
		}
		else
		{
			switch (texture->Format())
			{
			case PF_R5G6B5:
				desc.pixel_format.flags |= DDSPF_RGB;
				desc.pixel_format.rgb_bit_count = 16;

				desc.pixel_format.rgb_alpha_bit_mask = 0;
				desc.pixel_format.r_bit_mask = 0xF800;
				desc.pixel_format.g_bit_mask = 0x7E0;
				desc.pixel_format.b_bit_mask = 0x1F;
				break;

			case PF_A4R4G4B4:
				desc.pixel_format.flags |= DDSPF_RGB;
				desc.pixel_format.rgb_bit_count = 16;

				desc.pixel_format.rgb_alpha_bit_mask = 0xF000;
				desc.pixel_format.r_bit_mask = 0xF800;
				desc.pixel_format.g_bit_mask = 0x7E0;
				desc.pixel_format.b_bit_mask = 0x1F;
				break;

			case PF_A8R8G8B8:
				desc.pixel_format.flags |= DDSPF_RGB;
				desc.pixel_format.flags |= DDSPF_ALPHAPIXELS;
				desc.pixel_format.rgb_bit_count = 32;

				desc.pixel_format.rgb_alpha_bit_mask = 0xFF000000;
				desc.pixel_format.r_bit_mask = 0x00FF0000;
				desc.pixel_format.g_bit_mask = 0x0000FF00;
				desc.pixel_format.b_bit_mask = 0x000000FF;
				break;

			case PF_X8R8G8B8:
				desc.pixel_format.flags |= DDSPF_RGB;
				desc.pixel_format.rgb_bit_count = 32;

				desc.pixel_format.rgb_alpha_bit_mask = 0xF000;
				desc.pixel_format.r_bit_mask = 0x00FF0000;
				desc.pixel_format.g_bit_mask = 0x0000FF00;
				desc.pixel_format.b_bit_mask = 0x000000FF;
				break;

			case PF_A2R10G10B10:
				desc.pixel_format.flags |= DDSPF_RGB;
				desc.pixel_format.rgb_bit_count = 32;

				desc.pixel_format.rgb_alpha_bit_mask = 0xC0000000;
				desc.pixel_format.r_bit_mask = 0x3FF00000;
				desc.pixel_format.g_bit_mask = 0x000FFC00;
				desc.pixel_format.b_bit_mask = 0x000003FF;
				break;

			case PF_A4L4:
				desc.pixel_format.flags |= DDSPF_LUMINANCE;
				desc.pixel_format.flags |= DDSPF_ALPHAPIXELS;
				desc.pixel_format.rgb_bit_count = 8;
				break;

			case PF_L8:
				desc.pixel_format.flags |= DDSPF_LUMINANCE;
				desc.pixel_format.rgb_bit_count = 8;
				break;

			case PF_A8L8:
				desc.pixel_format.flags |= DDSPF_LUMINANCE;
				desc.pixel_format.flags |= DDSPF_ALPHAPIXELS;
				desc.pixel_format.rgb_bit_count = 16;
				break;

			case PF_A8:
				desc.pixel_format.flags |= DDSPF_ALPHAPIXELS;
				break;
			}
		}
		
		uint32_t main_image_size = texture->Width() * texture->Height() * PixelFormatBits(texture->Format()) / 8;
		if (IsCompressedFormat(texture->Format()))
		{
			if (PF_DXT1 == texture->Format())
			{
				main_image_size = texture->Width() * texture->Height() / 2;
			}
			else
			{
				main_image_size = texture->Width() * texture->Height();
			}

			desc.flags |= DDSD_LINEARSIZE;
			desc.linear_size = main_image_size;
		}

		switch (texture->Type())
		{
		case Texture::TT_1D:
		case Texture::TT_2D:
			{
				for (uint32_t level = 0; level < desc.mip_map_count; ++ level)
				{
					uint32_t image_size;
					if (IsCompressedFormat(texture->Format()))
					{
						int block_size;
						if (PF_DXT1 == texture->Format())
						{
							block_size = 8;
						}
						else
						{
							block_size = 16;
						}

						image_size = ((desc.width / (1UL << level) + 3) / 4) * ((desc.height / (1UL << level) + 3) / 4) * block_size;
					}
					else
					{
						image_size = main_image_size / (1UL << (level * 2));
					}

					std::vector<uint8_t> data(image_size);

					texture->CopyToMemory(level, &data[0]);

					file.write(reinterpret_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
					assert(file.gcount() == data.size());
				}
			}
			break;

		case Texture::TT_3D:
			desc.dds_caps.caps2 |= DDSCAPS2_VOLUME;

			{
				for (uint32_t level = 0; level < desc.mip_map_count; ++ level)
				{
					uint32_t image_size;
					if (IsCompressedFormat(texture->Format()))
					{
						int block_size;
						if (PF_DXT1 == texture->Format())
						{
							block_size = 8;
						}
						else
						{
							block_size = 16;
						}

						image_size = ((desc.width / (1UL << level) + 3) / 4) * ((desc.height / (1UL << level) + 3) / 4) * block_size;
					}
					else
					{
						image_size = main_image_size / (1UL << (level * 2));
					}
				
					std::vector<uint8_t> data(image_size * texture->Depth());

					texture->CopyToMemory(level, &data[0]);

					file.write(reinterpret_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
					assert(file.gcount() == data.size());
				}
			}
			break;

		case Texture::TT_Cube:
			desc.dds_caps.caps2 |= DDSCAPS2_CUBEMAP;

			{
				for (uint32_t face = Texture::CF_Positive_X; face < Texture::CF_Negative_Z; ++ face)
				{
					for (uint32_t level = 0; level < desc.mip_map_count; ++ level)
					{
						uint32_t image_size;
						if (IsCompressedFormat(texture->Format()))
						{
							int block_size;
							if (PF_DXT1 == texture->Format())
							{
								block_size = 8;
							}
							else
							{
								block_size = 16;
							}

							image_size = ((desc.width / (1UL << level) + 3) / 4) * ((desc.height / (1UL << level) + 3) / 4) * block_size;
						}
						else
						{
							image_size = main_image_size / (1UL << (level * 2));
						}
					
						std::vector<uint8_t> data(image_size * 6);

						texture->CopyToMemory(level, &data[0]);

						file.write(reinterpret_cast<char*>(&data[face * image_size]), static_cast<std::streamsize>(image_size));
						assert(file.gcount() == image_size);
					}
				}
			}
			break;
		}
	}


	Texture::Texture(Texture::TextureUsage usage, Texture::TextureType type)
			: usage_(usage), type_(type)
	{
	}

	Texture::~Texture()
	{
	}

	uint16_t Texture::NumMipMaps() const
	{
		return numMipMaps_;
	}

	Texture::TextureUsage Texture::Usage() const
	{
		return usage_;
	}

	uint32_t Texture::Width() const
	{
		return width_;
	}

	uint32_t Texture::Height() const
	{
		return height_;
	}

	uint32_t Texture::Depth() const
	{
		return depth_;
	}

	uint32_t Texture::Bpp() const
	{
		return bpp_;
	}

	PixelFormat Texture::Format() const
	{
		return format_;
	}

	Texture::TextureType Texture::Type() const
	{
		return type_;
	}
}
