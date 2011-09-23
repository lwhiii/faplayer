/*****************************************************************************
 * core.c: Core libvlc new API functions : initialization
 *****************************************************************************
 * Copyright (C) 2005 the VideoLAN team
 * $Id: 6e88b9481e6e5d55a5435f7f37f79760e6f97794 $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "libvlc_internal.h"
#include <vlc_modules.h>
#include <vlc/libvlc.h>

#include <vlc_interface.h>
#include <vlc_vlm.h>

#include <stdarg.h>
#include <limits.h>
#include <assert.h>

static const char nomemstr[] = "Insufficient memory";

libvlc_instance_t * libvlc_new( int argc, const char *const *argv )
{
    libvlc_instance_t *p_new = malloc (sizeof (*p_new));
    if (unlikely(p_new == NULL))
        return NULL;

    libvlc_init_threads ();

    const char *my_argv[argc + 2];
    my_argv[0] = "libvlc"; /* dummy arg0, skipped by getopt() et al */
    for( int i = 0; i < argc; i++ )
         my_argv[i + 1] = argv[i];
    my_argv[argc + 1] = NULL; /* C calling conventions require a NULL */

    libvlc_int_t *p_libvlc_int = libvlc_InternalCreate();
    if (unlikely (p_libvlc_int == NULL))
        goto error;

    if (libvlc_InternalInit( p_libvlc_int, argc + 1, my_argv ))
    {
        libvlc_InternalDestroy( p_libvlc_int );
        goto error;
    }

    p_new->p_libvlc_int = p_libvlc_int;
    p_new->libvlc_vlm.p_vlm = NULL;
    p_new->libvlc_vlm.p_event_manager = NULL;
    p_new->libvlc_vlm.pf_release = NULL;
    p_new->ref_count = 1;
    p_new->p_callback_list = NULL;
    vlc_mutex_init(&p_new->instance_lock);
    var_Create( p_libvlc_int, "http-user-agent",
                VLC_VAR_STRING|VLC_VAR_DOINHERIT );
    return p_new;

error:
    libvlc_deinit_threads ();
    free (p_new);
    return NULL;
}

void libvlc_retain( libvlc_instance_t *p_instance )
{
    assert( p_instance != NULL );
    assert( p_instance->ref_count < UINT_MAX );

    vlc_mutex_lock( &p_instance->instance_lock );
    p_instance->ref_count++;
    vlc_mutex_unlock( &p_instance->instance_lock );
}

void libvlc_release( libvlc_instance_t *p_instance )
{
    vlc_mutex_t *lock = &p_instance->instance_lock;
    int refs;

    vlc_mutex_lock( lock );
    assert( p_instance->ref_count > 0 );
    refs = --p_instance->ref_count;
    vlc_mutex_unlock( lock );

    if( refs == 0 )
    {
        vlc_mutex_destroy( lock );
        if( p_instance->libvlc_vlm.pf_release )
            p_instance->libvlc_vlm.pf_release( p_instance );
        libvlc_InternalCleanup( p_instance->p_libvlc_int );
        libvlc_InternalDestroy( p_instance->p_libvlc_int );
        free( p_instance );
        libvlc_deinit_threads ();
    }
}

int libvlc_add_intf( libvlc_instance_t *p_i, const char *name )
{
    if( libvlc_InternalAddIntf( p_i->p_libvlc_int, name ))
    {
        if( name )
        {
            libvlc_printerr("interface \"%s\" initialization failed",
                name );
        }
        else
        {
            libvlc_printerr("default interface initialization failed");
        }
        return -1;
    }
    return 0;
}

void libvlc_set_exit_handler( libvlc_instance_t *p_i, void (*cb) (void *),
                              void *data )
{
    libvlc_int_t *p_libvlc = p_i->p_libvlc_int;
    libvlc_SetExitHandler( p_libvlc, cb, data );
}

void libvlc_wait( libvlc_instance_t *p_i )
{
    libvlc_int_t *p_libvlc = p_i->p_libvlc_int;
    libvlc_InternalWait( p_libvlc );
}

void libvlc_set_user_agent (libvlc_instance_t *p_i,
                            const char *name, const char *http)
{
    libvlc_int_t *p_libvlc = p_i->p_libvlc_int;
    char *str;

    var_SetString (p_libvlc, "user-agent", name);
    if ((http != NULL)
     && (asprintf (&str, "%s LibVLC/"PACKAGE_VERSION, http) != -1))
    {
        var_SetString (p_libvlc, "http-user-agent", str);
        free (str);
    }
}

const char * libvlc_get_version(void)
{
    return VERSION_MESSAGE;
}

const char * libvlc_get_compiler(void)
{
    return VLC_Compiler();
}

const char * libvlc_get_changeset(void)
{
    extern const char psz_vlc_changeset[];
    return psz_vlc_changeset;
}

void libvlc_free( void *ptr )
{
    free( ptr );
}

static libvlc_module_description_t *module_description_list_get(
                libvlc_instance_t *p_instance, const char *capability )
{
    VLC_UNUSED( p_instance );
    libvlc_module_description_t *p_list = NULL,
                          *p_actual = NULL,
                          *p_previous = NULL;
    module_t **module_list = module_list_get( NULL );

    for (size_t i = 0; module_list[i]; i++)
    {
        module_t *p_module = module_list[i];

        if ( !module_provides( p_module, capability ) )
            continue;

        p_actual = ( libvlc_module_description_t * ) malloc( sizeof( libvlc_module_description_t ) );
        if ( p_actual == NULL )
        {
            libvlc_printerr( "Not enough memory" );
            libvlc_module_description_list_release( p_list );
            module_list_free( module_list );
            return NULL;
        }

        if ( p_list == NULL )
            p_list = p_actual;

        const char* name = module_get_object( p_module );
        const char* shortname = module_get_name( p_module, false );
        const char* longname = module_get_name( p_module, true );
        const char* help = module_get_help( p_module );
        p_actual->psz_name = name ? strdup( name ) : NULL;
        p_actual->psz_shortname = shortname ? strdup( shortname ) : NULL;
        p_actual->psz_longname = longname ? strdup( longname ) : NULL;
        p_actual->psz_help = help ? strdup( help ) : NULL;

        p_actual->p_next = NULL;
        if ( p_previous )
            p_previous->p_next = p_actual;
        p_previous = p_actual;
    }

    module_list_free( module_list );
    return p_list;
}

void libvlc_module_description_list_release( libvlc_module_description_t *p_list )
{
    libvlc_module_description_t *p_actual, *p_before;
    p_actual = p_list;

    while ( p_actual )
    {
        free( p_actual->psz_name );
        free( p_actual->psz_shortname );
        free( p_actual->psz_longname );
        free( p_actual->psz_help );
        p_before = p_actual;
        p_actual = p_before->p_next;
        free( p_before );
    }
}

libvlc_module_description_t *libvlc_audio_filter_list_get( libvlc_instance_t *p_instance )
{
    return module_description_list_get( p_instance, "audio filter" );
}

libvlc_module_description_t *libvlc_video_filter_list_get( libvlc_instance_t *p_instance )
{
    return module_description_list_get( p_instance, "video filter2" );
}

int64_t libvlc_clock(void)
{
    return mdate();
}