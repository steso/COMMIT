#define GL_GLEXT_PROTOTYPES 1
#ifdef __APPLE__
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <GL/glut.h>
#endif

#include "OPENGL_utils.h"
using namespace OPENGL_utils;


/* global variables */
GLfloat			id[16], rot[16], rot1[16], rot2[16], rot3[16];
Vec3Df			rotation, translation;
Vec3Di			start;
GLint			moving;
GLfloat			zoom;

float ScreenX = 800, ScreenY = 600;


void PrintConfig()
{
    printf( "=======================\n     CONFIGURATION     \n=======================\n" );
    printf( "\t- showPLANE = [ %d, %d, %d ]\n", showPlane[0], showPlane[1], showPlane[2] );
    printf( "\t- MAP_range = [ %.1f ... %.1f]\n", MAP_min_view, MAP_max_view );
    printf( "\t- MAP_opacity = %.1f\n", MAP_opacity );
    printf( "\n" );
    printf( "\t- PEAKS_doNormalize = %s\n", PEAKS_doNormalize?"true":"false" );
    printf( "\t- PEAKS_thr = %.1f\n", PEAKS_thr );
    printf( "\t- PEAKS_flip = [ %d, %d, %d ]\n", PEAKS_flip[0], PEAKS_flip[1], PEAKS_flip[2] );
    printf( "\t- PEAKS_width = %.1f\n", PEAKS_width );
    printf( "\n" );
    printf( "\t- TRK_offset = [ %.1f %.1f %.1f]    (voxel-size units)\n", TRK_offset.x, TRK_offset.y, TRK_offset.z );
    printf( "\t- TRK_crop = %.1f  (voxel-size units)\n", TRK_crop );
    printf( "\n" );
    printf( "\t- GLYPHS_b0_thr = %.1f\n", GLYPHS_b0_thr );
    printf( "\t- GLYPHS_flip = [ %d, %d, %d ]\n", GLYPHS_flip[0], GLYPHS_flip[1], GLYPHS_flip[2] );
    printf( "\n" );
}


// RESHAPE callback
// ----------------
void GLUT__reshape( GLint w, GLint h )
{
    ScreenX = w;
    ScreenY = h;

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 45.0f, (GLfloat)w / (GLfloat)h, 1.0f, 1000.0f );

    glMatrixMode( GL_MODELVIEW );
    glViewport( 0, 0, w, h );
}



