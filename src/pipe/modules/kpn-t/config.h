#define EL_PER_UVEC4 8 // how many float16_t elements per uvec4/128 bit load
#define WIDTH 64
#define SKEW 8  // or 0 if WIDTH % 16 != 0
#define N_BLOCKS (WIDTH / 16)     // how many blocks in the weights for one layer, when we can work on 16x16 at a time.
#define N_ITERS 8 // or 2 if WIDTH >= 256 // going over pixels: how many iterations working on batches of px
#define N_HIDDEN_LAYERS 6
#define GRAD_SCALE 128.0 // avoid underflow in f16 loss/gradients
// modes for convolution by kernel:
#define APPLY_SOFTMAX 1
// #define APPLY_SIGMOID 2
#define APPLY_PLAIN 3
#define APPLY_DEBUG 4
// #define APPLY_ACTIVATION APPLY_SOFTMAX
#define APPLY_ACTIVATION APPLY_PLAIN
// #define APPLY_ACTIVATION APPLY_DEBUG
// modes for alpha blending
#define ALPHA_CONST 1
#define ALPHA_PLAIN 2
#define ALPHA_SIGMOID 3
#define ALPHA_CLAMPED 4
// #define ALPHA_ACTIVATION ALPHA_PLAIN
// #define ALPHA_ACTIVATION ALPHA_CLAMPED
#define ALPHA_ACTIVATION ALPHA_CONST // XXX DEBUG
// #define ALPHA_ACTIVATION ALPHA_SIGMOID
// apply softmax + alpha plain seems to be a winning combination
// both plain also works (and through negative filter weights may be more expressive)

// XXX maybe the downsampling 2x2 is broken to begin with.
// XXX possible that it aliases to hell?
// XXX look at screenshot: lines + coarse lines at (y,x) swapped
// XXX look at other screenshot: very likely aliasing, very apparent starting with mip3
// #define POST_MLP_DIFF // only add (I-smooth(I))*alpha to the coarse (i.e. taking the diff post MLP)
// broken in interesting ways, way too smooth, can create negative halos around high contrast edges
#define PRE_MLP_DIFF // only pass laplacians, i.e. detail coefficients to the mlp for classification (noise/signal) or signal extraction
#define NO_ALIAS // use PRE_MLP_DIFF semantics, but on properly bandlimited mipmaps

#define MLP_ACTIVATION_RELU 1
#define MLP_ACTIVATION_LEAKY_RELU 2
#define MLP_ACTIVATION_NONE 3
// #define MLP_ACTIVATION MLP_ACTIVATION_RELU // best candidate for results
#define MLP_ACTIVATION MLP_ACTIVATION_LEAKY_RELU // best candidate for results
// #define MLP_ACTIVATION MLP_ACTIVATION_NONE // debug deriv outside mlp with large offset DERIV_EPS

#define LOSS_L2 1    // prefers strong contrast edges
#define LOSS_SMAPE 2 // prefers noise reduction
#define LOSS_COSH 3
#define LOSS LOSS_SMAPE

// #define DEBUG_DERIV // debug derivatives instead of training
#define DERIV_EPS 1 // lower will only show numeric jitter

#if 0 // check memory bounds before access
#define CHK_WGT(base, stride) if(base + 15 * stride + 16/EL_PER_UVEC4 <= WIDTH * WIDTH * (N_HIDDEN_LAYERS+1)/EL_PER_UVEC4)
#define CHK_SHM(base, stride) if(base + 15 * stride + 16/EL_PER_UVEC4 <= ((16 + 16*N_ITERS) * (WIDTH + SKEW)) / EL_PER_UVEC4)
#define CHK_OUT(base, stride) if(base + 15 * stride + 16/EL_PER_UVEC4 <= push.batch_size * 16 / EL_PER_UVEC4)
#else // nothing
#define CHK_WGT(base, stride)
#define CHK_SHM(base, stride)
#define CHK_OUT(base, stride)
#endif
