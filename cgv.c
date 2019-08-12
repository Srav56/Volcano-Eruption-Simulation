/*
	This program is a graphical simulation of a biplane flying around an active volcano with eruption and lava flow.
The program is mostly based on my knowledge of Mathematics and OpenGL and 9 practicals of CGV module. The books I use as a guide are "OpenGL Programming Guide (Redbook)" and "OpenGL Superbible". This is the final version of the program which is submitted for CGV open assessment.
*/
#include <stdio.h>
#include <stdlib.h>
#include "OPENGL/gl.h"
#include "OPENGL/glu.h"
#include "GLUT/glut.h"
#include <math.h>
#include <time.h>
#include <float.h>
#include "readtex.c"   
#define TEX0 "images/texture0.rgb"
#define TEX1 "images/texture1.rgb"
#define TEX2 "images/texture2.rgb"
#define TEX3 "images/texture3.rgb"
#define TEX4 "images/texture4.rgb"


#define VIEW1 1
#define VIEW2 2
#define VIEW3 3
#define VIEW4 4
#define T 64
#define XMT T/2
#define ZMT T/2
#define YMAX 150
#define UNDEFINED -999999
#define M 15.625
#define PI 3.14159265358979
#define SMOKE 10
#define SMOKE_MAX 700
#define ROCK 100
#define ROCK_MAX 2000
#define LAVA 100
#define LAVA_MAX 40000

/* global variables */
GLfloat gravity =9.8;
GLfloat smokePart[SMOKE_MAX][8];
GLfloat rockPart[ROCK_MAX][8];
GLfloat lavaPart[LAVA_MAX][6];
GLuint terrain, planeBody, planeEngine, planeTail, planeShield, planeWing;
GLfloat propAng  = 0.0;
GLfloat propDelta  = 41.0;
GLfloat rudderAng  = 0.0;
GLfloat elevatorAng  = 0.0;
GLint smokeFlag = 0;
GLint rockFlag = 0;
static GLuint tNum[7];

int width, height;
int rock_particles = 50;
int smoke_particles = 30;
int lava_particles = 0;
int smokeNum = 0;
int rockNum = 0;
int lavaNum = 0;
int view_flag = VIEW4;

float terrainY[T+1][T+1];
float tNorm[T+1][T+1][3];
float viewAng1 = 33.0*PI/90.0, viewAng2=0.0;
float planeX = 50.0, planeY = 350.0, planeZ = 50.0, pLatAng = 90.0, pAltAng = 0.0;

/* list of functions */
void left_menu(int);
void init_terrain(void);
void cylinder(float, float, GLuint);
void makeNormals( GLfloat[][3], int[], int, GLfloat[][3]);
void update_rock(int, float);
void update_smoke(int, float);
void update_lava(int);
void new_rock(int);
void new_smoke(int);
void new_lava(int);
void ground_test(int);
void roof_test(int);
void lava_test(int);
int collision(float[], float[], float[], float[], float[]);
/*************************************/
/*
	This function generates random Y values. When it is called for the first time it takes the oppisite edges of the terrain and recursively generates Y values for all 65X65 points of the terrain. In each call to this function it first generates a Y value for the point in the middle of the square. Then 4 values for 4 points which are the middles of the edges of the square. At this time there are 4 squares inside the current square. the function calls itself for each of these 4 squares and does the same. The recursion stops when all calls are returned. The function is blocked (does nothing and return) when there is no point on the edges of the square. Based on 64X64 terrain net these happen in depth 7.
	As it is possible that for some points on the common edges of squares the Y value is already calculated there is no need to redo it. Thus before calculation and assignment all Y values are checked to be still UNDEFINED.
*/
void gen_terrain(int x1, int z1, int x2, int z2, int depth){
	if (x2-x1>1){
		int x3 = (x2+x1)/2;
		int z3 = (z2+z1)/2;

		terrainY[x3][z3] = (terrainY[x1][z1]+terrainY[x2][z1]+terrainY[x1][z2]+terrainY[x2][z2])/4.0 + (drand48()-0.5)*YMAX/(float) depth;
		terrainY[x2][z3] = (terrainY[x2][z1]+terrainY[x2][z2])/2.0 + (drand48()-0.5)*YMAX/(float) depth;
		terrainY[x3][z2] = (terrainY[x1][z2]+terrainY[x2][z2])/2.0 + (drand48()-0.5)*YMAX/(float) depth;
		if (terrainY[x1][z3] == UNDEFINED)
			terrainY[x1][z3] = (terrainY[x1][z1]+terrainY[x1][z2])/2.0 + (drand48()-0.5)*YMAX/(float) depth;
		if (terrainY[x3][z1] == UNDEFINED)
			terrainY[x3][z1] = (terrainY[x1][z1]+terrainY[x2][z1])/2.0 + (drand48()-0.5)*YMAX/(float) depth;

		gen_terrain(x1 , z1, x3, z3, depth+1);
		gen_terrain(x1 , z3, x3, z2, depth+1);
		gen_terrain(x3 , z1, x2, z3, depth+1);
		gen_terrain(x3 , z3, x2, z2, depth+1);
	}
}
/*************************************/
/*
	This function takes X and Z values of a point and returns the distance from the center of the terrain net.
	It is used in "gen_volcano" function.
*/
float distance(int x, int z){
	return sqrt((XMT-x)*(XMT-x)+(ZMT-z)*(ZMT-z));
}
/*************************************/
/*
	This function adds some values to the Y values of points within a circle in the middle of the terrain to make it look like a volcano. The mathematical function that is used to generate the values is "Y = 780 * X / exp(X)". X is based on the distance of each point from the center of the terrain and is scaled to meet the requirements of the program. A random value between 0 to 40 is also added to it to make the volcanno even more random. Based on these new values for the points within a smaller circle in the center the Y values are modified to shape the crater. They are computed as the average of some points slightly below the edges of the crater.
*/
void gen_volcano(){
	int i,j;
	float d;

	for (i=3*T/16 ; i<=13*T/16 ; i++)
		for (j=3*T/16 ; j<=13*T/16 ; j++){
			d = distance(i,j);
			if (d < 5*T/16){
				if (d >= T/16)
					terrainY[i][j] += 780 * (d / 3.5) / exp( d / 3.5) + (drand48() * 40.0);
				else
					terrainY[i][j] = (terrainY[7*T/16 -1][T/2] + terrainY[T/2][7*T/16 -1] +
											terrainY[9*T/16 +1][T/2] + terrainY[T/2][9*T/16 +1]) /4.0 + 30.0;
			}
		}
}
/*************************************/
/*
	This function smooth the terrain to look like a real lanscape. To do this for every point on the terrain net the Y value is replaced with the average Y of its own and all its neighbours. There are 4 points on the corners with 3 neighbours. Any other point on the edges has 5 neighbours. Any point inside the terrain net has 8 neighbours.
*/
void smooth_terrain(void){
	int i,j;

	for (i=1; i<T; i++)
		terrainY[0][i] = (	terrainY[0][i-1]+terrainY[0][i]+terrainY[0][i+1]+terrainY[1][i-1]+terrainY[1][i]+terrainY[1][i+1] ) / 6.0;
		terrainY[T+1][i] = (	terrainY[T+1][i-1]+terrainY[T+1][i]+terrainY[T+1][i+1]+terrainY[T][i-1]+terrainY[T][i]+terrainY[T][i+1] ) / 6.0;

		terrainY[i][0] = (	terrainY[i-1][0]+terrainY[i][0]+terrainY[i+1][0]+terrainY[i-1][1]+terrainY[i][1]+terrainY[i+1][1] ) / 6.0;
		terrainY[i][T+1] = (	terrainY[i-1][T+1]+terrainY[i][T+1]+terrainY[i+1][T+1]+terrainY[i-1][T]+terrainY[i][T]+terrainY[i+1][T] ) / 6.0;

	for (i=1; i<T; i++)
		for (j=1; j<T; j++)
				terrainY[i][j] = (	terrainY[i-1][j-1]+terrainY[i-1][j]+terrainY[i-1][j+1]+
											terrainY[i][j-1]+terrainY[i][j]+terrainY[i][j+1]+
											terrainY[i+1][j-1]+terrainY[i+1][j]+terrainY[i+1][j+1])/9.0;

	terrainY[0][0] = ( terrainY[0][0]+terrainY[0][1]+terrainY[1][0]+terrainY[1][1] ) / 4.0;
	terrainY[T+1][0] = ( terrainY[T+1][0]+terrainY[T+1][1]+terrainY[T][0]+terrainY[T][1] ) / 4.0;
	terrainY[0][T+1] = ( terrainY[0][T+1]+terrainY[1][T+1]+terrainY[0][T]+terrainY[1][T] ) / 4.0;
	terrainY[T+1][T+1] = ( terrainY[T+1][T+1]+terrainY[T][T+1]+terrainY[T+1][T]+terrainY[T][T] ) / 4.0;

}
/*************************************/
/*
	The array of 1D texture for the terrain. It holds orange-red for the center of the volcano, black for the crater and green for the rest of the terrain.
*/
GLfloat terrainTex[16][3] ={{1.0,0.2,0.0},{0.0,0.0,0.0},{0.0,0.1,0.0},{0.0,0.2,0.0},
										{0.0,0.3,0.0},{0.0,0.35,0.0},{0.0,0.35,0.0},{0.0,0.35,0.0},
										{0.0,0.35,0.0},{0.0,0.35,0.0},{0.0,0.35,0.0},{0.0,0.35,0.0},
										{0.0,0.35,0.0},{0.0,0.35,0.0},{0.0,0.35,0.0},{0.0,0.35,0.0}};
