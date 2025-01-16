float
sigmoid(float x)
{
  return 0.5 * x / sqrt(1.0 + x * x) + 0.5;
}

float
inv_sigmoid(float s)
{
  s = clamp(s, 1e-8, 1.0-1e-8);
  return (2.0*s - 1.0)/(2.0*sqrt(s*(1.0-s)));
}

float
ddx_sigmoid(float x)
{
  return 0.5 * pow(x*x+1.0, -3.0/2.0);
}


// this is part of the sigma function for softmax.
// it takes care of all the clamping and numeric nonsense
float
sigma_exp(in float x)
{
  // if(!(x <  10.0)) return 1.0;
  // if(!(x > -10.0)) return 1.0;
  if(isnan(x)) return 1.0;
  return exp(clamp(.1 * x, -1000.0, 1000.0));
  // return exp(clamp(0.01 * x, -10.0, 26.0)); // XXX train with low factor and then increase for sharpness :)
}
