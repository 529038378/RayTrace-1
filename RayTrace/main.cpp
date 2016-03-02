#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define nelem(a) (sizeof(a) / (sizeof(a[0])))
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) < (y) ? (y) : (x))

#define PI 3.1415926536
#define WIDTH 500
#define HEIGHT 500

typedef unsigned char rgb[3];

struct vec3 {
	float x, y, z;

	vec3(float x_ = 0, float y_ = 0, float z_ = 0) {
		x = x_; y = y_; z = z_;
	}

	vec3 operator+(const vec3 &b) const {
		return vec3(x + b.x, y + b.y, z + b.z);
	}

	vec3 operator-(const vec3 &b) const {
		return vec3(x - b.x, y - b.y, z - b.z);
	}

	vec3 operator*(float b) const {
		return vec3(x * b, y * b, z * b);
	}

	vec3 mult(const vec3 &b) const {
		return vec3(x * b.x, y * b.y, z * b.z);
	}

	float dot(const vec3 &b) const {
		return x * b.x + y * b.y + z * b.z;
	}

	float length() const {
		return sqrtf(x * x + y * y + z * z);
	}
};

struct Material {
	vec3 color;
	float ka, kd, ks, ns, kr;
};

//output image
rgb *fb;

//vec3 viewpoint = { 0.0, 1.0, 0.0 }; //pic1.ppm
vec3 viewpoint = { 0.0, 0.0, 6.0 }; //pic2.ppm

//viewplane;
vec3 vp_u = { 1.0f, 0.0f, 0.0f };
vec3 vp_v = { 0.0f, 1.0f, 0.0f };
vec3 vp_w = { 0.0f, 0.0f, -1.0f };

//focal length
float vp_d = 1.0;

//set of spheres
vec3 mysph_pos[] = {
	{ 2.0f, 1.0f, -7.0f },
	{ -1.0f, 1.0f, -6.0f }
};

//material properties
Material myobj_mat[] = {
	{ { 0.6f, 0.8f, 0.6f }, 0.2f, 1.0f, 0.0f, 28, 0.0f },
	{ { 0.6f, 0.6f, 0.8f }, 0.4f, 0.8f, 0.7f, 28, 0.0f },
	{ { 1.0f, 1.0f, 1.0f }, 0.7f, 0.7f, 0.7f, 28, 0.5f }  //floor
};

//radius of spheres
float radius[] = { mysph_pos[0].y, mysph_pos[1].y };

//background color
vec3 bgcolor = { 0.5f, 0.5f, 0.5f };
vec3 ambient = { 0.2f, 0.2f, 0.2f };
vec3 black = { 0.0f, 0.0f, 0.0f };

//set of lights
vec3 light_pos[] = {
	{ 5.0f, 6.0f, -6.0f },
	{ -5.0f, 3.0f, -4.5f }
};

//pic2.ppm
vec3 light_color[] = {
	{ 0.5f, 0.5f, 0.5f },
	{ 0.7f, 0.0f, 0.0f }
};
/*
//pic1.ppm
vec3 light_color[] = {
	{ 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f }
};
*/

vec3 normalize(const vec3 &v)
{
	float n = v.length();
	if (n == 0.0)
		n = FLT_MIN;
	return v * (1.0 / n);
}


float clamp(float v, float l, float m)
{
	if (v > m) return m;
	if (v < l) return l;
	return v;
}

vec3 clamp(const vec3 &v)
{
	vec3 r = { clamp(v.x, 0.0, 1.0), clamp(v.y, 0.0, 1.0), clamp(v.z, 0.0, 1.0) };
	return r;
}

int intersect(const vec3 &origin, const vec3 &dir, float &t) {
	int obj = -1;
	float minDist = FLT_MAX;
	int mysph_size = sizeof(mysph_pos) / sizeof(vec3);

	int i = 0;
	while (i < mysph_size) {
		vec3 op = mysph_pos[i] - origin;

		float b = op.dot(dir), det = op.dot(op) - radius[i] * radius[i] - b * b ;

		if (op.length() - 1e-4 <= radius[i] || det > 0) {
			i++;
			continue;
		}
		
		float c = b - sqrtf(-det);

		if (c < minDist) {
			minDist = c;
			obj = i;
		}
		i++;
	}

	if (dir.y < 0) {
		float c = -(origin.y) / (dir.y);
		if (c < minDist) {
			minDist = c;
			obj = i;
		}
	}

	t = minDist;

	return obj;
}

