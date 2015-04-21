/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include "samline.vers.h"

#include <kfs/file.h>
#include <kfs/directory.h>

#include <kapp/main.h>
#include <kapp/args.h>

#include <klib/out.h>
#include <klib/rc.h>
#include <klib/printf.h>

#include <os-native.h>
#include <sysalloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "refbases.h"
#include "cigar.h"

#define DFLT_REFNAME		"NC_011752.1"
#define DFLT_REFPOS			10000
#define DFLT_CIGAR			"50M"
#define DFLT_INSBASES		"ACGTACGTACGT"
#define DFLT_MAPQ			20
#define DFLT_QNAME			"1"
#define DFLT_TLEN			0

static const char * refname_usage[]		= { "the ref-seq-id to use 'NC_011752.1'", NULL };
static const char * refpos_usage[]		= { "the position on the reference 0-based", NULL };
static const char * cigar_usage[]		= { "the cigar-string to use", NULL };
static const char * insbases_usage[]	= { "what bases to insert ( if needed )", NULL };
static const char * mapq_usage[]		= { "what mapq to use", NULL };
static const char * reverse_usage[]		= { "alignment is reverse", NULL };
static const char * qname_usage[]		= { "query template name", NULL };
static const char * sec_usage[]			= { "secondary alignment", NULL };
static const char * bad_usage[]			= { "did not pass quality control", NULL };
static const char * dup_usage[]			= { "is PCR or optical duplicate", NULL };
static const char * prop_usage[]		= { "each fragment is properly aligned", NULL };
static const char * show_usage[]		= { "show details of calculations", NULL };
static const char * ref_usage[]			= { "return only refbases (set cigar to 100M for len=100)", NULL };
static const char * flags_usage[]		= { "decode decimal flags-value", NULL };
static const char * header_usage[]		= { "produce header", NULL };
static const char * config_usage[]		= { "procuce config-file", NULL };

#define OPTION_REFNAME		"refname"
#define OPTION_REFPOS		"refpos"
#define OPTION_CIGAR		"cigar"
#define OPTION_INSBASES		"insbases"
#define OPTION_MAPQ			"mapq"
#define OPTION_REVERSE		"reverse"
#define OPTION_QNAME		"qname"
#define OPTION_SEC			"secondary"
#define OPTION_BAD			"bad"
#define OPTION_DUP			"duplicate"
#define OPTION_PROP			"proper"
#define OPTION_SHOW			"show"
#define OPTION_REF			"ref"
#define OPTION_FLAGS		"flags"
#define OPTION_HEADER		"header"
#define OPTION_CONFIG		"config"

#define ALIAS_REFNAME		"r"
#define ALIAS_REFPOS		"p"
#define ALIAS_CIGAR			"c"
#define ALIAS_INSBASES		"i"
#define ALIAS_MAPQ			"m"
#define ALIAS_REVERSE		"e"
#define ALIAS_QNAME			"q"
#define ALIAS_SEC			"2"
#define ALIAS_BAD			"a"
#define ALIAS_DUP			"u"
#define ALIAS_PROP			"o"
#define ALIAS_SHOW			"s"
#define ALIAS_REF			"f"
#define ALIAS_FLAGS			"l"
#define ALIAS_HEADER		"d"
#define ALIAS_CONFIG		"n"

OptDef Options[] =
{
    { OPTION_REFNAME, 	ALIAS_REFNAME,	NULL, refname_usage, 	2, 	true, 	false },
    { OPTION_REFPOS, 	ALIAS_REFPOS,	NULL, refpos_usage, 	2,	true, 	false },
    { OPTION_CIGAR, 	ALIAS_CIGAR,	NULL, cigar_usage, 	2,	true, 	false },
    { OPTION_INSBASES, 	ALIAS_INSBASES,	NULL, insbases_usage, 	1,	true, 	false },
    { OPTION_MAPQ, 		ALIAS_MAPQ,		NULL, mapq_usage, 		2,	true, 	false },
    { OPTION_REVERSE, 	ALIAS_REVERSE,	NULL, reverse_usage,	1,	false, 	false },
    { OPTION_QNAME, 	ALIAS_QNAME,	NULL, qname_usage,		2,	true, 	false },
    { OPTION_SEC, 		ALIAS_SEC,		NULL, sec_usage,		2,	false, 	false },
    { OPTION_BAD, 		ALIAS_BAD,		NULL, bad_usage,		2,	false, 	false },
    { OPTION_DUP, 		ALIAS_DUP,		NULL, dup_usage,		2,	false, 	false },
    { OPTION_PROP, 		ALIAS_PROP,		NULL, prop_usage,		2,	false, 	false },
    { OPTION_SHOW, 		ALIAS_SHOW,		NULL, show_usage,		1,	false, 	false },
    { OPTION_REF, 		ALIAS_REF,		NULL, ref_usage,		1,	false, 	false },
    { OPTION_FLAGS, 	ALIAS_FLAGS,	NULL, flags_usage,		1,	true, 	false },
    { OPTION_HEADER, 	ALIAS_HEADER,	NULL, header_usage,	1,	false, 	false },
    { OPTION_CONFIG, 	ALIAS_CONFIG,	NULL, config_usage,	1,	true, 	false }	
};

