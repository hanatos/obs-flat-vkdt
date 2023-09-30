# colour: essential colour handling

this is a unified module to handle important things concerning colour.
the input is in camera rgb and the output will be rec2020.

## input device transform

it supports two modes of operation: using 3x3 colour matrices and profiles
created from spectral response curves of the cameras colour filter array (CFA).
to use such a profile, first [create one for your camera using the profiling
toolchain](../../../tools/clut/readme.md). then set the `matrix` parameter to `clut`.
to route the profile data to the input here, there is a `clut.pst` preset.

the second, more simplistic mode of operation applies a 3x3 matrix. this kind
of input transform is known to be imprecise especially for saturated colours
and produce all kinds of artifacts (say way out of gamut dark blue tones).

## colour mapping

to fine tune calibration or for artistic purposes, this module comes with a
corrective radial basis function (RBF) applied after white balance and matrix
(or clut). it allows to map arbitrary source to target points in 3d rec2020
colour space.

this mechanism be used to calibrate the input colour against a measured
test chart (for instance an IT8), or for artistic colour manipulation.

so far, the gui only allows you to change the target values, the source values
can be filled using the colour picker (using the `import` button) and from a
preset (see the `colourcc.pst` preset for the 24 ColorChecker patches).or, of
course, by editing the `.cfg` file by hand, the `rbmap` parameter is a series
of `src r:src g:src b:tgt r:tgt g:tgt b` elements, the number of source and
target pairs is given by the `cnt` parameter.

## white balancing

white balancing (using CAT16) will adjust rec2020 D65 white to the colour
represented by the three white sliders. to input alternative source colours
instead of D65, connect the `picked` input from an instance of a [colour picker
module](../pick/readme.md), operating on the input to this colour module. the
picked patch will be used as source colour and white balancing will be adjusted
such that it will match the destination colour defined by the sliders. in
conjunction with skin tone presets (cf. `colour-monk-[0-9].pst`), this is
useful to match skin rendition precisely in difficult lighting situations.

to pick a neutral spot, you can use the `pick-neutral.pst` preset.

## gamut mapping

the `gamut` parameter employs a few lookup tables to
work. in particular the optional inputs `abney` and `spectra` have to be
connected. you can easily do this by applying the `gamut.pst`
preset (press `ctrl-p` and start to type `gamut.pst` until it shows as the
first entry in the list, then press enter).
once this is done, the parameter is closely intertwined with the `sat` control:
if `gamut` is set to unlimited, increasing chroma will quickly push colours
outside of the spectral locus, which can be observed by connecting an instance
of a [ciediag module](../ciediag/readme.md) between this module and the `hist`
display for instance. limiting to spectral locus instead will only show an
effect if `sat` is increased, and never push colours off limits. the two other
options (rec2020 and rec709) first scale down all colours such that the
theoretical maximum (on the spectral locus boundary) will fit into the
respective colour space. `sat` can then be used to increase saturation again
from there, but will never produce values outside the respective tristimulus
gamut.


## parameters

* `exposure` this is here for convenience, so we save the memory bandwidth to carry
  all the pixels to yet another module doing a trivial transform on the input data.
* `sat` saturation control using dt UCS or the optional lookup tables if they are connected.
* `mode` the module can be run in `parametric` mode, where the colour matrix and white
  balance coefficients are used and then the RBF only comes into play for gamut mapping.
  the `data driven` mode works on an aribtrary list of source and target points which
  can be given in the parameters as `source` and `target` list of points.
  this parameter runs a ui group, so it will show/hide a few other elements.
* `gamut` the gamut can be left untouched, projected to spectral locus, rec2020, or rec709.
  works only if the abney and spectra inputs are connected.
* `picked` what to do with the picked input colour, if it is connected to the `picked` connector.
  it can be used as source for white balancing and/or deflickering.
* `matrix` input device transform mode: use the image matrix or a selection of presets, or the colour lut.
* `temp` if a clut is used, this allows to blend between the two illuminants. usually this
  is illuminant A and D65 and reflected in the values of the temperature here.
* `white` the white balance destination colour
* `mat` a hidden parameter containing the coefficients of the image matrix.
* `cnt` the number of patches to use in the rbf mapping
* `rbmap` the patch source and destination colours
* `import` press this button to import source and destination colours from a
  colour picker by the instance name as entered to the right. the picker should
  be attached to the output of this module and you have to keep the saturation
  slider at 1.0 when doing this. also the rbf needs to be set to identity
  (reduce number of patches, double click the sliders to reset to src).
  reference numbers for the picker can be obtained by presets for your target (such as cc24).
  

## connectors

* `input` the input colour buffer in camera rgb
* `output` the output in rec2020 rgb
* `clut` (optional) the color lookup table generated by the mkclut utility
* `picked` (optional) route picked colour for white balancing source in here
* `abney` (optional) point to data/abney.lut for hue constant saturation and gamut compression, see the gamut.pst preset
* `spectra` (optional) point to data/spectra.lut for hue constant saturation and gamut compression, see the gamut.pst preset
