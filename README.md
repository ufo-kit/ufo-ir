## Iterative Reconstruction for UFO

### Installation

In order to use iterative reconstruction methods you have to compile the
projects in the following order:

1. Build [ufo-core](https://github.com/ufo-kit/ufo-core).
2. Build `ufo-ir-core`.
3. Build [ufo-filters](https://github.com/ufo-kit/ufo-filters) to produce
   all plugins include `ir`.
4. Build `ufo-ir-plugins` to produce the different geometry, projector and
   method plugins.