/*************************************/
/*
	This function draws a single point when it is called inside a glBegin-glEnd block. It takes the coordinates of a point, computes the texture value and assigns texture and normal.
*/
void points(int i, int j){
	float dist = sqrt((XMT-i)*(XMT-i)+(ZMT-j)*(ZMT-j));
	glTexCoord1f(dist / 46.0);
	glNormal3f(tNorm[i][j][0],tNorm[i][j][1],tNorm[i][j][2]);
	glVertex3f(i*M, terrainY[i][j], j*M);
}
/*************************************/
/*
	This function initialises the terrain as a display-list. At the beginning it sets all Y values to UNDEFINED which is a very large negative number. After assigning random values to 4 corner points of the terrain net, it calls functions to generate and smooth Y values of the points. The next stage is to create a normal vector for each vertex. The normal of a vertex is the average of the normals of triangles the vertex belongs to. Every vertex inside the terrain is common between 6 triangles and every vertex on the edges is common between 3 triangles. Corner vertices belong to either 1 or 2 triangles. After generation normals the "terrain" display-list is created which includes the terrain, volcano and a brown surrounding wall aroud the terrain to make it look like a piece of land (e.g. an iceland).
*/
void init_terrain(){
	int i, j;

	for (i=0; i<=T; i++)
		for (j=0; j<=T; j++)
			terrainY[i][j] = UNDEFINED;

	terrainY[0][0] = (drand48()-0.5)*YMAX;
	terrainY[T][0] = (drand48()-0.5)*YMAX;
	terrainY[0][T] = (drand48()-0.5)*YMAX;
	terrainY[T][T] = (drand48()-0.5)*YMAX;

	gen_terrain(0, 0, T, T, 1);
	gen_volcano();
	smooth_terrain();

	/* generating normals for vertices which are not on edges */
	for (i=1; i<T; i++)
		for (j=1; j<T; j++){
				GLfloat tVertices[][3] = {
					{i, terrainY[i][j] / M, j},					//0
					{i-1, terrainY[i-1][j] / M, j},			//1
					{i, terrainY[i][j-1] / M, j-1},			//2
					{i+1, terrainY[i+1][j-1] / M, j-1},	//3
					{i+1, terrainY[i+1][j] / M, j},			//4
					{i, terrainY[i][j+1] /M , j+1},			//5
					{i-1, terrainY[i-1][j+1] /M , j+1},	//6
				};
				
				int tFacets[] = {1,0,2,-1,
										2,0,3,-1,
										3,0,4,-1,
										4,0,5,-1,
										5,0,6,-1,
										6,0,1,-1};

				GLfloat tns[6][3];
				makeNormals(tVertices, tFacets, 6, tns);

				tNorm[i][j][0] = (tns[0][0]+tns[1][0]+tns[2][0]+tns[3][0]+tns[4][0]+tns[5][0])/6.0;
				tNorm[i][j][1] = (tns[0][1]+tns[1][1]+tns[2][1]+tns[3][1]+tns[4][1]+tns[5][1])/6.0;
				tNorm[i][j][2] = (tns[0][2]+tns[1][2]+tns[2][2]+tns[3][2]+tns[4][2]+tns[5][2])/6.0;
		}

	/* generating normals for vertices on the edges */
	for (i=1; i<T; i++){
		GLfloat tVertices[][3] = {
			{i, terrainY[i][0] / M, 0},					//0
			{i+1, terrainY[i+1][0] / M, 0},			//1
			{i, terrainY[i][1] /M , 1},					//2
			{i-1, terrainY[i-1][1] /M , 1},				//3
			{i-1, terrainY[i-1][0] / M, 0},				//4

			{i, terrainY[i][T] / M, T},						//5
			{i-1, terrainY[i-1][T] / M, T},				//6
			{i, terrainY[i][T-1] /M , T-1},				//7
			{i+1, terrainY[i+1][T-1] /M , T-1},		//8
			{i+1, terrainY[i+1][T] / M, T},			//9

			{0, terrainY[0][i] / M, i},					//10
			{0, terrainY[0][i-1] / M, i-1},				//11
			{1, terrainY[1][i-1] / M, i-1},				//12
			{1, terrainY[1][i] /M , i},					//13
			{0, terrainY[0][i+1] / M, i+1},			//14

			{T, terrainY[T][i] / M, i},						//15
			{T, terrainY[T][i+1] / M, i+1},				//16
			{T-1, terrainY[T-1][i+1] / M, i+1},		//17
			{T-1, terrainY[T-1][i] /M , i},				//18
			{T, terrainY[T][i-1] / M, i-1}				//19
		};

		int tFacets[] = {1,0,2,-1,
								2,0,3,-1,
								3,0,4,-1,
								6,5,7,-1,
								7,5,8,-1,
								8,5,9,-1,
								11,10,12,-1,
								12,10,13,-1,
								13,10,14,-1,
								16,15,17,-1,
								17,15,18,-1,
								18,15,19,-1};

		GLfloat tns[12][3];
		makeNormals(tVertices, tFacets, 12, tns);

		tNorm[i][0][0] = (tns[0][0]+tns[1][0]+tns[2][0])/3.0;
		tNorm[i][0][1] = (tns[0][1]+tns[1][1]+tns[2][1])/3.0;
		tNorm[i][0][2] = (tns[0][2]+tns[1][2]+tns[2][2])/3.0;

		tNorm[i][T][0] = (tns[3][0]+tns[4][0]+tns[5][0])/3.0;
		tNorm[i][T][1] = (tns[3][1]+tns[4][1]+tns[5][1])/3.0;
		tNorm[i][T][2] = (tns[3][2]+tns[4][2]+tns[5][2])/3.0;

		tNorm[0][i][0] = (tns[6][0]+tns[7][0]+tns[8][0])/3.0;
		tNorm[0][i][1] = (tns[6][1]+tns[7][1]+tns[8][1])/3.0;
		tNorm[0][i][2] = (tns[6][2]+tns[7][2]+tns[8][2])/3.0;

		tNorm[T][i][0] = (tns[9][0]+tns[10][0]+tns[11][0])/3.0;
		tNorm[T][i][1] = (tns[9][1]+tns[10][1]+tns[11][1])/3.0;
		tNorm[T][i][2] = (tns[9][2]+tns[10][2]+tns[11][2])/3.0;
	}


	GLfloat tVertices[][3] = {
		{0, terrainY[0][0] / M, 0},			//0
		{1, terrainY[1][0] / M, 0},			//1
		{0, terrainY[0][1] / M, 1},			//2

		{T, terrainY[T][0] / M, 0},			//3
		{T, terrainY[T][1] / M, 1},			//4
		{T-1, terrainY[T-1][1] / M, 1},		//5
		{T-1, terrainY[T-1][0] / M, 0},		//6

		{0, terrainY[0][T] / M, T},			//7
		{0, terrainY[0][T-1] / M, T-1},		//8
		{1, terrainY[1][T-1] / M, T-1},		//9
		{1, terrainY[1][T] / M, T},			//10

		{T, terrainY[T][T] / M, T},			//11
		{T-1, terrainY[T-1][T] / M, T},		//12
		{T, terrainY[T][T-1] / M, T-1}		//13
	};

	/* generating normals for vertices on the corners */
	int tFacets[] = {1,0,2,-1,
							4,3,5,-1,
							5,3,6,-1,
							8,7,9,-1,
							9,7,10,-1,
							12,11,13,-1};

	GLfloat tns[6][3];
	makeNormals(tVertices, tFacets, 6, tns);

	for (i=0 ; i<3 ; i++){
		tNorm[0][0][i] = tns[0][i];
		tNorm[0][T][i] = (tns[1][i] + tns[2][i]);
		tNorm[T][0][i] = (tns[3][i] + tns[4][i]);
		tNorm[T][T][i] = tns[5][i];
	}

	/* creating the terrain display-list */
	terrain = glGenLists(1);
	if(terrain!=0){
		glNewList(terrain, GL_COMPILE);
		glColor3f(1.0, 1.0, 1.0);
		glEnable (GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, tNum[5]);
			glBegin(GL_TRIANGLES);
				for (i=0; i<T; i++)
					for (j=0; j<T; j++){
							points(i, j);
							points(i, j+1);
							points(i+1, j);
							points(i+1, j);
							points(i, j+1);
							points(i+1, j+1);
					}
			glEnd();
			glDisable (GL_TEXTURE_1D);

	/* terrain surrounding walls */
		glColor3f(0.3, 0.2, 0.1);
		glBegin(GL_QUAD_STRIP);
			for (i=0; i<T; i++){
				glVertex3f(0.0, terrainY[0][i], i*M);
				glVertex3f(0.0, -100.0, i*M);
			}
			for (i=0; i<T; i++){
				glVertex3f(i*M, terrainY[i][T], T*M);
				glVertex3f(i*M, -100.0, T*M);
			}
			for (i=T; i>0; i--){
				glVertex3f(T*M, terrainY[T][i], i*M);
				glVertex3f(T*M, -100.0, i*M);
			}
			for (i=T; i>=0; i--){
				glVertex3f(i*M, terrainY[i][0], 0.0);
				glVertex3f(i*M, -100.0, 0.0);
			}
		glEnd();
		glEndList();
	}else
	fprintf(stderr, "Error: OpenGL could not generate the terrain\n");
  return;
}

/**************************************************************************/
/********************************   Biplane     ********************************/
/**************************************************************************/

