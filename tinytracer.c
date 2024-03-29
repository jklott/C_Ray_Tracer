#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include "tinysupport.h"

typedef struct {
    float position[3];
    float intensity;
} Light;

typedef struct {
    float diffuse_color[3];
    float albedo[2];
    float specular_exponent;

}  Material;

typedef struct {
    float center[3];
    float radius;
    Material material;

} Sphere;

bool ray_intersect(const float origin[], const float dir[], float * t0, Sphere s) {
    /*
    Ray-Sphere Intersection
    
    Vectors:
        origin (the zero vector)
        dir (direction vector)
        L (vector from origin to center of sphere)
    Scalars:
        tca
        d2
        thc
        t0
        t1    
    */
    float L[3] = {0,0,0}; //The zero vector
    arrSub(s.center, origin, L, 3); //L is now the vector from origin to the sphere's center

    float tca = dotProduct(L, dir, 3); //Projection of L onto dir
    float d2 = dotProduct(L, L, 3) - tca*tca;

    if (d2 > s.radius * s.radius) return false; //There is no intersection, so return false.

    float thc = sqrtf((s.radius*s.radius - d2));
    * t0 = tca - thc;
    float t1 = tca + thc;
    if (t0 < 0) {
        * t0 = t1;
    }
    if (* t0 < 0) return false;

    return true;
}

bool scene_intersect(const float origin[], const float dir[], const Sphere s[], int len, float hit[], float N[], Material * ptr_m) {
    float sphere_dist = INT_MAX;

    for (size_t i=0; i < len; i++) {
        float dist_i;
        float * t0 = &dist_i;
        if (ray_intersect(origin, dir, t0, s[i]) && dist_i < sphere_dist) {
            sphere_dist = dist_i;

            float dirDist[3];
            arrScalarMult(dir, dist_i, dirDist, 3);
            arrAdd(origin, dirDist, hit, 3);

            float hitMinusCenter[3];
            arrSub(hit, s[i].center, hitMinusCenter, 3);
            normalize(hitMinusCenter, 3);

            N[0] = hitMinusCenter[0];
            N[1] = hitMinusCenter[1];
            N[2] = hitMinusCenter[2];

            * ptr_m = s[i].material;
        }
    }
    return sphere_dist<1000;
}

int cast_ray(const float origin[], const float dir[], const Sphere s[], const Light l[], int l_size, unsigned char colorArr[]) {
    float point[3], N[3];
    Material m;
    Material * ptr_m = &m;

    if (!scene_intersect(origin, dir, s, 3, point, N, ptr_m)) {
        //background
        colorArr[0] = 5; //red
        colorArr[1] = 100; //green
        colorArr[2] = 250; //blue
    } else {
        float diffuse_light_intensity = 0, specular_light_intensity = 0;
        float light_dir[3];
        float tempArr[3];
        
        for (size_t i = 0; i < l_size; i++) {
            arrSub(l[i].position, point, light_dir, 3);
            normalize(light_dir, 3);
            diffuse_light_intensity += l[i].intensity * ((0.f >= dotProduct(light_dir, N, 3) ? (0.f) : (dotProduct(light_dir, N, 3))));
            arrScalarMult(light_dir, -1, tempArr, 3);
            reflect(tempArr, N, tempArr, 3);
            float specHelper = dotProduct(tempArr, dir, 3);
            specular_light_intensity += powf((0.f > -specHelper ? 0.f : -specHelper), m.specular_exponent) * l[i].intensity;
        }
        /*
        light up pixels and prevent overflow
        */
       float k = diffuse_light_intensity * m.albedo[0] + specular_light_intensity * m.albedo[1];
        colorArr[0] = ((m.diffuse_color[0] * k > 255) ? (255) : (m.diffuse_color[0] * k));
        colorArr[1] = ((m.diffuse_color[1] * k > 255) ? (255) : (m.diffuse_color[1] * k));
        colorArr[2] = ((m.diffuse_color[2] * k > 255) ? (255) : (m.diffuse_color[2] * k));
    }

    return 0;
}

int render(const Sphere s[], const Light l[], int l_length) {
    /*
    Creates image in a new color each step.
    */
    const int width = 1024;
    const int height = 768;

    FILE *fp = fopen("sixth.ppm", "wb"); // Write in binary mode
    (void) fprintf(fp, "P6\n%d %d\n255\n", width, height);

    float fov = 3.1415926535/2.; // Field of View

    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {

            float x = (2*(i+.5)/(float)width - 1)*tan(fov/2.)*width/(float)height;
            float y = -(2*(j+.5)/(float)height - 1)*tan(fov/2.);

            float dir[] = {x,y,-1};
            normalize(dir, 3);

            unsigned char color[3];
            const float origin[] = {0,0,0};
            cast_ray(origin, dir, s, l, l_length, color);
            (void) fwrite(color, 1, 3, fp);
        }
    }
    (void) fclose(fp);
    return 0;
}

int main(void) {
    Material red = {{255,0,0}, {.6, .3}, 50.};
    Material pink = {{150,10,150}, {.9, .5}, 50.};
    Material gold = {{255, 195, 0}, {.6, .4}, 50.};

    //Populate with spheres
    Sphere s[3];
    Sphere smallGold = {{-6,0,-16},2,gold};
    Sphere bigRed = {{-1.0, -1.5, -12}, 3, red};
    Sphere farPink = {{7,5,-18},2, pink};

    s[0] = smallGold;
    s[1] = bigRed;
    s[2] = farPink;

    //Add light source
    Light l[2];
    
    Light left_light = {{-20,20,20}, 1.25};

    Light top_light = {{0, 20, 0}, 1.0};

    l[0] = left_light;
    l[1] = top_light;

    render(s,l, 2);
    printf("Run success!\n");
    return 0;
}