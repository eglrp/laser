#include <GL/glut.h>
#include <math.h>

unsigned char rgb[256*256*3]={255};
//设置纹理坐标 
static const GLfloat texCoords[] = { 
0.0, 0.0,
1.0, 0.0,
0.0, 1.0,
1.0, 1.0
}; 

static const GLfloat vertices[] = {
-10.0f, -10.0f,
10.0f, -10.0f,
-10.0f, 10.0f,
10.0f, 10.0f
};

static const GLfloat color[] =
{
    0.6f,  0.6f,  0.6f,  1.0f,
    0.0f,  0.0f,  0.0f,  1.0f,
    0.6f,  0.6f,  0.6f,  1.0f,
    0.0f,  0.0f,  0.0f,  1.0f
};

void init();
void display();
void reshape(int width, int height);
void APIENTRY _gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez, 
                         GLdouble centerx, GLdouble centery, GLdouble centerz, 
                         GLdouble upx, GLdouble upy, GLdouble upz );
void _gluPerspective( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar );

int main(int argc, char* argv[]) {

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(300, 300);

    glutCreateWindow("OpenGL 3D View");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);

    init();

    glutMainLoop();
    return 0;
}

void init(){
        memset(rgb,255,256*128*3);
        memset(rgb+256*128*3,128,256*128*3);
//生成纹理名 
        glEnable(GL_TEXTURE_2D);   
        GLuint texture[1];  
        glGenTextures(1, &texture[0]); 
        //纹理绑定使得纹理处在活动状态 
        glBindTexture(GL_TEXTURE_2D, texture[0]); 
        //图像配置 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
        //加载图像数据,图像的宽高必须是2的次方（图像是240，176） 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0,GL_RGB , GL_UNSIGNED_BYTE, rgb); 
}

void display() {
    int i;
    glClear(GL_COLOR_BUFFER_BIT);        
    
    glSubTexImage2D();

    //启用坐标系
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    //传递纹理坐标
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords); 
    glVertexPointer(2, GL_FLOAT, 0, vertices);

    //draw 
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
    glDisableClientState(GL_TEXTURE_COORD_ARRAY); 
    glDisableClientState(GL_VERTEX_ARRAY); 

    glFlush();
}


void reshape(int width, int height){
    int dis = width < height ? width : height;      
    glViewport(0, 0, dis, dis);   /*这里dis应该是500*/ 
    // glViewport(0,0,300,300);
    glClearColor(0.0, 1.0, 0.0, 0.0);
    //glColor3f(0.0, 1.0, 1.0);
    glLoadIdentity();
    glOrtho(-10,10,-10,10,10,-10);
}


void APIENTRY _gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez, 
                         GLdouble centerx, GLdouble centery, GLdouble centerz, 
                         GLdouble upx, GLdouble upy, GLdouble upz ) 
{ 
   GLdouble m[16]; 
   GLdouble x[3], y[3], z[3]; 
   GLdouble mag; 
   /* Make rotation matrix */ 
   /* Z vector */ 
   z[0] = eyex - centerx; 
   z[1] = eyey - centery; 
   z[2] = eyez - centerz; 
   mag = sqrt( z[0]*z[0] + z[1]*z[1] + z[2]*z[2] ); 
   if (mag) {  /* mpichler, 19950515 */ 
      z[0] /= mag; 
      z[1] /= mag; 
      z[2] /= mag; 
   } 
   /* Y vector */ 
   y[0] = upx; 
   y[1] = upy; 
   y[2] = upz; 
   /* X vector = Y cross Z */ 
   x[0] =  y[1]*z[2] - y[2]*z[1]; 
   x[1] = -y[0]*z[2] + y[2]*z[0]; 
   x[2] =  y[0]*z[1] - y[1]*z[0]; 
   /* Recompute Y = Z cross X */ 
   y[0] =  z[1]*x[2] - z[2]*x[1]; 
   y[1] = -z[0]*x[2] + z[2]*x[0]; 
   y[2] =  z[0]*x[1] - z[1]*x[0]; 
   /* mpichler, 19950515 */ 
   /* cross product gives area of parallelogram, which is < 1.0 for 
    * non-perpendicular unit-length vectors; so normalize x, y here 
    */ 
   mag = sqrt( x[0]*x[0] + x[1]*x[1] + x[2]*x[2] ); 
   if (mag) { 
      x[0] /= mag; 
      x[1] /= mag; 
      x[2] /= mag; 
   } 
   mag = sqrt( y[0]*y[0] + y[1]*y[1] + y[2]*y[2] ); 
   if (mag) { 
      y[0] /= mag; 
      y[1] /= mag; 
      y[2] /= mag; 
   } 
#define M(row,col)  m[col*4+row] 
   M(0,0) = x[0];  M(0,1) = x[1];  M(0,2) = x[2];  M(0,3) = 0.0; 
   M(1,0) = y[0];  M(1,1) = y[1];  M(1,2) = y[2];  M(1,3) = 0.0; 
   M(2,0) = z[0];  M(2,1) = z[1];  M(2,2) = z[2];  M(2,3) = 0.0; 
   M(3,0) = 0.0;   M(3,1) = 0.0;   M(3,2) = 0.0;   M(3,3) = 1.0; 
#undef M 
   glMultMatrixd( m ); 
   /* Translate Eye to Origin */ 
   glTranslated( -eyex, -eyey, -eyez ); 
}

// Replaces gluPerspective. Sets the frustum to perspective mode.
// fovY     - Field of vision in degrees in the y direction
// aspect   - Aspect ratio of the viewport
// zNear    - The near clipping distance
// zFar     - The far clipping distance

void _gluPerspective( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
    //Very long (& in theory accurate!) version of Pi. Hopefully an optimizing compiler will replace references to this with the value!
    const GLdouble pi = 3.1415926535897932384626433832795;
    //Half of the size of the x and y clipping planes.
    GLdouble fW, fH;
    //Calculate the distance from 0 of the y clipping plane. Basically trig to calculate position of clipper at zNear.
    //Note: tan( double ) uses radians but OpenGL works in degrees so we convert degrees to radians by dividing by 360 then multiplying by pi.
    //Formula below corrected by Carsten Jurenz:
    fH = tan( (fovY / 2) / 180 * pi ) * zNear;
    //Which can be reduced to:
    fH = tan( fovY / 360 * pi ) * zNear;
    //Calculate the distance from 0 of the x clipping plane based on the aspect ratio.
    fW = fH * aspect;
    //Finally call glFrustum, this is all gluPerspective does anyway! This is why we calculate half the distance between the clipping planes - glFrustum takes an offset from zero for each clipping planes distance. (Saves 2 divides)
    glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}