// KEYBOARD callback
// -----------------
void GLUT__keyboard( unsigned char key, GLint x, GLint y )
{
    bool doRedraw = true;
    GLint modif = glutGetModifiers();
    GLint ALT   = modif & GLUT_ACTIVE_ALT;
    // GLint CTRL  = SHIFT & GLUT_ACTIVE_CTRL;
    // GLint SHIFT = SHIFT & GLUT_ACTIVE_SHIFT;

    switch( key )
    {
        case '1': showPlane[0] = 1 - showPlane[0]; break;
        case '2': showPlane[1] = 1 - showPlane[1]; break;
        case '3': showPlane[2] = 1 - showPlane[2]; break;

        case 'x': PEAKS_flip[0] = 1 - PEAKS_flip[0]; break;
        case 'y': PEAKS_flip[1] = 1 - PEAKS_flip[1]; break;
        case 'z': PEAKS_flip[2] = 1 - PEAKS_flip[2]; break;

        case 'X': GLYPHS_flip[0] = 1 - GLYPHS_flip[0]; for(int d=0; d < GLYPHS_dirs.size() ;d++) GLYPHS_dirs[d].x *= -1; break;
        case 'Y': GLYPHS_flip[1] = 1 - GLYPHS_flip[1]; for(int d=0; d < GLYPHS_dirs.size() ;d++) GLYPHS_dirs[d].y *= -1; break;
        case 'Z': GLYPHS_flip[2] = 1 - GLYPHS_flip[2]; for(int d=0; d < GLYPHS_dirs.size() ;d++) GLYPHS_dirs[d].z *= -1; break;

        case 't': PEAKS_thr = fmaxf(PEAKS_thr - 0.1, 0.0); break;
        case 'T': PEAKS_thr = fminf(PEAKS_thr + 0.1, 1.0); break;

        case 'n': PEAKS_doNormalize = 1 - PEAKS_doNormalize; break;

        case 'w': PEAKS_width = max(1,PEAKS_width-1); break;
        case 'W': PEAKS_width = min(5,PEAKS_width+1); break;

        case 'o': MAP_opacity = fmaxf(0.0,MAP_opacity-0.1); break;
        case 'O': MAP_opacity = fminf(1.0,MAP_opacity+0.1); break;

        case 'm': MAP_max_view = fmaxf(0.0,MAP_max_view-MAP_max*0.05); break;
        case 'M': MAP_max_view = fminf(MAP_max,MAP_max_view+MAP_max*0.05); break;

        case 'c': TRK_crop = fmaxf( 0.0,TRK_crop-0.5); break;
        case 'C': TRK_crop = fminf(max(dim.x,max(dim.y,dim.z)),TRK_crop+0.5); break;
        case ' ': TRK_crop_mode = 1 - TRK_crop_mode; break;

        case 'f': TRK_show    = 1 - TRK_show;    break;
        case 's': GLYPHS_show = 1 - GLYPHS_show; break;
        case 'p': PEAKS_show  = 1 - PEAKS_show;  break;

        case 'b': GLYPHS_b0_thr = fmaxf(0.0,GLYPHS_b0_thr-10.0); break;
        case 'B': GLYPHS_b0_thr = fminf(MAP_max,GLYPHS_b0_thr+10.0); break;

        case 'r':
            translation.x	= translation.y = 0;
            rotation.x		= rotation.y = rotation.z = 0;
            zoom			= 200;
            OPENGL_utils::identity( rot );
            break;

        case 27 : exit(0); break;

        default: doRedraw = false;
    }

    if ( doRedraw )
    {
        PrintConfig();
        glutPostRedisplay();
    }
}


// SPECIALKEY callback
// -------------------
void GLUT__specialkey( GLint key, GLint x, GLint y )
{
    bool doRedraw = true;
    GLint modif = glutGetModifiers();
    GLint ALT   = modif & GLUT_ACTIVE_ALT;

    switch( key )
    {
        case GLUT_KEY_LEFT:
            if ( ALT )
                TRK_offset.x -= 0.5;
            else
                VOXEL.x--;
            break;
        case GLUT_KEY_RIGHT:
            if ( ALT )
                TRK_offset.x += 0.5;
            else
                VOXEL.x++;
            break;
        case GLUT_KEY_DOWN:
            if ( ALT )
                TRK_offset.y -= 0.5;
            else
                VOXEL.y--;
            break;
        case GLUT_KEY_UP:
            if ( ALT )
                TRK_offset.y += 0.5;
            else
                VOXEL.y++;
            break;
        case GLUT_KEY_PAGE_DOWN:
            if ( ALT )
                TRK_offset.z -= 0.5;
            else
                VOXEL.z--;
            break;
        case GLUT_KEY_PAGE_UP:
            if ( ALT )
                TRK_offset.z += 0.5;
            else
                VOXEL.z++;
            break;

        default: 					doRedraw = false;
    }

    // check the bounds
    VOXEL.x = max( VOXEL.x, 0 );
    VOXEL.y = max( VOXEL.y, 0 );
    VOXEL.z = max( VOXEL.z, 0 );
    VOXEL.x = min( VOXEL.x, dim.x-1 );
    VOXEL.y = min( VOXEL.y, dim.y-1 );
    VOXEL.z = min( VOXEL.z, dim.z-1 );

    if ( doRedraw )
    {
        PrintConfig();
        glutPostRedisplay();
    }
}



