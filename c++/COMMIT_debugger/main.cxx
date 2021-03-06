#include <NIFTI.h>
#include <COLOR_ui.h>
#include <TrackVis.h>
#include <VECTOR.h>
#include <cmath>
#include <regex>

#include <blitz/array.h>
using namespace std;

NIFTI*                   niiDWI;
VECTOR<int>		         dim;
VECTOR<float>	         pixdim;

int                      SCHEME_version;
vector< VECTOR<float> >	 SCHEME_dirs;
vector<float>	         SCHEME_b;
vector<int>              SCHEME_idxB0;
vector<int>              SCHEME_idxDWI;
vector<float>	         SCHEME_shells_b;
vector< vector<int> >    SCHEME_shells_idx;

blitz::Array<float,3>    MAP;
VECTOR<int>		         VOXEL;
float                    MAP_min, MAP_min_view, MAP_max, MAP_max_view;
float 			         MAP_opacity = 0.8;
bool			         showPlane[3] = { false, false, true };

NIFTI*                   niiPEAKS;
int				         PEAKS_n;
bool			         PEAKS_show = false;
int				         PEAKS_width = 2;
float			         PEAKS_thr = 0.1;
bool			         PEAKS_doNormalize = false;
bool			         PEAKS_flip[3] = {false, false, false};
int 			         PEAKS_swap = 0;

TrackVis 		         TRK_file;
int				         TRK_skip;
int				         TRK_nTractsPlotted;
int*   			         TRK_nPoints;
float*			         TRK_coords;
float*			         TRK_colors;
float 			         TRK_crop = 1.0;
bool 			         TRK_crop_mode = true;
bool 			         TRK_show = false;
VECTOR<float> 	         TRK_offset;

bool 			         GLYPHS_show = false;
bool			         GLYPHS_flip[3] = {false, false, false};
vector< VECTOR<float> >  GLYPHS_dirs;
vector< int >	         GLYPHS_idx;
float	                 GLYPHS_b0_thr;

#include "OPENGL_callbacks.cxx"