vec3 mirrorDir(const vec3 &pos, const vec3 &dir, const int obj) {
	int obj_size = sizeof(mysph_pos) / sizeof(vec3);

	vec3 n;
	if (obj == obj_size)
		n = { 0.0f, 1.0f, 0.0f };
	else
		n = normalize(pos - mysph_pos[obj]);

	return dir - n * 2 * n.dot(dir);
}

vec3 traceray(const vec3 &origin, const vec3 &dir, int depth)
{
	if (depth == 0)
		return vec3();

	vec3 fcolor = bgcolor;
	int obj;
	float t;

	if ((obj = intersect(origin, dir, t)) != -1) {
		fcolor = ambient.mult(myobj_mat[obj].color);
		int light_size = sizeof(light_pos) / sizeof(vec3);
		int obj_size = sizeof(mysph_pos) / sizeof(vec3);
		vec3 pos = origin + (dir * t);

		for (int i = 0; i < light_size; i++) {
			vec3 lightray = normalize(light_pos[i] - pos);
			float temp;

			if (intersect(pos, lightray, temp) == -1) {
				if (myobj_mat[obj].kd > 0) {
					vec3 nor = normalize(pos - mysph_pos[obj]);
					fcolor = fcolor + ((light_color[i] * (nor.dot(lightray)) * myobj_mat[obj].kd).mult(myobj_mat[obj].color));
				}

				if (myobj_mat[obj].ks > 0) {
					vec3 refDir = mirrorDir(pos, lightray, obj) * (-1);
					fcolor = fcolor + ((light_color[i] * (dir.dot(refDir)) * myobj_mat[obj].ks).mult(myobj_mat[obj].color));
				}
			}

			if(myobj_mat[obj].kr > 0)
				fcolor = fcolor + ((traceray(pos, mirrorDir(pos, dir, obj), depth - 1) * myobj_mat[obj].kr).mult(myobj_mat[obj].color));
		}
	}

	// note that, before returning the color, the computed color may be rounded to [0.0, 1.0].
	fcolor = clamp(fcolor);
	return fcolor;
}

void render(void)
{
	int x, y;
	vec3 fcolor, origin, dir;

	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {

			origin = viewpoint + (vp_u * ((1.0 / WIDTH)*(x + .5) - .5))
				+ (vp_v * ((1.0 / HEIGHT)*(y + .5) - .5))
				+ (vp_w * vp_d);
			dir = normalize(origin - viewpoint);

			fcolor = traceray(origin, dir, 2);
			fb[(HEIGHT - y) * WIDTH + x][0] = fcolor.x * 255;
			fb[(HEIGHT - y) * WIDTH + x][1] = fcolor.y * 255;
			fb[(HEIGHT - y) * WIDTH + x][2] = fcolor.z * 255;
		}
	}
}

void WritePPM(char *fn, rgb *dataset)
{
	int i, j;
	FILE *fp;
	if ((fp = fopen(fn, "wb")) == NULL) {
		perror(fn);
		return;
	}
	printf("Begin writing to file %s....", fn);
	fflush(stdout);
	fprintf(fp, "P6\n%d %d\n%d\n", WIDTH, HEIGHT, 255);
	for (j = 0; j < HEIGHT; j++)
		for (i = 0; i < WIDTH; i++) {
			fputc(dataset[j*WIDTH + i][0], fp);
			fputc(dataset[j*WIDTH + i][1], fp);
			fputc(dataset[j*WIDTH + i][2], fp);
		}
	printf("done\n");
	fclose(fp);
}

int main(int argc, char *argv[])
{
	/*
	if(argc != 2) {
	fprintf(stderr, "Usage: %s <output_img>\n"
	"       <output_img>: PPM file\n", argv[0]);
	exit(-1);
	}
	*/

	fb = (rgb*)malloc(WIDTH * HEIGHT * sizeof(rgb));
	if (fb == NULL) {
		perror("malloc");
		exit(-1);
	}

	render();
	//WritePPM(argv[1], fb);
	WritePPM("test.ppm", fb);
	free(fb);

	return 0;
}