/*
	The biplane is made up of several parts. These are "body", "tail", "windshield", "wings", "propellers", "rudder flap", "elevator flap". For each part 4 arrays are defined; an array of vertices, an array of facets based on the order of vertices, an array of texture coordinates based on facets, and an array to hold the normals of facets. Technically propeller and flaps do not need texture arrays (as they are not textured in this program), but to generalise the generation of polygons for different parts of the biplane and have all parts generated by a single "polyhedron" function they are defined as arrays with no assignment.
*/
GLfloat bodyVertices[][3] = {
	{0.0,0.0,0.0}, //0
	{12.0,0.0,0.0}, 
	{12.0,12.0,0.0}, 
	{0.0,12.0,0.0}, 
	{12.0,12.0,-9.0},// 4
	{10.0,12.0,-9.0}, 
	{2.0,12.0,-9.0}, 
	{0.0,12.0,-9.0}, 
	{12.0,12.0,-23.0},//8 
	{10.0,12.0,-23.0}, 
	{2.0,12.0,-23.0}, 
	{0.0,12.0,-23.0}, 
	{0.0,0.0,-30.0},//12
	{12.0,0.0,-30.0},
	{12.0,12.0,-30.0},
	{0.0,12.0,-30.0},
	{2.0,6.0,-60.0},//16
	{10.0,6.0,-60.0},
	{10.0,12.0,-60.0},
	{2.0,12.0,-60.0}
};
int bodyFacets[] = {
	-2,0,1,2,3,-1,//front
	-2,3,2,4,7,-1,//top F
	-2,5,4,8,9,-1,//top L
	-2,7,6,10,11,-1,//top R
	-2,11,8,14,15,-1,//top M
	-2,15,14,18,19,-1,//top B
	-2,0,3,15,12,-1,//rifght F
	-2,12,15,19,16,-1,//right B
	-2,2,1,13,14,-1,//left F
	-2,14,13,17,18,-1,//left B
	-2,1,0,12,13,-1,//bottom F
	-2,12,16,17,13,-1,//bottom B
	-2,19,18,17,16,-1//back
};
GLfloat bodyTextures[][2] = {
{0.0,0.0},{1.0,0.0},{1.0,1.0},{0.0,1.0},
{0.0,0.25},{1.0,0.25},{1.0,1.0},{0.0,1.0},
{0.83,0.0},{1.0,0.0},{1.0,1.17},{0.83,1.17},
{0.0,0.0},{0.17,0.0},{0.17,1.17},{0.0,1.17},
{0.0,0.0},{1.0,0.0},{1.0,0.58},{0.0,0.58},
{0.0,0.0},{1.0,0.0},{0.83,2.5},{0.17,2.5},
{0.0,0.0},{1.0,0.0},{1.0,2.5},{0.0,2.5},
{0.0,0.0},{1.0,0.0},{1.0,2.5},{0.5,2.5},
{0.0,0.0},{1.0,0.0},{1.0,2.5},{0.0,2.5},
{0.0,0.0},{1.0,0.0},{0.5,2.5},{0.0,2.5},
{0.0,0.0},{1.0,0.0},{1.0,2.5},{0.0,2.5},
{0.0,0.0},{0.17,2.5},{0.83,2.5},{1.0,0.0},
{0.17,0.25},{0.83,0.25},{0.83,0.75},{0.17,0.75}
};

GLfloat bodyNormals[13][3];

/*************************************/
GLfloat tailVertices[][3] = {
	{0.0,0.0,0.0},//0
	{2.0,0.0,0.0},
	{0.0,8.0,-4.0},
	{2.0,8.0,-4.0},
	{0.0,0.0,-8.0},//4
	{0.0,8.0,-8.0},
	{2.0,0.0,-8.0},
	{2.0,8.0,-8.0}
};
int tailFacets[] = {
	-2,0,1,3,2,-1,
	-2,2,3,7,5,-1,
	-2,5,7,6,4,-1,
	-2,4,0,2,5,-1,
	-2,1,6,7,3,-1
};
GLfloat tailTextures[][2] = {
{0.83,0.0},{1.0,0.0},{1.0,0.75},{0.83,0.75},
{0.83,0.0},{1.0,0.0},{1.0,0.33},{0.83,0.33},
{0.83,0.0},{1.0,0.0},{1.0,0.67},{0.83,0.67},
{0.0,0.0},{0.67,0.0},{0.33,0.67},{0.0,0.67},
{0.67,0.0},{0.0,0.0},{0.0,0.67},{0.33,0.67},
};

GLfloat tailNormals[5][3];

/*************************************/
GLfloat shieldVertices[][3] = {
{0.0,0.0,0.0},
{8.0,0.0,0.0},
{0.0,0.0,-4.0},
{8.0,0.0,-4.0},
{0.0,6.0,-4.0},
{8.0,6.0,-4.0}
};

int shieldFacets[] = {
	-2,0,1,5,4,-1,
	2,0,4,-1,
	1,3,5,-1
};

GLfloat shieldTextures[][2] = {{0.0,0.0},{1.0,0.0},{1.0,1.0},{0.0,1.0}};

GLfloat shieldNormals[3][3];

/*************************************/
GLfloat wingVertices[][3] = {
	{0.0,0.0,0.0},//0
	{0.0,2.0,0.0},
	{0.0,2.0,-27.5},
	{0.0,1.0,-27.5},
	{64.0,0.0,0.0},//4
	{64.0,2.0,0.0},
	{64.0,2.0,-27.5},
	{64.0,1.0,-27.5},
	{0.0,22.0,0.0},//8
	{0.0,24.0,0.0},
	{0.0,23.0,-27.5},
	{0.0,22.0,-27.5},
	{64.0,22.0,0.0},//12
	{64.0,24.0,0.0},
	{64.0,23.0,-27.5},
	{64.0,22.0,-27.5},
	{26.0,2.0,0.0},//16
	{26.0,2.0,-27.5},
	{38.0,2.0,0.0},
	{38.0,2.0,-27.5}
};

int wingFacets[] = {
	-2,2,3,0,1,-1,//bottom right side
	-2,7,6,5,4,-1,// bottom left side
	-2,10,11,8,9,-1,// top right side
	-2,15,14,13,12,-1,// top left side
	-2,3,7,4,0,-1,// bottom bottom
	-2,1,16,17,2,-1,// bottom top right
	-2,18,5,6,19,-1,// bottom top left
	-2,9,13,14,10,-1,// top top
	-2,12,8,11,15,-1// top bottom
};
GLfloat wingTextures[][2] = {
{0.0,0.17},{0.0,0.08},{2.5,0.0},{2.5,0.17},
{0.0,0.08},{0.0,0.0},{2.5,0.0},{2.5,0.17},
{0.0,0.08},{0.0,0.0},{2.5,0.0},{2.5,0.17},
{0.0,0.17},{0.0,0.08},{2.5,0.0},{2.5,0.17},
{0.0,0.0},{5.33,0.0},{5.33,2.3},{0.0,2.3},
{0.0,0.0},{2.17,0.0},{2.17,2.3},{0.0,2.3},
{0.0,0.0},{2.17,0.0},{2.17,2.3},{0.0,2.3},
{0.0,0.0},{5.33,0.0},{5.33,2.3},{0.0,2.3},
{0.0,0.0},{5.33,0.0},{5.33,2.3},{0.0,2.3},
};

GLfloat wingNormals[9][3];

/*************************************/
GLfloat propellerVertices[][3] = {
	{2.0,-1.0,0.0},
	{10.0,-1.0,0.0},
	{10.0,1.0,0.0},
	{2.0,1.0,0.0},
	{2.0,-1.0,-1.0},
	{10.0,-1.0,-1.0},
	{10.0,1.0,-1.0},
	{2.0,1.0,-1.0}
};
int propellerFacets[] = {
0,1,2,3,-1,
3,2,6,7,-1,
7,6,5,4,-1,
4,5,1,0,-1,
0,3,7,4,-1,
1,5,6,2,-1
};

GLfloat propellerTextures[][2] = {};

GLfloat propellerNormals[6][3];

/*************************************/
GLfloat rudderVertices[][3] = {
	{0.0,0.0,0.0},
	{2.0,0.0,0.0},
	{0.0,0.0,8.0},
	{2.0,0.0,8.0},
	{0.0,2.0,8.0},
	{2.0,2.0,8.0},
	{0.0,4.0,0.0},
	{2.0,4.0,0.0}
};
int rudderFacets[] = {
	0,2,4,6,-1,
	1,7,5,3,-1,
	2,3,5,4,-1,
	4,5,7,6,-1,
	6,7,1,0,-1
};

GLfloat rudderTextures[][2] = {};

GLfloat rudderNormals[5][3];

/*************************************/
GLfloat elevatorVertices[][3] = {
	{0.0,0.0,0.0},
	{2.0,0.0,0.0},
	{0.0,0.0,8.0},
	{2.0,0.0,8.0},
	{0.0,4.0,8.0},
	{2.0,4.0,8.0},
	{0.0,4.0,0.0},
	{2.0,4.0,0.0}
};

int elevatorFacets[] = {
	0,2,4,6,-1,
	1,7,5,3,-1,
	2,3,5,4,-1,
	4,5,7,6,-1,
	6,7,1,0,-1
};

GLfloat elevatorTextures[][2] = {};

GLfloat elevatorNormals[5][3];