const char UsageDefaultName[] = "samline";

rc_t CC UsageSummary ( const char * progname )
{
    return KOutMsg( "\nUsage:\n %s [options]\n\n", progname );
}

rc_t CC Usage ( const Args * args )
{
    const char * progname = UsageDefaultName;
    const char * fullpath = UsageDefaultName;
	int i, n_options;
    rc_t rc;

    if ( args == NULL )
        rc = RC ( rcApp, rcArgv, rcAccessing, rcSelf, rcNull );
    else
        rc = ArgsProgram ( args, &fullpath, &progname );

    if ( rc != 0 )
        progname = fullpath = UsageDefaultName;

    UsageSummary ( progname );
    KOutMsg ( "Options:\n" );

	n_options = sizeof Options / sizeof Options[ 0 ];
	for ( i = 0; i < n_options; ++i )
	{
		OptDef * o = &Options[ i ];
		HelpOptionLine( o->aliases, o->name, NULL, o->help );
	}

	KOutMsg ( "\n" );
    HelpOptionsStandard ();
    HelpVersion ( fullpath, KAppVersion() );

    return rc;
}


/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
ver_t CC KAppVersion ( void )
{
    return SAMLINE_VERS;
}


static const char * get_str_option( const Args * args, const char * name, uint32_t idx, const char * dflt )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, name, &count );
    if ( ( rc == 0 )&&( count > idx ) )
	{
		const char * res = NULL;
        ArgsOptionValue( args, name, idx, &res );
		return res;
	}
	else
		return dflt;
}


static uint32_t get_uint32_option( const Args * args, const char * name, uint32_t idx, const uint32_t dflt )
{
	const char * s = get_str_option( args, name, idx, NULL );
	if ( s == NULL )
		return dflt;
	return atoi( s );
}

static uint32_t get_bool_option( const Args * args, const char * name )
{
    uint32_t count;
    rc_t rc = ArgsOptionCount( args, name, &count );
	return ( rc == 0 && count > 0 );
}

typedef struct alignment
{
	const char * refname;
	const char * cigar_str;
	const char * refbases;
	const char * read;
	const char * sam;
	
	int reverse, secondary, bad, dup, prop;

	uint32_t refpos, mapq, bases_in_ref, reflen;	
	
	cigar_opt * cigar;	
} alignment;

typedef struct gen_context
{
	const char * qname;
	const char * insbases;
	const char * config;
	uint32_t flags, header;
	int32_t tlen;
	
	alignment alig[ 2 ];
} gen_context;


static rc_t CC write_to_FILE ( void *f, const char *buffer, size_t bytes, size_t *num_writ )
{
    * num_writ = fwrite ( buffer, 1, bytes, f );
    if ( * num_writ != bytes )
        return RC ( rcExe, rcFile, rcWriting, rcTransfer, rcIncomplete );
    return 0;
}


static char * random_string( const char * char_set, size_t length )
{
	char * res = malloc( length + 1 );
	if ( res != NULL )
	{
		const char dflt_charset[] = "0123456789"
									"abcdefghijklmnopqrstuvwxyz"
									"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		const char * cs = ( char_set == NULL ) ? dflt_charset : char_set;
		size_t charset_len = strlen( cs ) - 1;
		size_t dst_idx = 0;
		while ( dst_idx < length )
		{
			size_t rand_idx = ( double ) rand() / RAND_MAX * charset_len;
			res[ dst_idx++ ] = cs[ rand_idx ];
		}
		res[ dst_idx++ ] = 0;
	}
	return res;
}


static char * random_quality( size_t length )
{
	const char qualities[] = "!\"#$%&'()*+,-./0123456789:;<=>?"
							 "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
							 "`abcdefghijklmnopqrstuvwxyz{|}~";
	return random_string( qualities, length );
}

