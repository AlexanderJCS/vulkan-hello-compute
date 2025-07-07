layout(binding = 0, rgba8) readonly  uniform image2D readImage;
layout(binding = 1, rgba8) writeonly uniform image2D writeImage;

//-----------------------------------------------------------------------------
// 1‑D Gaussian blur along an arbitrary axis (dir = (1,0) for X, (0,1) for Y)
//-----------------------------------------------------------------------------
vec4 gaussianBlur1D(ivec2 pix, ivec2 size, float sigma, ivec2 dir) {
    int   radius       = int(ceil(3.0 * sigma));
    float invTwoSigma2 = 1.0 / (2.0 * sigma * sigma);
    float norm         = 1.0 / (sqrt(2.0 * 3.14159265) * sigma);

    vec4  colSum = vec4(0.0);
    float wSum   = 0.0;

    for (int offset = -radius; offset <= radius; ++offset) {
        ivec2 samplePix = clamp(pix + dir * offset, ivec2(0), size - 1);
        float w = norm * exp(-float(offset * offset) * invTwoSigma2);
        colSum += imageLoad(readImage, samplePix) * w;
        wSum  += w;
    }

    return colSum / wSum;
}