// MOUSE callback
// --------------
void GLUT__mouse( GLint button, GLint state, GLint x, GLint y )
{
    if (state == GLUT_DOWN)
    {
        if ( button == GLUT_LEFT_BUTTON && glutGetModifiers() != GLUT_ACTIVE_ALT )
        {
            moving = 1;
            start.x = x;
            start.y = y;
        }
        else if ( button == GLUT_RIGHT_BUTTON )
        {
            moving = 2;
            start.x = x;
            start.y = y;
        }
        else if ( (button == GLUT_MIDDLE_BUTTON) || (button == GLUT_LEFT_BUTTON && glutGetModifiers() == GLUT_ACTIVE_ALT) )
        {
            moving = 3;
            start.x = x;
            start.y = y;
        }
    }
    else if (state == GLUT_UP)
    {
        moving = 0;
    }
}



// MOTION callback
// ---------------
void GLUT__motion( GLint x, GLint y )
{
    if (moving==1)
    {
        rotation.y = (start.x-x) / 1;
        rotation.x = (start.y-y) / 1;

        // porto il centro di rotazione nel baricentro
        OPENGL_utils::translate(id, 0,0,0, rot1);

        OPENGL_utils::rotateY(id,rotation.y,rot3);
        OPENGL_utils::matXMat(rot,rot1,rot2);
        OPENGL_utils::rotateX(id,rotation.x,rot1);
        OPENGL_utils::matXMat(rot2,rot1,rot);
        OPENGL_utils::matXMat(rot,rot3,rot2);

        // riporto il centro di rotazione in pos. originale
        OPENGL_utils::translate(id, 0,0,0, rot1);
        OPENGL_utils::matXMat(rot2,rot1,rot);

        start.x = x;
        start.y = y;
    }

    else if (moving==2)
    {
        zoom = zoom + (y-start.y)/2.0;
        start.y = y;
    }

    else if (moving==3)
    {
        translation.x = translation.x - (start.x-x)/3.0;
        translation.y = translation.y + (start.y-y)/3.0;
        start.x = x;
        start.y = y;
    }

    glutPostRedisplay();
}