static uint32_t sam_flags( const alignment * alig, const alignment * other, int first, int last )
{
	uint32_t res = 0;
	if ( first || last ) res |= 0x01;							/* multiple fragments */
	if ( alig->prop != 0 ) res |= 0x02;						/* each fragment properly aligned */
	if ( alig->refpos == 0 ) res |= 0x04; 						/* this fragment is unmapped */
	if ( other != NULL && other->refpos == 0 ) res |= 0x08; 	/* next fragment is unmapped */
	if ( alig->reverse ) res |= 0x10;							/* this fragment is reversed */
	if ( other != NULL && other->reverse ) res |= 0x20; 		/* next fragment is reversed */
	if ( first ) res |= 0x40;									/* this is the first fragment */
	if ( last ) res |= 0x80; 									/* this is the last fragment */
	if ( alig->secondary != 0 ) res |= 0x100;					/* this is a secondary alignment */	
	if ( alig->bad != 0 ) res |= 0x200; 						/* this is did not pass quality controls */	
	if ( alig->dup != 0 ) res |= 0x400;						/* this is PCR or optical duplicate */	
	return res;
}


static char * produce_merged_cigar( const cigar_opt * cigar )
{
	char * res = NULL;
	cigar_opt * temp = merge_match_and_mismatch( cigar );
	if ( temp != NULL )
	{
		res = to_string( temp );
		free_cigar( temp );
	}
	return res;
}

static char * produce_sam( const gen_context * gctx, const alignment * alig, const alignment * other )
{
	char * res = NULL;
	if ( gctx != NULL && alig != NULL )
	{
		size_t l_qname   = gctx->qname != NULL ? strlen( gctx->qname ) : 10;
		size_t l_refname = alig->refname != NULL ? strlen( alig->refname ) : 10;
		size_t l_read 	 = alig->read != NULL ? strlen( alig->read ) : 10;
		size_t l_cigar	 = alig->cigar_str != NULL ? strlen( alig->cigar_str ) : 10;
		
		size_t l = l_qname + ( 2 * l_refname ) + ( 2 * l_read ) + l_cigar + 100;
		res = malloc( l );
		if ( res != NULL )
		{
			const char * cig_str = produce_merged_cigar( alig->cigar );
			const char * quality = random_quality( l_read );
		
			int first = 0;
			int last = 0;
			const char * r_next = "*";
			uint32_t r_pos = 0;
			
			if ( other != NULL && other->refname != NULL && other->refpos == 0 )
			{
				r_next = other->refname;
				first = ( alig->refpos < other->refpos );
				last = !first;
				r_pos = other->refpos;
			}
			
			size_t num_writ;
			string_printf ( res, l, &num_writ,
							"%s\t%d\t%s\t%d\t%d\t%s\t%s\t%d\t%d\t%s\t%s",
							gctx->qname,
							sam_flags( alig, other, first, last ),
							alig->refname,
							alig->refpos,
							alig->mapq,
							cig_str,
							r_next,
							r_pos,
							gctx->tlen,
							alig->read,
							quality );

			if ( cig_str != NULL )	free( ( void * )cig_str );
			if ( quality != NULL )	free( ( void * )quality );
		}
	}
	return res;
}

static void show_alig_details( const alignment * alig )
{
	KOutMsg ( "REFNAME  : %s\n", alig->refname );
	KOutMsg ( "REFPOS   : %d\n", alig->refpos );
	KOutMsg ( "CIGAR    : %s\n", alig->cigar_str );
	KOutMsg ( "MAPQ     : %d\n", alig->mapq );
	KOutMsg ( "REVERSE  : %s\n", alig->reverse ? "YES" : "NO" );
	KOutMsg ( "SECONDARY: %s\n", alig->secondary ? "YES" : "NO" );
	KOutMsg ( "BAD      : %s\n", alig->bad ? "YES" : "NO" );
	KOutMsg ( "DUPLICATE: %s\n", alig->dup ? "YES" : "NO" );
	KOutMsg ( "PROPERLY : %s\n", alig->prop ? "YES" : "NO" );
	KOutMsg ( "REFLEN   : %d\n", alig->reflen );
	KOutMsg ( "READLEN  : %d\n", calc_readlen( alig->cigar ) );	
	KOutMsg ( "INSLEN   : %d\n", calc_inslen( alig->cigar ) );
	KOutMsg ( "REFBASES : %s\n", alig->refbases );
	KOutMsg ( "READ     : %s\n", alig->read );
	KOutMsg ( "SAM      : %s\n", alig->sam );
	
}