/*----------------------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    if ( argc<5 )
    {
        COLOR_error( "USAGE: viewer <.nii DWI> <.scheme SCHEME> <.nii PEAKS> <.trk TRACTS>" );
        return EXIT_FAILURE;
    }


    // ===================
    // Reading DWI dataset
    // ===================
    COLOR_msg( "-> Reading 'DWI' dataset:", "\n" );

    string DWI_filename = argv[1];
    niiDWI = new NIFTI;
    niiDWI->open( DWI_filename, true );
    if ( !niiDWI->isValid() )
    {
        COLOR_error( "Unable to open file", "\t" );
        return EXIT_FAILURE;
    }
    dim.x = niiDWI->hdr->dim[1];
    dim.y = niiDWI->hdr->dim[2];
    dim.z = niiDWI->hdr->dim[3];
    pixdim.x = niiDWI->hdr->pixdim[1];
    pixdim.y = niiDWI->hdr->pixdim[2];
    pixdim.z = niiDWI->hdr->pixdim[3];
    printf( "\tdim    : %d x %d x %d x %d\n", dim.x, dim.y, dim.z, niiDWI->hdr->dim[4] );
    printf( "\tpixdim : %.4f x %.4f x %.4f\n", 	pixdim.x, pixdim.y, pixdim.z );
    printf( "\tqform  : '%s', quaternion = ( %f %f %f ), offset = ( %f %f %f ), qfac = %f\n", nifti_xform_string( niiDWI->hdr->qform_code ), niiDWI->hdr->quatern_b, niiDWI->hdr->quatern_c, niiDWI->hdr->quatern_d, niiDWI->hdr->qoffset_x, niiDWI->hdr->qoffset_y, niiDWI->hdr->qoffset_z, niiDWI->hdr->pixdim[0] );

    COLOR_msg( "   [OK]" );



    // ===================
    // Reading SCHEME file
    // ===================
    COLOR_msg( "-> Reading 'SCHEME' file:", "\n" );

    char line[1000];
    string SCHEME_filename = argv[2];
    FILE* pFile = fopen( SCHEME_filename.c_str(), "rt" );

    // read the version
    // ----------------
    try
    {
        while( fgets(line, 1000 , pFile) )
            if ( line[0]!='#' )
                break;

        strtok(line, "\n"); // remove newline

        std::regex reVersion("^VERSION: (.*)\\s*$");
        std::smatch reMatches;

        if ( !std::regex_match(string(line), reMatches, reVersion) )
            throw "Header not found";
        else
        {
            if( strcmp(reMatches[1].str().c_str(),"0")==0 || strcmp(reMatches[1].str().c_str(),"BVECTOR")==0 )
                SCHEME_version = 0;
            else if( strcmp(reMatches[1].str().c_str(),"1")==0 || strcmp(reMatches[1].str().c_str(),"STEJSKALTANNER")==0 )
                SCHEME_version = 1;
            else
                throw "Version not recognized";
        }
    }
    catch( const char* msg )
    {
        COLOR_error( msg, "\t" );
        return EXIT_FAILURE;
    }
    printf( "\tversion   : %s\n", SCHEME_version==0?"BVECTOR":"STEJSKALTANNER" );

    // read the data
    // -------------
    try
    {
        string      reFLOAT( "[-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?" );
        std::regex  reVERSION0( "^\\s*("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s*$" );
        std::regex  reVERSION1( "^\\s*("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s+("+reFLOAT+")\\s*$" );
        std::regex  reEMPTY( "^\\s*$" );
        std::smatch reMatches;
        int         Ns = 0;
        float       b;
        while( fgets(line, 1000 , pFile) )
        {
            strtok(line, "\n"); // remove newline
            if( std::regex_match(string(line), reMatches, reEMPTY) )
                continue;   // skip empty lines

            if( SCHEME_version == 0 )
            {
                if ( !std::regex_match(string(line), reMatches, reVERSION0) )
                    throw "Wrong row format";
                VECTOR<float> tmp( std::stof(reMatches[1].str().c_str()), std::stof(reMatches[2].str().c_str()), std::stof(reMatches[3].str().c_str()) );
                SCHEME_dirs.push_back( tmp );
                b = std::stof(reMatches[4].str().c_str()); // in mm^2/s
                SCHEME_b.push_back( b );
            }
            else
            {
                if ( !std::regex_match(string(line), reMatches, reVERSION1) )
                    throw "Wrong row format";
                VECTOR<float> tmp( std::stof(reMatches[1].str().c_str()), std::stof(reMatches[2].str().c_str()), std::stof(reMatches[3].str().c_str()) );
                SCHEME_dirs.push_back( tmp );
                b = std::pow( 267.513e6 * std::stof(reMatches[4].str().c_str()) * std::stof(reMatches[6].str().c_str()), 2 ) * (std::stof(reMatches[5].str().c_str()) - std::stof(reMatches[6].str().c_str())/3.0) * 1e-6; // in mm^2/s
                SCHEME_b.push_back( b );
            }


            if ( b<5 )
            {
                SCHEME_idxB0.push_back( Ns );
            }
            else
            {
                SCHEME_idxDWI.push_back( Ns );
                if ( std::find(SCHEME_shells_b.begin(), SCHEME_shells_b.end(), b) == SCHEME_shells_b.end() )
                {
                    SCHEME_shells_b.push_back( b ) ;
                    vector<int> tmp;
                    SCHEME_shells_idx.push_back( tmp ) ;
                }
            }
            Ns++;
        }
    }
    catch( const char* msg )
    {
        COLOR_error( msg, "\t" );
        return EXIT_FAILURE;
    }
    fclose(pFile);

    printf( "\tgradients : %d\n", SCHEME_b.size() );
    if ( niiDWI->hdr->dim[4] != SCHEME_b.size() )
    {
        COLOR_error( "The scheme does not math the DWI dataset", "\t" );
        return EXIT_FAILURE;
    }

    // fill data structure about the SCHEME
    // ------------------------------------
    for(int i=0; i < SCHEME_b.size() ;i++)
    {
        if ( SCHEME_b[i] < 5 )
            continue;
        int s = std::find( SCHEME_shells_b.begin(), SCHEME_shells_b.end(), SCHEME_b[i] ) - SCHEME_shells_b.begin();
        SCHEME_shells_idx[s].push_back( i );
    }

    printf( "\tscheme    : %d b0 and %d shells (", SCHEME_idxB0.size(), SCHEME_shells_idx.size() );
    for(int i=0; i < SCHEME_shells_b.size() ;i++)
        printf( " [%d @ b=%.1f]", SCHEME_shells_idx[i].size(), SCHEME_shells_b[i] );
    printf( " )\n" );

    COLOR_msg( "   [OK]" );



    // =======================
    // Creating BACKGROUND map
    // =======================
    COLOR_msg( "-> Preparing 'B0' map:", "\n" );

    MAP.resize(dim.x,dim.y,dim.z);
    if ( SCHEME_idxB0.size() > 0 )
    {
        FLOAT32 MIN = (*niiDWI->img)(0,0,0,SCHEME_idxB0[0]);
        FLOAT32 MAX = MIN;

        for(int i=0; i<dim.x ;i++)
        for(int j=0; j<dim.y ;j++)
        for(int k=0; k<dim.z ;k++)
        {
            MAP(i,j,k) = (*niiDWI->img)(i,j,k,SCHEME_idxB0[0]);
            if ( MAP(i,j,k) > MAX )
                MAX = MAP(i,j,k);
            if ( MAP(i,j,k) < MIN )
                MIN = MAP(i,j,k);
        }
        if ( MAX - MIN <= 0 )
        {
            COLOR_error( "The dynamic range is zero", "\t" );
            return EXIT_FAILURE;
        }
        MAP_min	= MIN;
        MAP_min_view = 0;
        MAP_max	= MAP_max_view = MAX;

        printf( "\tvalues : [%.2e ... %.2e]\n", MAP_min, MAP_max );
        COLOR_msg( "   [OK]" );
    }
    else
    {
        MAP = 0;
        MAP_min	= MAP_min_view = 0;
        MAP_max	= MAP_max_view = 1;
        COLOR_msg( "   [no b0 found]" );
    }



    // ==============================
    // Preparing GLYPHS visualization
    // ==============================
    COLOR_msg( "-> Preparing 'GLYPHS' visualization:", "\n" );

    int s = SCHEME_shells_b.size()-1; // extract last shell ([FIXME] possibility to choose)
    for(int i=0; i < SCHEME_shells_idx[s].size() ;i++)
    {
        int idx = SCHEME_shells_idx[s][i];
        GLYPHS_dirs.push_back( SCHEME_dirs[idx] );
        GLYPHS_idx.push_back( idx );
    }
    GLYPHS_b0_thr = 50.0;
    printf( "\tscheme    : %d dirs from %d^ shell (b=%.1f)\n", GLYPHS_dirs.size(), s+1, SCHEME_shells_b[s] );
    printf( "\tb0_thr    : %.1f\n", GLYPHS_b0_thr );

    COLOR_msg( "   [OK]" );



    // ==================
    // Reading PEAKS file
    // ==================
    COLOR_msg( "-> Reading 'PEAKS' dataset:", "\n" );

    string PEAKS_filename = argv[3];
    niiPEAKS = new NIFTI;
    niiPEAKS->open( PEAKS_filename, true );
    if ( !niiPEAKS->isValid() )
    {
        COLOR_error( "Unable to open the file", "\t" );
        return false;
    }

    printf( "\tdim    : %d x %d x %d x %d\n" , niiPEAKS->hdr->dim[1],    niiPEAKS->hdr->dim[2],    niiPEAKS->hdr->dim[3], niiPEAKS->hdr->dim[4] );
    printf( "\tpixdim : %.4f x %.4f x %.4f\n", niiPEAKS->hdr->pixdim[1], niiPEAKS->hdr->pixdim[2], niiPEAKS->hdr->pixdim[3] );
    printf( "\tqform  : '%s', quaternion = ( %f %f %f ), offset = ( %f %f %f ), qfac = %f\n", nifti_xform_string( niiPEAKS->hdr->qform_code ), niiPEAKS->hdr->quatern_b, niiPEAKS->hdr->quatern_c, niiPEAKS->hdr->quatern_d, niiPEAKS->hdr->qoffset_x, niiPEAKS->hdr->qoffset_y, niiPEAKS->hdr->qoffset_z, niiDWI->hdr->pixdim[0] );

    if ( niiPEAKS->hdr->dim[0] != 4 || niiPEAKS->hdr->dim[4]%3 != 0 )
    {
        COLOR_error( "The size must be (*,*,*,3*k)", "\t" );
        return EXIT_FAILURE;
    }
    PEAKS_n = niiPEAKS->hdr->dim[4]/3;

    if ( niiPEAKS->hdr->dim[1] != dim.x || niiPEAKS->hdr->dim[2] != dim.y || niiPEAKS->hdr->dim[3] != dim.z )
    {
        COLOR_error( "The DIMENSIONS do not math those of DWI images", "\t" );
        return EXIT_FAILURE;
    }
    if ( abs(niiPEAKS->hdr->pixdim[1]-pixdim.x) > 1e-4 || abs(niiPEAKS->hdr->pixdim[2]-pixdim.y) > 1e-4 || abs(niiPEAKS->hdr->pixdim[3]-pixdim.z) > 1e-4 )
    {
        COLOR_warning( "The VOXEL SIZE does not math that of DWI images", "\t" );
    }
    if (
        niiPEAKS->hdr->qform_code != niiDWI->hdr->qform_code || niiPEAKS->hdr->pixdim[0] != niiDWI->hdr->pixdim[0] ||
        niiPEAKS->hdr->quatern_b != niiDWI->hdr->quatern_b || niiPEAKS->hdr->quatern_c != niiDWI->hdr->quatern_c || niiPEAKS->hdr->quatern_d != niiDWI->hdr->quatern_d ||
        niiPEAKS->hdr->qoffset_x != niiDWI->hdr->qoffset_x || niiPEAKS->hdr->qoffset_y != niiDWI->hdr->qoffset_y || niiPEAKS->hdr->qoffset_z != niiDWI->hdr->qoffset_z
        )
    {

        COLOR_warning( "The GEOMETRY does not math that of DWI images", "\t" );
    }

    COLOR_msg( "   [OK]" );



    // ===================
    // Reading TRACTS file
    // ===================
    COLOR_msg( "-> Reading 'TRK' dataset:", "\n" );

    string TRK_filename = argv[4];
    TRK_file = TrackVis();
    if ( !TRK_file.open( TRK_filename ) )
    {
        COLOR_error( "Unable to open the file", "\t" );
        return false;
    }

    printf("\tcount      : %d\n" , TRK_file.hdr.n_count );
    printf("\tdim        : %d x %d x %d\n" , TRK_file.hdr.dim[0], TRK_file.hdr.dim[1], TRK_file.hdr.dim[2] );
    printf("\tpixdim     : %.4f x %.4f x %.4f\n", TRK_file.hdr.voxel_size[0], TRK_file.hdr.voxel_size[1], TRK_file.hdr.voxel_size[2] );
    printf("\tscalars    : %d\n" , TRK_file.hdr.n_scalars );
    printf("\tproperties : %d\n" , TRK_file.hdr.n_properties );

    if ( TRK_file.hdr.dim[0] != dim.x || TRK_file.hdr.dim[1] != dim.y || TRK_file.hdr.dim[2] != dim.z ||
         abs(TRK_file.hdr.voxel_size[0]-pixdim.x) > 1e-4 || abs(TRK_file.hdr.voxel_size[1]-pixdim.y) > 1e-4 || abs(TRK_file.hdr.voxel_size[2]-pixdim.z) > 1e-4 )
    {
        COLOR_error( "The GEOMETRY does not math those of DWI images", "\t" );
        return EXIT_FAILURE;
    }

    TRK_skip = ceil( TRK_file.hdr.n_count / 25000.0 );

    // count how many points I need to store in memory
    int N;
    int n_s = TRK_file.hdr.n_scalars;
    int n_p = TRK_file.hdr.n_properties;

    int TractsRead = 0;
    int CoordsRead = 0;
    FILE* fp = TRK_file.getFilePtr();
    fseek(fp, 1000, SEEK_SET);
    for(int f=0; f < TRK_file.hdr.n_count ; f++)
    {
        if ( f%TRK_skip==0 )
        {
            fread( (char*)&N, 1, 4, fp );
            fseek( fp, N*(3+n_s)*4 + n_p*4, SEEK_CUR );
            TractsRead++;
            CoordsRead += N;
        }
        else
        {
            fread( (char*)&N, 1, 4, fp );
            fseek( fp, N*(3+n_s)*4 + n_p*4, SEEK_CUR );
        }
    }
    printf("\tin memory  : %d (%d points)\n" , TractsRead, CoordsRead );

    // create data structure for drawing the tracts
    TRK_nTractsPlotted = TractsRead;
    TRK_nPoints = new int[TRK_nTractsPlotted];
    TRK_coords  = new float[3*CoordsRead];
    TRK_colors  = new float[3*CoordsRead];

    float* ptr  = TRK_coords;
    float* ptrc = TRK_colors;
    float norm;
    VECTOR<float> dir;
    TractsRead = 0;
    fseek(fp, 1000, SEEK_SET);
    for(int f=0; f < TRK_file.hdr.n_count ; f++)
    {
        if ( f%TRK_skip==0 )
        {
            fread( (char*)&N, 1, 4, fp );
            TRK_nPoints[TractsRead] = N;

            for(int i=0; i<N; i++)
            {
                fread((char*)ptr, 1, 12, fp);
                fseek( fp, n_s*4, SEEK_CUR );

                // coordinates
                ptr[0] /= pixdim.x;
                ptr[1] /= pixdim.y;
                ptr[2] /= pixdim.z;

                // colors
                if ( i > 0 )
                {
                    dir.x = *(ptr  ) - *(ptr-3);
                    dir.y = *(ptr+1) - *(ptr-2);
                    dir.z = *(ptr+2) - *(ptr-1);
                    norm = dir.norm();
                    ptrc[0] = abs( dir.x / norm );
                    ptrc[1] = abs( dir.y / norm );
                    ptrc[2] = abs( dir.z / norm );
                }
                else
                {
                    ptrc[0] = 0;
                    ptrc[1] = 0;
                    ptrc[2] = 0;
                }

                ptr  += 3;
                ptrc += 3;
            }
            fseek( fp, n_p*4, SEEK_CUR );
            TractsRead++;
        }
        else
        {
            fread( (char*)&N, 1, 4, fp );
            fseek( fp, N*(3+n_s)*4 + n_p*4, SEEK_CUR );
        }
    }

    TRK_offset.x = 0;
    TRK_offset.y = 0;
    TRK_offset.z = 0;

    COLOR_msg( "   [OK]" );
    printf( "\n" );


    // ============
    // SETUP OpenGL
    // ============
    VOXEL.x = round( dim.x / 2.0 );
    VOXEL.y = round( dim.y / 2.0 );
    VOXEL.z = round( dim.z / 2.0 );
    OpenGL_init( argc, argv );

    return EXIT_SUCCESS;
}
