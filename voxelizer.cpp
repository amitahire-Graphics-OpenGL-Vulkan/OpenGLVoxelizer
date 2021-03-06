#include "voxelizer.h"
#include "mc_space_partition.h"
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

#include <cmath>

using namespace std;
using namespace glm;


Voxelizer::Voxelizer(std::function<float(ivec3)> getDensityFn, glm::ivec3 domainMin, glm::ivec3 domainMax, glm::ivec3 mirrorAxis, float isolevel)
        :getDensityFn(getDensityFn), isolevel(isolevel), domainMin(domainMin), domainMax(domainMax), mirrorAxis(mirrorAxis)
{
}

vec3 Voxelizer::getGradientSobel(vec3 pos){
    // get values
    float x = pos[0];
    float y = pos[1];
    float z = pos[2];

    float l = 0.5;

    // get values between 0 and 2) here we assume that pos is at position 1.
    float   value000 = getLinearInterpolate(vec3{x-l, y-l, z-l}),
            value001 = getLinearInterpolate(vec3{x-l, y-l, z  }),
            value002 = getLinearInterpolate(vec3{x-l, y-l, z+l}),
            value100 = getLinearInterpolate(vec3{x  , y-l, z-l}),
            value101 = getLinearInterpolate(vec3{x  , y-l, z  }),
            value102 = getLinearInterpolate(vec3{x  , y-l, z+l}),
            value200 = getLinearInterpolate(vec3{x+l, y-l, z-l}),
            value201 = getLinearInterpolate(vec3{x+l, y-l, z  }),
            value202 = getLinearInterpolate(vec3{x+l, y-l, z+l}),
            value010 = getLinearInterpolate(vec3{x-l, y  , z-l}),
            value011 = getLinearInterpolate(vec3{x-l, y  , z  }),
            value012 = getLinearInterpolate(vec3{x-l, y  , z+l}),
            value110 = getLinearInterpolate(vec3{x  , y  , z-l}),
            // value111 = getLinearInterpolate({{x  , y  , z  }), // unused
            value112 = getLinearInterpolate(vec3{x  , y  , z+l}),
            value210 = getLinearInterpolate(vec3{x+l, y  , z-l}),
            value211 = getLinearInterpolate(vec3{x+l, y  , z  }),
            value212 = getLinearInterpolate(vec3{x+l, y  , z+l}),
            value020 = getLinearInterpolate(vec3{x-l, y+l, z-l}),
            value021 = getLinearInterpolate(vec3{x-l, y+l, z  }),
            value022 = getLinearInterpolate(vec3{x-l, y+l, z+l}),
            value120 = getLinearInterpolate(vec3{x  , y+l, z-l}),
            value121 = getLinearInterpolate(vec3{x  , y+l, z  }),
            value122 = getLinearInterpolate(vec3{x  , y+l, z+l}),
            value220 = getLinearInterpolate(vec3{x+l, y+l, z-l}),
            value221 = getLinearInterpolate(vec3{x+l, y+l, z  }),
            value222 = getLinearInterpolate(vec3{x+l, y+l, z+l});

    auto res = vec3(value000+2*value001+value002+
                2*value010+4*value011+2*value012+
                value020+2*value021+value022+
                -value200-2*value201-value202+
                -2*value210-4*value211-2*value212+
                -value220-2*value221-value222,

                   value000+2*value001  +value002+
                 2*value100+4*value101+2*value102+
                   value200+2*value201  +value202+
                  -value020-2*value021  -value022+
                -2*value120-4*value121-2*value122+
                  -value220-2*value221  -value222,

                   value000+2*value010  +value020+
                 2*value100+4*value110+2*value120+
                   value200+2*value210  +value220+
                  -value002-2*value012  -value022+
                -2*value102-4*value112-2*value122+
                  -value202-2*value212  -value222
                );
    if (res[0] != 0 || res[1] != 0 || res[2] != 0){
        res = normalize(res);
    }
    return -res; // note flipped normal here
}