// DISPLAY callback
// ----------------
void GLUT__display( void )
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // MOUSE translation + rotation
    glPushMatrix();
    glTranslatef(translation.x, translation.y, -zoom);
    glMultMatrixf(rot);

    // center the FOV
    glTranslatef( -dim.x/2.0, -dim.y/2.0, -dim.z/2.0 );


    /* ============= */
    /* Draw the AXES */
    /* ============= */
    glLineWidth(2);
    glBegin(GL_LINES);
        glColor4f( 1,0,0,1); glVertex3f( 0,0,0 ); glVertex3f( 10,  0,  0 );
        glColor4f( 0,1,0,1); glVertex3f( 0,0,0 ); glVertex3f(  0, 10,  0 );
        glColor4f( 0,0,1,1); glVertex3f( 0,0,0 ); glVertex3f(  0,  0, 10 );
    glEnd();


    /* =============== */
    /* Draw the TRACTS */
    /* =============== */
    if ( TRK_show )
    {
        glPushMatrix();
        glTranslatef(TRK_offset.x, TRK_offset.y, TRK_offset.z);

        glLineWidth(1.0f);

        float *ptr  = TRK_coords, *ptrc = TRK_colors;
        VECTOR<float> Vc( VOXEL.x+0.5, VOXEL.y+0.5, VOXEL.z+0.5 ); // voxel center
        float thr = 0.5*TRK_crop;
        for(int f=0; f < TRK_nTractsPlotted; f++)
        {
            glBegin(GL_LINE_STRIP);
            for(int i=0; i < TRK_nPoints[f]; i++)
            {
                // plot segment only if it's close to center of VOXEL
                if (
                      (
                        TRK_crop_mode && (
                        ( showPlane[0] && abs( (ptr[0]+TRK_offset.x) - Vc.x ) <= thr ) ||
                        ( showPlane[1] && abs( (ptr[1]+TRK_offset.y) - Vc.y ) <= thr ) ||
                        ( showPlane[2] && abs( (ptr[2]+TRK_offset.z) - Vc.z ) <= thr ) )
                      )
                      ||
                      (
                        !TRK_crop_mode && (
                        ( abs( (ptr[0]+TRK_offset.x) - Vc.x ) <= thr ) &&
                        ( abs( (ptr[1]+TRK_offset.y) - Vc.y ) <= thr ) &&
                        ( abs( (ptr[2]+TRK_offset.z) - Vc.z ) <= thr ) )
                      )
                    )
                {
                    glColor3f(  *ptrc++, *ptrc++, *ptrc++ );
                    glVertex3f( *ptr++,  *ptr++,  *ptr++  );
                }
                else
                {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    ptr  += 3;
                    ptrc += 3;
                }
            }
            glEnd();
        }

        glPopMatrix();
    }


    /* ============== */
    /* Draw the PEAKS */
    /* ============== */
    if ( PEAKS_show || GLYPHS_show )
    {
        glDisable( GL_BLEND );
        glLineWidth( PEAKS_width );
        glPointSize( PEAKS_width );

        //
        glPushMatrix();
        glTranslatef(.5,.5,.5);

        Vec3Df dir;
        int x,y,z,d;
        float norms[PEAKS_n], normMax, b0, w;

        // plane YZ
        if ( showPlane[0]  )
        {
            x = (int)VOXEL.x;
            for(y=0; y<dim.y ;y++)
            for(z=0; z<dim.z ;z++)
            {
                if ( PEAKS_show )
                {
                    normMax = 0;
                    for(d=0; d<PEAKS_n; d++)
                    {
                        dir.x = (*niiPEAKS->img)(x,y,z,3*d+0);
                        dir.y = (*niiPEAKS->img)(x,y,z,3*d+1);
                        dir.z = (*niiPEAKS->img)(x,y,z,3*d+2);
                        norms[d] = dir.norm();
                        if ( norms[d] > normMax )
                            normMax = norms[d];
                    }

                    for(d=0; d<PEAKS_n; d++)
                    {
                        if ( norms[d] < PEAKS_thr*normMax )
                            continue;

                        if ( PEAKS_doNormalize )
                        {
                            dir.x = 0.5 * (PEAKS_flip[0]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+0) / norms[d];
                            dir.y = 0.5 * (PEAKS_flip[1]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+1) / norms[d];
                            dir.z = 0.5 * (PEAKS_flip[2]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+2) / norms[d];
                        }
                        else
                        {
                            dir.x = 0.5 * (PEAKS_flip[0]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+0) / normMax;
                            dir.y = 0.5 * (PEAKS_flip[1]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+1) / normMax;
                            dir.z = 0.5 * (PEAKS_flip[2]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+2) / normMax;
                        }

                        glColor3f( fabs(2.0*dir.x), fabs(2.0*dir.y), fabs(2.0*dir.z) );
                        glBegin(GL_LINES);
                            glVertex3f( x-dir.x, y-dir.y, z-dir.z );
                            glVertex3f( x+dir.x, y+dir.y, z+dir.z );
                        glEnd();
                    }
                }
                if ( GLYPHS_show )
                {
                    b0 = (*niiDWI->img)(x,y,z,SCHEME_idxB0[0]);
                    if ( b0 > GLYPHS_b0_thr )
                    {
                        glBegin(GL_POINTS);
                        for(d=0; d < GLYPHS_dirs.size() ;d++)
                        {
                            w = (float)(*niiDWI->img)(x,y,z,GLYPHS_idx[d]) / b0;
                            dir.x = 0.5 * w * GLYPHS_dirs[d].x;
                            dir.y = 0.5 * w * GLYPHS_dirs[d].y;
                            dir.z = 0.5 * w * GLYPHS_dirs[d].z;

                            normMax = dir.norm();
                            glColor3f( fabs(dir.x)/normMax, fabs(dir.y)/normMax, fabs(dir.z)/normMax );
                            glVertex3f( x+dir.x, y+dir.y, z+dir.z );
                            glVertex3f( x-dir.x, y-dir.y, z-dir.z );
                        }
                        glEnd();
                    }
                }
            }
        }

        // plane XZ
        if ( showPlane[1] )
        {
            y = (int)VOXEL.y;
            for(x=0; x<dim.x ;x++)
            for(z=0; z<dim.z ;z++)
            {
                if ( PEAKS_show )
                {
                    normMax = 0;
                    for(d=0; d<PEAKS_n; d++)
                    {
                        dir.x = (*niiPEAKS->img)(x,y,z,3*d+0);
                        dir.y = (*niiPEAKS->img)(x,y,z,3*d+1);
                        dir.z = (*niiPEAKS->img)(x,y,z,3*d+2);
                        norms[d] = dir.norm();
                        if ( norms[d] > normMax )
                            normMax = norms[d];
                    }

                    for(d=0; d<PEAKS_n; d++)
                    {
                        if ( norms[d] < normMax*PEAKS_thr )
                            continue;

                        if ( PEAKS_doNormalize )
                        {
                            dir.x = 0.5 * (PEAKS_flip[0]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+0) / norms[d];
                            dir.y = 0.5 * (PEAKS_flip[1]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+1) / norms[d];
                            dir.z = 0.5 * (PEAKS_flip[2]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+2) / norms[d];
                        }
                        else
                        {
                            dir.x = 0.5 * (PEAKS_flip[0]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+0) / normMax;
                            dir.y = 0.5 * (PEAKS_flip[1]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+1) / normMax;
                            dir.z = 0.5 * (PEAKS_flip[2]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+2) / normMax;
                        }

                        glColor3f( fabs(2.0*dir.x), fabs(2.0*dir.y), fabs(2.0*dir.z) );
                        glBegin(GL_LINES);
                            glVertex3f( x-dir.x, y-dir.y, z-dir.z );
                            glVertex3f( x+dir.x, y+dir.y, z+dir.z );
                        glEnd();
                    }
                }
                if ( GLYPHS_show )
                {
                    b0 = (*niiDWI->img)(x,y,z,SCHEME_idxB0[0]);
                    if ( b0 > GLYPHS_b0_thr )
                    {
                        glBegin(GL_POINTS);
                        for(d=0; d < GLYPHS_dirs.size() ;d++)
                        {
                            w = (float)(*niiDWI->img)(x,y,z,GLYPHS_idx[d]) / b0;
                            dir.x = 0.5 * w * GLYPHS_dirs[d].x;
                            dir.y = 0.5 * w * GLYPHS_dirs[d].y;
                            dir.z = 0.5 * w * GLYPHS_dirs[d].z;

                            normMax = dir.norm();
                            glColor3f( fabs(dir.x)/normMax, fabs(dir.y)/normMax, fabs(dir.z)/normMax );
                            glVertex3f( x+dir.x, y+dir.y, z+dir.z );
                            glVertex3f( x-dir.x, y-dir.y, z-dir.z );
                        }
                        glEnd();
                    }
                }
            }
        }

        // plane XY
        if ( showPlane[2] )
        {
            z = (int)VOXEL.z;
            for(y=0; y<dim.y ;y++)
            for(x=0; x<dim.x ;x++)
            {
                if ( PEAKS_show )
                {
                    normMax = 0;
                    for(d=0; d<PEAKS_n; d++)
                    {
                        dir.x = (*niiPEAKS->img)(x,y,z,3*d+0);
                        dir.y = (*niiPEAKS->img)(x,y,z,3*d+1);
                        dir.z = (*niiPEAKS->img)(x,y,z,3*d+2);
                        norms[d] = dir.norm();
                        if ( norms[d] > normMax )
                            normMax = norms[d];
                    }

                    for(d=0; d<PEAKS_n; d++)
                    {
                        if ( norms[d] < normMax*PEAKS_thr )
                            continue;

                        if ( PEAKS_doNormalize )
                        {
                            dir.x = 0.5 * (PEAKS_flip[0]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+0) / norms[d];
                            dir.y = 0.5 * (PEAKS_flip[1]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+1) / norms[d];
                            dir.z = 0.5 * (PEAKS_flip[2]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+2) / norms[d];
                        }
                        else
                        {
                            dir.x = 0.5 * (PEAKS_flip[0]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+0) / normMax;
                            dir.y = 0.5 * (PEAKS_flip[1]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+1) / normMax;
                            dir.z = 0.5 * (PEAKS_flip[2]?-1:1) * (*niiPEAKS->img)(x,y,z,3*d+2) / normMax;
                        }

                        glColor3f( fabs(2.0*dir.x), fabs(2.0*dir.y), fabs(2.0*dir.z) );
                        glBegin(GL_LINES);
                            glVertex3f( x-dir.x, y-dir.y, z-dir.z );
                            glVertex3f( x+dir.x, y+dir.y, z+dir.z );
                        glEnd();
                    }
                }

                if( GLYPHS_show)
                {
                    b0 = (*niiDWI->img)(x,y,z,SCHEME_idxB0[0]);
                    if ( b0 > GLYPHS_b0_thr )
                    {
                        glBegin(GL_POINTS);
                        for(d=0; d < GLYPHS_dirs.size() ;d++)
                        {
                            w = (float)(*niiDWI->img)(x,y,z,GLYPHS_idx[d]) / b0;
                            dir.x = 0.5 * w * GLYPHS_dirs[d].x;
                            dir.y = 0.5 * w * GLYPHS_dirs[d].y;
                            dir.z = 0.5 * w * GLYPHS_dirs[d].z;

                            normMax = dir.norm();
                            glColor3f( fabs(dir.x)/normMax, fabs(dir.y)/normMax, fabs(dir.z)/normMax );
                            glVertex3f( x+dir.x, y+dir.y, z+dir.z );
                            glVertex3f( x-dir.x, y-dir.y, z-dir.z );
                        }
                        glEnd();
                    }
                }
            }
        }

        glPopMatrix();
    }



    /* =================== */
    /* Draw the SCALAR MAP */
    /* =================== */
    if ( showPlane[0] || showPlane[1] || showPlane[2] )
    {
        glDisable( GL_CULL_FACE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        // to avoid z-fighting
        glPolygonOffset( 1.0, 1.0 );
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glLineWidth( 3 );

        int x, y, z; // voxel coordinates NB: (0,0,0) -> corner of voxel
        float color;

        // plane YZ
        if ( showPlane[0]  )
        {
            glPushMatrix();
            glTranslatef(0.5,0,0);

            x = (int)VOXEL.x;
            for(y=0; y<dim.y ;y++)
            for(z=0; z<dim.z ;z++)
            {
                color = ( MAP(x,y,z) - MAP_min_view) / ( MAP_max_view - MAP_min_view );
                glColor4f(color,color,color,MAP_opacity);
                glBegin(GL_QUADS);
                    glVertex3f(x, y,   z);
                    glVertex3f(x, y,   z+1);
                    glVertex3f(x, y+1, z+1);
                    glVertex3f(x, y+1, z);
                glEnd();
            }
            // colored frame
            glColor3f(1,0,0);
            glBegin(GL_LINE_STRIP);
                glVertex3f(x,0,0);
                glVertex3f(x,dim.y,0);
                glVertex3f(x,dim.y,dim.z);
                glVertex3f(x,0,dim.z);
                glVertex3f(x,0,0);
            glEnd();

            glPopMatrix();
        }

        // plane XZ
        if ( showPlane[1] )
        {
            glPushMatrix();
            glTranslatef(0,0.5,0);

            y = (int)VOXEL.y;
            for(x=0; x<dim.x ;x++)
            for(z=0; z<dim.z ;z++)
            {
                color = ( MAP(x,y,z) - MAP_min_view) / ( MAP_max_view - MAP_min_view );
                glColor4f(color,color,color,MAP_opacity);
                glBegin(GL_QUADS);
                    glVertex3f(x,   y, z);
                    glVertex3f(x,   y, z+1);
                    glVertex3f(x+1, y, z+1);
                    glVertex3f(x+1, y, z);
                glEnd();
            }
            // colored frame
            glColor3f(0,1,0);
            glBegin(GL_LINE_STRIP);
                glVertex3f(0,y,0);
                glVertex3f(dim.x,y,0);
                glVertex3f(dim.x,y,dim.z);
                glVertex3f(0,y,dim.z);
                glVertex3f(0,y,0);
            glEnd();

            glPopMatrix();
        }

        // plane XY
        if ( showPlane[2] )
        {
            glPushMatrix();
            glTranslatef(0,0,0.5);

            z = (int)VOXEL.z;
            for(y=0; y<dim.y ;y++)
            for(x=0; x<dim.x ;x++)
            {
                color = ( MAP(x,y,z) - MAP_min_view) / ( MAP_max_view - MAP_min_view );
                glColor4f(color,color,color,MAP_opacity);
                glBegin(GL_QUADS);
                    glVertex3f(x,   y,   z);
                    glVertex3f(x+1, y,   z);
                    glVertex3f(x+1, y+1, z);
                    glVertex3f(x,   y+1, z);
                glEnd();
            }

            // colored frame
            glColor3f(0,0,1);
            glBegin(GL_LINE_STRIP);
                glVertex3f(0,0,z);
                glVertex3f(dim.x,0,z);
                glVertex3f(dim.x,dim.y,z);
                glVertex3f(0,dim.y,z);
                glVertex3f(0,0,z);
            glEnd();

            glPopMatrix();
        }

        glEnable(GL_CULL_FACE);
        glDisable( GL_BLEND );
        glDisable(GL_POLYGON_OFFSET_FILL);
    }


    /* ====================== */
    /* Draw the CURRENT VOXEL */
    /* ====================== */
    glPushMatrix();
    glTranslatef( VOXEL.x+0.5, VOXEL.y+0.5, VOXEL.z+0.5 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glLineWidth(1);
    glColor4f( 1,1,0,1 );
    glutWireCube( 1 );
    glColor4f( 1,1,0,0.25 );
    glutSolidCube( 1 );
    glDisable( GL_BLEND );

    glPopMatrix();


    glPopMatrix();
    glutSwapBuffers();
}


// INITIALIZATION
// --------------
void OpenGL_init( int argc, char** argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA );
    glutInitWindowSize( ScreenX, ScreenY );
    glutCreateWindow( "COMMIT debugger" );

    // Projection and model matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0f, (GLfloat)ScreenX / (GLfloat)ScreenY, 10.0f,1000.0f);
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(
        0.0, 0.0, 120.0,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0
    );

    translation.x	= translation.y = 0;
    rotation.x		= rotation.y = rotation.z = 0;
    zoom			= 200;
    OPENGL_utils::identity( rot );
    OPENGL_utils::identity( id );

    // basic settings
    // glEnable( GL_LINE_SMOOTH );
    // glEnable( GL_POLYGON_SMOOTH );
    // glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    // glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

// 	glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glClearColor( 0.1, 0.1, 0.1, 0.0 );

    // lighting
    glShadeModel( GL_SMOOTH );
    glEnable( GL_NORMALIZE );

    GLfloat white[] = {.5f, .5f, .5f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    GLfloat shininess[] = {32};
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    GLfloat global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glEnable ( GL_COLOR_MATERIAL );	// use glColor3f() to colorize polygons

    // register CALLBACKS and open window
    glutKeyboardFunc( GLUT__keyboard );
    glutSpecialFunc(  GLUT__specialkey );
    glutDisplayFunc(  GLUT__display );
    glutReshapeFunc(  GLUT__reshape );
    glutMouseFunc(    GLUT__mouse );
    glutMotionFunc(   GLUT__motion );

    PrintConfig();

    glutMainLoop();
}
