#extension GL_EXT_nonuniform_qualifier    : enable
#extension GL_EXT_shader_16bit_storage    : enable
#extension GL_KHR_shader_subgroup_basic   : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : enable
#extension GL_KHR_shader_subgroup_arithmetic               : enable

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
// layout(std140, set = 0, binding = 0) uniform global_t { } global;
layout(push_constant, std140) uniform push_t
{
  int batch_size; // number of pixels
  int level;      // mip level 0, 1, ..
} push;
// layout(std140, set = 0, binding = 1) uniform params_t { } params;

layout(set = 1, binding = 0) uniform sampler2D img_in; // the input image
layout(std430, set = 1, binding = 1) readonly  buffer ssbo_dEdK_t { float16_t v[]; } ssbo_dEdK; // dEdK of last layer, straight from loss/dapply
layout(std430, set = 1, binding = 2) readonly  buffer ssbo_A_t    { float16_t v[]; } ssbo_A;    // intermediate fwd: activations
layout(std430, set = 1, binding = 3) readonly  buffer ssbo_dEdA_t { float16_t v[]; } ssbo_dEdA; // intermediate bck: dE/dO
layout(std430, set = 1, binding = 4) writeonly buffer ssbo_dEdw_t { float     v[]; } ssbo_dEdw; // dE/dw
layout(std430, set = 1, binding = 5) readonly  buffer ssbo_nab_t  { float noise_a; float noise_b; } ssbo_nab; // noise profile a b

shared float shm_tmp[32];

float load_input(uint c, uint p)
{
#ifdef INFERENCE
  const vec2 noise_ab = vec2(params.noise_a, params.noise_b);
#else
  const vec2 noise_ab = vec2(ssbo_nab.noise_a, ssbo_nab.noise_b);
#endif
  return load_input_tap(img_in, p, c, noise_ab);
}

float c16(in float v)
{
  return clamp(v, -65000.0, 65000.0);
}

float get_dEdA(in uint c, in uint p, in int l)
{
  if(p >= push.batch_size) return 0.0;
  const int layer_stride = WIDTH * push.batch_size;
  if(l == N_HIDDEN_LAYERS+1)
  {
    if(c >= 16) // output stride
      return 0.0;
    return c16(ssbo_dEdK.v[nonuniformEXT(p * 16 + c)]); // loss derivative of last layer
  }
  else // l in [1..HIDDEN_LAYERS]
    return c16(ssbo_dEdA.v[nonuniformEXT(layer_stride * (N_HIDDEN_LAYERS - l) + p * WIDTH + c)]); // channels are fast, then pixels, then layers
}

float get_activation(in uint c, in uint p, in int l)
{
  if(p >= push.batch_size) return 0.0;
  const int layer_stride = WIDTH * push.batch_size;
  if(l == 0) // TODO: if input stride != WIDTH handle here
    return c16(load_input(c, p));
  else
    return c16(ssbo_A.v[nonuniformEXT(layer_stride * (l - 1) + p * WIDTH + c)]); // channels are fast, then pixels, then layers
}

// compute the derivative of the loss wrt weights.
// this needs to sum over the whole batch size, i.e. all pixels in the tiles used for learning (for instance 1024=32x32).
// the implementation could clearly be improved but don't care about backpropagation speed so much for now.
//
// configuration: 32x32 local threads work on computing one dot product row x col (dE/dO x A)
// launch one such workgroup per 32x32 entry. for resolutions > 1024px, we iterate.
//
// multiply matrices (no activations, plain) to get weight gradients:
//      (dE/dw)^T = A . (dE/dO)^T
//    computed as
//       dE/dw = dE/dO . A^T
//    i.e. using the transpose of the activations from the forward pass
//    A comes from the fwd pass, dE/dO comes from the backward pass
void
main()
{
  for(int l=0;l<N_HIDDEN_LAYERS+1;l++)
  {
    // for instance for hidden layers=3, we have
    // l=0  l=1    l=2    l=3    l=4
    // I    A[0]   A[1]   A[2]    K
    //   w[0]   w[1]   w[2]   w[3]     <= loop runs l over these indices
    float total = 0.0;
    for(int it=0;it<(push.batch_size+1023)/1024;it++)
    { // TODO: this loop depends on subgroup size = 32, also during invocation
      const uint px = 1024*it + 32*gl_LocalInvocationID.y + gl_LocalInvocationID.x;

      float res = 0.0;
      if(px < push.batch_size) res = get_dEdA(gl_WorkGroupID.y, px, l+1) * get_activation(gl_WorkGroupID.x, px, l);
      res = subgroupAdd(res);
      if(gl_SubgroupInvocationID == 0)
        shm_tmp[nonuniformEXT(gl_SubgroupID)] = res;

      barrier();

      if(gl_SubgroupID == 0)
      {
        res = shm_tmp[nonuniformEXT(gl_SubgroupInvocationID)];
        total += subgroupAdd(res);
      }
      barrier();
    }

    if(gl_SubgroupID == 0 && gl_SubgroupInvocationID == 0)
    {
      const int weights_stride = WIDTH * WIDTH;
      ssbo_dEdw.v[nonuniformEXT(l * weights_stride + gl_WorkGroupID.x + WIDTH*gl_WorkGroupID.y)] = clamp(total, -65000.0, 65000.0);
    }
    barrier();
  }
}
