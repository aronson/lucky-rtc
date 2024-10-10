#ifndef BN_SPRITE_ITEMS_FULL_H
#define BN_SPRITE_ITEMS_FULL_H

#include "bn_sprite_item.h"

//{{BLOCK(full_bn_gfx)

//======================================================================
//
//	full_bn_gfx, 16x16@4, 
//	+ palette 16 entries, not compressed
//	+ 4 tiles not compressed
//	Total size: 32 + 128 = 160
//
//	Time-stamp: 2024-10-09, 16:20:06
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_FULL_BN_GFX_H
#define GRIT_FULL_BN_GFX_H

#define full_bn_gfxTilesLen 128
extern const bn::tile full_bn_gfxTiles[4];

#define full_bn_gfxPalLen 32
extern const bn::color full_bn_gfxPal[16];

#endif // GRIT_FULL_BN_GFX_H

//}}BLOCK(full_bn_gfx)

namespace bn::sprite_items
{
    constexpr inline sprite_item full(sprite_shape_size(sprite_shape::SQUARE, sprite_size::NORMAL), 
            sprite_tiles_item(span<const tile>(full_bn_gfxTiles, 4), bpp_mode::BPP_4, compression_type::NONE, 1), 
            sprite_palette_item(span<const color>(full_bn_gfxPal, 16), bpp_mode::BPP_4, compression_type::NONE));
}

#endif