vec3 Voxelizer::getGradientLinear(vec3 pos){
    float x = pos[0];
    float y = pos[1];
    float z = pos[2];

    int xi = (int) floor(x);
    float xf = x - xi;
    int yi = (int) floor(y);
    float yf = y - yi;
    int zi = (int) floor(z);
    float zf = z - zi;
    int xim = (int) floor(x + 0.5f);
    float xfm = x + 0.5f - xi;
    int yim = (int) floor(y + 0.5f);
    float yfm = y + 0.5f - yi;
    int zim = (int) floor(z + 0.5f);
    float zfm = z + 0.5f - zi;

    vec3 res;

    float xd0 = yf*(          zf *getDensity(ivec3{xim - 1, yi+1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xim - 1, yi+1, zi}))
                +(1.0f - yf)*(zf *getDensity(ivec3{xim - 1, yi  , zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xim - 1, yi  , zi}));
    float xd1 = yf*(          zf *getDensity(ivec3{xim,     yi+1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xim,     yi+1, zi}))
                +(1.0f - yf)*(zf *getDensity(ivec3{xim,     yi  , zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xim,     yi  , zi}));
    float xd2 = yf*(          zf *getDensity(ivec3{xim + 1, yi+1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xim + 1, yi+1, zi}))
                +(1.0f - yf)*(zf *getDensity(ivec3{xim + 1, yi  , zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xim + 1, yi  , zi}));
    res[0] = (xd1 - xd0) * (1.0f - xfm) + (xd2 - xd1) * xfm;

    float yd0 = xf*(          zf *getDensity(ivec3{xi+1, yim-1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xi+1, yim-1, zi}))
                +(1.0f - xf)*(zf *getDensity(ivec3{xi  , yim-1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xi  , yim-1, zi}));
    float yd1 = xf*(          zf *getDensity(ivec3{xi+1, yim  , zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xi+1, yim  , zi}))
                +(1.0f - xf)*(zf *getDensity(ivec3{xi  , yim  , zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xi  , yim  , zi}));
    float yd2 = xf*(          zf *getDensity(ivec3{xi+1, yim+1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xi+1, yim+1, zi}))
                +(1.0f - xf)*(zf *getDensity(ivec3{xi  , yim+1, zi+1})
                    + (1.0f - zf)*getDensity(ivec3{xi  , yim+1, zi}));
    res[1] = (yd1 - yd0) * (1.0f - yfm) + (yd2 - yd1) * yfm;

    float zd0 = xf*(          yf *getDensity(ivec3{xi+1, yi+1, zim-1})
                    + (1.0f - yf)*getDensity(ivec3{xi+1, yi  , zim-1}))
                +(1.0f - xf)*(yf *getDensity(ivec3{xi,   yi+1, zim-1})
                    + (1.0f - yf)*getDensity(ivec3{xi,   yi  , zim-1}));
    float zd1 = xf*(          yf *getDensity(ivec3{xi+1, yi+1, zim})
                    + (1.0f - yf)*getDensity(ivec3{xi+1, yi  , zim}))
                +(1.0f - xf)*(yf *getDensity(ivec3{xi,   yi+1, zim})
                    + (1.0f - yf)*getDensity(ivec3{xi,   yi  , zim}));
    float zd2 = xf*(          yf *getDensity(ivec3{xi+1, yi+1, zim+1})
                    + (1.0f - yf)*getDensity(ivec3{xi+1, yi  , zim+1}))
                +(1.0f - xf)*(yf *getDensity(ivec3{xi,   yi+1, zim+1})
                    + (1.0f - yf)*getDensity(ivec3{xi,   yi  , zim+1}));
    res[2] = (zd1 - zd0) * (1.0f - zfm) + (zd2 - zd1) * zfm;
    if (res[0] != 0 || res[1] != 0 || res[2] != 0){
        res = normalize(res);
    }
    return res;
}

float Voxelizer::getLinearInterpolate(vec3 pos){
    int x = floor(pos[0]);
    int y = floor(pos[1]);
    int z = floor(pos[2]);
    float x0 = (float)fmod(x,1.0);
    float x1 = 1.0f-x0;
    float y0 = (float)fmod(y,1.0);
    float y1 = 1.0f-y0;
    float z0 = (float)fmod(z,1.0);
    float z1 = 1.0f-z0;

    float x000 = getDensity(ivec3{x,y,z});
    float x001 = getDensity(ivec3{x,y,z+1});
    float x010 = getDensity(ivec3{x,y+1,z});
    float x011 = getDensity(ivec3{x,y+1,z+1});
    float x100 = getDensity(ivec3{x,y,z});
    float x101 = getDensity(ivec3{x,y,z+1});
    float x110 = getDensity(ivec3{x,y+1,z});
    float x111 = getDensity(ivec3{x,y+1,z+1});

    return ((x000*z0+x001*z1)*y0+(x010*z0+x011*z1)*y1)*x0+
           ((x100*z0+x101*z1)*y0+(x110*z0+x111*z1)*y1)*x1;
}

