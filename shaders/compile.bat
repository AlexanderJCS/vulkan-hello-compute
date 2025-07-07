glslc -O -I "../polyglot" -fshader-stage=vert --target-env=vulkan1.3 ./raster/display.vert.glsl -o ./raster/display.vert.spv
glslc -O -I "../polyglot" -fshader-stage=frag --target-env=vulkan1.3 ./raster/display.frag.glsl -o ./raster/display.frag.spv

glslc -O -I "../polyglot" -fshader-stage=comp --target-env=vulkan1.3 ./blur/blurx.comp.glsl -o ./blur/blurx.comp.spv
glslc -O -I "../polyglot" -fshader-stage=comp --target-env=vulkan1.3 ./update/update.comp.glsl -o ./update/update.comp.spv