/*************************************/
/*
	This function takes a vector and normalises it.
	It is called in "makeNormals", "yMap" and "Collision" functions.
*/
void normalize(GLfloat p[]) {
	double sqrt();
	float d =0.0;
	int i;

	for(i=0; i<3; i++) 
		d += p[i]*p[i];
	d=sqrt(d);
	if (d > 0.0) 
		for(i=0; i<3; i++) p[i]/=d;
}
/*************************************/
/*
	This function computes the cross-product of two vectors.
	It is called in "makeNormals", "yMap" and "Collision" functions.
*/
void vectorXProduct( GLfloat v1[3], GLfloat v2[3], GLfloat ans[3] ) {
	ans[0] = v1[1]*v2[2] - v1[2]*v2[1];
	ans[1] = v1[2]*v2[0] - v1[0]*v2[2];
	ans[2] = v1[0]*v2[1] - v1[1]*v2[0];
}
/*************************************/
/*
	This function generates per face normals. It takes a set of vertices, a set of facets and the number of facets and finds a normal for each facet using its first 3 vertices with "vectorXProduct" function.
*/
void makeNormals(GLfloat vs[][3], int fs[], int nrFacets, GLfloat ns[][3]){
	int f, c;
	int i = 0;
	GLfloat vec1[3], vec2[3];
		
	for (f = 0; f < nrFacets; f++){
		for (c = 0; c < 3; c++){
			vec1[c] = vs[fs[i+2]][c] - vs[fs[i+1]][c];
			vec2[c] = vs[fs[i]][c] - vs[fs[i+1] ][c];
		}
		vectorXProduct( vec1, vec2, ns[f] );
		normalize( ns[f] );
		while ( fs[i] >= 0) i++;
			i++;
	}
}
/*************************************/
/*
	This function creates normals for all different parts of the biplane. For each part it calls "makeNormals" once and passes the set of vertices and facets to it. The normals need to be generated only once. So this function is called within "init" function.
*/
void myinit(void) {
	makeNormals( bodyVertices, bodyFacets, 13, bodyNormals);
	makeNormals( tailVertices, tailFacets, 5, tailNormals);
	makeNormals( shieldVertices, shieldFacets, 3, shieldNormals);
	makeNormals( wingVertices, wingFacets, 9, wingNormals);
	makeNormals( propellerVertices, propellerFacets, 6, propellerNormals);
	makeNormals( rudderVertices, rudderFacets, 5, rudderNormals);
	makeNormals( elevatorVertices, elevatorFacets, 5, elevatorNormals);
}
/*************************************/
/*
	This function draws polygons. It takes an array of vertices, an array of facets, the number of facets, an array of normals of the facets and an array of the texture coordinates of the vertices of each facet. This fuction must be able to draw polygons and textured polygons both. So, it is hard-coded to recognise a -2 as the first item of a textured facet. The end of each facet (polygon) is recognised by a negative number as well. 
*/
void polyhedron(GLfloat vs[][3], int fs[], int nrFacets, GLfloat ns[][3], GLfloat tx[][2]){
	int f;
	int i = 0;
	int t = 0;

	for (f=0; f<nrFacets; f++){
		glBegin(GL_POLYGON);
		glNormal3fv(ns[f]);
		if (fs[i]==-2){
			i++;
			while (fs[i] >= 0){
				glTexCoord2fv(tx[t++]);
				glVertex3fv(vs[fs[i++]]);
			}
		}
		else
			while (fs[i] >= 0)
				glVertex3fv(vs[fs[i++]]);
		glEnd();
			i++;
	}
}
/*************************************/
/*
	This function generates a cylinders with a required size and adds 2 disks to the sides to close it and look like a solid cylinder. It uses quadratic functions "gluCylinder" and "gluDisk" and enables the texturing if applicable. This function is called in "init_wing", "rudderFlap" and "elevatorFlap".
*/
void cylinder(float w, float h, GLuint texNum){	
	GLUquadricObj *q = gluNewQuadric();
	gluQuadricNormals(q, GL_SMOOTH);
	gluQuadricTexture(q, GL_TRUE);
	if (texNum != -1){
		glEnable (GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texNum);
	}
	gluCylinder(q, w, w, h, 24, 1);
	glTranslatef(0.0, 0.0, h);
	gluDisk(q, 0.0, w, 24, 1);
	glTranslatef(0.0, 0.0, -h);
	gluQuadricOrientation(q, GLU_INSIDE);
	gluDisk(q, 0.0, w, 24, 1);
	glDisable (GL_TEXTURE_2D);
	gluQuadricTexture(q, GL_FALSE);
	gluDeleteQuadric(q);
}
/*************************************/
/*
	This function is almost identical to "cylinder" function except that it initialises a display-list for the engine cylinder. It has only on disk on front side. If initialisation fails an error message is displayed on the terminal.
*/
void init_engine(){	
  planeEngine = glGenLists(1);
	if(planeEngine!=0){
		glNewList(planeEngine, GL_COMPILE);
		GLUquadricObj *q = gluNewQuadric();
		gluQuadricOrientation(q, GLU_OUTSIDE);
		gluQuadricNormals(q, GL_SMOOTH);
		gluQuadricTexture(q, GL_TRUE);
		glColor4f(1.0,1.0,1.0,1.0);
		glEnable (GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tNum[4]);
		gluCylinder(q, 4.0, 4.0, 4.0, 24, 1);
		glTranslatef(0.0, 0.0, 4.0);
		gluDisk(q, 0.0, 4.0, 24, 1);
		glDisable (GL_TEXTURE_2D);
		gluQuadricTexture(q, GL_FALSE);
		gluDeleteQuadric(q);
		glEndList();
	}else
    fprintf(stderr, "Error: Unable to obtain a list for the plane engine\n");
  return;
}
/*************************************/
/*
	This function initialises the "planeBody" display-list. After binding the suitable texture the "polyhedron" function is called with all predefined arrays for polygons to be generated. The texture is disabled afterwards.
*/
void init_body(){
  planeBody = glGenLists(1);
	if(planeBody!=0){
		glNewList(planeBody, GL_COMPILE);
		glColor4f(1.0,1.0,1.0,1.0);
		glEnable (GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tNum[0]);
		polyhedron( 
		bodyVertices, 
		bodyFacets, 
		13, 
		bodyNormals, 
		bodyTextures);
		glDisable (GL_TEXTURE_2D);
		glEndList();
	}else
    fprintf(stderr, "Error: Unable to obtain a list for the plane body\n");
  return;
}
/*************************************/
/*
	This function initialises a display-list for the tail of the biplane with 5 facets. The display-list is called 3 times to generate 3 parts of the tail at the back of the body.
*/
void init_tail(){
  planeTail = glGenLists(1);
	if(planeTail!=0){
		glNewList(planeTail, GL_COMPILE);
		glColor4f(1.0,1.0,1.0,1.0);
		glEnable (GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tNum[3]);
		polyhedron( 
			tailVertices, 
			tailFacets, 
			5, 
			tailNormals, 
			tailTextures);
		glDisable(GL_TEXTURE_2D);
		glEndList();
	}else
    fprintf(stderr, "Error: Unable to obtain a list for the plane tail\n");
  return;
}
/*************************************/
/*
	This function initialises a display-list for the windshield of the biplane with 3 facets. The windshield is semi-transparent with a texture mapped to its front facet to make it more realistic.
*/
void init_shield(){
	planeShield = glGenLists(1);
	if(planeShield!=0){
		glNewList(planeShield, GL_COMPILE);
		glColor4f(0.8, 0.8, 0.8, 0.4);
		glEnable (GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tNum[1]);
		polyhedron( 
			shieldVertices, 
			shieldFacets, 
			3, 
			shieldNormals, 
			shieldTextures);
		glDisable(GL_TEXTURE_2D);
		glEndList();
	}else
    fprintf(stderr, "Error: Unable to obtain a list for the plane windshield\n");
  return;
}
/*************************************/
/*
	This function initialises a display-list for the wings of the biplane. It is divided into 3 parts; wings parts over and below the body, wing edges and struts. the wings part is done the same as before with binding a texture and a call to "polyhedron" function. It has 9 facets to be generated. The edges are 4 calls to "cylinder" function with passing appropriate radius, height and texture. The struts are 12 calls to "cylinder" with required sizes and texture. 
*/
void init_wing(){
  planeWing = glGenLists(1);
	if(planeWing!=0){
		glNewList(planeWing, GL_COMPILE);
		glColor4f(1.0,1.0,1.0,1.0);

		/* The wing planes */
		glPushMatrix();
		glTranslatef(0.0,-1.0,0.0);
		glEnable (GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tNum[0]);
		polyhedron( 
			wingVertices, 
			wingFacets, 
			9, 
			wingNormals, 
			wingTextures);
		glDisable (GL_TEXTURE_2D);
		glPopMatrix();

		/* The edges of the wings */
		glPushMatrix();
		glRotatef(90.0,0.0,1.0,0.0);
		cylinder(1.0, 64.0, tNum[0]);
		glTranslatef(0.0,22.0,0.0);
		cylinder(1.0, 64.0, tNum[0]);
		glTranslatef(27.5,-0.5,0.0);
		cylinder(0.5, 64.0, tNum[0]);
		glTranslatef(0.0,-21.0,0.0);
		cylinder(0.5, 64.0, tNum[0]);
		glPopMatrix();

		/* The struts */
		glPushMatrix();
		glTranslatef(3.0,21.0,-2.0);
		glRotatef(-90.0,0.0,0.0,1.0);
		glRotatef(90.0,0.0,1.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(0.0,14.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(0.0,11.0,0.0);
		cylinder(0.5, 8.0, tNum[2]);
		glTranslatef(0.0,8.0,0.0);
		cylinder(0.5, 8.0, tNum[2]);
		glTranslatef(0.0,11.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(0.0,14.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(23.0,0.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(0.0,-14.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(0.0,-11.0,0.0);
		cylinder(0.5, 8.0, tNum[2]);
		glTranslatef(0.0,-8.0,0.0);
		cylinder(0.5, 8.0, tNum[2]);
		glTranslatef(0.0,-11.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glTranslatef(0.0,-14.0,0.0);
		cylinder(0.5, 20.0, tNum[2]);
		glPopMatrix();
		glEndList();
	}else
    fprintf(stderr, "Error: Unable to obtain a list for the plane wings\n");
  return;
}
/*************************************/
/*
	This function generates a single propeller. It is called in "render_biplane" 4 times with glRotatef to simulate 4 propellers of the biplane. Each propeller is rotated according to the arguement propNum. Each propeller has 6 facets. As the propellers are not textured (in this program), an empty array is passed to "polyhedron" function.
*/
void propeller(int propNum) {
	glPushMatrix();
	glRotatef(propAng+propNum*90.0,0.0,0.0,1.0);
	glColor4f(0.5, 0.5, 0.5, 0.6);
	polyhedron( 
		propellerVertices, 
		propellerFacets, 
		6, 
		propellerNormals, 
		propellerTextures); 
	glPopMatrix();
}
/*************************************/
/*
	This function generates the rudder flap of the biplane. It is made up of a cylinder and 5 facets generated by "cylinder" and "polyhedron" functions respectively.
*/
void rudderFlap(void) {
    glPushMatrix();
	glTranslatef(6.0,12.0,-62.0);	
	glRotatef(rudderAng,0.0,1.0,0.0);
	glRotatef(-90.0,1.0,0.0,0.0);
	glColor4f(0.4, 0.4, 0.4, 1.0);
	cylinder(1.0,8.0, -1);
	glTranslatef(-1.0,0.0,0.0);	
    polyhedron( 
        rudderVertices, 
        rudderFacets, 
        5, 
        rudderNormals, 
        rudderTextures);
    glPopMatrix();
}
/*************************************/
/*
	This function generates the elevator flap. It is called twice within "render_biplane" to generate 2 identical flaps. Each flap consists of a cylinder and 5 facets generated by "cylinder" and "polyhedron" functions.
*/
void elevatorFlap(void) {
    glPushMatrix();
	glTranslatef(-6.0,9.0,-62.0);	
	glRotatef(elevatorAng,1.0,0.0,0.0);
	glRotatef(-90.0,1.0,0.0,0.0);
	glRotatef(90.0,0.0,1.0,0.0);
	cylinder(1.0,8.0, -1);
	glTranslatef(-1.0,0.0,0.0);	
    polyhedron( 
        elevatorVertices, 
        elevatorFacets, 
        5, 
        elevatorNormals, 
        elevatorTextures);
    glPopMatrix();
}
/*************************************/
/*
	This function generates the biplane with appropriate latitude and altitude angles. The "pAltAng" is the angle between the biplane and X-Z plane. When mapping the reference point of the biplane (here the center) to the X-Z plane the "pLatAng" is the angle between that vector and Z axis. Each part of the plane is called within a set of push and pop matrix functions to avoid moving the reference point of the biplane. "planeTail" is called 3 times to generate 3 parts of the tail of the biplane. "propeller" is called 4 times with different parameters to generate propellers with different rotations. The windshield which is semi-transparent is generated at the end to avoid affecting on ather parts of the biplane.
*/
void render_biplane(){

    int s;

	glRotatef(pLatAng,0.0,1.0,0.0);
	glRotatef(-pAltAng,1.0,0.0,0.0);
	glTranslatef(-6.0,-6.0,30.0);		// just to place the centre of biplane at (0.0,0.0,0.0)

	/* generate plane body */
	glCallList(planeBody);

	/* generate plane engine */
	glPushMatrix();
	glTranslatef(6.0, 6.0, 0.0);
	glCallList(planeEngine);
	glPopMatrix();

	/* generate plane tails */
	glPushMatrix();
	glTranslatef(5.0, 12.0, -52.0);
	glCallList(planeTail);
	glPushMatrix();
	glTranslatef(-3.0, -4.0, 0.0);
	glRotatef(90.0,0.0,0.0,1.0);
	glCallList(planeTail);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(5.0, -2.0, 0.0);
	glRotatef(-90.0,0.0,0.0,1.0);
	glCallList(planeTail);
	glPopMatrix();
	glPopMatrix();

	/* generate plane wings */
	glPushMatrix();
	glTranslatef(-26.0,-1.0,0.0);
	glCallList(planeWing);
	glPopMatrix();


	/* generate plane rudder flap */
	glPushMatrix();
	rudderFlap();
	glPopMatrix();

	/* generate plane elevator flaps */
	glPushMatrix();
	elevatorFlap();
	glTranslatef(16.0,0.0,0.0);
	elevatorFlap();
	glPopMatrix();
	
	/* generate plane propellers */
	glPushMatrix();
	glTranslatef(6.0,6.0,6.0);
	for (s = 0; s < 4; s++) 
		propeller(s);
	glPopMatrix();
	
	/* generate plane shield */
	glPushMatrix();
	glTranslatef(2.0,12.0,-9.0);
	glCallList(planeShield);
	glPopMatrix();
}

/**************************************************************************/
/*************  Volcano Eruption , Lava Flow , Collision Detection *********************/
/**************************************************************************/

/*
	This function takes (x,z) coordinates of a point an returns the Y value of that point on a triangle with vertices of p1, p2 and p3. It is called in "getY" function. It first finds the normal of the triangle. Then having normal vector and a point (here p1) the plane equation is found. Passing x and z to the equation it retruns the Y value of the point.
*/
float yMap(float x, float z, float p1[3], float p2[3], float p3[3]){
	int c;
	float d;
	GLfloat vec1[3], vec2[3], n[3];

	for (c = 0; c < 3; c++) {
		vec1[c] = p3[c] - p2[c];
		vec2[c] = p1[c] - p2[c];
	}
	vectorXProduct( vec1, vec2, n);
	normalize(n);
																				// ax + by + cz + d = 0
	d = -(p1[0]*n[0] + p1[1]*n[1] + p1[2]*n[2]);		// d = -(ax + by + cz)
	d = - (n[0] * x + n[2] * z + d) / n[1];					// y = - (ax + cz + d) / b

	return d;
}
/*************************************/
/*
	This function takes a (x,z) coordinates and finds the vertices of the triangle on the terrain it is in. firstX and firstZ are the coordinates of a vertex of the square the point is in. p1 to p4 are calculated based on these coordinates. Then a simple check shows in which triangle the point is located. A call to "yMap" returns the required Y value. This function is called in "new_lava" and "update_lava".
*/
float getY(float x, float z){
	float p1[3], p2[3], p3[3], p4[3];
	int firstX = (int) (x/M);
	int firstZ = (int) (z/M);

	p1[0] = firstX * M;
	p1[1] = terrainY[firstX][firstZ];
	p1[2] = firstZ * M;

	p2[0] = firstX * M;
	p2[1] = terrainY[firstX][firstZ+1];
	p2[2] = (firstZ+1) * M;

	p3[0] = (firstX+1) * M;
	p3[1] = terrainY[firstX+1][firstZ];
	p3[2] = firstZ * M;

	p4[0] = (firstX+1) * M;
	p4[1] = terrainY[firstX+1][firstZ+1];
	p4[2] = (firstZ+1) * M;

	/*	the equation of the common line between 2 triangles is:   x + z = m
		for points below this line we have:  x + z < m
		for points above this line we have:  x + z > m   */
	if ((x - firstX*M) + (z - firstZ*M) <= M)
		return yMap(x, z, p3, p1, p2);
	else
		return yMap(x, z, p2, p4, p3);
}
/*************************************/
/*
	This function sets a new lava particle. There are 5 values to be set for new particle; a random angle on XZ plane, 3 coordinates of its position based on the angle and terrain. X and Z are set to be within the cilcle of the crater. Y is then computed with a "getY" call using X and Z values. The next value is a random value for the color of the particle. Last value is alpha value which is set to 1.
*/
void new_lava(int i){

	/* random initial angle */
	lavaPart[i][5] = ( (drand48() - 0.5) * PI * 2.0);

	/* X, Y, Z positions */
    lavaPart[i][0] = 0.5 * sin(lavaPart[i][5]) * (1000.0/16.0) + 500.0;
    lavaPart[i][2] = 0.5 * cos(lavaPart[i][5]) * (1000.0/16.0) + 500.0;
	lavaPart[i][1] = getY(lavaPart[i][0], lavaPart[i][2]);

	/* random color from red to orange */
	lavaPart[i][3] = drand48() * 0.5 + 0.5;

	/* alpha value */
	lavaPart[i][4] = 1.0;
}
/*************************************/
/*
	This function updates the values of a lava particle array. As lava particles are initialised in the crater it first checks if the particle can go up in the direction of its angle. when it reaches the edge of the crater and cannot go up anymore it checks all 8 points around it and moves to the one with the lowest Y value. If the current location has the lowest Y value it does not move anymore and creates a pool of particles. The color value is also reduced slowly (by multiplying to 0.999) to slowly change the color to black which is what happens in reality.
*/
void update_lava(int part){

	int i,j;
	float x = lavaPart[part][0];
	float y = lavaPart[part][1];
	float z = lavaPart[part][2];
	float tmpX = x;
	float tmpY;
	float tmpZ = z;
	
	if ((tmpY = getY(x + 0.5 * sin(lavaPart[part][5]) , z + 0.5 * cos(lavaPart[part][5]))) > y){
		tmpX = x + 0.5 * sin(lavaPart[part][5]);
		y = tmpY;
		tmpZ = z + 0.5 * cos(lavaPart[part][5]);
	}
	else{
		for (i=-1; i<2; i++)
			for (j=-1; j<2; j++){
				if ((tmpY = getY(x+i, z+j)) < y){
					tmpX = x+i;
					y = tmpY;
					tmpZ = z+j;
				}
			}
	}
	
	lavaPart[part][0] = tmpX;
	lavaPart[part][1] = y;
	lavaPart[part][2] = tmpZ;
	
	lavaPart[part][3] *= 0.999;
}
/*************************************/
/*
	This function tests if lava particle has reached the edges of the terrain and if so the alpha value is set to 0 to make it disapear.
*/
void lava_test(int i){
	if(lavaPart[i][0]<=0.0 || lavaPart[i][0]>=1000.0 || lavaPart[i][2]<=0.0 || lavaPart[i][2]>=1000.0)
		lavaPart[i][4] = 0.0;
}
/*************************************/
/*
	This function sets a new smoke particle. The array of each particle has 8 values; 3 first values are initial (x,y,z) coordinates which are based on a random angle and a distance from the center of the volcano. To make the smoke look like a mushroom 2 angles are needed, one for from Z axis in XZ plane and the other one from XZ plane (90 minus angle with Y axis). Next 3 values are the velocities of the particle in each direction. These velocities are based on the second initial angle and the initial velocity of the particle (here 60).
*/
void new_smoke(int i){
	float velocity = 60.0;
	float xzAngle = (drand48() * PI * 2.0);
	float yAngle = (drand48() * PI / 18.0) + (PI / 2.25);	// 80-90 degree
	float distance = drand48() * 40.0;

	/* X, Y, Z positions */
    smokePart[i][0] = distance * sin(xzAngle);
    smokePart[i][1] = 0.0;
    smokePart[i][2] = distance * cos(xzAngle);
	
	/* initial X, Y, Z velocities */
	smokePart[i][3] = smokePart[i][0] / sqrt(smokePart[i][0]*smokePart[i][0] + smokePart[i][2]*smokePart[i][2])* cos(yAngle)* velocity;
	smokePart[i][4] = sin(yAngle)* velocity;
	smokePart[i][5] = smokePart[i][2] / sqrt(smokePart[i][0]*smokePart[i][0] + smokePart[i][2]*smokePart[i][2])* cos(yAngle)* velocity;

	/* scale parameter */
	smokePart[i][6] = 15.0;

	/* alpha value */
	smokePart[i][7] = 0.8;
	
}
/*************************************/
/*
	This function updates the position of the particle with " X = X0 + V*T " equation. The equation for updating Y velocity is " V = V0 - G*T ". But the gravity does not affect the velocity hereand so V=V0. Nevertheless the velocity in Y direction is reduced slowly until it becomes 0. This is to limit the height for the smoke to end up having a mushroom shape. The scale value is increased by the time to increase the size of the smoke particle. The alpha value is reduced by the time to make the smoke particle slowly disapear.
*/
void update_smoke(int part, float time){
	smokePart[part][0] += smokePart[part][3] * time;
	smokePart[part][1] += smokePart[part][4] * time;
	smokePart[part][2] += smokePart[part][5] * time;

	if (smokePart[part][4] > time*6.0)
		smokePart[part][4] -= time*6.0;

	smokePart[part][6] += time;
	smokePart[part][7] -= time/25.0;
}
/*************************************/
/*
	This function checks if the alpha value of the smoke has reached 0 and resets it to a new position using a call to "new_smoke". The smokeNum holds the number of regenerated smoke particles which is used in "animate" to check if enough smoke particles are generated.
*/
void roof_test(int i){
	if(smokePart[i][7]<= 0.0){
	    new_smoke(i);
		smokeNum++;
	}
  return;
}
/*************************************/
/*
	This function set the values of an array entry for a rock particle. A random velocity is generated which is divided to 3 velocities of 3 directions based on a random angle. The initial position of the particle is inside the volcano with a random offset from the center. Scale parameter is also random to generate different size of rock particles. Smaller ones represent ash debris and larger ones represent rock parts. Color is also a random value which gives us a color between orange and black as the rock and ash debris look.
*/
void new_rock(int i){
	float velocity = 40.0 * drand48() +40.0;
	float angle = (drand48() * PI / 18.0) + (PI / 2.4);	// 75-85 degree

	/* X, Y, Z positions */
    rockPart[i][0] = (drand48()-0.5)* 60.0;
    rockPart[i][1] = 0.0;
    rockPart[i][2] = (drand48()-0.5)* 60.0;
	
	/* initial X, Y, Z velocities */
	rockPart[i][3] = rockPart[i][0] / sqrt(rockPart[i][0]*rockPart[i][0] + rockPart[i][2]*rockPart[i][2])* cos(angle)* velocity;
	rockPart[i][4] = sin(angle)* velocity;
	rockPart[i][5] = rockPart[i][2] / sqrt(rockPart[i][0]*rockPart[i][0] + rockPart[i][2]*rockPart[i][2])* cos(angle)* velocity;

	/* random scale parameter */
	rockPart[i][6] = drand48() * 1.5 +0.1;

	/* random color from orange to black */
	rockPart[i][7] = drand48() * 0.5 + 0.3;
	
}
/*************************************/
/*
	The position of the particle is updated using " X = X0 + V*T " equation. The Y velocity is updated with " V = V0 - G*T ".
*/
void update_rock(int part, float time){
	rockPart[part][0] += rockPart[part][3] * time;
	rockPart[part][1] += rockPart[part][4] * time;
	rockPart[part][2] += rockPart[part][5] * time;

	rockPart[part][4] -= gravity* time;
}
/*************************************/
/*
	This function checks if the particle is below a specific height (here: under the terrain) and regenerates the particle. "rockNum" holds the number of regenerated particles to be used in "animate" function.
*/
void ground_test(int i){
	if(rockPart[i][1]<=-200.0){
		new_rock(i);
		rockNum++;
    }
  return;
}
/*************************************/
/*
	This function takes 3 vertices of a triangle and 2 points of a line and checks if they have an intersection. After finding the normal of the plane (triangle) the plane equation is known: 

(1)		a*X + b*Y + c*Z + d = 0			--> the plane equation where a, b and c are the normal values

we can define the line equation based on its 2 points Ia and Ib:

(2)		Ia + (Ib - Ia) * t				--> the line equation where Ia and Ib are the 2 points on the line

when Ia=(Xa,Ya,Za) and Ib=(Xb,Yb,Zb) from (2) we have:

(3)		(Xb + (1-t)*Xa , Yb + (1-t)*Ya , Zb + (1-t)*Za) 		--> the coordinates of every point on this line

if there is an intersection between the line and the plane, replacing (3) in (1) must have an answer and returns a valid "t" value:

(4)		 t = (d - a*Xa - b*Ya - c*Za) / ( a*(Xb-Xa) + b*(Yb-Ya) + c*(Zb-Za) )

before computing "t" we need to make sure that the denominator is not 0. If the denominator is 0  it means that the line is perpendicular to the normal of the plane which also means it is parallel with the plane and thus has no intersection.

when t is found it is important to check if the intersection point is between the 2 points of the line Ia and Ib. Because of the way I defined the points of the line (2), it is enough to check if  " 0 < t <= 1 " is true. This is because

t = 0		-->		Ia + (Ib - Ia) * t   =  Ia
t = 1		-->		Ia + (Ib - Ia) * t   =  Ib

So if there is a "t" value that is between 0 and 1 there is an intersection between the triangle and the partial line.	

reference: http://en.wikipedia.org/wiki/Line-plane_intersection
*/

int collision(float p1[3], float p2[3], float p3[3], float l1[3], float l2[3]){
	int c;
	float d, t;
	GLfloat vec1[3], vec2[3], n[3];

	for (c = 0; c < 3; c++) {
		vec1[c] = p3[c] - p2[c];
		vec2[c] = p1[c] - p2[c];
	}
	vectorXProduct( vec1, vec2, n);
	normalize(n);
	d = p1[0]*n[0] + p1[1]*n[1] + p1[2]*n[2];
	t = n[0] * (l2[0]-l1[0]) + n[1] * (l2[1]-l1[1]) + n[2] * (l2[2]-l1[2]);
	if (t != 0){
		t = (d - n[0]*l1[0] - n[1]*l1[1] - n[2]*l1[2]) / t;	
		if (t>0 && t<=1)
			return 1;
	}
	return 0;
}

/**************************************************************************/
/**********************  Basic Functions and Main  ******************************/
/**************************************************************************/

void init_lights(){
	GLfloat light0_position[]={0.0, 1000.0, 500.0, 1.0};

	GLfloat amb_and_diff[]={0.4, 0.4, 0.4, 1.0};
	GLfloat global_amb[]={0.4, 0.4, 0.4, 1.0};
	GLfloat specular[] = {0.1, 0.1, 0.1, 1.0};

	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb_and_diff);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, amb_and_diff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_amb);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_LIGHT0);
}
/*************************************/
/*
	This fuction sets the viewport window. The size of the viewport is assigned to 2 global variables to be used by "left_menu" function. "left_menu" is called to set the projection.
*/
void reshape(int w, int h){
	glViewport(0, 0, w, h);
	width = w;
	height = h;
	left_menu(view_flag);
}
/*************************************/
void display(){
	int i;
	float x1, x2, y1, y2, z1, z2;
	float p0, p1, p2;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

/*
	This part is to calculate and set the view. There are 4 different views;
	"VIEW1" is the cockpit view. In this view the eye point and the reference point are based on the position(reference point) and altitude and latitude angles of the biplane. (x1,y1,z1) is the unit vector showing the biplane direction. (x2,y2,z2) is the vector perpendicular to (x1,y1,z1) vector and pointing from bottom to the roof of the plane. This is the up vector. To set the eye point inside the cockpit 12 units of first vector is added to the reference point of the biplane. To set the eye point to be a bit above outside the body of the biplane 9 units of the second vector is added to it as well. To set the reference point of the view it is enough to add 1 or more unit of (x1,y1,z1) to the eye point to move it to the front.
*/
	switch (view_flag){
	case VIEW1:
		x1 = cos(pAltAng*PI/180)*sin(pLatAng*PI/180);
		y1 = sin(pAltAng*PI/180);
		z1 = cos(pAltAng*PI/180)*cos(pLatAng*PI/180);

		x2 = cos((pAltAng+90.0)*PI/180)*sin(pLatAng*PI/180);
		y2 = sin((pAltAng+90.0)*PI/180);
		z2 = cos((pAltAng+90.0)*PI/180)*cos(pLatAng*PI/180);

		gluLookAt(planeX+12.0*x1+9.0*x2, planeY+12.0*y1+9.0*y2, planeZ+12.0*z1+9.0*z2,
	    				planeX+13.0*x1+9.0*x2, planeY+13.0*y1+9.0*y2, planeZ+13.0*z1+9.0*z2,
	    				x2, y2, z2);
		break;

/*
	This view is the outside view which is placed right behind the biplane. It moves with the biplane but does not change its angle when biplane rotates. So, the up vector is always (0,1,0). The reference point of this view is always the reference point of the biplane (center). For the eye point the Y value is only based on biplane Y value. To move the eye point to 30 units behind the biplane the eye point should move 60 units to the opposite of its pAltAng direction from the biplane reference point.
*/
	case VIEW2:
    gluLookAt(planeX-60.0*sin(pLatAng*PI/180), planeY+30.0, planeZ-60.0*cos(pLatAng*PI/180),
	    planeX, planeY, planeZ,
	    0.0, 1.0, 0.0);
		break;

/*
	This view is the fixed view. It is fixed 700 units above the terrain looking at the volcano.
*/
	case VIEW3:
		gluLookAt(500.0, 700.0, 500.0, 500.0, 0.0, 500.0, -1.0, 0.0, 0.0);
		break;

/*
	This is an extra view which gives flexibility to the the previous view. In this view the eye point can move on the surface of a half-sphere around the terrain. It is based on 2 angles. "viewAng1" is the angle between eye vector and Y axis. "viewAng2" is the angle between eye-vector-mapped-to-XZ-plane and Z axis. The eye point is 700 units away from the center of the terrain. 500 units are added to X and Z value to move it to the center of the terrain. The reference point is the center of the terrain. The up vector is calculated to be perpendicular to the eye vector. The only change from eye vector to calculate up vector is to add 90 degrees to the "viewAng2" to point to the up direction. The up vector should be of size 1 and thus there is no need for multiplying by any number.
*/
	case VIEW4:
		gluLookAt(	700.0*sin(viewAng1)*sin(viewAng2)+500.0, 700.0*cos(viewAng1), 700.0*sin(viewAng1)*cos(viewAng2)+500.0,
						500.0, 0.0, 500.0,
						sin(viewAng1)*sin(viewAng2+PI), cos(viewAng1), sin(viewAng1)*cos(viewAng2+PI));
		break;

	}

	/* locate the light source */
	GLfloat light0_position[]={1000.0, 500.0, 500.0, 0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

/*
	calling the terrain display-list to display the terrain and volcano. The GL_CULL_FACE is enable to eliminate displaying the bottom side of the terrain which is not displayed in this simulation.
*/
	glPushMatrix();
	glEnable(GL_CULL_FACE);
	glCallList(terrain);
	glDisable(GL_CULL_FACE);
	glPopMatrix();

/*
	Display smoke particles : when the smokeFlag is raised all generated smoke particles are displayed. The shape of the smoke particles are spheres with 4 slices and 4 stacks which makes them look more like cubes, but as they are blended the edges are not visible. The trick I used to make smoke more realistic is to disable the depth test while displaying smoke particles.
*/
	if (smokeFlag == 1){
		glPushMatrix();
		glTranslatef(500.0, 180.0, 500.0);
		glDepthMask(GL_FALSE);
		for(i=0; i<smoke_particles; i++){
			glPushMatrix();
			glTranslatef(smokePart[i][0], smokePart[i][1], smokePart[i][2]);
			glColor4f(0.7, 0.7, 0.7, smokePart[i][7]);
			glutSolidSphere(smokePart[i][6], 4, 4);
			glPopMatrix();
		}
		glDepthMask(GL_TRUE);
		glPopMatrix();
	}

/*
	Display rocks and ashes : when rockFlag is raised all generated rock and ash particles are displayed. The shape of these particles is cubes. "glutSolidDodecahedron()" can be used here as well which look much more like rocks. The problem with that is as it is a quadratic object and there are over 1000 of them generated, it slows the whole program down. Besides when looking at them from distance dodecahedron and cubes look almost alike.
*/
	if (rockFlag == 1){
		glPushMatrix();
		glTranslatef(500.0, terrainY[T/2][T/2], 500.0);
		glBegin(GL_QUADS);
		for(i=0; i<rock_particles; i++){
			glColor4f(rockPart[i][7], rockPart[i][7]/3, 0.0, 1.0);
			p0 = rockPart[i][0];
			p1 = rockPart[i][1];
			p2 = rockPart[i][2];
			glVertex3f(p0 - rockPart[i][6], p1 - rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 - rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 + rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 + rockPart[i][6], p2 - rockPart[i][6]);

			glVertex3f(p0 + rockPart[i][6], p1 - rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 - rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 + rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 + rockPart[i][6], p2 - rockPart[i][6]);

			glVertex3f(p0 - rockPart[i][6], p1 - rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 - rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 - rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 - rockPart[i][6], p2 + rockPart[i][6]);

			glVertex3f(p0 - rockPart[i][6], p1 + rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 + rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 + rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 + rockPart[i][6], p2 + rockPart[i][6]);

			glVertex3f(p0 - rockPart[i][6], p1 - rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 + rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 + rockPart[i][6], p2 - rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 - rockPart[i][6], p2 - rockPart[i][6]);

			glVertex3f(p0 - rockPart[i][6], p1 - rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 - rockPart[i][6], p1 + rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 + rockPart[i][6], p2 + rockPart[i][6]);
			glVertex3f(p0 + rockPart[i][6], p1 - rockPart[i][6], p2 + rockPart[i][6]);
		}
		glEnd();

		glPopMatrix();

/*
	Display lava particles : when rockFlag is raised all generated lava particles are displayed as well. Lava particles are points of size 6 which which both make lava flow more realistic and are generated much faster than any other shape and thus lots of them can be generated to simulate the lava flow.
*/
		glPushMatrix();
		glPointSize(6.0);
		glBegin(GL_POINTS);
		for(i=0; i<lava_particles; i++){
			glColor4f(lavaPart[i][3], lavaPart[i][3]/3, 0.0, lavaPart[i][4]);
			glVertex3fv(lavaPart[i]);
		}
		glEnd();
		glPopMatrix();
	}
	
/*
	Generate biplane : The biplane is displayed in the appropriate position.
*/
	glPushMatrix();
	glTranslatef(planeX, planeY, planeZ);
	render_biplane();
	glPopMatrix();

	glFlush();
	glutSwapBuffers();
}
/*************************************/
/*
	This function binds 6 different textures to be used in multi texturing. It is taken from "Lecture 20", "Practical 7" and "Redbook: texbind.c file". There are 5 2D texture and one 1D texture for the terrain and volcano. The 2D textures are read into the program using "readtex.c" file from practical 7 which makes mipmap textures. The 1D texture uses "terrainTex" array of size 16. The "GL_TEXTURE_MIN_FILTER" is set to "GL_LINEAR_MIPMAP_LINEAR" to make textured object look nicer and smoother from distance.
*/
void	init_Textures(void){

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glGenTextures(6, tNum);
	glBindTexture(GL_TEXTURE_2D, tNum[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
	LoadRGBMipmaps(TEX0, GL_RGB);

	glBindTexture(GL_TEXTURE_2D, tNum[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	LoadRGBMipmaps(TEX1, GL_RGB);

	glBindTexture(GL_TEXTURE_2D, tNum[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	LoadRGBMipmaps(TEX2, GL_RGB);

	glBindTexture(GL_TEXTURE_2D, tNum[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	LoadRGBMipmaps(TEX3, GL_RGB);

	glBindTexture(GL_TEXTURE_2D, tNum[4]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	LoadRGBMipmaps(TEX4, GL_RGB);

	glBindTexture(GL_TEXTURE_1D, tNum[5]);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
	glTexImage1D(GL_TEXTURE_1D, 0, 3, 16, 0, GL_RGB, GL_FLOAT, terrainTex);
}
/*************************************/
/*
	This function initialises the settings of the world. "glFrontFace" is set to GL_CCW to make GL_CULL_FACE display only the top side of the terrain. The textures are bined, the normals are computed (myinit), and all ather initialisations are performed before starting the simulation.
*/
void init(){
	glClearColor (0.6, 0.7, 1.0, 0.0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFrontFace(GL_CCW);
	init_Textures();
	myinit();
	init_lights();
	init_terrain();
	init_body();
	init_engine();
	init_tail();
	init_shield();
	init_wing();
}
/*************************************/
/*
	This function performs the animation in the world. It is called by "glutTimerFunc" and calls it every 40 miliseconds. The animation is divided into 6 sections; 1. animating biplane movement ,  2. animating propellers and flaps , 3. collision detections of the biplane , 4. animating smoke , 5. animating volcano eruption and 6. animating lava flow.
*/
void animate(){
	int i, j;
	float plane[3], fplane[3], p1[3], p2[3], p3[3], p4[3];
	float firstX, firstZ, lastX, lastZ;

	planeX += cos(pAltAng*PI/180)*sin(pLatAng*PI/180);
	planeY += sin(pAltAng*PI/180);
	planeZ += cos(pAltAng*PI/180)*cos(pLatAng*PI/180);

/*
	The "propAng" is increased by propDelta value. It must be reset to 0 if it reaches 360 degrees.
	The "rudderAng" and "elevatorAng" are set slowly back to 0 if they have a value larger or smaller than 0. This is to simulate the automatic return of the flaps to their default position. It makes the rotation of the biplane more realistic.
*/
	propAng += propDelta;
	if (propAng >= 360.0)
		propAng = 0.0;

	if (rudderAng > 0.0)
		rudderAng -= 1.0;
	if (rudderAng < 0.0)
		rudderAng += 1.0;

	if (elevatorAng > 0.0)
		elevatorAng -= 1.0;
	if (elevatorAng < 0.0)
		elevatorAng += 1.0;

/*
	Collision detection between biplane and terrain borders : After resetting the biplane to its initial position it skips the second collision detection test which is not the case to happen when the biplane is just reset.
*/
	if (planeX>=1000.0 || planeX<0.0 || planeZ>=1000.0 || planeZ<0.0){
		printf("Message : Collision is detected between biplane and terrain border.\n");
		planeX = 50.0;
		planeY = 300.0;
		planeZ = 50.0;
		pLatAng = 90.0;
		pAltAng = 0.0;
		goto skip;
	}
	
/*
	Collision Detection between biplane and terrain and volcano : For this collision detection 2 vectors "plane" and "fplane" are computed. "plane" holds the coordinates of the point in front of the biplane (right behind the engine) and "fplane" is a point which is located 1 unit in front of it in the biplane direction. The "plane" is intentionally set to the front of the biplane to make the crash more realistic.
	firstX, firstZ, LastX and LastZ are computed to be the (x,z) coordinates of the opposite vertices of the rectangle on the terrain net that plane-fplane line is mapped into. Then for every small square of size 1(out of 64) inside this rectangle 4 points p1 to p4 are generated which are the edges of 2 triangles. Then the "collision" function is called to check if the plan-fplan line intersects with either of these triangles. If the collision is detected a message is displayed on the terminal and the biplane is reset to its initial position. Detection a collision the function skips checking other triangles.
*/
	plane[0] = planeX+ 30*cos(pAltAng*PI/180)*sin(pLatAng*PI/180);
	plane[1] = planeY + 30*sin(pAltAng*PI/180);
	plane[2] = planeZ + 30*cos(pAltAng*PI/180)*cos(pLatAng*PI/180);
	fplane[0] = plane[0] + cos(pAltAng*PI/180)*sin(pLatAng*PI/180);
	fplane[1] = plane[1] + sin(pAltAng*PI/180);
	fplane[2] = plane[2] + cos(pAltAng*PI/180)*cos(pLatAng*PI/180);

	firstX = plane[0];
	firstZ = plane[2];
	lastX = fplane[0];
	lastZ = fplane[2];

	if (firstX > lastX){
		firstX = fplane[0];
		lastX = plane[0];
	}
	if (firstZ > lastZ){
		firstZ = fplane[2];
		lastZ = plane[2];
	}
	
	firstX = (int) (firstX/M);
	lastX = (int) (lastX/M) + 1;
	firstZ = (int) (firstZ/M);
	lastZ = (int) (lastZ/M) + 1;
	
	for (i = firstX ; i < lastX ; i++)
		for (j = firstZ ; j < lastZ ; j++){
			p1[0] = i * M;
			p1[1] = terrainY[i][j];
			p1[2] = j * M;

			p2[0] = i * M;
			p2[1] = terrainY[i][j+1];
			p2[2] = (j+1) * M;

			p3[0] = (i+1) * M;
			p3[1] = terrainY[i+1][j];
			p3[2] = j * M;

			p4[0] = (i+1) * M;
			p4[1] = terrainY[i+1][j+1];
			p4[2] = (j+1) * M;
				
			if (collision(p3, p1, p2, fplane, plane) || collision(p2, p4, p3, fplane, plane)){
				printf("Message : Collision is detected between biplane and terrain.\n");
				planeX = 50.0;
				planeY = 300.0;
				planeZ = 50.0;
				pLatAng = 90.0;
				pAltAng = 0.0;
				goto skip;
			}
		}

	skip:

/*
	Volcano Smoke : when smokeFlag is raised it checks if there are still some smoke particles to generate. If so it generates SMOKE number of particles and initialises them using calls to "new_smoke". Also the values of current smoke particles are updated using calls to "update_smoke" function. Finally it checks if the number of generated particles has reached a specific number. If so it raises the rockFlag to enable the volcano eruption and lava flow. Otherwise it calls the "roof_test" to test and generate new particles.
*/
	if (smokeFlag == 1){
		if (smoke_particles<=(SMOKE_MAX-SMOKE)){
			for(i=0; i<SMOKE; i++)
				new_smoke(i+smoke_particles);
			smoke_particles += SMOKE;
		}
		for(i=0; i<smoke_particles; i++){
			update_smoke(i, 0.3);
			if (smoke_particles + smokeNum < SMOKE_MAX * 2)
				roof_test(i);
			else
				rockFlag = 1;
		}
	}

/*
	Volcano Eruption : This part works the same as the smoke generating part (above). It is triggered by rockFlag and generates and checks rock particles.
*/

	if (rockFlag == 1){
		if(rock_particles<=(ROCK_MAX-ROCK)){
			for(i=0; i<ROCK; i++)
				new_rock(i+rock_particles);
			rock_particles += ROCK;
		}

		for(i=0; i<rock_particles; i++){
			update_rock(i, 0.25);
			if (rock_particles + rockNum < ROCK_MAX * 4)
				ground_test(i);
		}

/*
	Lava Flow : This part is triggered and performed simultaneously with volcano eruption and generates the lava particles.
*/
		if(lava_particles<=(LAVA_MAX-LAVA)){
			for(i=0; i<LAVA; i++)
				new_lava(i+lava_particles);
			lava_particles += LAVA;
		}

		for(i=0; i<lava_particles; i++){
			update_lava(i);
			lava_test(i);
		}
	}

	glutTimerFunc(40, animate, 0);
	glutPostRedisplay();
}
/*************************************/
/*
	The 4 keys "w", "s", "a" and "d" in both small and capital letters are applied only when the view is set to "Motion View".
*/
void keyboard(unsigned char key, int x, int y){
	switch(key){
		case 'W':
		case 'w':
			if (view_flag == VIEW4 && viewAng1>PI/90.0)
				viewAng1 -= PI/90.0;
			break;

		case 'S':
		case 's':
			if (view_flag == VIEW4 && viewAng1<42.0*PI/90.0)
				viewAng1 += PI/90.0;
			break;

		case 'A':
		case 'a':
			if (view_flag == VIEW4)
				viewAng2 -= PI/90.0;
			break;

		case 'D':
		case 'd':
			if (view_flag == VIEW4)
				viewAng2 += PI/90.0;
			break;

		case 27:
			exit(0);
		break;

		case 32:
				smokeFlag = 1;
		break;

		default:
			fprintf(stdout, "No function attached to key %c.\n", key);
	}
	glutPostRedisplay();
	return;
}
/*************************************/
void special(int key, int x, int y){
	switch(key){
	case GLUT_KEY_DOWN:
		if (pAltAng>=-68.0){				// applicable before reaching -70 degree.
			pAltAng -= 2.0;
			elevatorAng -= 2.0;
		}
    break;

	case GLUT_KEY_UP:
		if (pAltAng<=68.0){				// applicable before reaching +70 degree.
			pAltAng += 2.0;
			elevatorAng += 2.0;
		}
		break;

	case GLUT_KEY_RIGHT:
		pLatAng -= 2.0;
		if (rudderAng<=38.0)			// 	applicable before reaching +40 degree to limit the rotation.
			rudderAng += 2.0;
	break;

	case GLUT_KEY_LEFT:
		pLatAng += 2.0;
		if (rudderAng>=-38.0)			// 	applicable before reaching -40 degree to limit the rotation.
			rudderAng -= 2.0;
	break;

  default:
    fprintf(stdout, "No function attached to key.\n");
  }
  glutPostRedisplay();
  return;
}
/*************************************/
/*
	The only projection that is used in this simulation is perspective and the only requirement is to set it according to the window size.
*/
void left_menu(int id) {
	view_flag = id;
	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0, ((float) width)/((float)height), 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
}


/***************   Main  **************/
int main(int argc, char **argv) 
{  
	srand48(time(NULL));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(1200,900);
	glutInitWindowPosition(0,0);
	glutCreateWindow("CGV open assessment");

	/* Menu */
	glutCreateMenu(left_menu);
		glutAddMenuEntry("Cockpit View",VIEW1);
		glutAddMenuEntry("Outside View", VIEW2);
		glutAddMenuEntry("Fixed View",VIEW3);
		glutAddMenuEntry("Motion View",VIEW4);
		glutAttachMenu(GLUT_LEFT_BUTTON);

	init();

	glutDisplayFunc(display);
    glutTimerFunc(40, animate, 0);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMainLoop();
	return 0;
}