// linear interpolation (based on http://stackoverflow.com/a/21323490/420250 by MooseBoys)
vec3 Voxelizer::getGradientComponentWiseLinear(vec3 pos){
    float x = pos[0];
    float y = pos[1];
    float z = pos[2];
    vec3 res;
    float dist = 0.5f;
    // x
    int xi = (int) floor(x + dist);
    float xf = x + dist - xi;
    float xd0 = getDensity(ivec3{xi - 1, (int)y, (int)z});
    float xd1 = getDensity(ivec3{xi, (int)y, (int)z});
    float xd2 = getDensity(ivec3{xi + 1, (int)y, (int)z});
    res[0] = (xd1 - xd0) * (1.0f - xf) + (xd2 - xd1) * xf; // lerp
    // y
    int yi = (int) floor(y + dist);
    float yf = y + dist - yi;
    float yd0 = getDensity(ivec3{(int)x, yi - 1, (int)z});
    float yd1 = getDensity(ivec3{(int)x, yi, (int)z});
    float yd2 = getDensity(ivec3{(int)x, yi + 1, (int)z});
    res[1] = (yd1 - yd0) * (dist - yf) + (yd2 - yd1) * yf; // lerp
    // z
    int zi = (int) floor(z + dist);
    float zf = z + dist - zi;
    float zd0 = getDensity(ivec3{(int)x, (int)y, zi - 1});
    float zd1 = getDensity(ivec3{(int)x, (int)y, zi});
    float zd2 = getDensity(ivec3{(int)x, (int)y, zi + 1});
    res[2] = (zd1 - zd0) * (1.0f - zf) + (zd2 - zd1) * zf; // lerp
    if (res[0] != 0 || res[1] != 0 || res[2] != 0){
        res = normalize(res);
    }
    return res;
}


int edgeTable[]={
    0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };

int triTable[256][16] =
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
 {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
 {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
 {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
 {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
 {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
 {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
 {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
 {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
 {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
 {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
 {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
 {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
 {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
 {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
 {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
 {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
 {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
 {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
 {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
 {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
 {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
 {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
 {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
 {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
 {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
 {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
 {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
 {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
 {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
 {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
 {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
 {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
 {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
 {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
 {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
 {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
 {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
 {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
 {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
 {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
 {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
 {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
 {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
 {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
 {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
 {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
 {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
 {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
 {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
 {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
 {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
 {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
 {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
 {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
 {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
 {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
 {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
 {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
 {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
 {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
 {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
 {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
 {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
 {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
 {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
 {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
 {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
 {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
 {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
 {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
 {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
 {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
 {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
 {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
 {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
 {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
 {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
 {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
 {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
 {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
 {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
 {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
 {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
 {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
 {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
 {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
 {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
 {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
 {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
 {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
 {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
 {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
 {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
 {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
 {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
 {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
 {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
 {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
 {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
 {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
 {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
 {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
 {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
 {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
 {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
 {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
 {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
 {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
 {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
 {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
 {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
 {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
 {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
 {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
 {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
 {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
 {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
 {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
 {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
 {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
 {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
 {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
 {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
 {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
 {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
 {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
 {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
 {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
 {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
 {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
 {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
 {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
 {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
 {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
 {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
 {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
 {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
 {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
 {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
 {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
 {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
 {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
 {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
 {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
 {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
 {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
 {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
 {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
 {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
 {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
 {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
 {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
 {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
 {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
 {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
 {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
 {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
 {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
 {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
 {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
 {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
 {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
 {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
 {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
 {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
 {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
 {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
 {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
 {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
 {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
 {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
 {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
 {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
 {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
 {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
 {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
 {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
 {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
 {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

vec3 VertexInterp(float isolevel,vec3 p1,vec3 p2,float valp1,float valp2)
{
    vec3 p;

    if (abs(isolevel-valp1) < 0.00001)
        return(p1);
    if (abs(isolevel-valp2) < 0.00001)
        return(p2);
    if (abs(valp1-valp2) < 0.00001)
        return(p1);
    float mu = (isolevel - valp1) / (valp2 - valp1);
    for (int i=0;i<3;i++){
        p[i] = p1[i] + mu * (p2[i] - p1[i]);
    }

    return p;
}

float Voxelizer::getDensity(glm::ivec3 p){
    for (int i=0;i<3;i++){
        if (p[i]==domainMin[i] && mirrorAxis[i]==-1){
            p[i]++;
        } else if (p[i]==domainMax[i]-1 && mirrorAxis[i]==1){
            p[i]--;
        }
    }
    return getDensityFn(p);
}

// Based on http://paulbourke.net/geometry/polygonise/
std::vector<vec3> Voxelizer::march(ivec3 pos){
    std::vector<vec3> res;

    int x = pos[0];
    int y = pos[1];
    int z = pos[2];

    vec3 p000{x+0.0f, y+0.0f, z+0.0f};
    vec3 p001{x+0.0f, y+0.0f, z+1.0f};
    vec3 p010{x+0.0f, y+1.0f, z+0.0f};
    vec3 p011{x+0.0f, y+1.0f, z+1.0f};
    vec3 p100{x+1.0f, y+0.0f, z+0.0f};
    vec3 p101{x+1.0f, y+0.0f, z+1.0f};
    vec3 p110{x+1.0f, y+1.0f, z+0.0f};
    vec3 p111{x+1.0f, y+1.0f, z+1.0f};

    vec3 p[] = {
        p000,
        p100,
        p101,
        p001,
        p010,
        p110,
        p111,
        p011
    };

    float f000 = getDensity({ivec3{x+0, y+0, z+0}});
    float f001 = getDensity({ivec3{x+0, y+0, z+1}});
    float f010 = getDensity({ivec3{x+0, y+1, z+0}});
    float f011 = getDensity({ivec3{x+0, y+1, z+1}});
    float f100 = getDensity({ivec3{x+1, y+0, z+0}});
    float f101 = getDensity({ivec3{x+1, y+0, z+1}});
    float f110 = getDensity({ivec3{x+1, y+1, z+0}});
    float f111 = getDensity({ivec3{x+1, y+1, z+1}});

    float gridCell[] = {
        f000,
        f100,
        f101,
        f001,
        f010,
        f110,
        f111,
        f011
    };

    //Determine the index into the edge table which
    //tells us which vertices are inside of the surface
    ///
    int cubeindex = 0;
    if (gridCell[0] < isolevel) cubeindex |= 1;
    if (gridCell[1] < isolevel) cubeindex |= 2;
    if (gridCell[2] < isolevel) cubeindex |= 4;
    if (gridCell[3] < isolevel) cubeindex |= 8;
    if (gridCell[4] < isolevel) cubeindex |= 16;
    if (gridCell[5] < isolevel) cubeindex |= 32;
    if (gridCell[6] < isolevel) cubeindex |= 64;
    if (gridCell[7] < isolevel) cubeindex |= 128;

    // Cube is entirely in/out of the surface //
    if (edgeTable[cubeindex] == 0)
        return res;

    vec3 vertlist[12];

    // Find the vertices where the surface intersects the cube //
    if ((edgeTable[cubeindex] & 1) != 0)
        vertlist[0] = VertexInterp(isolevel,p[0],p[1],gridCell[0],gridCell[1]);
    if ((edgeTable[cubeindex] & 2) != 0)
        vertlist[1] = VertexInterp(isolevel,p[1],p[2],gridCell[1],gridCell[2]);
    if ((edgeTable[cubeindex] & 4) != 0)
        vertlist[2] = VertexInterp(isolevel,p[2],p[3],gridCell[2],gridCell[3]);
    if ((edgeTable[cubeindex] & 8) != 0)
        vertlist[3] = VertexInterp(isolevel,p[3],p[0],gridCell[3],gridCell[0]);
    if ((edgeTable[cubeindex] & 16) != 0)
        vertlist[4] = VertexInterp(isolevel,p[4],p[5],gridCell[4],gridCell[5]);
    if ((edgeTable[cubeindex] & 32) != 0)
        vertlist[5] = VertexInterp(isolevel,p[5],p[6],gridCell[5],gridCell[6]);
    if ((edgeTable[cubeindex] & 64) != 0)
        vertlist[6] = VertexInterp(isolevel,p[6],p[7],gridCell[6],gridCell[7]);
    if ((edgeTable[cubeindex] & 128) != 0)
        vertlist[7] = VertexInterp(isolevel,p[7],p[4],gridCell[7],gridCell[4]);
    if ((edgeTable[cubeindex] & 256) != 0)
        vertlist[8] = VertexInterp(isolevel,p[0],p[4],gridCell[0],gridCell[4]);
    if ((edgeTable[cubeindex] & 512) != 0)
        vertlist[9] = VertexInterp(isolevel,p[1],p[5],gridCell[1],gridCell[5]);
    if ((edgeTable[cubeindex] & 1024) != 0)
        vertlist[10] = VertexInterp(isolevel,p[2],p[6],gridCell[2],gridCell[6]);
    if ((edgeTable[cubeindex] & 2048) != 0)
        vertlist[11] = VertexInterp(isolevel,p[3],p[7],gridCell[3],gridCell[7]);

    // Create the triangle //

    for (int i=0;triTable[cubeindex][i]!=-1;i+=3) {
        res.push_back(vertlist[triTable[cubeindex][i  ]]);
        res.push_back(vertlist[triTable[cubeindex][i+1]]);
        res.push_back(vertlist[triTable[cubeindex][i+2]]);
    }

    return res;
}

void Voxelizer::getData(std::vector<vec3> &vertexPositions, std::vector<vec3> &vertexNormals, std::vector<int> &indices){

    MCSpacePartition partition{domainMax.x, domainMax.y};

    // get vertexData
    for (int z = domainMin.z; z<domainMax.z; z++) {
        for (int x=domainMin.x; x<domainMax.x; x++) {
            for (int y=domainMin.y; y<domainMax.y; y++) {

                ivec3 p{x,y,z};

                auto createdTriangles = march(p);

                for (auto vertexPosition : createdTriangles){
                    int pos = partition.findPoint(vertexPosition, p);
                    if (pos >= 0){
                        indices.push_back(pos);
                    } else {
                        int newIndex = vertexPositions.size();
                        partition.insertPoint(vertexPosition, newIndex, p);
                        indices.push_back(newIndex);
                        vertexPositions.push_back(vertexPosition);
                    }

                }
            }
        }
    }

    computeNormals(vertexPositions, vertexNormals, indices);
}

void Voxelizer::computeNormals(vector<vec3> &vertexPositions, vector<vec3> &vertexNormals, vector<int> &indices) {
    if (angleWeightedNormals){
        computeAngleWeightedNormals(vertexPositions, vertexNormals, indices);
    } else {
        for (auto vertexPosition : vertexPositions) {
            if (interpolation == Interpolation::ComponentWiseLinear) {
                vertexNormals.push_back(getGradientComponentWiseLinear(vertexPosition));
            } else if (interpolation == Interpolation::Linear) {
                vertexNormals.push_back(getGradientLinear(vertexPosition));
            } else if (interpolation == Interpolation::Sobel) {
                vertexNormals.push_back(getGradientSobel(vertexPosition));
            }
        }
    }
}

void Voxelizer::computeAngleWeightedNormals(vector<vec3> &vertexPositions, vector<vec3> &vertexNormals, vector<int> &indices) {
    assert(vertexNormals.size()==0);
    vertexNormals.resize (vertexPositions.size(), vec3{0});
    for (int i=0;i<indices.size();i=i+3){
        int i1 = indices[i];
        int i2 = indices[i+1];
        int i3 = indices[i+2];
        vec3 p1 = vertexPositions[i1];
        vec3 p2 = vertexPositions[i2];
        vec3 p3 = vertexPositions[i3];
        vec3 p2p1 = p2-p1;
        vec3 p3p1 = p3-p1;
        vec3 p3p2 = p3-p2;
        float p2p1Len = length(p2p1);
        float p3p1Len = length(p3p1);
        float p3p2Len = length(p3p2);

        bool skipNeedle = p2p1Len<0.0001f || p3p1Len<0.0001f || p3p2Len<0.0001f ;
        if (skipNeedle){
            continue;
        }
        // normalize
        p2p1 = p2p1 / p2p1Len;
        p3p1 = p3p1 / p3p1Len;
        p3p2 = p3p2 / p3p2Len;

        vec3 faceNormal = normalize(cross(p2p1, p3p1));
        float p1Angle = angle(p2p1, p3p1);
        float p2Angle = angle(-p2p1, p3p2);
        float p3Angle = 180 - p2Angle - p1Angle;
        vertexNormals[i1] += faceNormal * p1Angle;
        vertexNormals[i2] += faceNormal * p2Angle;
        vertexNormals[i3] += faceNormal * p3Angle;
    }
    for (auto & n: vertexNormals){
        n = normalize(n);
    }
}

bool Voxelizer::isAngleWeightedNormals() const {
    return angleWeightedNormals;
}

void Voxelizer::setAngleWeightedNormals(bool angleWeightedNormals) {
    this->angleWeightedNormals = angleWeightedNormals;
}

void Voxelizer::setInterpolation(Interpolation i) { interpolation = i; }

Interpolation Voxelizer::getInterpolation() { return interpolation; }