static void show_details( const gen_context * gctx )
{
	KOutMsg ( "QNAME    : %s\n", gctx->qname );
	KOutMsg ( "INSBASES : %s\n", gctx->insbases );
	KOutMsg ( "TLEN     : %d\n", gctx->tlen );
	KOutMsg ( "CONFIG   : %s\n", gctx->config );
	if ( gctx->tlen != 0 )
	{
		KOutMsg ( "----- ALIGNMENT #1 -----\n" );
		show_alig_details( &gctx->alig[ 0 ] );
		KOutMsg ( "----- ALIGNMENT #2 -----\n" );
		show_alig_details( &gctx->alig[ 1 ] );
	}
	else
		show_alig_details( &gctx->alig[ 0 ] );
}


static void explain_flags( const uint32_t flags )
{
	if ( ( flags & 0x01 ) == 0x01 )
		KOutMsg ( "0x001 ... multiple fragments\n" );
	if ( ( flags & 0x02 ) == 0x02 )
		KOutMsg ( "0x002 ... each fragment properly aligned\n" );
	if ( ( flags & 0x04 ) == 0x04 )
		KOutMsg ( "0x004 ... this fragment is unmapped\n" );
	if ( ( flags & 0x08 ) == 0x08 )
		KOutMsg ( "0x008 ... next fragment is unmapped\n" );
	if ( ( flags & 0x10 ) == 0x10 )
		KOutMsg ( "0x010 ... this fragment is reversed\n" );
	if ( ( flags & 0x20 ) == 0x20 )
		KOutMsg ( "0x020 ... next fragment is reversed\n" );
	if ( ( flags & 0x40 ) == 0x40 )
		KOutMsg ( "0x040 ... this is the first fragment\n" );
	if ( ( flags & 0x80 ) == 0x80 )
		KOutMsg ( "0x080 ... this is the last fragment\n" );
	if ( ( flags & 0x100 ) == 0x100 )
		KOutMsg ( "0x100 ... this is a secondary alignment\n" );
	if ( ( flags & 0x200 ) == 0x200 )
		KOutMsg ( "0x200 ... this is did not pass quality controls\n" );
	if ( ( flags & 0x400 ) == 0x400 )
		KOutMsg ( "0x400 ... this is PCR or optical duplicate\n" );
}


static size_t write_config_line( KFile * dst, size_t at, const char * refname0, const char * refname1 )
{
	size_t num_in_buffer, res = 0;
	char buffer[ 4096 ];
	rc_t rc = string_printf ( buffer, sizeof buffer, &num_in_buffer, "%s\t%s\n", refname0, refname1 );
	if ( rc == 0 )
	{
		size_t written_to_file;
		rc = KFileWriteAll ( dst, at, buffer, num_in_buffer, &written_to_file );
		if ( rc == 0 )
			res = at + written_to_file;
	}
	return res;
}


static void write_config_file( const char * filename, const char * refname0, const char * refname1 )
{
	KDirectory *dir;
	rc_t rc = KDirectoryNativeDir( &dir );
	if ( rc == 0 )
	{
		KFile * dst;
		rc = KDirectoryCreateFile ( dir, &dst, false, 0664, kcmInit, filename );
		if ( rc == 0 )
		{
			size_t pos = write_config_line( dst, 0, refname0, refname0 );
			if ( pos > 0 && ( strcmp( refname0, refname1 ) != 0 ) )
				write_config_line( dst, pos, refname1, refname1 );
			KFileRelease( dst );			
		}
		KDirectoryRelease( dir );
	}
}


static void generate_alignment( const gen_context * gctx )
{
	/* write reference names into config-file for bam-load */
	if ( gctx->config != NULL )
		write_config_file( gctx->config, gctx->alig[ 0 ].refname, gctx->alig[ 1 ].refname );
	
	/* procude SAM-header on stdout */
	if ( gctx->header )
	{
		const char * refname = gctx->alig[ 0 ].refname;
		int bases_in_ref = gctx->alig[ 0 ].bases_in_ref;
		KOutMsg( "@HDVN:1.3\n" );
		KOutMsg( "@SQ\tSN:%s\tAS:%s\tLN:%d\n", refname, refname, bases_in_ref );
	}

	/* produces SAM-line for 1st alignment */
	KOutMsg( "%s\n", gctx->alig[ 0 ].sam );
	
	/* produces SAM-line for 2nd alignment ( mate ) */
	if ( gctx->tlen != 0 )
		KOutMsg( "%s\n", gctx->alig[ 1 ].sam );
}


