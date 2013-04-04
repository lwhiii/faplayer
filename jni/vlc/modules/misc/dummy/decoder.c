/*****************************************************************************
 * decoder.c: dummy decoder plugin for vlc.
 *****************************************************************************
 * Copyright (C) 2002 the VideoLAN team
 * $Id: a1b5dc0abdf2bf05e896eff03a9144eff620a92c $
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_codec.h>
#include <vlc_fs.h>


#include "dummy.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void *DecodeBlock( decoder_t *p_dec, block_t **pp_block );

/*****************************************************************************
 * OpenDecoder: Open the decoder
 *****************************************************************************/
static int OpenDecoderCommon( vlc_object_t *p_this, bool b_force_dump )
{
    decoder_t *p_dec = (decoder_t*)p_this;
    char psz_file[10 + 3 * sizeof (p_dec)];

    snprintf( psz_file, sizeof( psz_file), "stream.%p", p_dec );

    if( !b_force_dump )
        b_force_dump = var_InheritBool( p_dec, "dummy-save-es" );
    if( b_force_dump )
    {
        FILE *stream = vlc_fopen( psz_file, "wb" );
        if( stream == NULL )
        {
            msg_Err( p_dec, "cannot create `%s'", psz_file );
            return VLC_EGENERIC;
        }
        msg_Dbg( p_dec, "dumping stream to file `%s'", psz_file );
        p_dec->p_sys = (void *)stream;
    }
    else
        p_dec->p_sys = NULL;

    /* Set callbacks */
    p_dec->pf_decode_video = (picture_t *(*)(decoder_t *, block_t **))
        DecodeBlock;
    p_dec->pf_decode_audio = (aout_buffer_t *(*)(decoder_t *, block_t **))
        DecodeBlock;
    p_dec->pf_decode_sub = (subpicture_t *(*)(decoder_t *, block_t **))
        DecodeBlock;

    es_format_Copy( &p_dec->fmt_out, &p_dec->fmt_in );

    return VLC_SUCCESS;
}

int OpenDecoder( vlc_object_t *p_this )
{
    return OpenDecoderCommon( p_this, false );
}

int  OpenDecoderDump( vlc_object_t *p_this )
{
    return OpenDecoderCommon( p_this, true );
}

/****************************************************************************
 * RunDecoder: the whole thing
 ****************************************************************************
 * This function must be fed with ogg packets.
 ****************************************************************************/
static void *DecodeBlock( decoder_t *p_dec, block_t **pp_block )
{
    FILE *stream = (void *)p_dec->p_sys;
    block_t *p_block;

    if( !pp_block || !*pp_block ) return NULL;
    p_block = *pp_block;

    if( stream != NULL
     && p_block->i_buffer > 0
     && !(p_block->i_flags & (BLOCK_FLAG_DISCONTINUITY|BLOCK_FLAG_CORRUPTED)) )
    {
        fwrite( p_block->p_buffer, 1, p_block->i_buffer, stream );
        msg_Dbg( p_dec, "dumped %zu bytes", p_block->i_buffer );
    }
    block_Release( p_block );

    return NULL;
}

/*****************************************************************************
 * CloseDecoder: decoder destruction
 *****************************************************************************/
void CloseDecoder ( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t *)p_this;
    FILE *stream = (void *)p_dec->p_sys;

    if( stream != NULL )
        fclose( stream );
}
