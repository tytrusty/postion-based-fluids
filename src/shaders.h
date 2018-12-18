#ifndef SHADER_H
#define SHADER_H
const char* vertex_shader =
#include "shaders/default.vert"
;
const char* geometry_shader =
#include "shaders/default.geom"
;
const char* fragment_shader =
#include "shaders/default.frag"
;
const char* floor_fragment_shader =
#include "shaders/floor.frag"
;
const char* bounds_fragment_shader =
#include "shaders/bounds.frag"
;
const char* particle_vertex_shader =
#include "shaders/particle.vert"
;
const char* particle_fragment_shader =
#include "shaders/particle.frag"
;
const char* quad_vertex_shader =
#include "shaders/depth.vert"
;
const char* depth_fragment_shader =
#include "shaders/depth.frag"
;
const char* filter_fragment_shader =
#include "shaders/filter.frag"
;
const char* normal_fragment_shader =
#include "shaders/normal.frag"
;
const char* fluid_fragment_shader =
#include "shaders/fluid.frag"
;
#endif