static void read_alig_context( Args * args, gen_context * gctx, alignment * alig, uint32_t idx )
{
	alig->refname	= get_str_option( args, OPTION_REFNAME, 	idx,	idx == 0 ? DFLT_REFNAME : NULL );
	alig->refpos	= get_uint32_option( args, OPTION_REFPOS, 	idx,	idx == 0 ? DFLT_REFPOS : 0 );
	alig->cigar_str	= get_str_option( args, OPTION_CIGAR, 		idx,	DFLT_CIGAR );
	alig->mapq		= get_uint32_option( args, OPTION_MAPQ, 	idx,	DFLT_MAPQ );
	
	alig->reverse	= get_uint32_option( args, OPTION_REVERSE,	idx,	0 );
	alig->secondary	= get_uint32_option( args, OPTION_SEC,		idx,	0 );
	alig->bad		= get_uint32_option( args, OPTION_BAD,		idx,	0 );
	alig->dup		= get_uint32_option( args, OPTION_DUP,		idx,	0 );
	alig->prop		= get_uint32_option( args, OPTION_PROP,		idx,	0 );
	
	/* precalculate values need in all functions */
	alig->cigar		= parse_cigar( alig->cigar_str );
	alig->reflen	= calc_reflen( alig->cigar );
}

static void release_alig( alignment * alig )
{
	if ( alig->refbases != NULL ) free( ( void* ) alig->refbases );
	if ( alig->read != NULL ) free( ( void* ) alig->read );
	if ( alig->sam != NULL ) free( ( void* ) alig->sam );
	free_cigar( alig->cigar );
}

static void read_context( Args * args, gen_context * gctx )
{
	alignment *alig0, *alig1;

	gctx->qname 	= get_str_option( args, OPTION_QNAME, 		0, 	DFLT_QNAME );
	gctx->insbases	= get_str_option( args, OPTION_INSBASES,	0,	DFLT_INSBASES );
	gctx->flags		= get_uint32_option( args, OPTION_FLAGS, 	0,	0 );
	gctx->header	= get_bool_option( args, OPTION_HEADER );
	gctx->config	= get_str_option( args, OPTION_CONFIG, 		0, 	NULL );
	gctx->tlen		= 0;
	
	read_alig_context( args, gctx, &gctx->alig[ 0 ], 0 );
	read_alig_context( args, gctx, &gctx->alig[ 1 ], 1 );

	alig0 = &gctx->alig[ 0 ];
	alig1 = &gctx->alig[ 1 ];
	if ( gctx->alig[ 1 ].refpos > 0 )
	{
		if ( gctx->alig[ 1 ].refname == NULL )
			gctx->alig[ 1 ].refname = gctx->alig[ 0 ].refname;

		uint32_t end = alig1->refpos + alig1->reflen;
		gctx->tlen = ( end - alig0->refpos );
		
		alig0->refbases = read_refbases( alig0->refname, alig0->refpos, alig0->reflen, &alig0->bases_in_ref );
		alig1->refbases = read_refbases( alig1->refname, alig1->refpos, alig1->reflen, &alig1->bases_in_ref );

		alig0->read		= produce_read( alig0->cigar, alig0->refbases, gctx->insbases );
		alig1->read		= produce_read( alig1->cigar, alig1->refbases, gctx->insbases );
		
		alig0->sam = produce_sam( gctx, alig0, alig1 );
		alig1->sam = produce_sam( gctx, alig1, alig0 )	;
	}
	else
	{
		alig0->refbases = read_refbases( alig0->refname, alig0->refpos, alig0->reflen, &alig0->bases_in_ref );
		alig1->refbases = NULL;
		alig1->bases_in_ref	= 0;
		
		alig0->read		= produce_read( alig0->cigar, alig0->refbases, gctx->insbases );
		alig1->read		= NULL;
		
		alig0->sam = produce_sam( gctx, alig0, alig1 );
		alig1->sam = NULL;
	}
}

rc_t CC KMain( int argc, char *argv [] )
{
    rc_t rc = KOutHandlerSet ( write_to_FILE, stdout );
    if ( rc == 0 )
	{
		Args * args;

		int n_options = sizeof Options / sizeof Options[ 0 ];
        rc = ArgsMakeAndHandle( &args, argc, argv, 1,  Options, n_options );
		if ( rc == 0 )
		{
			gen_context	gctx;
			
			read_context( args, &gctx );
			
			if ( get_bool_option( args, OPTION_SHOW ) )
				show_details( &gctx );
			else if ( get_bool_option( args, OPTION_REF ) )
				KOutMsg ( "%s\n", gctx.alig[ 0 ].refbases );
			else if ( gctx.flags > 0 )
				explain_flags( gctx.flags );
			else
				generate_alignment( &gctx );

			release_alig( &gctx.alig[ 0 ] );
			if ( gctx.tlen != 0 )
				release_alig( &gctx.alig[ 1 ] );
			
			ArgsWhack( args );
		}
	}
    return rc;
}
