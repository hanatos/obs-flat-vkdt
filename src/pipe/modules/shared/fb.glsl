// ssbo frame buffer access functions, assumes buf_fb is the 4-strided ssbo:f32 to be written/read

void fb_set(ivec2 px, ivec2 dim, vec3 L)
{
  uint idx = 4*(px.y * dim.x + px.x);
  buf_fb.v[idx+0] = L.r;
  buf_fb.v[idx+1] = L.g;
  buf_fb.v[idx+2] = L.b;
}

vec3 fb_get(ivec2 px, ivec2 dim)
{
  uint idx = 4*(px.y * dim.x + px.x);
  return vec3(buf_fb.v[idx+0], buf_fb.v[idx+1], buf_fb.v[idx+2]);
}

void fb_add(ivec2 px, ivec2 dim, vec3 L)
{
  uint idx = 4*(px.y * dim.x + px.x);
  atomicAdd(buf_fb.v[idx+0], L.r);
  atomicAdd(buf_fb.v[idx+1], L.g);
  atomicAdd(buf_fb.v[idx+2], L.b);
  atomicAdd(buf_fb.v[idx+3], dot(L,L));
}
